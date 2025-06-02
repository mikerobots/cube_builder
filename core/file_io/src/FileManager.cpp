#include "../include/file_io/FileManager.h"
#include "../include/file_io/BinaryIO.h"
#include "logging/Logger.h"
#include <fstream>
#include <filesystem>
#include <algorithm>
#include <chrono>

namespace VoxelEditor {
namespace FileIO {

// FileManager implementation
FileManager::FileManager() 
    : m_binaryFormat(std::make_unique<BinaryFormat>())
    , m_stlExporter(std::make_unique<STLExporter>())
    , m_versioning(std::make_unique<FileVersioning>())
    , m_compression(std::make_unique<Compression>()) {
    loadRecentFiles();
}

FileManager::~FileManager() {
    if (m_autoSaveThreadRunning) {
        m_autoSaveThreadRunning = false;
        if (m_autoSaveThread.joinable()) {
            m_autoSaveThread.join();
        }
    }
    saveRecentFiles();
}

FileResult FileManager::saveProject(const std::string& filename, const Project& project,
                                  const SaveOptions& options) {
    auto startTime = std::chrono::steady_clock::now();
    
    reportProgress(0.0f, "Starting save...");
    
    if (m_backupEnabled && fileExists(filename)) {
        createBackup(filename);
    }
    
    FileResult result = saveProjectInternal(filename, project, options);
    
    if (result.success) {
        addToRecentFiles(filename);
        auto endTime = std::chrono::steady_clock::now();
        float saveTime = std::chrono::duration<float>(endTime - startTime).count();
        updateSaveStats(getFileSize(filename), saveTime, 1.0f); // TODO: Calculate actual compression ratio
        reportSaveComplete(true, filename);
    } else {
        reportSaveComplete(false, filename);
    }
    
    return result;
}

FileResult FileManager::loadProject(const std::string& filename, Project& project,
                                  const LoadOptions& options) {
    auto startTime = std::chrono::steady_clock::now();
    
    reportProgress(0.0f, "Starting load...");
    
    FileResult result = loadProjectInternal(filename, project, options);
    
    if (result.success) {
        addToRecentFiles(filename);
        auto endTime = std::chrono::steady_clock::now();
        float loadTime = std::chrono::duration<float>(endTime - startTime).count();
        updateLoadStats(getFileSize(filename), loadTime);
        reportLoadComplete(true, filename);
    } else {
        reportLoadComplete(false, filename);
    }
    
    return result;
}

bool FileManager::hasProject(const std::string& filename) const {
    return fileExists(filename) && isValidProjectFile(filename);
}

FileResult FileManager::exportSTL(const std::string& filename, const Rendering::Mesh& mesh,
                                const STLExportOptions& options) {
    try {
        reportProgress(0.0f, "Exporting STL...");
        
        if (!m_stlExporter->exportMesh(filename, mesh, options)) {
            return FileResult::Error(m_stlExporter->getLastError(), 
                                   m_stlExporter->getLastErrorMessage());
        }
        
        reportProgress(1.0f, "STL export complete");
        return FileResult::Success();
    } catch (const std::exception& e) {
        return FileResult::Error(FileError::WriteError, e.what());
    }
}

FileResult FileManager::exportMultiSTL(const std::string& filename, 
                                     const std::vector<Rendering::Mesh>& meshes,
                                     const STLExportOptions& options) {
    try {
        reportProgress(0.0f, "Exporting STL meshes...");
        
        if (!m_stlExporter->exportMeshes(filename, meshes, options)) {
            return FileResult::Error(m_stlExporter->getLastError(), 
                                   m_stlExporter->getLastErrorMessage());
        }
        
        reportProgress(1.0f, "STL export complete");
        return FileResult::Success();
    } catch (const std::exception& e) {
        return FileResult::Error(FileError::WriteError, e.what());
    }
}

FileInfo FileManager::getFileInfo(const std::string& filename) const {
    FileInfo info;
    info.filename = getFilename(filename);
    info.path = getDirectory(filename);
    info.fileSize = getFileSize(filename);
    info.lastModified = getFileModificationTime(filename);
    
    if (isValidProjectFile(filename)) {
        info.version = getFileVersion(filename);
        
        // Check if compressed
        std::ifstream file(filename, std::ios::binary);
        if (file.is_open()) {
            BinaryReader reader(file);
            FileHeader header;
            if (m_binaryFormat->readHeader(reader, header)) {
                info.compressed = (header.compressionFlags != 0);
            }
        }
    }
    
    info.readonly = !isFileWritable(filename);
    return info;
}

std::vector<std::string> FileManager::getRecentFiles() const {
    return m_recentFiles;
}

void FileManager::addToRecentFiles(const std::string& filename) {
    addToRecentFilesInternal(filename);
    saveRecentFiles();
}

void FileManager::clearRecentFiles() {
    m_recentFiles.clear();
    saveRecentFiles();
}

bool FileManager::isValidProjectFile(const std::string& filename) const {
    if (!fileExists(filename)) {
        return false;
    }
    
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    BinaryReader reader(file);
    return m_binaryFormat->validateFile(reader);
}

FileVersion FileManager::getFileVersion(const std::string& filename) const {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return FileVersion{0, 0, 0};
    }
    
