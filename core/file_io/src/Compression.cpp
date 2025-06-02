#include "../include/file_io/Compression.h"
#include "logging/Logger.h"
#include <cstring>
#include <algorithm>

namespace VoxelEditor {
namespace FileIO {

Compression::Compression() = default;
Compression::~Compression() = default;

bool Compression::compress(const uint8_t* input, size_t inputSize, 
                         std::vector<uint8_t>& output, int level) {
    clearError();
    m_compressionLevel = level;
    
    // For now, implement simple no-compression storage
    // TODO: Integrate LZ4 when available
    output.clear();
    output.resize(CompressionHeader::SIZE + inputSize);
    
    // Write header
    CompressionHeader header;
    header.originalSize = static_cast<uint32_t>(inputSize);
    header.compressedSize = static_cast<uint32_t>(inputSize);
    header.checksum = 0; // TODO: Calculate checksum
    header.write(output.data());
    
    // Copy data
    std::memcpy(output.data() + CompressionHeader::SIZE, input, inputSize);
    
    m_lastCompressionRatio = 1.0f;
    return true;
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
    
    // For now, just copy the data
    output.resize(header.originalSize);
    std::memcpy(output.data(), input + CompressionHeader::SIZE, header.originalSize);
    
    return true;
}

size_t Compression::getMaxCompressedSize(size_t inputSize) const {
    // Conservative estimate: header + original size + 10%
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

bool Compression::compressVoxelData(const VoxelData::VoxelGrid& grid, std::vector<uint8_t>& output, int level) {
    // TODO: Implement voxel-specific compression
    setError("Voxel compression not implemented");
    return false;
}

bool Compression::decompressVoxelData(const uint8_t* input, size_t inputSize, VoxelData::VoxelGrid& grid) {
    // TODO: Implement voxel-specific decompression
    setError("Voxel decompression not implemented");
    return false;
}

bool Compression::compressLZ4(const uint8_t* input, size_t inputSize,
                            std::vector<uint8_t>& output, int level) {
    // TODO: Implement LZ4 compression
    return false;
}

bool Compression::decompressLZ4(const uint8_t* input, size_t inputSize,
                              std::vector<uint8_t>& output, size_t expectedSize) {
    // TODO: Implement LZ4 decompression
    return false;
}

std::vector<uint8_t> Compression::optimizeVoxelDataForCompression(const VoxelData::VoxelGrid& grid) {
    // TODO: Implement voxel data optimization
    return {};
}

void Compression::restoreVoxelDataFromOptimized(const std::vector<uint8_t>& data, VoxelData::VoxelGrid& grid) {
    // TODO: Implement voxel data restoration
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
    return magic == 0x4C5A3443 && originalSize > 0 && compressedSize > 0;
}

} // namespace FileIO
} // namespace VoxelEditor