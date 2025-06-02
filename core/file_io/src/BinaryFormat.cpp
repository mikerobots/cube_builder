#include "../include/file_io/BinaryFormat.h"
#include "../include/file_io/Compression.h"
#include "../include/file_io/FileVersioning.h"
#include "logging/Logger.h"
#include <cstring>
#include <sstream>

namespace VoxelEditor {
namespace FileIO {

// FileHeader implementation
bool FileHeader::isValid() const {
    return std::memcmp(magic, "CVEF", 4) == 0 &&
           version.major > 0 &&
           fileSize > 0;
}

void FileHeader::updateChecksum(const uint8_t* data, size_t size) {
    checksum = ChecksumCalculator::calculate(data, size);
}

uint64_t FileHeader::calculateChecksum() const {
    // Calculate checksum of header excluding the checksum field itself
    std::vector<uint8_t> buffer(sizeof(FileHeader));
    std::memcpy(buffer.data(), this, sizeof(FileHeader));
    // Zero out checksum field
    std::memset(buffer.data() + offsetof(FileHeader, checksum), 0, sizeof(checksum));
    return ChecksumCalculator::calculate(buffer.data(), sizeof(FileHeader));
}

// ChunkHeader implementation
bool ChunkHeader::isValid() const {
    return size > 0 && static_cast<uint32_t>(type) < 0xFFFFFFFF;
}

// BinaryFormat implementation
BinaryFormat::BinaryFormat() = default;
BinaryFormat::~BinaryFormat() = default;

bool BinaryFormat::writeProject(BinaryWriter& writer, const Project& project, const SaveOptions& options) {
    clearError();
    
    // Write placeholder header
    FileHeader header;
    header.version = FileVersion::Current();
    header.compressionFlags = options.compress ? 1 : 0;
    
    size_t headerPos = writer.getBytesWritten();
    if (!writeHeader(writer, header)) {
        return false;
    }
    
    // Write chunks
    if (!writeMetadataChunk(writer, project.metadata)) {
        return false;
    }
    
    if (!writeVoxelDataChunk(writer, *project.voxelData, options)) {
        return false;
    }
    
    if (project.groupData && !writeGroupDataChunk(writer, *project.groupData)) {
        return false;
    }
    
    if (project.camera && !writeCameraStateChunk(writer, *project.camera)) {
        return false;
    }
    
    if (!writeSelectionDataChunk(writer, project)) {
        return false;
    }
    
    if (!writeSettingsChunk(writer, project.workspace)) {
        return false;
    }
    
    // Write custom data chunks
    for (const auto& [key, data] : project.customData) {
        if (!writeCustomDataChunk(writer, key, data)) {
            return false;
        }
    }
    
    
    // TODO: Update header with final file size
    
    return true;
}

bool BinaryFormat::readProject(BinaryReader& reader, Project& project, const LoadOptions& options) {
    clearError();
    
    // Read and validate header
    FileHeader header;
    if (!readHeader(reader, header)) {
        return false;
    }
    
    if (!header.isValid()) {
        setError(FileError::InvalidFormat, "Invalid file header");
        return false;
    }
    
    // Check version compatibility
    if (!VersionCompatibility::canRead(header.version, FileVersion::Current())) {
        setError(FileError::VersionMismatch, "Incompatible file version");
        return false;
    }
    
    // Read chunks
    while (reader.isValid() && !reader.isAtEnd()) {
        ChunkHeader chunkHeader;
        std::vector<uint8_t> chunkData;
        if (!readChunk(reader, chunkHeader, chunkData)) {
            break;
        }
        
        switch (chunkHeader.type) {
            case ChunkType::Metadata:
                if (!readMetadataChunk(reader, project.metadata)) {
                    return false;
                }
                break;
                
            case ChunkType::VoxelData:
                if (!project.voxelData) {
                    skipChunk(reader, chunkHeader);
                } else if (!readVoxelDataChunk(reader, *project.voxelData, options)) {
                    return false;
                }
                break;
                
            case ChunkType::GroupData:
                if (!project.groupData) {
                    skipChunk(reader, chunkHeader);
                } else if (!readGroupDataChunk(reader, *project.groupData)) {
                    return false;
                }
                break;
                
            case ChunkType::CameraState:
                if (!project.camera) {
                    skipChunk(reader, chunkHeader);
                } else if (!readCameraStateChunk(reader, *project.camera)) {
                    return false;
                }
                break;
                
            case ChunkType::SelectionData:
                if (!readSelectionDataChunk(reader, project)) {
                    return false;
                }
                break;
                
            case ChunkType::Settings:
                if (!readSettingsChunk(reader, project.workspace)) {
                    return false;
                }
                break;
                
            case ChunkType::CustomData: {
                std::string key;
                std::vector<uint8_t> data;
                if (readCustomDataChunk(reader, key, data)) {
                    project.customData[key] = data;
                }
                break;
            }
                
                
            default:
                // Unknown chunk, skip it
                skipChunk(reader, chunkHeader);
                break;
        }
    }
    
    return reader.isValid();
}

bool BinaryFormat::validateFile(BinaryReader& reader) {
    FileHeader header;
    return readHeader(reader, header) && header.isValid();
}

FileVersion BinaryFormat::detectVersion(BinaryReader& reader) {
    FileHeader header;
    if (readHeader(reader, header)) {
        return header.version;
    }
    return FileVersion{0, 0, 0};
}

bool BinaryFormat::writeHeader(BinaryWriter& writer, const FileHeader& header) {
    writer.writeBytes(header.magic, 4);
    writer.writeUInt16(header.version.major);
    writer.writeUInt16(header.version.minor);
    writer.writeUInt16(header.version.patch);
    writer.writeUInt16(0); // padding
    writer.writeUInt64(header.fileSize);
    writer.writeUInt32(header.compressionFlags);
    writer.writeUInt32(0); // padding
    writer.writeUInt64(header.checksum);
    writer.writeBytes(header.reserved, sizeof(header.reserved));
    return writer.isValid();
}

bool BinaryFormat::readHeader(BinaryReader& reader, FileHeader& header) {
    reader.readBytes(header.magic, 4);
    header.version.major = reader.readUInt16();
    header.version.minor = reader.readUInt16();
    header.version.patch = reader.readUInt16();
    reader.readUInt16(); // padding
    header.fileSize = reader.readUInt64();
    header.compressionFlags = reader.readUInt32();
    reader.readUInt32(); // padding
    header.checksum = reader.readUInt64();
    reader.readBytes(header.reserved, sizeof(header.reserved));
    return reader.isValid();
}

bool BinaryFormat::writeChunk(BinaryWriter& writer, ChunkType type, const std::vector<uint8_t>& data) {
    ChunkHeader header;
    header.type = type;
    header.size = static_cast<uint32_t>(data.size());
    header.uncompressedSize = header.size;
    header.checksum = ChecksumCalculator::calculateCRC32(data.data(), data.size());
    
    writer.writeUInt32(static_cast<uint32_t>(type));
    writer.writeUInt32(header.size);
    writer.writeUInt32(header.uncompressedSize);
    writer.writeUInt32(header.checksum);
    
    if (!data.empty()) {
        writer.writeBytes(data.data(), data.size());
    }
    
    return writer.isValid();
}

bool BinaryFormat::readChunk(BinaryReader& reader, ChunkHeader& header, std::vector<uint8_t>& data) {
    header.type = static_cast<ChunkType>(reader.readUInt32());
    header.size = reader.readUInt32();
    header.uncompressedSize = reader.readUInt32();
    header.checksum = reader.readUInt32();
    
    if (!reader.isValid() || !header.isValid()) {
        return false;
    }
    
    if (header.size > 0) {
        data = reader.readBytes(header.size);
        
        // Verify checksum
        uint32_t calculatedChecksum = ChecksumCalculator::calculateCRC32(data.data(), data.size());
        if (calculatedChecksum != header.checksum) {
            setError(FileError::CorruptedData, "Chunk checksum mismatch");
            return false;
        }
    }
    
    return reader.isValid();
}

bool BinaryFormat::skipChunk(BinaryReader& reader, const ChunkHeader& header) {
    reader.skip(header.size);
    return reader.isValid();
}

bool BinaryFormat::writeMetadataChunk(BinaryWriter& writer, const ProjectMetadata& metadata) {
    auto buffer = serializeToBuffer([&](BinaryWriter& w) {
        w.writeString(metadata.name);
        w.writeString(metadata.description);
        w.writeString(metadata.author);
        w.writeUInt64(std::chrono::duration_cast<std::chrono::seconds>(
            metadata.created.time_since_epoch()).count());
        w.writeUInt64(std::chrono::duration_cast<std::chrono::seconds>(
            metadata.modified.time_since_epoch()).count());
        w.writeString(metadata.application);
        w.writeString(metadata.applicationVersion);
        w.writeUInt32(static_cast<uint32_t>(metadata.customProperties.size()));
        for (const auto& [key, value] : metadata.customProperties) {
            w.writeString(key);
            w.writeString(value);
        }
    });
    
    return writeChunk(writer, ChunkType::Metadata, buffer);
}

bool BinaryFormat::readMetadataChunk(BinaryReader& reader, ProjectMetadata& metadata) {
    metadata.name = reader.readString();
    metadata.description = reader.readString();
    metadata.author = reader.readString();
    
    uint64_t createdSeconds = reader.readUInt64();
    uint64_t modifiedSeconds = reader.readUInt64();
    metadata.created = std::chrono::system_clock::time_point(
        std::chrono::seconds(createdSeconds));
    metadata.modified = std::chrono::system_clock::time_point(
        std::chrono::seconds(modifiedSeconds));
    
    metadata.application = reader.readString();
    metadata.applicationVersion = reader.readString();
    
    uint32_t propCount = reader.readUInt32();
    metadata.customProperties.clear();
    for (uint32_t i = 0; i < propCount; i++) {
        std::string key = reader.readString();
        std::string value = reader.readString();
        metadata.customProperties[key] = value;
    }
    
    return reader.isValid();
}

// Stub implementations for other chunk types - to be implemented
bool BinaryFormat::writeVoxelDataChunk(BinaryWriter& writer, const ::VoxelEditor::VoxelData::VoxelDataManager& voxelData, const SaveOptions& options) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::readVoxelDataChunk(BinaryReader& reader, ::VoxelEditor::VoxelData::VoxelDataManager& voxelData, const LoadOptions& options) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::writeGroupDataChunk(BinaryWriter& writer, const Groups::GroupManager& groupData) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::readGroupDataChunk(BinaryReader& reader, Groups::GroupManager& groupData) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::writeCameraStateChunk(BinaryWriter& writer, const Camera::OrbitCamera& camera) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::readCameraStateChunk(BinaryReader& reader, Camera::OrbitCamera& camera) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::writeSelectionDataChunk(BinaryWriter& writer, const Project& project) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::readSelectionDataChunk(BinaryReader& reader, Project& project) {
    // TODO: Implement
    return true;
}

bool BinaryFormat::writeSettingsChunk(BinaryWriter& writer, const WorkspaceSettings& settings) {
    auto buffer = serializeToBuffer([&](BinaryWriter& w) {
        w.writeVector3f(settings.size);
        w.writeVector3f(settings.origin);
        w.writeUInt8(static_cast<uint8_t>(settings.defaultResolution));
        w.writeBool(settings.gridVisible);
        w.writeBool(settings.axesVisible);
        w.writeFloat(settings.backgroundColor.r);
        w.writeFloat(settings.backgroundColor.g);
        w.writeFloat(settings.backgroundColor.b);
        w.writeFloat(settings.backgroundColor.a);
    });
    
    return writeChunk(writer, ChunkType::Settings, buffer);
}

bool BinaryFormat::readSettingsChunk(BinaryReader& reader, WorkspaceSettings& settings) {
    settings.size = reader.readVector3f();
    settings.origin = reader.readVector3f();
    settings.defaultResolution = static_cast<::VoxelEditor::VoxelData::VoxelResolution>(reader.readUInt8());
    settings.gridVisible = reader.readBool();
    settings.axesVisible = reader.readBool();
    settings.backgroundColor.r = reader.readFloat();
    settings.backgroundColor.g = reader.readFloat();
    settings.backgroundColor.b = reader.readFloat();
    settings.backgroundColor.a = reader.readFloat();
    
    return reader.isValid();
}

bool BinaryFormat::writeCustomDataChunk(BinaryWriter& writer, const std::string& key, const std::vector<uint8_t>& data) {
    auto buffer = serializeToBuffer([&](BinaryWriter& w) {
        w.writeString(key);
        w.writeUInt32(static_cast<uint32_t>(data.size()));
        w.writeBytes(data.data(), data.size());
    });
    
    return writeChunk(writer, ChunkType::CustomData, buffer);
}

bool BinaryFormat::readCustomDataChunk(BinaryReader& reader, std::string& key, std::vector<uint8_t>& data) {
    key = reader.readString();
    uint32_t dataSize = reader.readUInt32();
    data = reader.readBytes(dataSize);
    
    return reader.isValid();
}

std::vector<uint8_t> BinaryFormat::serializeToBuffer(const std::function<void(BinaryWriter&)>& writeFunc) {
    std::ostringstream stream;
    BinaryWriter writer(stream);
    writeFunc(writer);
    
    std::string str = stream.str();
    return std::vector<uint8_t>(str.begin(), str.end());
}

bool BinaryFormat::deserializeFromBuffer(const std::vector<uint8_t>& buffer, const std::function<void(BinaryReader&)>& readFunc) {
    std::string str(buffer.begin(), buffer.end());
    std::istringstream stream(str);
    BinaryReader reader(stream);
    readFunc(reader);
    return reader.isValid();
}

void BinaryFormat::setError(FileError error, const std::string& message) {
    m_lastError = error;
    m_lastErrorMessage = message;
    LOG_ERROR("BinaryFormat error: " + message);
}

void BinaryFormat::clearError() {
    m_lastError = FileError::None;
    m_lastErrorMessage.clear();
}

// ChecksumCalculator implementation
uint64_t ChecksumCalculator::calculate(const uint8_t* data, size_t size) {
    // Simple 64-bit checksum
    uint64_t checksum = 0;
    for (size_t i = 0; i < size; i++) {
        checksum = (checksum << 1) ^ data[i];
    }
    return checksum;
}

uint32_t ChecksumCalculator::calculateCRC32(const uint8_t* data, size_t size) {
    // Simple CRC32 implementation
    uint32_t crc = 0xFFFFFFFF;
    for (size_t i = 0; i < size; i++) {
        crc ^= data[i];
        for (int j = 0; j < 8; j++) {
            crc = (crc >> 1) ^ (0xEDB88320 & -(crc & 1));
        }
    }
    return ~crc;
}


} // namespace FileIO
} // namespace VoxelEditor