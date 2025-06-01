#pragma once

#include "FileTypes.h"
#include "Project.h"
#include "BinaryFormat.h"
#include "STLExporter.h"
#include "FileVersioning.h"
#include "Compression.h"
#include <memory>
#include <string>
#include <vector>
#include <chrono>
#include <thread>
#include <mutex>

namespace VoxelEditor {
namespace FileIO {

// Main file management interface
class FileManager {
public:
    FileManager();
    ~FileManager();
    
    // Project operations
    FileResult saveProject(const std::string& filename, const Project& project,
                          const SaveOptions& options = SaveOptions::Default());
    FileResult loadProject(const std::string& filename, Project& project,
                          const LoadOptions& options = LoadOptions::Default());
    bool hasProject(const std::string& filename) const;
    
    // Export operations
    FileResult exportSTL(const std::string& filename, const Rendering::Mesh& mesh,
                        const STLExportOptions& options = STLExportOptions::Default());
    FileResult exportMultiSTL(const std::string& filename, const std::vector<Rendering::Mesh>& meshes,
                             const STLExportOptions& options = STLExportOptions::Default());
    
    // File information
    FileInfo getFileInfo(const std::string& filename) const;
    std::vector<std::string> getRecentFiles() const;
    void addToRecentFiles(const std::string& filename);
    void clearRecentFiles();
    
    // Format validation
    bool isValidProjectFile(const std::string& filename) const;
    FileVersion getFileVersion(const std::string& filename) const;
    bool canUpgradeFile(const std::string& filename) const;
    FileResult upgradeFile(const std::string& filename, const std::string& outputFilename);
    
    // Progress callbacks
    void setProgressCallback(ProgressCallback callback) { m_progressCallback = callback; }
    void setSaveCompleteCallback(SaveCompleteCallback callback) { m_saveCompleteCallback = callback; }
    void setLoadCompleteCallback(LoadCompleteCallback callback) { m_loadCompleteCallback = callback; }
    
    // Configuration
    void setCompressionEnabled(bool enabled) { m_compressionEnabled = enabled; }
    bool isCompressionEnabled() const { return m_compressionEnabled; }
    void setCompressionLevel(int level) { m_compressionLevel = level; }
    int getCompressionLevel() const { return m_compressionLevel; }
    void setBackupEnabled(bool enabled) { m_backupEnabled = enabled; }
    bool isBackupEnabled() const { return m_backupEnabled; }
    void setAutoSaveEnabled(bool enabled, float intervalSeconds = 300.0f);
    bool isAutoSaveEnabled() const { return m_autoSaveEnabled; }
    float getAutoSaveInterval() const { return m_autoSaveInterval; }
    
    // Auto-save management
    void registerProjectForAutoSave(const std::string& filename, Project* project);
    void unregisterProjectFromAutoSave(const std::string& filename);
    void triggerAutoSave();
    void updateAutoSave(float deltaTime);
    
    // Backup management
    std::string getBackupFilename(const std::string& originalFilename) const;
    std::vector<std::string> getBackupFiles(const std::string& filename) const;
    bool restoreFromBackup(const std::string& backupFilename, const std::string& targetFilename);
    void setMaxBackupCount(int count) { m_maxBackupCount = count; }
    int getMaxBackupCount() const { return m_maxBackupCount; }
    
    // Statistics
    struct IOStats {
        size_t totalBytesRead = 0;
        size_t totalBytesWritten = 0;
        size_t filesLoaded = 0;
        size_t filesSaved = 0;
        float averageLoadTime = 0.0f;
        float averageSaveTime = 0.0f;
        float compressionRatio = 1.0f;
    };
    
    IOStats getStatistics() const { return m_stats; }
    void resetStatistics() { m_stats = IOStats(); }
    
private:
    // Components
    std::unique_ptr<BinaryFormat> m_binaryFormat;
    std::unique_ptr<STLExporter> m_stlExporter;
    std::unique_ptr<FileVersioning> m_versioning;
    std::unique_ptr<Compression> m_compression;
    
    // Callbacks
    ProgressCallback m_progressCallback;
    SaveCompleteCallback m_saveCompleteCallback;
    LoadCompleteCallback m_loadCompleteCallback;
    
    // Configuration
    bool m_compressionEnabled = true;
    int m_compressionLevel = 6;
    bool m_backupEnabled = true;
    bool m_autoSaveEnabled = false;
    float m_autoSaveInterval = 300.0f; // 5 minutes
    int m_maxBackupCount = 5;
    std::string m_backupSuffix = ".bak";
    
    // Recent files
    std::vector<std::string> m_recentFiles;
    static constexpr size_t MAX_RECENT_FILES = 10;
    
    // Auto-save data
    struct AutoSaveEntry {
        std::string filename;
        Project* project;
        float timeSinceLastSave = 0.0f;
        bool needsSave = false;
        std::chrono::steady_clock::time_point lastModified;
    };
    
    std::vector<AutoSaveEntry> m_autoSaveEntries;
    std::mutex m_autoSaveMutex;
    std::thread m_autoSaveThread;
    bool m_autoSaveThreadRunning = false;
    
    // Statistics
    mutable IOStats m_stats;
    
    // Internal operations
    FileResult saveProjectInternal(const std::string& filename, const Project& project,
                                  const SaveOptions& options);
    FileResult loadProjectInternal(const std::string& filename, Project& project,
                                  const LoadOptions& options);
    
    // Backup operations
    bool createBackup(const std::string& filename);
    void cleanupOldBackups(const std::string& filename);
    std::vector<std::string> findBackupFiles(const std::string& filename) const;
    
    // Auto-save operations
    void autoSaveThreadFunc();
    void performAutoSave(AutoSaveEntry& entry);
    std::string getAutoSaveFilename(const std::string& originalFilename) const;
    
    // Progress reporting
    void reportProgress(float progress, const std::string& message);
    void reportSaveComplete(bool success, const std::string& filename);
    void reportLoadComplete(bool success, const std::string& filename);
    
    // File operations
    bool fileExists(const std::string& filename) const;
    size_t getFileSize(const std::string& filename) const;
    std::chrono::system_clock::time_point getFileModificationTime(const std::string& filename) const;
    bool isFileWritable(const std::string& filename) const;
    bool ensureDirectoryExists(const std::string& path) const;
    std::string getDirectory(const std::string& filepath) const;
    std::string getFilename(const std::string& filepath) const;
    std::string getExtension(const std::string& filepath) const;
    
    // Recent files management
    void loadRecentFiles();
    void saveRecentFiles();
    void addToRecentFilesInternal(const std::string& filename);
    
    // Statistics tracking
    void updateLoadStats(size_t bytesRead, float loadTime);
    void updateSaveStats(size_t bytesWritten, float saveTime, float compressionRatio);
};

// Global file manager instance
class FileManagerInstance {
public:
    static FileManager& getInstance();
    static void destroy();
    
private:
    static std::unique_ptr<FileManager> s_instance;
    static std::mutex s_mutex;
};

} // namespace FileIO
} // namespace VoxelEditor