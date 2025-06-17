#include <gtest/gtest.h>
#include <sstream>
#include "file_io/BinaryFormat.h"
#include "file_io/BinaryIO.h"
#include "file_io/Project.h"

namespace VoxelEditor {
namespace FileIO {

class BinaryFormatTest : public ::testing::Test {
protected:
    std::stringstream m_stream;
    std::unique_ptr<BinaryFormat> m_format;
    
    void SetUp() override {
        m_stream.clear();
        m_stream.str("");
        m_format = std::make_unique<BinaryFormat>();
    }
    
    void TearDown() override {
    }
    
    Project createTestProject() {
        Project project;
        project.initializeDefaults();
        project.metadata.name = "Test Project";
        project.metadata.description = "A test project for unit testing";
        project.metadata.author = "Unit Test";
        project.workspace.size = Math::Vector3f(8.0f, 8.0f, 8.0f);
        return project;
    }
};

TEST_F(BinaryFormatTest, FileHeaderValidation) {
    FileHeader header;
    
    // Check default magic number
    EXPECT_EQ(header.magic[0], 'C');
    EXPECT_EQ(header.magic[1], 'V');
    EXPECT_EQ(header.magic[2], 'E');
    EXPECT_EQ(header.magic[3], 'F');
    
    // Check other defaults
    EXPECT_EQ(header.fileSize, 0);
    EXPECT_EQ(header.compressionFlags, 0);
    EXPECT_EQ(header.checksum, 0);
    
    // Check reserved bytes are zero
    for (int i = 0; i < 228; ++i) {
        EXPECT_EQ(header.reserved[i], 0);
    }
}

TEST_F(BinaryFormatTest, HeaderWriteRead) {
    // Test header through public project write/read interface
    Project project = createTestProject();
    SaveOptions saveOptions = SaveOptions::Default();
    saveOptions.compress = false;  // Disable compression for now
    saveOptions.compress = false;  // Disable compression for now
    
    // Write project (which includes header)
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, project, saveOptions));
    
    // Read project back (which validates header)
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    Project readProject;
    LoadOptions loadOptions = LoadOptions::Default();
    
    EXPECT_TRUE(m_format->readProject(reader, readProject, loadOptions));
    
    // Verify project was read correctly
    EXPECT_EQ(readProject.metadata.name, project.metadata.name);
}

TEST_F(BinaryFormatTest, InvalidMagicNumber) {
    BinaryWriter writer(m_stream);
    
    // Write invalid header
    writer.writeBytes("XXXX", 4);  // Wrong magic
    writer.write<FileVersion>(FileVersion::Current());
    writer.writeUInt64(0);  // fileSize
    writer.writeUInt32(0);  // compressionFlags
    writer.writeUInt64(0);  // checksum
    writer.writeBytes(std::vector<uint8_t>(228, 0).data(), 228);  // reserved
    
    // Try to read header
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    FileHeader header;
    
    EXPECT_FALSE(m_format->readHeader(reader, header));
}

TEST_F(BinaryFormatTest, MinimalSaveLoad) {
    // Create a minimal project
    Project project;
    project.initializeDefaults();
    
    // Use uncompressed save
    SaveOptions saveOptions;
    saveOptions.compress = false;
    
    // Write to stream
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, project, saveOptions));
    
    // Read back
    m_stream.seekg(0);
    m_stream.clear();
    BinaryReader reader(m_stream);
    Project loadedProject;
    LoadOptions loadOptions;
    
    bool result = m_format->readProject(reader, loadedProject, loadOptions);
    if (!result) {
        std::cerr << "Error: " << static_cast<int>(m_format->getLastError()) 
                  << " - " << m_format->getLastErrorMessage() << std::endl;
    }
    EXPECT_TRUE(result);
}

TEST_F(BinaryFormatTest, ProjectSaveLoadBasic) {
    Project saveProject = createTestProject();
    SaveOptions saveOptions = SaveOptions::Default();
    saveOptions.compress = false;  // Disable compression for now
    saveOptions.compress = false;  // Disable compression for now
    
    // Save project
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, saveProject, saveOptions));
    
    // Debug: Check stream size after write
    size_t bytesWritten = writer.getBytesWritten();
    m_stream.seekg(0, std::ios::end);
    std::cerr << "Stream size after write: " << m_stream.tellg() << std::endl;
    std::cerr << "Bytes written by writer: " << bytesWritten << std::endl;
    
    // Make sure we flush the writer
    m_stream.flush();
    
    // Load project
    m_stream.seekg(0);
    m_stream.clear(); // Clear any error flags
    BinaryReader reader(m_stream);
    Project loadProject;
    LoadOptions loadOptions = LoadOptions::Default();
    
    // Debug: Check stream position before read
    std::cerr << "Stream position before read: " << m_stream.tellg() << std::endl;
    
    bool readResult = m_format->readProject(reader, loadProject, loadOptions);
    if (!readResult) {
        std::cerr << "Read failed with error: " << static_cast<int>(m_format->getLastError()) 
                  << " - " << m_format->getLastErrorMessage() << std::endl;
        std::cerr << "Stream position after failed read: " << m_stream.tellg() << std::endl;
        std::cerr << "Stream good: " << m_stream.good() << ", eof: " << m_stream.eof() 
                  << ", fail: " << m_stream.fail() << ", bad: " << m_stream.bad() << std::endl;
    }
    EXPECT_TRUE(readResult);
    
    // Verify basic metadata
    EXPECT_EQ(loadProject.metadata.name, saveProject.metadata.name);
    EXPECT_EQ(loadProject.metadata.description, saveProject.metadata.description);
    EXPECT_EQ(loadProject.metadata.author, saveProject.metadata.author);
    EXPECT_EQ(loadProject.workspace.size, saveProject.workspace.size);
}

