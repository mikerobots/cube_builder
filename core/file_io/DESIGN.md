# File I/O System Subsystem

## Purpose
Handles project save/load operations using custom binary format, STL export functionality, file versioning, and compression for efficient storage.

## Current Implementation Status
The implementation closely matches the design with most components properly implemented:
- **FileManager** - Fully implemented with auto-save and backup functionality integrated
- **BinaryFormat** - Implemented as specified
- **STLExporter** - Implemented for mesh export
- **FileVersioning** - Implemented for version management
- **Compression** - Implemented with LZ4 support
- **BinaryIO** - Contains BinaryReader/BinaryWriter utilities
- **FileTypes** - Contains options and type definitions
- **Project** - Project structure definition

Missing separate components (functionality integrated into FileManager):
- **AutoSave** - Implemented within FileManager rather than as separate class
- **FileValidator** - Validation functionality not visible as separate component

## Key Components

### FileManager
**Responsibility**: Main interface for file operations
- Coordinate save/load operations
- Manage file format versioning
- Handle compression and decompression
- Provide progress callbacks for large files

### BinaryFormat
**Responsibility**: Custom binary format implementation
- Efficient serialization of voxel data
- Multi-resolution data storage
- Metadata and settings persistence
- Forward/backward compatibility

### STLExporter
**Responsibility**: STL file export functionality
- Convert mesh data to STL format
- ASCII and binary STL support
- Watertight mesh validation
- Scale and unit conversion

### FileVersioning
**Responsibility**: File format version management
- Version detection and upgrade
- Migration between format versions
- Deprecation warnings
- Compatibility validation

### Compression
**Responsibility**: LZ4 compression wrapper
- Fast compression for real-time saves
- Sparse data optimization
- Incremental compression
- Memory-efficient streaming

## Interface Design

```cpp
class FileManager {
public:
    // Project operations
    bool saveProject(const std::string& filename, const Project& project, const SaveOptions& options = SaveOptions::Default());
    bool loadProject(const std::string& filename, Project& project, LoadOptions& options = LoadOptions::Default());
    bool hasProject(const std::string& filename) const;
    
    // Export operations
    bool exportSTL(const std::string& filename, const Mesh& mesh, const STLExportOptions& options = STLExportOptions::Default());
    bool exportMultiSTL(const std::string& filename, const std::vector<Mesh>& meshes, const STLExportOptions& options);
    
    // File information
    FileInfo getFileInfo(const std::string& filename) const;
    std::vector<std::string> getRecentFiles() const;
    void addToRecentFiles(const std::string& filename);
    
    // Format validation
    bool isValidProjectFile(const std::string& filename) const;
    FileVersion getFileVersion(const std::string& filename) const;
    bool canUpgradeFile(const std::string& filename) const;
    
    // Progress callbacks
    void setProgressCallback(ProgressCallback callback);
    void setSaveCompleteCallback(SaveCompleteCallback callback);
    void setLoadCompleteCallback(LoadCompleteCallback callback);
    
    // Configuration
    void setCompressionEnabled(bool enabled);
    void setCompressionLevel(int level);
    void setBackupEnabled(bool enabled);
    void setAutoSaveEnabled(bool enabled, float intervalSeconds = 300.0f);
    
private:
    std::unique_ptr<BinaryFormat> m_binaryFormat;
    std::unique_ptr<STLExporter> m_stlExporter;
    std::unique_ptr<FileVersioning> m_versioning;
    std::unique_ptr<Compression> m_compression;
    
    ProgressCallback m_progressCallback;
    SaveCompleteCallback m_saveCompleteCallback;
    LoadCompleteCallback m_loadCompleteCallback;
    
    std::vector<std::string> m_recentFiles;
    bool m_compressionEnabled;
    int m_compressionLevel;
    bool m_backupEnabled;
    bool m_autoSaveEnabled;
    float m_autoSaveInterval;
    
    bool createBackup(const std::string& filename);
    void cleanupOldBackups(const std::string& filename);
    void triggerAutoSave();
};
```

