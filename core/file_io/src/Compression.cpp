#include "../include/file_io/Compression.h"
#include "logging/Logger.h"
#include "../voxel_data/VoxelGrid.h"
#include "../../foundation/math/Vector3i.h"
#include <cstring>
#include <algorithm>

#ifdef HAVE_LZ4
#include <lz4.h>
#include <lz4hc.h>
#endif

namespace VoxelEditor {
namespace FileIO {

Compression::Compression() = default;
Compression::~Compression() = default;

bool Compression::compress(const uint8_t* input, size_t inputSize, 
                         std::vector<uint8_t>& output, int level) {
    clearError();
    m_compressionLevel = level;
    
#ifdef HAVE_LZ4
    // Use LZ4 compression
    return compressLZ4(input, inputSize, output, level);
#else
    // Fallback: no compression storage
    output.clear();
    output.resize(CompressionHeader::SIZE + inputSize);
    
    // Write header
    CompressionHeader header;
    header.originalSize = static_cast<uint32_t>(inputSize);
    header.compressedSize = static_cast<uint32_t>(inputSize);
    header.checksum = calculateChecksum(input, inputSize);
    header.write(output.data());
    
    // Copy data
    std::memcpy(output.data() + CompressionHeader::SIZE, input, inputSize);
    
    m_lastCompressionRatio = 1.0f;
    return true;
#endif
}

bool Compression::decompress(const uint8_t* input, size_t inputSize,
                           std::vector<uint8_t>& output, size_t expectedSize) {
    clearError();
    
    if (inputSize < CompressionHeader::SIZE) {
        setError("Input too small for compression header");
        return false;
    }
    
    // Read header
    CompressionHeader header;
    header.read(input);
    
    if (!header.isValid()) {
        setError("Invalid compression header");
        return false;
    }
    
    if (expectedSize > 0 && header.originalSize != expectedSize) {
        setError("Size mismatch");
        return false;
    }
    
#ifdef HAVE_LZ4
    // Use LZ4 decompression if data is compressed
    if (header.compressedSize < header.originalSize) {
        return decompressLZ4(input + CompressionHeader::SIZE, header.compressedSize, output, header.originalSize);
    }
#endif
    
    // Uncompressed data or fallback
    output.resize(header.originalSize);
    if (header.originalSize > 0) {
        std::memcpy(output.data(), input + CompressionHeader::SIZE, header.originalSize);
        
        // Verify checksum if available
        if (header.checksum != 0) {
            uint32_t calculatedChecksum = calculateChecksum(output.data(), output.size());
            if (calculatedChecksum != header.checksum) {
                setError("Checksum mismatch - data may be corrupted");
                return false;
            }
        }
    }
    
    return true;
}

size_t Compression::getMaxCompressedSize(size_t inputSize) const {
#ifdef HAVE_LZ4
    if (inputSize == 0) {
        return CompressionHeader::SIZE;
    }
    
    // Use LZ4's bound calculation for accuracy
    int lz4Bound = LZ4_compressBound(static_cast<int>(inputSize));
    if (lz4Bound > 0) {
        return CompressionHeader::SIZE + static_cast<size_t>(lz4Bound);
    }
#endif
    
    // Fallback: conservative estimate
    return CompressionHeader::SIZE + inputSize + (inputSize / 10);
}

bool Compression::compressStream(std::istream& input, std::ostream& output, int level) {
    // Read all input
    input.seekg(0, std::ios::end);
    size_t size = input.tellg();
    input.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> inputData(size);
    input.read(reinterpret_cast<char*>(inputData.data()), size);
    
    std::vector<uint8_t> outputData;
    if (!compress(inputData.data(), inputData.size(), outputData, level)) {
        return false;
    }
    
    output.write(reinterpret_cast<const char*>(outputData.data()), outputData.size());
    return output.good();
}

bool Compression::decompressStream(std::istream& input, std::ostream& output, size_t expectedSize) {
    // Read all input
    input.seekg(0, std::ios::end);
    size_t size = input.tellg();
    input.seekg(0, std::ios::beg);
    
    std::vector<uint8_t> inputData(size);
    input.read(reinterpret_cast<char*>(inputData.data()), size);
    
    std::vector<uint8_t> outputData;
    if (!decompress(inputData.data(), inputData.size(), outputData, expectedSize)) {
        return false;
    }
    
    output.write(reinterpret_cast<const char*>(outputData.data()), outputData.size());
    return output.good();
}

bool Compression::compressVoxelData(const ::VoxelEditor::VoxelData::VoxelGrid& grid, std::vector<uint8_t>& output, int level) {
    clearError();
    
    // Optimize voxel data for compression (sparse representation)
    std::vector<uint8_t> optimizedData = optimizeVoxelDataForCompression(grid);
    
    if (optimizedData.empty()) {
        setError("Failed to optimize voxel data");
        return false;
    }
    
    // Apply run-length encoding first for sparse data
    std::vector<uint8_t> rleData;
    runLengthEncode(optimizedData, rleData);
    
    // Choose the more efficient representation
    const std::vector<uint8_t>& dataToCompress = (rleData.size() < optimizedData.size()) ? rleData : optimizedData;
    bool usedRLE = (rleData.size() < optimizedData.size());
    
    // Compress with LZ4
    std::vector<uint8_t> compressedData;
    if (!compress(dataToCompress.data(), dataToCompress.size(), compressedData, level)) {
        return false;
    }
    
    // Add metadata header to indicate voxel-specific compression
    output.clear();
    output.resize(4 + compressedData.size()); // 4 bytes for metadata
    
    // Metadata: [version(1) | flags(1) | reserved(2)]
    output[0] = 1; // version
    output[1] = usedRLE ? 0x01 : 0x00; // flags: bit 0 = RLE used
    output[2] = 0; // reserved
    output[3] = 0; // reserved
    
    // Copy compressed data
    std::memcpy(output.data() + 4, compressedData.data(), compressedData.size());
    
    return true;
}

bool Compression::decompressVoxelData(const uint8_t* input, size_t inputSize, ::VoxelEditor::VoxelData::VoxelGrid& grid) {
    clearError();
    
    if (inputSize < 4) {
        setError("Invalid voxel compression header");
        return false;
    }
    
    // Read metadata header
    uint8_t version = input[0];
    uint8_t flags = input[1];
    bool usedRLE = (flags & 0x01) != 0;
    
    if (version != 1) {
        setError("Unsupported voxel compression version");
        return false;
    }
    
    // Decompress the main data
    std::vector<uint8_t> decompressedData;
    if (!decompress(input + 4, inputSize - 4, decompressedData, 0)) {
        return false;
    }
    
    // Apply RLE decompression if it was used
    std::vector<uint8_t> finalData;
    if (usedRLE) {
        runLengthDecode(decompressedData, finalData);
    } else {
        finalData = std::move(decompressedData);
    }
    
    // Restore voxel data from optimized format
    restoreVoxelDataFromOptimized(finalData, grid);
    
    return true;
}

bool Compression::compressLZ4(const uint8_t* input, size_t inputSize,
                            std::vector<uint8_t>& output, int level) {
#ifdef HAVE_LZ4
    if (inputSize == 0) {
        output.clear();
        output.resize(CompressionHeader::SIZE);
        
        CompressionHeader header;
        header.originalSize = 0;
        header.compressedSize = 0;
        header.checksum = 0;
        header.write(output.data());
        
        m_lastCompressionRatio = 1.0f;
        return true;
    }
    
    // Calculate maximum compressed size
    int maxCompressedSize = LZ4_compressBound(static_cast<int>(inputSize));
    if (maxCompressedSize <= 0) {
        setError("Invalid input size for compression");
        return false;
    }
    
    // Allocate output buffer
    output.clear();
    output.resize(CompressionHeader::SIZE + maxCompressedSize);
    
    // Perform compression
    int compressedSize;
    if (level <= 3) {
        // Fast compression
        compressedSize = LZ4_compress_default(
            reinterpret_cast<const char*>(input),
            reinterpret_cast<char*>(output.data() + CompressionHeader::SIZE),
            static_cast<int>(inputSize),
            maxCompressedSize
        );
    } else {
        // High compression
        compressedSize = LZ4_compress_HC(
            reinterpret_cast<const char*>(input),
            reinterpret_cast<char*>(output.data() + CompressionHeader::SIZE),
            static_cast<int>(inputSize),
            maxCompressedSize,
            level
        );
    }
    
    if (compressedSize <= 0) {
        setError("LZ4 compression failed");
        return false;
    }
    
    // Check if compression was beneficial
    if (static_cast<size_t>(compressedSize) >= inputSize) {
        // Store uncompressed if compression didn't help
        output.resize(CompressionHeader::SIZE + inputSize);
        std::memcpy(output.data() + CompressionHeader::SIZE, input, inputSize);
        
        CompressionHeader header;
        header.originalSize = static_cast<uint32_t>(inputSize);
        header.compressedSize = static_cast<uint32_t>(inputSize);
        header.checksum = calculateChecksum(input, inputSize);
        header.write(output.data());
        
        m_lastCompressionRatio = 1.0f;
    } else {
        // Use compressed data
        output.resize(CompressionHeader::SIZE + compressedSize);
        
        CompressionHeader header;
        header.originalSize = static_cast<uint32_t>(inputSize);
        header.compressedSize = static_cast<uint32_t>(compressedSize);
        header.checksum = calculateChecksum(input, inputSize);
        header.write(output.data());
        
        m_lastCompressionRatio = static_cast<float>(inputSize) / static_cast<float>(compressedSize);
    }
    
    return true;
#else
    setError("LZ4 compression not available");
    return false;
#endif
}

bool Compression::decompressLZ4(const uint8_t* input, size_t inputSize,
                              std::vector<uint8_t>& output, size_t expectedSize) {
#ifdef HAVE_LZ4
    if (expectedSize == 0) {
        output.clear();
        return true;
    }
    
    // Allocate output buffer
    output.resize(expectedSize);
    
    // Perform decompression
    int decompressedSize = LZ4_decompress_safe(
        reinterpret_cast<const char*>(input),
        reinterpret_cast<char*>(output.data()),
        static_cast<int>(inputSize),
        static_cast<int>(expectedSize)
    );
    
    if (decompressedSize < 0) {
        setError("LZ4 decompression failed");
        return false;
    }
    
    if (static_cast<size_t>(decompressedSize) != expectedSize) {
        setError("Decompressed size mismatch");
        return false;
    }
    
    return true;
#else
    setError("LZ4 decompression not available");
    return false;
#endif
}

std::vector<uint8_t> Compression::optimizeVoxelDataForCompression(const ::VoxelEditor::VoxelData::VoxelGrid& grid) {
    // Get all active voxels from the sparse grid
    auto voxels = grid.getAllVoxels();
    
    if (voxels.empty()) {
        // Return empty data marker
        return {0, 0, 0, 0}; // 4 zero bytes to indicate empty grid
    }
    
    // Sparse format: [count(4)] + [pos1(12) + pos2(12) + ...]
    // Each position is stored as 3 * int32_t = 12 bytes
    std::vector<uint8_t> data;
    data.reserve(4 + voxels.size() * 12);
    
    // Write voxel count (little endian)
    uint32_t count = static_cast<uint32_t>(voxels.size());
    data.push_back(count & 0xFF);
    data.push_back((count >> 8) & 0xFF);
    data.push_back((count >> 16) & 0xFF);
    data.push_back((count >> 24) & 0xFF);
    
    // Write each voxel position
    for (const auto& voxel : voxels) {
        // X coordinate
        int32_t x = voxel.gridPos.x;
        data.push_back(x & 0xFF);
        data.push_back((x >> 8) & 0xFF);
        data.push_back((x >> 16) & 0xFF);
        data.push_back((x >> 24) & 0xFF);
        
        // Y coordinate
        int32_t y = voxel.gridPos.y;
        data.push_back(y & 0xFF);
        data.push_back((y >> 8) & 0xFF);
        data.push_back((y >> 16) & 0xFF);
        data.push_back((y >> 24) & 0xFF);
        
        // Z coordinate
        int32_t z = voxel.gridPos.z;
        data.push_back(z & 0xFF);
        data.push_back((z >> 8) & 0xFF);
        data.push_back((z >> 16) & 0xFF);
        data.push_back((z >> 24) & 0xFF);
    }
    
    return data;
}

void Compression::restoreVoxelDataFromOptimized(const std::vector<uint8_t>& data, ::VoxelEditor::VoxelData::VoxelGrid& grid) {
    // Clear the grid first
    grid.clear();
    
    if (data.size() < 4) {
        return; // Invalid data
    }
    
    // Read voxel count (little endian)
    uint32_t count = data[0] | (data[1] << 8) | (data[2] << 16) | (data[3] << 24);
    
    if (count == 0) {
        return; // Empty grid
    }
    
    // Verify data size is correct
    if (data.size() != 4 + count * 12) {
        return; // Invalid data size
    }
    
    // Restore each voxel
    size_t offset = 4;
    for (uint32_t i = 0; i < count; ++i) {
        // Read X coordinate
        int32_t x = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        
        // Read Y coordinate
        int32_t y = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        
        // Read Z coordinate
        int32_t z = data[offset] | (data[offset + 1] << 8) | (data[offset + 2] << 16) | (data[offset + 3] << 24);
        offset += 4;
        
        // Set the voxel in the grid
        grid.setVoxel(Math::Vector3i(x, y, z), true);
    }
}

void Compression::runLengthEncode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();
    if (input.empty()) return;
    