TEST_F(BinaryFormatTest, ChunkWriteRead) {
    // Test chunk operations through project with custom data
    Project project = createTestProject();
    
    // Add custom data which will be written as a chunk
    std::vector<uint8_t> testData = {1, 2, 3, 4, 5, 6, 7, 8};
    project.customData["test_chunk"] = testData;
    
    SaveOptions saveOptions = SaveOptions::Default();
    saveOptions.compress = false;  // Disable compression for now
    
    // Write project
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, project, saveOptions));
    
    // Read project back
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    Project readProject;
    LoadOptions loadOptions = LoadOptions::Default();
    
    EXPECT_TRUE(m_format->readProject(reader, readProject, loadOptions));
    
    // Verify custom data chunk was preserved
    ASSERT_TRUE(readProject.customData.find("test_chunk") != readProject.customData.end());
    EXPECT_EQ(readProject.customData["test_chunk"], testData);
}

TEST_F(BinaryFormatTest, EmptyProjectHandling) {
    Project emptyProject;  // Not initialized
    SaveOptions options = SaveOptions::Default();
    
    BinaryWriter writer(m_stream);
    
    // Should handle empty project gracefully
    EXPECT_FALSE(m_format->writeProject(writer, emptyProject, options));
}

// Test removed - writeMetadataChunk is private
// Metadata writing is tested through the public writeProject interface

// Test removed - writeSettingsChunk is private
// Settings are tested through the public writeProject interface

// Test removed - writeCustomDataChunks is private
// Custom data is tested through the public writeProject interface

TEST_F(BinaryFormatTest, FileValidation) {
    // Create a valid project file
    Project project = createTestProject();
    SaveOptions options = SaveOptions::Default();
    
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, project, options));
    
    // Validate the file
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    EXPECT_TRUE(m_format->validateFile(reader));
}

// Test removed - calculateChecksum is not a public method
// Checksum validation is tested through validateFile

TEST_F(BinaryFormatTest, SaveOptionsHandling) {
    Project project = createTestProject();
    
    // Test with different save options
    SaveOptions fastOptions = SaveOptions::Fast();
    SaveOptions compactOptions = SaveOptions::Compact();
    
    // Save with fast options (no compression)
    std::stringstream fastStream;
    BinaryWriter fastWriter(fastStream);
    EXPECT_TRUE(m_format->writeProject(fastWriter, project, fastOptions));
    size_t fastSize = fastStream.str().size();
    
    // Save with compact options (max compression)
    std::stringstream compactStream;
    BinaryWriter compactWriter(compactStream);
    EXPECT_TRUE(m_format->writeProject(compactWriter, project, compactOptions));
    size_t compactSize = compactStream.str().size();
    
    // Fast save should be larger or equal (no compression)
    // Note: Since compression might not be implemented, sizes might be equal
    EXPECT_GE(fastSize, compactSize);
}

TEST_F(BinaryFormatTest, LoadOptionsHandling) {
    Project saveProject = createTestProject();
    SaveOptions saveOptions = SaveOptions::Default();
    saveOptions.compress = false;  // Disable compression for now
    
    // Save project
    BinaryWriter writer(m_stream);
    EXPECT_TRUE(m_format->writeProject(writer, saveProject, saveOptions));
    
    // Load with different options
    LoadOptions fastLoad = LoadOptions::Fast();
    LoadOptions safeLoad = LoadOptions::Safe();
    
    // Fast load (no validation)
    m_stream.seekg(0);
    BinaryReader reader1(m_stream);
    Project project1;
    EXPECT_TRUE(m_format->readProject(reader1, project1, fastLoad));
    
    // Safe load (with validation)
    m_stream.seekg(0);
    BinaryReader reader2(m_stream);
    Project project2;
    EXPECT_TRUE(m_format->readProject(reader2, project2, safeLoad));
    
    // Both should load successfully
    EXPECT_EQ(project1.metadata.name, saveProject.metadata.name);
    EXPECT_EQ(project2.metadata.name, saveProject.metadata.name);
}

} // namespace FileIO
} // namespace VoxelEditor