## Data Structures

### Project Structure
```cpp
struct Project {
    ProjectMetadata metadata;
    MultiResolutionGrid voxelData;
    GroupManager groupData;
    CameraState cameraState;
    SelectionSet currentSelection;
    std::vector<SelectionSet> namedSelections;
    WorkspaceSettings workspace;
    RenderSettings renderSettings;
    
    // Validation
    bool isValid() const;
    size_t calculateSize() const;
    void clear();
};

struct ProjectMetadata {
    std::string name;
    std::string description;
    std::string author;
    std::chrono::system_clock::time_point created;
    std::chrono::system_clock::time_point modified;
    FileVersion version;
    std::string application;
    std::string applicationVersion;
    std::unordered_map<std::string, std::string> customProperties;
};

struct WorkspaceSettings {
    Vector3f size = Vector3f(5.0f, 5.0f, 5.0f);
    Vector3f origin = Vector3f::Zero();
    VoxelResolution defaultResolution = VoxelResolution::Size_1cm;
    bool gridVisible = true;
    bool axesVisible = true;
    Color backgroundColor = Color::Gray();
};
```

### File Options
```cpp
struct SaveOptions {
    bool compress = true;
    int compressionLevel = 6;
    bool includeHistory = false;
    bool includeCache = false;
    bool createBackup = true;
    bool validateBeforeSave = true;
    
    static SaveOptions Default();
    static SaveOptions Fast();
    static SaveOptions Compact();
    static SaveOptions Development();
};

struct LoadOptions {
    bool loadHistory = false;
    bool loadCache = false;
    bool validateAfterLoad = true;
    bool upgradeVersion = true;
    bool ignoreVersionMismatch = false;
    
    static LoadOptions Default();
    static LoadOptions Fast();
    static LoadOptions Safe();
};

struct STLExportOptions {
    STLFormat format = STLFormat::Binary;
    STLUnits units = STLUnits::Millimeters;
    float scale = 1.0f;
    bool mergeMeshes = true;
    bool validateWatertight = true;
    Vector3f translation = Vector3f::Zero();
    
    static STLExportOptions Default();
    static STLExportOptions Printing3D();
    static STLExportOptions CAD();
};
```

## Binary Format Implementation

### File Structure
```
Custom Voxel Editor File Format (.cvef)

Header (256 bytes):
- Magic number (4 bytes): "CVEF"
- Version (4 bytes): Major.Minor.Patch.Build
- File size (8 bytes)
- Compression flags (4 bytes)
- Checksum (8 bytes)
- Reserved (228 bytes)

Chunks:
- Metadata chunk
- Voxel data chunk (per resolution)
- Group data chunk
- Camera state chunk
- Selection data chunk
- Settings chunk
- Custom data chunks
```

### BinaryFormat
```cpp
class BinaryFormat {
public:
    bool writeProject(BinaryWriter& writer, const Project& project, const SaveOptions& options);
    bool readProject(BinaryReader& reader, Project& project, const LoadOptions& options);
    
    // Chunk operations
    bool writeChunk(BinaryWriter& writer, ChunkType type, const void* data, size_t size);
    bool readChunk(BinaryReader& reader, ChunkType& type, std::vector<uint8_t>& data);
    
    // Validation
    bool validateFile(BinaryReader& reader);
    uint64_t calculateChecksum(BinaryReader& reader);
    
private:
    struct FileHeader {
        char magic[4] = {'C', 'V', 'E', 'F'};
        FileVersion version;
        uint64_t fileSize;
        uint32_t compressionFlags;
        uint64_t checksum;
        uint8_t reserved[228] = {};
        
        bool isValid() const;
        void updateChecksum(const uint8_t* data, size_t size);
    };
    
    struct ChunkHeader {
        ChunkType type;
        uint32_t size;
        uint32_t uncompressedSize;
        uint32_t checksum;
    };
    
    enum class ChunkType : uint32_t {
        Metadata = 0x4D455441,        // 'META'
        VoxelData = 0x564F5845,       // 'VOXE'
        GroupData = 0x47525550,       // 'GRUP'
        CameraState = 0x43414D45,     // 'CAME'
        SelectionData = 0x53454C45,   // 'SELE'
        Settings = 0x53455454,        // 'SETT'
        CustomData = 0x43555354       // 'CUST'
    };
    
    bool writeHeader(BinaryWriter& writer, const FileHeader& header);
    bool readHeader(BinaryReader& reader, FileHeader& header);
    bool writeVoxelDataChunk(BinaryWriter& writer, const MultiResolutionGrid& grid, const SaveOptions& options);
    bool readVoxelDataChunk(BinaryReader& reader, MultiResolutionGrid& grid, const LoadOptions& options);
};
```

