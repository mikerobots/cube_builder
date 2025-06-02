#pragma once

#include "FileTypes.h"
#include "BinaryIO.h"
#include "Project.h"
#include <memory>
#include <vector>

namespace VoxelEditor {
namespace FileIO {

// File header structure
struct FileHeader {
    char magic[4] = {'C', 'V', 'E', 'F'};
    FileVersion version;
    uint64_t fileSize = 0;
    uint32_t compressionFlags = 0;
    uint64_t checksum = 0;
    uint8_t reserved[228] = {};
    
    bool isValid() const;
    void updateChecksum(const uint8_t* data, size_t size);
    uint64_t calculateChecksum() const;
};

// Chunk header structure
struct ChunkHeader {
    ChunkType type;
    uint32_t size = 0;
    uint32_t uncompressedSize = 0;
    uint32_t checksum = 0;
    
    bool isValid() const;
};

// Binary format reader/writer
class BinaryFormat {
public:
    BinaryFormat();
    ~BinaryFormat();
    
    // Main operations
    bool writeProject(BinaryWriter& writer, const Project& project, const SaveOptions& options);
    bool readProject(BinaryReader& reader, Project& project, const LoadOptions& options);
    
    // Validation
    bool validateFile(BinaryReader& reader);
    FileVersion detectVersion(BinaryReader& reader);
    
    // Error handling
    FileError getLastError() const { return m_lastError; }
    std::string getLastErrorMessage() const { return m_lastErrorMessage; }
    
    // Header operations (public for FileManager)
    bool readHeader(BinaryReader& reader, FileHeader& header);
    
private:
    FileError m_lastError = FileError::None;
    std::string m_lastErrorMessage;
    
    // Header operations
    bool writeHeader(BinaryWriter& writer, const FileHeader& header);
    
    // Chunk operations
    bool writeChunk(BinaryWriter& writer, ChunkType type, const std::vector<uint8_t>& data);
    bool readChunk(BinaryReader& reader, ChunkHeader& header, std::vector<uint8_t>& data);
    bool skipChunk(BinaryReader& reader, const ChunkHeader& header);
    
    // Specific chunk writers
    bool writeMetadataChunk(BinaryWriter& writer, const ProjectMetadata& metadata);
    bool writeVoxelDataChunk(BinaryWriter& writer, const VoxelData::VoxelDataManager& voxelData, const SaveOptions& options);
    bool writeGroupDataChunk(BinaryWriter& writer, const Groups::GroupManager& groupData);
    bool writeCameraStateChunk(BinaryWriter& writer, const Camera::OrbitCamera& camera);
    bool writeSelectionDataChunk(BinaryWriter& writer, const Project& project);
    bool writeSettingsChunk(BinaryWriter& writer, const WorkspaceSettings& settings);
    bool writeCustomDataChunk(BinaryWriter& writer, const std::string& key, const std::vector<uint8_t>& data);
    
    // Specific chunk readers
    bool readMetadataChunk(BinaryReader& reader, ProjectMetadata& metadata);
    bool readVoxelDataChunk(BinaryReader& reader, VoxelData::VoxelDataManager& voxelData, const LoadOptions& options);
    bool readGroupDataChunk(BinaryReader& reader, Groups::GroupManager& groupData);
    bool readCameraStateChunk(BinaryReader& reader, Camera::OrbitCamera& camera);
    bool readSelectionDataChunk(BinaryReader& reader, Project& project);
    bool readSettingsChunk(BinaryReader& reader, WorkspaceSettings& settings);
    bool readCustomDataChunk(BinaryReader& reader, std::string& key, std::vector<uint8_t>& data);
    
    // Utility functions
    std::vector<uint8_t> serializeToBuffer(const std::function<void(BinaryWriter&)>& writeFunc);
    bool deserializeFromBuffer(const std::vector<uint8_t>& buffer, const std::function<void(BinaryReader&)>& readFunc);
    
    void setError(FileError error, const std::string& message);
    void clearError();
};

// Checksum utilities
class ChecksumCalculator {
public:
    static uint64_t calculate(const uint8_t* data, size_t size);
    static uint32_t calculateCRC32(const uint8_t* data, size_t size);
    
private:
    static const uint32_t CRC32_TABLE[256];
    static void initializeCRC32Table();
};

} // namespace FileIO
} // namespace VoxelEditor