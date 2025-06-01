#pragma once

#include <vector>
#include <cstdint>
#include <iostream>

namespace VoxelEditor {
namespace FileIO {

// Forward declarations
namespace VoxelData {
    class VoxelGrid;
}

// Compression wrapper for LZ4
class Compression {
public:
    Compression();
    ~Compression();
    
    // Basic compression/decompression
    bool compress(const uint8_t* input, size_t inputSize, 
                 std::vector<uint8_t>& output, int level = 6);
    bool decompress(const uint8_t* input, size_t inputSize,
                   std::vector<uint8_t>& output, size_t expectedSize);
    
    // Size calculations
    size_t getMaxCompressedSize(size_t inputSize) const;
    float getCompressionRatio() const { return m_lastCompressionRatio; }
    
    // Stream compression
    bool compressStream(std::istream& input, std::ostream& output, int level = 6);
    bool decompressStream(std::istream& input, std::ostream& output, size_t expectedSize);
    
    // Specialized voxel compression
    bool compressVoxelData(const VoxelData::VoxelGrid& grid, std::vector<uint8_t>& output, int level = 6);
    bool decompressVoxelData(const uint8_t* input, size_t inputSize, VoxelData::VoxelGrid& grid);
    
    // Configuration
    void setCompressionLevel(int level) { m_compressionLevel = level; }
    int getCompressionLevel() const { return m_compressionLevel; }
    
    // Error handling
    const std::string& getLastError() const { return m_lastError; }
    
private:
    float m_lastCompressionRatio = 1.0f;
    int m_compressionLevel = 6;
    std::string m_lastError;
    
    // LZ4-specific implementation
    bool compressLZ4(const uint8_t* input, size_t inputSize,
                    std::vector<uint8_t>& output, int level);
    bool decompressLZ4(const uint8_t* input, size_t inputSize,
                      std::vector<uint8_t>& output, size_t expectedSize);
    
    // Voxel data optimization
    std::vector<uint8_t> optimizeVoxelDataForCompression(const VoxelData::VoxelGrid& grid);
    void restoreVoxelDataFromOptimized(const std::vector<uint8_t>& data, VoxelData::VoxelGrid& grid);
    
    // Run-length encoding for sparse voxel data
    void runLengthEncode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    void runLengthDecode(const std::vector<uint8_t>& input, std::vector<uint8_t>& output);
    
    // Error handling
    void setError(const std::string& error);
    void clearError();
};

// Compression utilities
class CompressionUtils {
public:
    // Check if data appears to be compressed
    static bool isCompressed(const uint8_t* data, size_t size);
    
    // Calculate compression ratio
    static float calculateRatio(size_t originalSize, size_t compressedSize);
    
    // Estimate compressed size
    static size_t estimateCompressedSize(size_t originalSize, float expectedRatio = 0.5f);
};

// Compression header for identifying compressed data
struct CompressionHeader {
    uint32_t magic = 0x4C5A3443; // 'LZ4C'
    uint32_t originalSize = 0;
    uint32_t compressedSize = 0;
    uint32_t checksum = 0;
    
    static constexpr size_t SIZE = 16;
    
    void write(uint8_t* buffer) const;
    void read(const uint8_t* buffer);
    bool isValid() const;
};

} // namespace FileIO
} // namespace VoxelEditor