### Binary I/O Utilities
```cpp
class BinaryWriter {
public:
    BinaryWriter(std::ostream& stream);
    
    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeString(const std::string& str);
    void writeVector3f(const Vector3f& vec);
    void writeMatrix4f(const Matrix4f& mat);
    void writeBytes(const void* data, size_t size);
    
    size_t getBytesWritten() const;
    bool isValid() const;
    
private:
    std::ostream& m_stream;
    size_t m_bytesWritten;
    bool m_valid;
};

class BinaryReader {
public:
    BinaryReader(std::istream& stream);
    
    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();
    float readFloat();
    double readDouble();
    std::string readString();
    Vector3f readVector3f();
    Matrix4f readMatrix4f();
    void readBytes(void* data, size_t size);
    
    size_t getBytesRead() const;
    bool isValid() const;
    bool isAtEnd() const;
    
private:
    std::istream& m_stream;
    size_t m_bytesRead;
    bool m_valid;
};
```

## STL Export Implementation

### STLExporter
```cpp
class STLExporter {
public:
    bool exportMesh(const std::string& filename, const Mesh& mesh, const STLExportOptions& options);
    bool exportMeshes(const std::string& filename, const std::vector<Mesh>& meshes, const STLExportOptions& options);
    
    // Format-specific exports
    bool exportBinarySTL(const std::string& filename, const Mesh& mesh, const STLExportOptions& options);
    bool exportASCIISTL(const std::string& filename, const Mesh& mesh, const STLExportOptions& options);
    
    // Validation
    bool validateMesh(const Mesh& mesh, std::vector<std::string>& issues);
    bool isWatertight(const Mesh& mesh);
    Mesh repairMesh(const Mesh& mesh);
    
    // Statistics
    STLExportStats getLastExportStats() const;
    
private:
    STLExportStats m_lastStats;
    
    Mesh preprocessMesh(const Mesh& mesh, const STLExportOptions& options);
    Mesh scaleMesh(const Mesh& mesh, float scale);
    Mesh translateMesh(const Mesh& mesh, const Vector3f& translation);
    Mesh mergeMeshes(const std::vector<Mesh>& meshes);
    
    void writeBinaryTriangle(BinaryWriter& writer, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);
    void writeASCIITriangle(std::ostream& stream, const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);
    Vector3f calculateTriangleNormal(const Vector3f& v0, const Vector3f& v1, const Vector3f& v2);
};

struct STLExportStats {
    size_t triangleCount = 0;
    size_t vertexCount = 0;
    float exportTime = 0.0f;
    size_t fileSize = 0;
    bool watertight = false;
    std::vector<std::string> warnings;
};

enum class STLFormat {
    Binary,
    ASCII
};

enum class STLUnits {
    Millimeters,
    Centimeters,
    Meters,
    Inches
};
```

## File Versioning

