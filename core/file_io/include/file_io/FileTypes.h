#pragma once

#include <string>
#include <vector>
#include <chrono>
#include <unordered_map>
#include <functional>
#include "Vector3f.h"
#include "VoxelTypes.h"
#include "Color.h"

namespace VoxelEditor {
namespace FileIO {

// Version information
struct FileVersion {
    uint16_t major = 1;
    uint16_t minor = 0;
    uint16_t patch = 0;
    uint16_t build = 0;
    
    bool operator==(const FileVersion& other) const {
        return major == other.major && minor == other.minor && 
               patch == other.patch && build == other.build;
    }
    
    bool operator<(const FileVersion& other) const {
        if (major != other.major) return major < other.major;
        if (minor != other.minor) return minor < other.minor;
        if (patch != other.patch) return patch < other.patch;
        return build < other.build;
    }
    
    bool isCompatible(const FileVersion& other) const {
        return major == other.major && minor <= other.minor;
    }
    
    std::string toString() const;
    static FileVersion fromString(const std::string& str);
    static FileVersion Current();
};

// Project metadata
struct ProjectMetadata {
    std::string name;
    std::string description;
    std::string author;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    FileVersion version;
    std::string application = "VoxelEditor";
    std::string applicationVersion;
    std::unordered_map<std::string, std::string> customProperties;
    
    void updateModified() {
        modified = std::chrono::system_clock::now();
    }
};

// Workspace settings
struct WorkspaceSettings {
    Math::Vector3f size = Math::Vector3f(5.0f, 5.0f, 5.0f);
    Math::Vector3f origin = Math::Vector3f(0, 0, 0);
    VoxelData::VoxelResolution defaultResolution = VoxelData::VoxelResolution::Size_1cm;
    bool gridVisible = true;
    bool axesVisible = true;
    Rendering::Color backgroundColor = Rendering::Color(0.2f, 0.2f, 0.2f, 1.0f);
};

// File info
struct FileInfo {
    std::string filename;
    std::string path;
    size_t fileSize = 0;
    FileVersion version;
    std::chrono::system_clock::time_point lastModified;
    bool compressed = false;
    bool readonly = false;
    
    std::string getFullPath() const {
        return path + "/" + filename;
    }
};

// Save options
struct SaveOptions {
    bool compress = true;
    int compressionLevel = 6;
    bool includeHistory = false;
    bool includeCache = false;
    bool createBackup = true;
    bool validateBeforeSave = true;
    
    static SaveOptions Default() { return SaveOptions(); }
    
    static SaveOptions Fast() {
        SaveOptions options;
        options.compress = false;
        options.validateBeforeSave = false;
        return options;
    }
    
    static SaveOptions Compact() {
        SaveOptions options;
        options.compressionLevel = 9;
        options.includeHistory = false;
        options.includeCache = false;
        return options;
    }
    
    static SaveOptions Development() {
        SaveOptions options;
        options.includeHistory = true;
        options.includeCache = true;
        return options;
    }
};

// Load options
struct LoadOptions {
    bool loadHistory = false;
    bool loadCache = false;
    bool validateAfterLoad = true;
    bool upgradeVersion = true;
    bool ignoreVersionMismatch = false;
    
    static LoadOptions Default() { return LoadOptions(); }
    
    static LoadOptions Fast() {
        LoadOptions options;
        options.validateAfterLoad = false;
        return options;
    }
    
    static LoadOptions Safe() {
        LoadOptions options;
        options.validateAfterLoad = true;
        options.ignoreVersionMismatch = false;
        return options;
    }
};

// STL export formats
enum class STLFormat {
    Binary,
    ASCII
};

// STL units
enum class STLUnits {
    Millimeters,
    Centimeters,
    Meters,
    Inches
};

// STL export options
struct STLExportOptions {
    STLFormat format = STLFormat::Binary;
    STLUnits units = STLUnits::Millimeters;
    float scale = 1.0f;
    bool mergeMeshes = true;
    bool validateWatertight = true;
    Math::Vector3f translation = Math::Vector3f(0, 0, 0);
    
    static STLExportOptions Default() { return STLExportOptions(); }
    
    static STLExportOptions Printing3D() {
        STLExportOptions options;
        options.units = STLUnits::Millimeters;
        options.validateWatertight = true;
        options.mergeMeshes = true;
        return options;
    }
    
    static STLExportOptions CAD() {
        STLExportOptions options;
        options.format = STLFormat::ASCII;
        options.units = STLUnits::Meters;
        return options;
    }
};

// STL export statistics
struct STLExportStats {
    size_t triangleCount = 0;
    size_t vertexCount = 0;
    float exportTime = 0.0f;
    size_t fileSize = 0;
    bool watertight = false;
    std::vector<std::string> warnings;
};

// Progress callback
using ProgressCallback = std::function<void(float progress, const std::string& message)>;
using SaveCompleteCallback = std::function<void(bool success, const std::string& filename)>;
using LoadCompleteCallback = std::function<void(bool success, const std::string& filename)>;

// Chunk types for binary format
enum class ChunkType : uint32_t {
    Metadata = 0x4D455441,        // 'META'
    VoxelData = 0x564F5845,       // 'VOXE'
    GroupData = 0x47525550,       // 'GRUP'
    CameraState = 0x43414D45,     // 'CAME'
    SelectionData = 0x53454C45,   // 'SELE'
    Settings = 0x53455454,        // 'SETT'
    CustomData = 0x43555354       // 'CUST'
};

// File format constants
namespace FileConstants {
    constexpr char MAGIC[4] = {'C', 'V', 'E', 'F'};
    constexpr size_t HEADER_SIZE = 256;
    constexpr size_t MAX_CHUNK_SIZE = 1024 * 1024 * 100; // 100MB max chunk
    constexpr char FILE_EXTENSION[] = ".cvef";
    constexpr char BACKUP_SUFFIX[] = ".bak";
}

// Error codes
enum class FileError {
    None,
    FileNotFound,
    AccessDenied,
    InvalidFormat,
    VersionMismatch,
    CorruptedData,
    CompressionError,
    WriteError,
    ReadError,
    OutOfMemory,
    DiskFull
};

// File operation result
struct FileResult {
    bool success = false;
    FileError error = FileError::None;
    std::string message;
    
    static FileResult Success() {
        return FileResult{true, FileError::None, ""};
    }
    
    static FileResult Error(FileError err, const std::string& msg) {
        return FileResult{false, err, msg};
    }
};

} // namespace FileIO
} // namespace VoxelEditor