    size_t i = 0;
    while (i < input.size()) {
        uint8_t value = input[i];
        size_t count = 1;
        
        while (i + count < input.size() && count < 255 && input[i + count] == value) {
            count++;
        }
        
        output.push_back(static_cast<uint8_t>(count));
        output.push_back(value);
        i += count;
    }
}

void Compression::runLengthDecode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output) {
    output.clear();
    
    for (size_t i = 0; i + 1 < input.size(); i += 2) {
        uint8_t count = input[i];
        uint8_t value = input[i + 1];
        
        for (uint8_t j = 0; j < count; j++) {
            output.push_back(value);
        }
    }
}

void Compression::setError(const std::string& error) {
    m_lastError = error;
    LOG_ERROR("Compression error: " + error);
}

void Compression::clearError() {
    m_lastError.clear();
}

uint32_t Compression::calculateChecksum(const uint8_t* data, size_t size) {
    // Simple CRC32-like checksum calculation
    uint32_t checksum = 0xFFFFFFFF;
    
    for (size_t i = 0; i < size; ++i) {
        checksum ^= data[i];
        for (int j = 0; j < 8; ++j) {
            if (checksum & 1) {
                checksum = (checksum >> 1) ^ 0xEDB88320;
            } else {
                checksum >>= 1;
            }
        }
    }
    
    return ~checksum;
}