    BinaryReader reader(file);
    return m_binaryFormat->detectVersion(reader);
}

bool FileManager::canUpgradeFile(const std::string& filename) const {
    if (!isValidProjectFile(filename)) {
        return false;
    }
    
    FileVersion fileVersion = getFileVersion(filename);
    return m_versioning->canUpgrade(fileVersion, FileVersion::Current());
}

FileResult FileManager::upgradeFile(const std::string& filename, 
                                  const std::string& outputFilename) {
    try {
        Project project;
        LoadOptions loadOptions = LoadOptions::Default();
        loadOptions.upgradeVersion = true;
        
        FileResult loadResult = loadProjectInternal(filename, project, loadOptions);
        if (!loadResult.success) {
            return loadResult;
        }
        
        SaveOptions saveOptions = SaveOptions::Default();
        return saveProjectInternal(outputFilename, project, saveOptions);
    } catch (const std::exception& e) {
        return FileResult::Error(FileError::WriteError, e.what());
    }
}

void FileManager::setAutoSaveEnabled(bool enabled, float intervalSeconds) {
    if (m_autoSaveEnabled == enabled) {
        return;
    }
    
    m_autoSaveEnabled = enabled;
    m_autoSaveInterval = intervalSeconds;
    
    if (enabled && !m_autoSaveThreadRunning) {
        m_autoSaveThreadRunning = true;
        m_autoSaveThread = std::thread(&FileManager::autoSaveThreadFunc, this);
    } else if (!enabled && m_autoSaveThreadRunning) {
        m_autoSaveThreadRunning = false;
        if (m_autoSaveThread.joinable()) {
            m_autoSaveThread.join();
        }
    }
}

void FileManager::registerProjectForAutoSave(const std::string& filename, Project* project) {
    std::lock_guard<std::mutex> lock(m_autoSaveMutex);
    
    auto it = std::find_if(m_autoSaveEntries.begin(), m_autoSaveEntries.end(),
        [&filename](const AutoSaveEntry& entry) { return entry.filename == filename; });
    
    if (it == m_autoSaveEntries.end()) {
        AutoSaveEntry entry;
        entry.filename = filename;
        entry.project = project;
        entry.lastModified = std::chrono::steady_clock::now();
        m_autoSaveEntries.push_back(entry);
    } else {
        it->project = project;
        it->lastModified = std::chrono::steady_clock::now();
    }
}

void FileManager::unregisterProjectFromAutoSave(const std::string& filename) {
    std::lock_guard<std::mutex> lock(m_autoSaveMutex);
    
    auto it = std::remove_if(m_autoSaveEntries.begin(), m_autoSaveEntries.end(),
        [&filename](const AutoSaveEntry& entry) { return entry.filename == filename; });
    
    m_autoSaveEntries.erase(it, m_autoSaveEntries.end());
}

void FileManager::triggerAutoSave() {
    std::lock_guard<std::mutex> lock(m_autoSaveMutex);
    
    for (auto& entry : m_autoSaveEntries) {
        entry.needsSave = true;
    }
}

void FileManager::updateAutoSave(float deltaTime) {
    if (!m_autoSaveEnabled) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(m_autoSaveMutex);
    
    for (auto& entry : m_autoSaveEntries) {
        entry.timeSinceLastSave += deltaTime;
        
        if (entry.timeSinceLastSave >= m_autoSaveInterval) {
            entry.needsSave = true;
        }
    }
}

std::string FileManager::getBackupFilename(const std::string& originalFilename) const {
    auto now = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        now.time_since_epoch()).count();
    
    std::string base = originalFilename.substr(0, originalFilename.find_last_of('.'));
    std::string ext = getExtension(originalFilename);
    
    return base + "_" + std::to_string(timestamp) + m_backupSuffix + ext;
}

std::vector<std::string> FileManager::getBackupFiles(const std::string& filename) const {
    return findBackupFiles(filename);
}

bool FileManager::restoreFromBackup(const std::string& backupFilename, 
                                  const std::string& targetFilename) {
    try {
        std::filesystem::copy(backupFilename, targetFilename, 
                            std::filesystem::copy_options::overwrite_existing);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to restore from backup: " + std::string(e.what()));
        return false;
    }
}

