#include <gtest/gtest.h>
#include "file_io/FileTypes.h"
#include "file_io/FileManager.h"

namespace VoxelEditor {
namespace FileIO {

class FileTypesTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(FileTypesTest, FileVersionConstruction) {
    FileVersion version{1, 2, 3, 4};
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 2);
    EXPECT_EQ(version.patch, 3);
    EXPECT_EQ(version.build, 4);
}

TEST_F(FileTypesTest, FileVersionEquality) {
    FileVersion v1{1, 2, 3, 4};
    FileVersion v2{1, 2, 3, 4};
    FileVersion v3{1, 2, 3, 5};
    
    EXPECT_EQ(v1, v2);
    EXPECT_NE(v1, v3);
}

TEST_F(FileTypesTest, FileVersionComparison) {
    FileVersion v1{1, 2, 3, 4};
    FileVersion v2{1, 2, 4, 0};
    FileVersion v3{2, 0, 0, 0};
    
    EXPECT_LT(v1, v2);
    EXPECT_LT(v2, v3);
    EXPECT_LT(v1, v3);
}

TEST_F(FileTypesTest, FileVersionCompatibility) {
    FileVersion v1{1, 2, 3, 4};
    FileVersion v2{1, 2, 5, 0};
    FileVersion v3{1, 3, 0, 0};
    FileVersion v4{2, 0, 0, 0};
    
    // Same major and minor versions should be compatible
    EXPECT_TRUE(v1.isCompatible(v2));
    EXPECT_TRUE(v2.isCompatible(v1));
    
    // Different minor version should not be compatible
    EXPECT_FALSE(v1.isCompatible(v3));
    EXPECT_FALSE(v3.isCompatible(v1));
    
    // Different major version should not be compatible
    EXPECT_FALSE(v1.isCompatible(v4));
    EXPECT_FALSE(v4.isCompatible(v1));
}

TEST_F(FileTypesTest, FileVersionToString) {
    FileVersion version{1, 2, 3, 4};
    EXPECT_EQ(version.toString(), "1.2.3.4");
    
    FileVersion version2{10, 0, 0, 0};
    EXPECT_EQ(version2.toString(), "10.0.0.0");
}

TEST_F(FileTypesTest, FileVersionFromString) {
    FileVersion version = FileVersion::fromString("1.2.3.4");
    EXPECT_EQ(version.major, 1);
    EXPECT_EQ(version.minor, 2);
    EXPECT_EQ(version.patch, 3);
    EXPECT_EQ(version.build, 4);
    
    // Test with missing components (should default to 0)
    FileVersion version2 = FileVersion::fromString("2.1");
    EXPECT_EQ(version2.major, 2);
    EXPECT_EQ(version2.minor, 1);
    EXPECT_EQ(version2.patch, 0);
    EXPECT_EQ(version2.build, 0);
}

TEST_F(FileTypesTest, FileVersionCurrent) {
    FileVersion current = FileVersion::Current();
    EXPECT_EQ(current.major, 1);
    EXPECT_EQ(current.minor, 0);
    EXPECT_EQ(current.patch, 0);
    EXPECT_EQ(current.build, 0);
}

TEST_F(FileTypesTest, FileResultSuccess) {
    FileResult result = FileResult::Success();
    EXPECT_TRUE(result.success);
    EXPECT_EQ(result.error, FileError::None);
    EXPECT_TRUE(result.message.empty());
}

TEST_F(FileTypesTest, FileResultError) {
    FileResult result = FileResult::Error(FileError::FileNotFound, "File not found: test.cvef");
    EXPECT_FALSE(result.success);
    EXPECT_EQ(result.error, FileError::FileNotFound);
    EXPECT_EQ(result.message, "File not found: test.cvef");
}

TEST_F(FileTypesTest, SaveOptionsDefault) {
    SaveOptions options = SaveOptions::Default();
    EXPECT_FALSE(options.compress);  // TODO: Enable once compression read is implemented
    EXPECT_EQ(options.compressionLevel, 6);
    EXPECT_FALSE(options.includeHistory);
    EXPECT_FALSE(options.includeCache);
    EXPECT_TRUE(options.createBackup);
    EXPECT_TRUE(options.validateBeforeSave);
}

TEST_F(FileTypesTest, SaveOptionsFast) {
    SaveOptions options = SaveOptions::Fast();
    EXPECT_FALSE(options.compress);
    EXPECT_FALSE(options.createBackup);
    EXPECT_FALSE(options.validateBeforeSave);
}

TEST_F(FileTypesTest, SaveOptionsCompact) {
    SaveOptions options = SaveOptions::Compact();
    EXPECT_FALSE(options.compress);  // TODO: Enable once compression read is implemented
    EXPECT_EQ(options.compressionLevel, 9);
    EXPECT_FALSE(options.includeHistory);
    EXPECT_FALSE(options.includeCache);
}

TEST_F(FileTypesTest, SaveOptionsDevelopment) {
    SaveOptions options = SaveOptions::Development();
    EXPECT_TRUE(options.includeHistory);
    EXPECT_TRUE(options.includeCache);
    EXPECT_TRUE(options.validateBeforeSave);
}

TEST_F(FileTypesTest, LoadOptionsDefault) {
    LoadOptions options = LoadOptions::Default();
    EXPECT_FALSE(options.loadHistory);
    EXPECT_FALSE(options.loadCache);
    EXPECT_TRUE(options.validateAfterLoad);
    EXPECT_TRUE(options.upgradeVersion);
    EXPECT_FALSE(options.ignoreVersionMismatch);
}

TEST_F(FileTypesTest, LoadOptionsFast) {
    LoadOptions options = LoadOptions::Fast();
    EXPECT_FALSE(options.validateAfterLoad);
}

TEST_F(FileTypesTest, LoadOptionsSafe) {
    LoadOptions options = LoadOptions::Safe();
    EXPECT_TRUE(options.validateAfterLoad);
    EXPECT_FALSE(options.ignoreVersionMismatch);
}

TEST_F(FileTypesTest, STLExportOptionsDefault) {
    STLExportOptions options = STLExportOptions::Default();
    EXPECT_EQ(options.format, STLFormat::Binary);
    EXPECT_EQ(options.units, STLUnits::Millimeters);
    EXPECT_EQ(options.scale, 1.0f);
    EXPECT_TRUE(options.mergeMeshes);
    EXPECT_TRUE(options.validateWatertight);
    EXPECT_EQ(options.translation, Math::Vector3f(0, 0, 0));
}

TEST_F(FileTypesTest, STLExportOptionsPrinting3D) {
    STLExportOptions options = STLExportOptions::Printing3D();
    EXPECT_EQ(options.format, STLFormat::Binary);
    EXPECT_EQ(options.units, STLUnits::Millimeters);
    EXPECT_TRUE(options.validateWatertight);
    EXPECT_TRUE(options.mergeMeshes);
}

TEST_F(FileTypesTest, STLExportOptionsCAD) {
    STLExportOptions options = STLExportOptions::CAD();
    EXPECT_EQ(options.format, STLFormat::ASCII);
    EXPECT_EQ(options.units, STLUnits::Meters);
    EXPECT_TRUE(options.validateWatertight);
}

TEST_F(FileTypesTest, ProjectMetadataInitialization) {
    ProjectMetadata metadata;
    EXPECT_TRUE(metadata.name.empty());
    EXPECT_TRUE(metadata.description.empty());
    EXPECT_TRUE(metadata.author.empty());
    EXPECT_TRUE(metadata.application.empty());
    EXPECT_TRUE(metadata.applicationVersion.empty());
    EXPECT_TRUE(metadata.customProperties.empty());
}

TEST_F(FileTypesTest, WorkspaceSettingsDefaults) {
    WorkspaceSettings settings;
    EXPECT_EQ(settings.size, Math::Vector3f(5.0f, 5.0f, 5.0f));
    EXPECT_EQ(settings.origin, Math::Vector3f(0, 0, 0));
    EXPECT_EQ(settings.defaultResolution, ::VoxelEditor::VoxelData::VoxelResolution::Size_1cm);
    EXPECT_TRUE(settings.gridVisible);
    EXPECT_TRUE(settings.axesVisible);
}

TEST_F(FileTypesTest, IOStatsInitialization) {
    FileManager::IOStats stats;
    EXPECT_EQ(stats.totalBytesRead, 0);
    EXPECT_EQ(stats.totalBytesWritten, 0);
    EXPECT_EQ(stats.filesLoaded, 0);
    EXPECT_EQ(stats.filesSaved, 0);
    EXPECT_EQ(stats.averageLoadTime, 0.0f);
    EXPECT_EQ(stats.averageSaveTime, 0.0f);
    EXPECT_EQ(stats.compressionRatio, 1.0f);
}

TEST_F(FileTypesTest, STLExportStatsInitialization) {
    STLExportStats stats;
    EXPECT_EQ(stats.triangleCount, 0);
    EXPECT_EQ(stats.vertexCount, 0);
    EXPECT_EQ(stats.exportTime, 0.0f);
    EXPECT_EQ(stats.fileSize, 0);
    EXPECT_FALSE(stats.watertight);
    EXPECT_TRUE(stats.warnings.empty());
}

} // namespace FileIO
} // namespace VoxelEditor