### FileVersioning
```cpp
class FileVersioning {
public:
    FileVersion getCurrentVersion() const;
    FileVersion detectVersion(BinaryReader& reader);
    
    bool isCompatible(FileVersion version) const;
    bool canUpgrade(FileVersion from, FileVersion to) const;
    bool upgradeFile(const std::string& filename, FileVersion targetVersion);
    
    // Migration functions
    bool migrateV1ToV2(BinaryReader& reader, BinaryWriter& writer);
    bool migrateV2ToV3(BinaryReader& reader, BinaryWriter& writer);
    
    std::vector<std::string> getUpgradeWarnings(FileVersion from, FileVersion to) const;
    
private:
    using MigrationFunction = std::function<bool(BinaryReader&, BinaryWriter&)>;
    std::unordered_map<std::pair<FileVersion, FileVersion>, MigrationFunction> m_migrations;
    
    void registerMigrations();
    std::vector<FileVersion> findUpgradePath(FileVersion from, FileVersion to) const;
};

struct FileVersion {
    uint16_t major = 1;
    uint16_t minor = 0;
    uint16_t patch = 0;
    uint16_t build = 0;
    
    bool operator==(const FileVersion& other) const;
    bool operator<(const FileVersion& other) const;
    bool isCompatible(const FileVersion& other) const;
    std::string toString() const;
    
    static FileVersion fromString(const std::string& str);
    static FileVersion Current();
};
```

## Compression Implementation

### Compression
```cpp
class Compression {
public:
    bool compress(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& output, int level = 6);
    bool decompress(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& output, size_t expectedSize);
    
    size_t getMaxCompressedSize(size_t inputSize) const;
    float getCompressionRatio() const;
    
    // Streaming compression
    bool compressStream(std::istream& input, std::ostream& output, int level = 6);
    bool decompressStream(std::istream& input, std::ostream& output, size_t expectedSize);
    
    // Specialized compression for voxel data
    bool compressVoxelData(const VoxelGrid& grid, std::vector<uint8_t>& output, int level = 6);
    bool decompressVoxelData(const uint8_t* input, size_t inputSize, VoxelGrid& grid);
    
private:
    float m_lastCompressionRatio = 1.0f;
    
    // LZ4-specific compression
    bool compressLZ4(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& output, int level);
    bool decompressLZ4(const uint8_t* input, size_t inputSize, std::vector<uint8_t>& output, size_t expectedSize);
    
    // Voxel-specific optimization
    std::vector<uint8_t> optimizeVoxelData(const VoxelGrid& grid);
    VoxelGrid restoreVoxelData(const std::vector<uint8_t>& data);
};
```

## Auto-Save and Backup

### AutoSave
```cpp
class AutoSave {
public:
    void enable(bool enabled);
    void setInterval(float seconds);
    void setMaxBackups(int count);
    void setBackupDirectory(const std::string& directory);
    
    void registerProject(const std::string& filename, const Project* project);
    void unregisterProject(const std::string& filename);
    
    void update(float deltaTime);
    void forceSave();
    
    std::vector<std::string> getBackupFiles(const std::string& filename) const;
    bool restoreFromBackup(const std::string& backupFilename, const std::string& targetFilename);
    
private:
    struct AutoSaveEntry {
        std::string filename;
        const Project* project;
        float timeSinceLastSave = 0.0f;
        bool needsSave = false;
    };
    
    std::vector<AutoSaveEntry> m_entries;
    bool m_enabled = false;
    float m_interval = 300.0f; // 5 minutes
    int m_maxBackups = 10;
    std::string m_backupDirectory;
    
    void performAutoSave(AutoSaveEntry& entry);
    std::string generateBackupFilename(const std::string& originalFilename) const;
    void cleanupOldBackups(const std::string& filename);
};
```

## Error Handling and Validation