// Private methods
FileResult FileManager::saveProjectInternal(const std::string& filename, 
                                          const Project& project,
                                          const SaveOptions& options) {
    try {
        ensureDirectoryExists(getDirectory(filename));
        
        std::ofstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return FileResult::Error(FileError::AccessDenied, 
                                   "Cannot open file for writing");
        }
        
        BinaryWriter writer(file);
        
        reportProgress(0.1f, "Writing project data...");
        
        if (!m_binaryFormat->writeProject(writer, project, options)) {
            return FileResult::Error(m_binaryFormat->getLastError(), 
                                   m_binaryFormat->getLastErrorMessage());
        }
        
        reportProgress(1.0f, "Save complete");
        
        return FileResult::Success();
    } catch (const std::exception& e) {
        return FileResult::Error(FileError::WriteError, e.what());
    }
}

FileResult FileManager::loadProjectInternal(const std::string& filename, 
                                          Project& project,
                                          const LoadOptions& options) {
    try {
        if (!fileExists(filename)) {
            return FileResult::Error(FileError::FileNotFound, 
                                   "File not found: " + filename);
        }
        
        std::ifstream file(filename, std::ios::binary);
        if (!file.is_open()) {
            return FileResult::Error(FileError::AccessDenied, 
                                   "Cannot open file for reading");
        }
        
        BinaryReader reader(file);
        
        reportProgress(0.1f, "Reading project data...");
        
        if (!m_binaryFormat->readProject(reader, project, options)) {
            return FileResult::Error(m_binaryFormat->getLastError(), 
                                   m_binaryFormat->getLastErrorMessage());
        }
        
        if (options.validateAfterLoad) {
            reportProgress(0.9f, "Validating project...");
            ProjectValidator validator;
            std::vector<std::string> errors;
            if (!validator.validate(project, errors)) {
                return FileResult::Error(FileError::CorruptedData, 
                                       "Project validation failed");
            }
        }
        
        reportProgress(1.0f, "Load complete");
        
        return FileResult::Success();
    } catch (const std::exception& e) {
        return FileResult::Error(FileError::ReadError, e.what());
    }
}

bool FileManager::createBackup(const std::string& filename) {
    try {
        std::string backupFilename = getBackupFilename(filename);
        std::filesystem::copy(filename, backupFilename);
        
        cleanupOldBackups(filename);
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to create backup: " + std::string(e.what()));
        return false;
    }
}

void FileManager::cleanupOldBackups(const std::string& filename) {
    auto backups = findBackupFiles(filename);
    
    if (backups.size() > m_maxBackupCount) {
        // Sort by modification time
        std::sort(backups.begin(), backups.end(), 
            [this](const std::string& a, const std::string& b) {
                return getFileModificationTime(a) < getFileModificationTime(b);
            });
        
        // Remove oldest backups
        size_t toRemove = backups.size() - m_maxBackupCount;
        for (size_t i = 0; i < toRemove; i++) {
            try {
                std::filesystem::remove(backups[i]);
            } catch (const std::exception& e) {
                LOG_ERROR("Failed to remove old backup: " + std::string(e.what()));
            }
        }
    }
}

