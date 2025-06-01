#pragma once

#include "FileTypes.h"
#include "BinaryIO.h"
#include <functional>
#include <unordered_map>
#include <vector>

namespace VoxelEditor {
namespace FileIO {

// File version management and migration
class FileVersioning {
public:
    FileVersioning();
    ~FileVersioning();
    
    // Version information
    FileVersion getCurrentVersion() const;
    FileVersion detectVersion(BinaryReader& reader);
    
    // Compatibility checks
    bool isCompatible(FileVersion version) const;
    bool canUpgrade(FileVersion from, FileVersion to) const;
    bool needsUpgrade(FileVersion version) const;
    
    // Version migration
    bool upgradeFile(const std::string& inputFile, const std::string& outputFile, 
                    FileVersion targetVersion = FileVersion::Current());
    bool migrateData(BinaryReader& reader, BinaryWriter& writer,
                    FileVersion fromVersion, FileVersion toVersion);
    
    // Migration information
    std::vector<std::string> getUpgradeWarnings(FileVersion from, FileVersion to) const;
    std::vector<std::string> getBreakingChanges(FileVersion from, FileVersion to) const;
    std::string getMigrationNotes(FileVersion from, FileVersion to) const;
    
    // Version history
    std::vector<FileVersion> getVersionHistory() const;
    std::string getVersionChangelog(FileVersion version) const;
    
private:
    // Migration function type
    using MigrationFunction = std::function<bool(BinaryReader&, BinaryWriter&)>;
    
    // Version pair for migration mapping
    struct VersionPair {
        FileVersion from;
        FileVersion to;
        
        bool operator==(const VersionPair& other) const {
            return from == other.from && to == other.to;
        }
    };
    
    // Hash function for VersionPair
    struct VersionPairHash {
        size_t operator()(const VersionPair& pair) const {
            size_t h1 = std::hash<uint64_t>{}(
                (uint64_t(pair.from.major) << 48) |
                (uint64_t(pair.from.minor) << 32) |
                (uint64_t(pair.from.patch) << 16) |
                uint64_t(pair.from.build)
            );
            size_t h2 = std::hash<uint64_t>{}(
                (uint64_t(pair.to.major) << 48) |
                (uint64_t(pair.to.minor) << 32) |
                (uint64_t(pair.to.patch) << 16) |
                uint64_t(pair.to.build)
            );
            return h1 ^ (h2 << 1);
        }
    };
    
    // Migration registry
    std::unordered_map<VersionPair, MigrationFunction, VersionPairHash> m_migrations;
    std::unordered_map<VersionPair, std::vector<std::string>, VersionPairHash> m_migrationWarnings;
    std::unordered_map<FileVersion, std::string> m_versionChangelogs;
    
    // Register all migrations
    void registerMigrations();
    
    // Specific version migrations
    bool migrateV1_0ToV1_1(BinaryReader& reader, BinaryWriter& writer);
    bool migrateV1_1ToV1_2(BinaryReader& reader, BinaryWriter& writer);
    bool migrateV1_2ToV2_0(BinaryReader& reader, BinaryWriter& writer);
    
    // Helper functions
    std::vector<FileVersion> findUpgradePath(FileVersion from, FileVersion to) const;
    bool executeMigrationPath(BinaryReader& reader, BinaryWriter& writer,
                            const std::vector<FileVersion>& path);
    
    // Data migration helpers
    bool migrateMetadata(BinaryReader& reader, BinaryWriter& writer,
                        FileVersion from, FileVersion to);
    bool migrateVoxelData(BinaryReader& reader, BinaryWriter& writer,
                         FileVersion from, FileVersion to);
    bool migrateGroupData(BinaryReader& reader, BinaryWriter& writer,
                         FileVersion from, FileVersion to);
    
    // Version-specific readers for old formats
    bool readV1_0Header(BinaryReader& reader, FileHeader& header);
    bool readV1_0VoxelData(BinaryReader& reader, std::vector<uint8_t>& data);
};

// Version compatibility rules
class VersionCompatibility {
public:
    // Check if versions are compatible for reading
    static bool canRead(FileVersion fileVersion, FileVersion appVersion);
    
    // Check if versions are compatible for writing
    static bool canWrite(FileVersion fileVersion, FileVersion appVersion);
    
    // Get minimum compatible version
    static FileVersion getMinimumCompatibleVersion(FileVersion version);
    
    // Get recommended version for saving
    static FileVersion getRecommendedSaveVersion();
    
private:
    // Compatibility rules
    static bool isMajorCompatible(FileVersion v1, FileVersion v2);
    static bool isMinorCompatible(FileVersion v1, FileVersion v2);
};

} // namespace FileIO
} // namespace VoxelEditor