### File Validation
```cpp
class FileValidator {
public:
    bool validateProjectFile(const std::string& filename, std::vector<std::string>& errors);
    bool validateProject(const Project& project, std::vector<std::string>& errors);
    bool validateVoxelData(const MultiResolutionGrid& grid, std::vector<std::string>& errors);
    bool validateGroupData(const GroupManager& groups, std::vector<std::string>& errors);
    
    // Repair functions
    bool repairProject(Project& project, std::vector<std::string>& repairActions);
    bool repairVoxelData(MultiResolutionGrid& grid, std::vector<std::string>& repairActions);
    
private:
    bool checkFileIntegrity(const std::string& filename);
    bool checkVoxelBounds(const MultiResolutionGrid& grid);
    bool checkGroupConsistency(const GroupManager& groups);
    bool checkSelectionValidity(const SelectionSet& selection, const MultiResolutionGrid& grid);
};
```

## Testing Strategy

### Unit Tests
- Binary format read/write correctness
- Compression/decompression validation
- STL export accuracy
- File version migration
- Error handling and recovery

### Integration Tests
- Full save/load roundtrip testing
- Cross-platform file compatibility
- Large file handling
- Memory usage optimization

### Performance Tests
- Save/load speed benchmarks
- Compression efficiency measurement
- Memory usage during I/O operations
- Auto-save performance impact

### Stress Tests
- Very large projects
- Corrupted file handling
- Disk space limitations
- Concurrent file access

## Dependencies
- **Core/VoxelData**: Voxel grid serialization
- **Core/Groups**: Group data serialization
- **Core/Selection**: Selection set serialization
- **Core/Camera**: Camera state serialization
- **Foundation/Math**: Vector/matrix serialization
- **External/LZ4**: Compression library

## Platform Considerations

### File System Integration
- Native file dialogs
- File association registration
- Thumbnail generation
- Recent files integration

### Performance Optimization
- Memory-mapped files for large projects
- Background I/O for auto-save
- Incremental saves for large changes
- Async loading with progress indication

## Known Issues and Technical Debt

### Issue 1: Missing File Validation Component
- **Severity**: Medium
- **Impact**: No dedicated validation system for checking file integrity and repairing corrupted files
- **Proposed Solution**: Implement FileValidator class as designed or add validation methods to FileManager
- **Dependencies**: None

### Issue 2: AutoSave Threading Concerns
- **Severity**: High
- **Impact**: Auto-save uses std::thread but lacks proper synchronization for Project access, potential race conditions
- **Proposed Solution**: Implement proper thread-safe project snapshotting or use async I/O instead
- **Dependencies**: Thread-safe Project design

### Issue 3: Global Singleton Pattern
- **Severity**: Low
- **Impact**: FileManagerInstance uses singleton pattern, making testing harder and creating global state
- **Proposed Solution**: Use dependency injection instead of singleton
- **Dependencies**: Application architecture refactoring

### Issue 4: Missing Streaming Support
- **Severity**: Medium
- **Impact**: Large files must be loaded entirely into memory, limiting scalability
- **Proposed Solution**: Implement streaming I/O for large voxel grids
- **Dependencies**: VoxelData streaming support

### Issue 5: No Format Migration Implementation
- **Severity**: Medium
- **Impact**: FileVersioning interface exists but no actual migration functions implemented
- **Proposed Solution**: Implement version migration functions as files evolve
- **Dependencies**: Format stability

### Issue 6: Hardcoded File Extension
- **Severity**: Low
- **Impact**: File format uses ".cvef" extension but it's not clearly defined or registered
- **Proposed Solution**: Define file extension constants and implement proper file type registration
- **Dependencies**: Platform-specific file association APIs

### Issue 7: Progress Callback Threading
- **Severity**: Medium
- **Impact**: Progress callbacks may be called from background threads without synchronization
- **Proposed Solution**: Document thread safety requirements or provide thread-safe callback wrapper
- **Dependencies**: UI thread synchronization

### Issue 8: Memory Management for Large Projects
- **Severity**: High
- **Impact**: No memory limits or streaming for very large projects (approaching VR memory constraints)
- **Proposed Solution**: Implement memory-aware loading with partial project support
- **Dependencies**: VoxelData memory management improvements