// CompressionUtils implementation
bool CompressionUtils::isCompressed(const uint8_t* data, size_t size) {
    if (size < CompressionHeader::SIZE) {
        return false;
    }
    
    CompressionHeader header;
    header.read(data);
    return header.isValid();
}

float CompressionUtils::calculateRatio(size_t originalSize, size_t compressedSize) {
    if (compressedSize == 0) return 0.0f;
    return static_cast<float>(originalSize) / static_cast<float>(compressedSize);
}

size_t CompressionUtils::estimateCompressedSize(size_t originalSize, float expectedRatio) {
    return static_cast<size_t>(originalSize / expectedRatio) + CompressionHeader::SIZE;
}

// CompressionHeader implementation
void CompressionHeader::write(uint8_t* buffer) const {
    std::memcpy(buffer, &magic, sizeof(magic));
    std::memcpy(buffer + 4, &originalSize, sizeof(originalSize));
    std::memcpy(buffer + 8, &compressedSize, sizeof(compressedSize));
    std::memcpy(buffer + 12, &checksum, sizeof(checksum));
}

void CompressionHeader::read(const uint8_t* buffer) {
    std::memcpy(&magic, buffer, sizeof(magic));
    std::memcpy(&originalSize, buffer + 4, sizeof(originalSize));
    std::memcpy(&compressedSize, buffer + 8, sizeof(compressedSize));
    std::memcpy(&checksum, buffer + 12, sizeof(checksum));
}

bool CompressionHeader::isValid() const {
    return magic == 0x4C5A3443;  // Empty data (size 0) is valid
}

} // namespace FileIO
} // namespace VoxelEditor