std::vector<std::string> FileManager::findBackupFiles(const std::string& filename) const {
    std::vector<std::string> backups;
    
    std::string base = filename.substr(0, filename.find_last_of('.'));
    std::string ext = getExtension(filename);
    std::string dir = getDirectory(filename);
    
    try {
        for (const auto& entry : std::filesystem::directory_iterator(dir)) {
            if (entry.is_regular_file()) {
                std::string file = entry.path().filename().string();
                if (file.find(base) == 0 && file.find(m_backupSuffix) != std::string::npos) {
                    backups.push_back(entry.path().string());
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to find backup files: " + std::string(e.what()));
    }
    
    return backups;
}

void FileManager::autoSaveThreadFunc() {
    while (m_autoSaveThreadRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
        
        std::lock_guard<std::mutex> lock(m_autoSaveMutex);
        
        for (auto& entry : m_autoSaveEntries) {
            if (entry.needsSave && entry.project) {
                performAutoSave(entry);
            }
        }
    }
}

void FileManager::performAutoSave(AutoSaveEntry& entry) {
    std::string autoSaveFile = getAutoSaveFilename(entry.filename);
    
    SaveOptions options = SaveOptions::Fast();
    FileResult result = saveProjectInternal(autoSaveFile, *entry.project, options);
    
    if (result.success) {
        entry.timeSinceLastSave = 0.0f;
        entry.needsSave = false;
        entry.lastModified = std::chrono::steady_clock::now();
        LOG_INFO("Auto-saved: " + autoSaveFile);
    } else {
        LOG_ERROR("Auto-save failed: " + result.message);
    }
}

std::string FileManager::getAutoSaveFilename(const std::string& originalFilename) const {
    std::string base = originalFilename.substr(0, originalFilename.find_last_of('.'));
    std::string ext = getExtension(originalFilename);
    return base + ".autosave" + ext;
}

void FileManager::reportProgress(float progress, const std::string& message) {
    if (m_progressCallback) {
        m_progressCallback(progress, message);
    }
}

void FileManager::reportSaveComplete(bool success, const std::string& filename) {
    if (m_saveCompleteCallback) {
        m_saveCompleteCallback(success, filename);
    }
}

void FileManager::reportLoadComplete(bool success, const std::string& filename) {
    if (m_loadCompleteCallback) {
        m_loadCompleteCallback(success, filename);
    }
}

bool FileManager::fileExists(const std::string& filename) const {
    return std::filesystem::exists(filename);
}

size_t FileManager::getFileSize(const std::string& filename) const {
    try {
        return std::filesystem::file_size(filename);
    } catch (const std::exception&) {
        return 0;
    }
}

std::chrono::system_clock::time_point FileManager::getFileModificationTime(
    const std::string& filename) const {
    try {
        auto ftime = std::filesystem::last_write_time(filename);
        return std::chrono::time_point_cast<std::chrono::system_clock::duration>(
            ftime - std::filesystem::file_time_type::clock::now() + 
            std::chrono::system_clock::now());
    } catch (const std::exception&) {
        return std::chrono::system_clock::time_point();
    }
}

bool FileManager::isFileWritable(const std::string& filename) const {
    try {
        auto status = std::filesystem::status(filename);
        return (status.permissions() & std::filesystem::perms::owner_write) != 
               std::filesystem::perms::none;
    } catch (const std::exception&) {
        return false;
    }
}

bool FileManager::ensureDirectoryExists(const std::string& path) const {
    try {
        return std::filesystem::create_directories(path);
    } catch (const std::exception&) {
        return false;
    }
}

std::string FileManager::getDirectory(const std::string& filepath) const {
    return std::filesystem::path(filepath).parent_path().string();
}

std::string FileManager::getFilename(const std::string& filepath) const {
    return std::filesystem::path(filepath).filename().string();
}

std::string FileManager::getExtension(const std::string& filepath) const {
    return std::filesystem::path(filepath).extension().string();
}

void FileManager::loadRecentFiles() {
    // TODO: Load from configuration file
}

void FileManager::saveRecentFiles() {
    // TODO: Save to configuration file
}

void FileManager::addToRecentFilesInternal(const std::string& filename) {
    // Remove if already exists
    auto it = std::find(m_recentFiles.begin(), m_recentFiles.end(), filename);
    if (it != m_recentFiles.end()) {
        m_recentFiles.erase(it);
    }
    
    // Add to front
    m_recentFiles.insert(m_recentFiles.begin(), filename);
    
    // Limit size
    if (m_recentFiles.size() > MAX_RECENT_FILES) {
        m_recentFiles.resize(MAX_RECENT_FILES);
    }
}

void FileManager::updateLoadStats(size_t bytesRead, float loadTime) {
    m_stats.totalBytesRead += bytesRead;
    m_stats.filesLoaded++;
    
    // Update average load time
    float totalTime = m_stats.averageLoadTime * (m_stats.filesLoaded - 1) + loadTime;
    m_stats.averageLoadTime = totalTime / m_stats.filesLoaded;
}

void FileManager::updateSaveStats(size_t bytesWritten, float saveTime, float compressionRatio) {
    m_stats.totalBytesWritten += bytesWritten;
    m_stats.filesSaved++;
    
    // Update average save time
    float totalTime = m_stats.averageSaveTime * (m_stats.filesSaved - 1) + saveTime;
    m_stats.averageSaveTime = totalTime / m_stats.filesSaved;
    
    // Update compression ratio
    m_stats.compressionRatio = compressionRatio;
}

// FileManagerInstance implementation
std::unique_ptr<FileManager> FileManagerInstance::s_instance;
std::mutex FileManagerInstance::s_mutex;

FileManager& FileManagerInstance::getInstance() {
    std::lock_guard<std::mutex> lock(s_mutex);
    if (!s_instance) {
        s_instance = std::make_unique<FileManager>();
    }
    return *s_instance;
}

void FileManagerInstance::destroy() {
    std::lock_guard<std::mutex> lock(s_mutex);
    s_instance.reset();
}

} // namespace FileIO
} // namespace VoxelEditor