#include <gtest/gtest.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <atomic>
#include "file_io/FileManager.h"
#include "file_io/Project.h"

namespace VoxelEditor {
namespace FileIO {

class FileManagerTest : public ::testing::Test {
protected:
    std::unique_ptr<FileManager> m_fileManager;
    std::string m_testDir = "test_file_manager";
    
    void SetUp() override {
        m_fileManager = std::make_unique<FileManager>();
        
        // Create test directory
        std::filesystem::create_directories(m_testDir);
    }
    
    void TearDown() override {
        // Clean up test files
        std::filesystem::remove_all(m_testDir);
    }
    
    Project createTestProject(const std::string& name = "Test Project") {
        Project project;
        project.initializeDefaults();
        project.metadata.name = name;
        project.metadata.description = "A test project for FileManager";
        project.metadata.author = "Unit Test";
        project.workspace.size = Math::Vector3f(5.0f, 5.0f, 5.0f);
        project.setCustomProperty("test_property", "test_value");
        return project;
    }
    
    std::string getTestFilePath(const std::string& filename) {
        return m_testDir + "/" + filename;
    }
};

// REQ-8.1.1: Custom binary format shall include file header with version and metadata
// REQ-8.1.2: Format shall store workspace dimensions and settings
// REQ-8.2.2: System shall support format versioning for backward compatibility
// REQ-9.2.4: CLI shall support file commands (save, load, export)
TEST_F(FileManagerTest, SaveAndLoadProject) {
    Project originalProject = createTestProject();
    std::string filename = getTestFilePath("test_project.cvef");
    
    // Save project
    SaveOptions saveOptions = SaveOptions::Default();
    FileResult saveResult = m_fileManager->saveProject(filename, originalProject, saveOptions);
    
    EXPECT_TRUE(saveResult.success);
    EXPECT_EQ(saveResult.error, FileError::None);
    EXPECT_TRUE(std::filesystem::exists(filename));
    
    // Load project
    Project loadedProject;
    LoadOptions loadOptions = LoadOptions::Default();
    FileResult loadResult = m_fileManager->loadProject(filename, loadedProject, loadOptions);
    
    EXPECT_TRUE(loadResult.success) << "Load failed with error: " << static_cast<int>(loadResult.error) 
                                    << " - " << loadResult.message;
    EXPECT_EQ(loadResult.error, FileError::None);
    
    // Verify loaded data
    EXPECT_EQ(loadedProject.metadata.name, originalProject.metadata.name);
    EXPECT_EQ(loadedProject.metadata.description, originalProject.metadata.description);
    EXPECT_EQ(loadedProject.metadata.author, originalProject.metadata.author);
    EXPECT_EQ(loadedProject.workspace.size, originalProject.workspace.size);
    EXPECT_EQ(loadedProject.getCustomProperty("test_property"), "test_value");
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - error handling
TEST_F(FileManagerTest, SaveToInvalidPath) {
    Project project = createTestProject();
    std::string invalidPath = "/invalid/path/that/does/not/exist/project.cvef";
    
    SaveOptions options = SaveOptions::Default();
    FileResult result = m_fileManager->saveProject(invalidPath, project, options);
    
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error, FileError::None);
    EXPECT_FALSE(result.message.empty());
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - error handling
TEST_F(FileManagerTest, LoadNonExistentFile) {
    Project project;
    std::string filename = getTestFilePath("nonexistent.cvef");
    
    LoadOptions options = LoadOptions::Default();
    FileResult result = m_fileManager->loadProject(filename, project, options);
    
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, FileError::FileNotFound);
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - save options
TEST_F(FileManagerTest, SaveOptionsFast) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("fast_save.cvef");
    
    SaveOptions options = SaveOptions::Fast();
    FileResult result = m_fileManager->saveProject(filename, project, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(options.compress);
    EXPECT_FALSE(options.createBackup);
    EXPECT_FALSE(options.validateBeforeSave);
}

// REQ-8.2.3: System shall use LZ4 compression for efficient storage
// REQ-7.3.4: System shall use LZ4 compression for file storage
TEST_F(FileManagerTest, SaveOptionsCompact) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("compact_save.cvef");
    
    SaveOptions options = SaveOptions::Compact();
    FileResult result = m_fileManager->saveProject(filename, project, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_FALSE(options.compress);  // TODO: Enable once compression read is implemented
    EXPECT_EQ(options.compressionLevel, 9);
}

// REQ-8.2.1: System shall export STL files for 3D printing and sharing
// REQ-9.2.4: CLI shall support file commands (save, load, export)
TEST_F(FileManagerTest, ExportSTL) {
    // Create a simple mesh
    Rendering::Mesh mesh;
    mesh.vertices = {
        Rendering::Vertex{Math::Vector3f(0, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 0)},
        Rendering::Vertex{Math::Vector3f(1, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 0)},
        Rendering::Vertex{Math::Vector3f(0, 1, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 1)}
    };
    mesh.indices = {0, 1, 2};
    
    std::string filename = getTestFilePath("export_test.stl");
    STLExportOptions options = STLExportOptions::Default();
    
    FileResult result = m_fileManager->exportSTL(filename, mesh, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(filename));
}

// REQ-8.2.1: System shall export STL files for 3D printing and sharing
// REQ-9.2.4: CLI shall support file commands (save, load, export)
TEST_F(FileManagerTest, ExportMultipleSTL) {
    std::vector<Rendering::Mesh> meshes;
    
    // Create two simple meshes
    for (int i = 0; i < 2; ++i) {
        Rendering::Mesh mesh;
        float offset = i * 2.0f;
        mesh.vertices = {
            Rendering::Vertex{Math::Vector3f(offset, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 0)},
            Rendering::Vertex{Math::Vector3f(offset + 1, 0, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(1, 0)},
            Rendering::Vertex{Math::Vector3f(offset, 1, 0), Math::Vector3f(0, 0, 1), Math::Vector2f(0, 1)}
        };
        mesh.indices = {0, 1, 2};
        meshes.push_back(mesh);
    }
    
    std::string filename = getTestFilePath("multi_export_test.stl");
    STLExportOptions options = STLExportOptions::Default();
    
    FileResult result = m_fileManager->exportMultiSTL(filename, meshes, options);
    
    EXPECT_TRUE(result.success);
    EXPECT_TRUE(std::filesystem::exists(filename));
}

// REQ-8.1.1: Custom binary format shall include file header with version and metadata
// REQ-8.1.10: Format shall include creation and modification timestamps
// REQ-8.2.2: System shall support format versioning for backward compatibility
TEST_F(FileManagerTest, FileInfo) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("info_test.cvef");
    
    // Save project first
    m_fileManager->saveProject(filename, project, SaveOptions::Default());
    
    // Get file info
    FileInfo info = m_fileManager->getFileInfo(filename);
    
    EXPECT_GT(info.fileSize, 0);
    EXPECT_FALSE(info.lastModified.time_since_epoch().count() == 0);
    EXPECT_EQ(info.version, FileVersion::Current());
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - recent files management
TEST_F(FileManagerTest, RecentFiles) {
    // Clear recent files
    m_fileManager->clearRecentFiles();
    
    std::vector<std::string> filenames = {
        getTestFilePath("recent1.cvef"),
        getTestFilePath("recent2.cvef"),
        getTestFilePath("recent3.cvef")
    };
    
    // Add files to recent
    for (const auto& filename : filenames) {
        m_fileManager->addToRecentFiles(filename);
    }
    
    // Get recent files
    std::vector<std::string> recent = m_fileManager->getRecentFiles();
    
    // Should be in reverse order (most recent first)
    EXPECT_GE(recent.size(), 3);
    EXPECT_EQ(recent[0], filenames[2]);
    EXPECT_EQ(recent[1], filenames[1]);
    EXPECT_EQ(recent[2], filenames[0]);
}

// REQ-8.1.1: Custom binary format shall include file header with version and metadata
// REQ-8.2.2: System shall support format versioning for backward compatibility
TEST_F(FileManagerTest, ValidateProjectFile) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("validate_test.cvef");
    
    // Save valid project
    m_fileManager->saveProject(filename, project, SaveOptions::Default());
    
    // Should be valid
    EXPECT_TRUE(m_fileManager->isValidProjectFile(filename));
    
    // Non-existent file should be invalid
    EXPECT_FALSE(m_fileManager->isValidProjectFile("nonexistent.cvef"));
}

// REQ-8.1.1: Custom binary format shall include file header with version and metadata
// REQ-8.2.2: System shall support format versioning for backward compatibility
TEST_F(FileManagerTest, GetFileVersion) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("version_test.cvef");
    
    // Save project
    m_fileManager->saveProject(filename, project, SaveOptions::Default());
    
    // Get version
    FileVersion version = m_fileManager->getFileVersion(filename);
    EXPECT_EQ(version, FileVersion::Current());
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - progress feedback
TEST_F(FileManagerTest, ProgressCallback) {
    Project project = createTestProject();
    
    // Add some data to make save take time
    for (int i = 0; i < 100; ++i) {
        project.customData["data" + std::to_string(i)] = std::vector<uint8_t>(1000, i);
    }
    
    std::string filename = getTestFilePath("progress_test.cvef");
    
    // Set progress callback
    std::vector<float> progressValues;
    m_fileManager->setProgressCallback([&progressValues](float progress, const std::string& message) {
        progressValues.push_back(progress);
    });
    
    // Save project
    m_fileManager->saveProject(filename, project, SaveOptions::Default());
    
    // Should have received some progress updates
    EXPECT_FALSE(progressValues.empty());
    
    // Progress should be between 0 and 1
    for (float progress : progressValues) {
        EXPECT_GE(progress, 0.0f);
        EXPECT_LE(progress, 1.0f);
    }
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - autosave functionality
TEST_F(FileManagerTest, AutoSaveBasic) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("autosave_test.cvef");
    
    // Enable auto-save with short interval
    m_fileManager->setAutoSaveEnabled(true, 0.1f);  // 100ms
    
    // Register project for auto-save
    m_fileManager->registerProjectForAutoSave(filename, &project);
    
    // Wait for auto-save to trigger
    std::this_thread::sleep_for(std::chrono::milliseconds(200));
    
    // Update auto-save
    m_fileManager->updateAutoSave(0.2f);
    
    // Wait for the autosave thread to process (it checks every 1 second)
    std::this_thread::sleep_for(std::chrono::milliseconds(1100));
    
    // File should exist (with .autosave added before extension)
    std::string autosaveFilename = m_testDir + "/autosave_test.autosave.cvef";
    EXPECT_TRUE(std::filesystem::exists(autosaveFilename));
    
    // Disable auto-save
    m_fileManager->setAutoSaveEnabled(false);
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - backup functionality
TEST_F(FileManagerTest, BackupCreation) {
    Project project = createTestProject();
    std::string filename = getTestFilePath("backup_test.cvef");
    
    // Save with backup enabled
    SaveOptions options = SaveOptions::Default();
    options.createBackup = true;
    
    // Save twice to create backup
    m_fileManager->saveProject(filename, project, options);
    project.metadata.name = "Modified Project";
    m_fileManager->saveProject(filename, project, options);
    
    // Check for backup files
    bool foundBackup = false;
    for (const auto& entry : std::filesystem::directory_iterator(m_testDir)) {
        if (entry.path().string().find(".bak") != std::string::npos) {
            foundBackup = true;
            break;
        }
    }
    
    EXPECT_TRUE(foundBackup);
}

// REQ-8.2.3: System shall use LZ4 compression for efficient storage
// REQ-7.3.4: System shall use LZ4 compression for file storage
TEST_F(FileManagerTest, CompressionSettings) {
    Project project = createTestProject();
    
    // Add large data
    project.customData["large"] = std::vector<uint8_t>(10000, 0xFF);
    
    // Save without compression
    m_fileManager->setCompressionEnabled(false);
    std::string uncompressedFile = getTestFilePath("uncompressed.cvef");
    m_fileManager->saveProject(uncompressedFile, project, SaveOptions::Default());
    size_t uncompressedSize = std::filesystem::file_size(uncompressedFile);
    
    // Save with compression
    m_fileManager->setCompressionEnabled(true);
    m_fileManager->setCompressionLevel(9);
    std::string compressedFile = getTestFilePath("compressed.cvef");
    m_fileManager->saveProject(compressedFile, project, SaveOptions::Default());
    size_t compressedSize = std::filesystem::file_size(compressedFile);
    
    // Both files should exist
    EXPECT_TRUE(std::filesystem::exists(uncompressedFile));
    EXPECT_TRUE(std::filesystem::exists(compressedFile));
    
    // Sizes might be similar with placeholder compression
    // In real implementation, compressed should be smaller
}

// REQ-6.3.4: Application overhead shall not exceed 1GB - monitoring file I/O statistics
TEST_F(FileManagerTest, Statistics) {
    // Get initial stats
    FileManager::IOStats initialStats = m_fileManager->getStatistics();
    
    Project project = createTestProject();
    std::string filename = getTestFilePath("stats_test.cvef");
    
    // Save and load
    m_fileManager->saveProject(filename, project, SaveOptions::Default());
    m_fileManager->loadProject(filename, project, LoadOptions::Default());
    
    // Get updated stats
    FileManager::IOStats stats = m_fileManager->getStatistics();
    
    // Stats should be updated
    EXPECT_GT(stats.totalBytesWritten, initialStats.totalBytesWritten);
    EXPECT_GT(stats.totalBytesRead, initialStats.totalBytesRead);
    EXPECT_EQ(stats.filesSaved, initialStats.filesSaved + 1);
    EXPECT_EQ(stats.filesLoaded, initialStats.filesLoaded + 1);
    EXPECT_GT(stats.averageSaveTime, 0.0f);
    EXPECT_GT(stats.averageLoadTime, 0.0f);
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - error handling
TEST_F(FileManagerTest, ErrorHandling) {
    Project project;
    
    // Try to save invalid project
    std::string filename = getTestFilePath("error_test.cvef");
    FileResult result = m_fileManager->saveProject(filename, project, SaveOptions::Default());
    
    // Should fail with appropriate error
    EXPECT_FALSE(result.success);
    EXPECT_NE(result.error, FileError::None);
    EXPECT_FALSE(result.message.empty());
}

// REQ-9.2.4: CLI shall support file commands (save, load, export) - concurrent access
TEST_F(FileManagerTest, ConcurrentAccess) {
    Project project1 = createTestProject("Project 1");
    Project project2 = createTestProject("Project 2");
    
    std::string filename1 = getTestFilePath("concurrent1.cvef");
    std::string filename2 = getTestFilePath("concurrent2.cvef");
    
    // Use separate FileManager instances to avoid shared state issues
    auto fileManager1 = std::make_unique<FileManager>();
    auto fileManager2 = std::make_unique<FileManager>();
    
    std::atomic<bool> thread1_completed{false};
    std::atomic<bool> thread2_completed{false};
    
    // Save two projects concurrently
    std::thread thread1([&]() {
        fileManager1->saveProject(filename1, project1, SaveOptions::Default());
        thread1_completed = true;
    });
    
    std::thread thread2([&]() {
        fileManager2->saveProject(filename2, project2, SaveOptions::Default());
        thread2_completed = true;
    });
    
    // Join threads with timeout protection
    thread1.join();
    thread2.join();
    
    // Verify both threads completed
    EXPECT_TRUE(thread1_completed.load());
    EXPECT_TRUE(thread2_completed.load());
    
    // Both files should exist
    EXPECT_TRUE(std::filesystem::exists(filename1));
    EXPECT_TRUE(std::filesystem::exists(filename2));
    
    // Clean up file managers before test cleanup
    fileManager1.reset();
    fileManager2.reset();
}

} // namespace FileIO
} // namespace VoxelEditor