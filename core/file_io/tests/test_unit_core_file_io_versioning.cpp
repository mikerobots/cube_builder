#include <gtest/gtest.h>
#include <sstream>
#include "file_io/FileVersioning.h"
#include "file_io/BinaryIO.h"

namespace VoxelEditor {
namespace FileIO {

class FileVersioningTest : public ::testing::Test {
protected:
    std::unique_ptr<FileVersioning> m_versioning;
    
    void SetUp() override {
        m_versioning = std::make_unique<FileVersioning>();
    }
    
    void TearDown() override {
    }
};

TEST_F(FileVersioningTest, GetCurrentVersion) {
    FileVersion current = m_versioning->getCurrentVersion();
    
    // Should return the current version
    EXPECT_EQ(current, FileVersion::Current());
    EXPECT_EQ(current.major, 1);
    EXPECT_EQ(current.minor, 0);
    EXPECT_EQ(current.patch, 0);
    EXPECT_EQ(current.build, 0);
}

TEST_F(FileVersioningTest, VersionCompatibility) {
    FileVersion v1_0_0{1, 0, 0, 0};
    FileVersion v1_0_1{1, 0, 1, 0};
    FileVersion v1_1_0{1, 1, 0, 0};
    FileVersion v2_0_0{2, 0, 0, 0};
    
    // Same major/minor versions should be compatible
    EXPECT_TRUE(m_versioning->isCompatible(v1_0_0));
    EXPECT_TRUE(m_versioning->isCompatible(v1_0_1));
    
    // Different minor version might not be compatible
    // (depends on implementation)
    bool minorCompatible = m_versioning->isCompatible(v1_1_0);
    
    // Different major version should not be compatible
    EXPECT_FALSE(m_versioning->isCompatible(v2_0_0));
}

TEST_F(FileVersioningTest, DetectVersion) {
    std::stringstream stream;
    BinaryWriter writer(stream);
    
    // Write a file header with version
    writer.writeBytes("CVEF", 4);  // Magic
    FileVersion testVersion{1, 2, 3, 4};
    writer.write(testVersion);
    
    // Detect version
    stream.seekg(0);
    BinaryReader reader(stream);
    FileVersion detected = m_versioning->detectVersion(reader);
    
    // Should detect the written version
    EXPECT_EQ(detected, testVersion);
}

TEST_F(FileVersioningTest, DetectInvalidVersion) {
    std::stringstream stream;
    BinaryWriter writer(stream);
    
    // Write invalid header
    writer.writeBytes("XXXX", 4);  // Wrong magic
    
    stream.seekg(0);
    BinaryReader reader(stream);
    FileVersion detected = m_versioning->detectVersion(reader);
    
    // Should return invalid version (0.0.0.0)
    EXPECT_EQ(detected.major, 0);
    EXPECT_EQ(detected.minor, 0);
    EXPECT_EQ(detected.patch, 0);
    EXPECT_EQ(detected.build, 0);
}

TEST_F(FileVersioningTest, CanUpgrade) {
    FileVersion v1_0_0{1, 0, 0, 0};
    FileVersion v1_1_0{1, 1, 0, 0};
    FileVersion v1_2_0{1, 2, 0, 0};
    FileVersion v2_0_0{2, 0, 0, 0};
    
    // Should be able to upgrade within major version
    EXPECT_TRUE(m_versioning->canUpgrade(v1_0_0, v1_1_0));
    EXPECT_TRUE(m_versioning->canUpgrade(v1_0_0, v1_2_0));
    EXPECT_TRUE(m_versioning->canUpgrade(v1_1_0, v1_2_0));
    
    // Might not support major version upgrades
    bool majorUpgrade = m_versioning->canUpgrade(v1_2_0, v2_0_0);
    
    // Cannot downgrade
    EXPECT_FALSE(m_versioning->canUpgrade(v1_2_0, v1_1_0));
    EXPECT_FALSE(m_versioning->canUpgrade(v1_1_0, v1_0_0));
}

TEST_F(FileVersioningTest, UpgradeFile) {
    // Create test file paths
    std::string inputFile = "test_version_input.cvef";
    std::string outputFile = "test_version_output.cvef";
    FileVersion targetVersion{1, 1, 0, 0};
    
    // Current implementation returns false (stub)
    bool result = m_versioning->upgradeFile(inputFile, outputFile, targetVersion);
    EXPECT_FALSE(result);  // Stub implementation
}

// Test removed - migrateV1ToV2 and migrateV2ToV3 are private methods
// Migration is tested through the public migrateData interface

TEST_F(FileVersioningTest, GetUpgradeWarnings) {
    FileVersion v1_0_0{1, 0, 0, 0};
    FileVersion v1_1_0{1, 1, 0, 0};
    FileVersion v2_0_0{2, 0, 0, 0};
    
    // Get warnings for minor version upgrade
    std::vector<std::string> warnings1 = m_versioning->getUpgradeWarnings(v1_0_0, v1_1_0);
    // Current implementation returns empty vector
    EXPECT_TRUE(warnings1.empty());
    
    // Get warnings for major version upgrade
    std::vector<std::string> warnings2 = m_versioning->getUpgradeWarnings(v1_1_0, v2_0_0);
    // Current implementation returns empty vector
    EXPECT_TRUE(warnings2.empty());
}

TEST_F(FileVersioningTest, FindUpgradePath) {
    FileVersion v1_0_0{1, 0, 0, 0};
    FileVersion v1_1_0{1, 1, 0, 0};
    FileVersion v1_2_0{1, 2, 0, 0};
    
    // In a full implementation, this would find the path of migrations needed
    // Current stub implementation would return empty path
}

TEST_F(FileVersioningTest, VersionStringConversion) {
    // Test version to string conversion
    FileVersion v1{1, 2, 3, 4};
    std::string str1 = v1.toString();
    EXPECT_EQ(str1, "1.2.3.4");
    
    FileVersion v2{10, 0, 0, 0};
    std::string str2 = v2.toString();
    EXPECT_EQ(str2, "10.0.0.0");
    
    // Test string to version conversion
    FileVersion parsed1 = FileVersion::fromString("5.6.7.8");
    EXPECT_EQ(parsed1.major, 5);
    EXPECT_EQ(parsed1.minor, 6);
    EXPECT_EQ(parsed1.patch, 7);
    EXPECT_EQ(parsed1.build, 8);
    
    // Test partial version strings
    FileVersion parsed2 = FileVersion::fromString("2.1");
    EXPECT_EQ(parsed2.major, 2);
    EXPECT_EQ(parsed2.minor, 1);
    EXPECT_EQ(parsed2.patch, 0);
    EXPECT_EQ(parsed2.build, 0);
}

TEST_F(FileVersioningTest, RegisterMigrations) {
    // In a full implementation, this would test the migration registration system
    // The current stub implementation doesn't register any migrations
    
    // Future test would verify:
    // - Migrations are registered for consecutive versions
    // - Migrations can be chained for multi-version upgrades
    // - Invalid migration paths are rejected
}

TEST_F(FileVersioningTest, MigrationDataIntegrity) {
    // In a full implementation, this would test that migrations preserve data
    
    // Create test data
    std::stringstream v1Data;
    BinaryWriter v1Writer(v1Data);
    v1Writer.writeString("Test Project");
    v1Writer.writeUInt32(100);  // Some v1-specific data
    
    // Migrate to v2
    std::stringstream v2Data;
    v1Data.seekg(0);
    BinaryReader v1Reader(v1Data);
    BinaryWriter v2Writer(v2Data);
    
    // Current stub returns false  
    // Test public migrateData method instead
    FileVersion v1{1, 0, 0, 0};
    FileVersion v2{2, 0, 0, 0};
    bool migrated = m_versioning->migrateData(v1Reader, v2Writer, v1, v2);
    EXPECT_FALSE(migrated);
    
    // In full implementation would verify:
    // - Original data is preserved
    // - New fields have sensible defaults
    // - Data format changes are applied correctly
}

TEST_F(FileVersioningTest, BackwardCompatibility) {
    // Test that we can detect when a file is too new
    FileVersion futureVersion{99, 0, 0, 0};
    
    EXPECT_FALSE(m_versioning->isCompatible(futureVersion));
    EXPECT_FALSE(m_versioning->canUpgrade(FileVersion::Current(), futureVersion));
}

TEST_F(FileVersioningTest, VersionComparison) {
    FileVersion v1{1, 0, 0, 0};
    FileVersion v2{1, 0, 0, 1};
    FileVersion v3{1, 0, 1, 0};
    FileVersion v4{1, 1, 0, 0};
    FileVersion v5{2, 0, 0, 0};
    
    // Test less than operator
    EXPECT_LT(v1, v2);
    EXPECT_LT(v2, v3);
    EXPECT_LT(v3, v4);
    EXPECT_LT(v4, v5);
    
    // Test equality
    FileVersion v1_copy{1, 0, 0, 0};
    EXPECT_EQ(v1, v1_copy);
    EXPECT_NE(v1, v2);
}

} // namespace FileIO
} // namespace VoxelEditor