#include <gtest/gtest.h>
#include "file_io/Project.h"
#include "voxel_data/VoxelDataManager.h"
#include "groups/GroupManager.h"
#include "camera/OrbitCamera.h"
#include "selection/SelectionSet.h"

namespace VoxelEditor {
namespace FileIO {

class ProjectTest : public ::testing::Test {
protected:
    void SetUp() override {
    }
    
    void TearDown() override {
    }
};

TEST_F(ProjectTest, DefaultConstruction) {
    Project project;
    
    // Check metadata is initialized
    EXPECT_TRUE(project.metadata.name.empty());
    EXPECT_TRUE(project.metadata.description.empty());
    EXPECT_TRUE(project.metadata.author.empty());
    
    // Check pointers are null by default
    EXPECT_EQ(project.voxelData, nullptr);
    EXPECT_EQ(project.groupData, nullptr);
    EXPECT_EQ(project.camera, nullptr);
    EXPECT_EQ(project.currentSelection, nullptr);
    
    // Check collections are empty
    EXPECT_TRUE(project.namedSelections.empty());
    EXPECT_TRUE(project.customData.empty());
}

TEST_F(ProjectTest, DefaultInitialization) {
    Project project;
    project.initializeDefaults(); // Explicitly initialize
    
    // After initialization, all components should be created
    EXPECT_NE(project.voxelData, nullptr);
    EXPECT_NE(project.groupData, nullptr);
    EXPECT_NE(project.camera, nullptr);
    EXPECT_NE(project.currentSelection, nullptr);
    
    // Workspace should have default settings
    EXPECT_EQ(project.workspace.size, Math::Vector3f(5.0f, 5.0f, 5.0f));
    EXPECT_EQ(project.workspace.defaultResolution, VoxelData::VoxelResolution::Size_1cm);
}

TEST_F(ProjectTest, ClearMethod) {
    Project project;
    project.initializeDefaults(); // Initialize first
    
    // Add some data
    project.metadata.name = "Test Project";
    project.customData["test"] = std::vector<uint8_t>{1, 2, 3};
    if (project.currentSelection) {
        project.namedSelections.push_back({"selection1", *project.currentSelection});
    }
    
    // Clear should reset everything
    project.clear();
    
    EXPECT_TRUE(project.metadata.name.empty());
    // After clear, components should be re-initialized
    EXPECT_NE(project.voxelData, nullptr);
    EXPECT_NE(project.groupData, nullptr);
    EXPECT_NE(project.camera, nullptr);
    EXPECT_NE(project.currentSelection, nullptr);
    EXPECT_TRUE(project.namedSelections.empty());
    EXPECT_TRUE(project.customData.empty());
}

TEST_F(ProjectTest, IsValidMethod) {
    Project emptyProject;
    emptyProject.voxelData = nullptr;  // Make it invalid
    
    // Empty project is not valid
    EXPECT_FALSE(emptyProject.isValid());
    
    // Create project with default initialization
    Project project;
    project.initializeDefaults();
    project.metadata.name = "Valid Project";
    
    // Should be valid now
    EXPECT_TRUE(project.isValid());
    
    // Remove a required component
    project.voxelData = nullptr;
    EXPECT_FALSE(project.isValid());
}

TEST_F(ProjectTest, UpdateMetadata) {
    Project project;
    project.initializeDefaults();
    
    // Set initial metadata
    project.metadata.name = "Test Project";
    project.metadata.author = "Test Author";
    
    // Get initial timestamps
    auto createdBefore = project.metadata.created;
    auto modifiedBefore = project.metadata.modified;
    
    // Wait a tiny bit to ensure timestamp difference
    std::this_thread::sleep_for(std::chrono::milliseconds(10));
    
    // Update metadata
    project.updateMetadata();
    
    // Created time should not change
    EXPECT_EQ(project.metadata.created, createdBefore);
    
    // Modified time should be updated
    EXPECT_GT(project.metadata.modified, modifiedBefore);
    
    // Version should be set to current
    EXPECT_EQ(project.metadata.version, FileVersion::Current());
}

TEST_F(ProjectTest, CustomProperties) {
    Project project;
    
    // Set custom property
    project.setCustomProperty("key1", "value1");
    project.setCustomProperty("key2", "value2");
    
    // Get custom property
    std::string value1 = project.getCustomProperty("key1");
    std::string value2 = project.getCustomProperty("key2");
    std::string value3 = project.getCustomProperty("nonexistent");
    
    EXPECT_EQ(value1, "value1");
    EXPECT_EQ(value2, "value2");
    EXPECT_TRUE(value3.empty());
    
    // Check custom property exists by checking if value is not empty
    EXPECT_FALSE(project.getCustomProperty("key1").empty());
    EXPECT_FALSE(project.getCustomProperty("key2").empty());
    EXPECT_TRUE(project.getCustomProperty("nonexistent").empty());
    
    // Remove custom property by setting it to empty
    project.setCustomProperty("key1", "");
    EXPECT_TRUE(project.getCustomProperty("key1").empty());
    EXPECT_FALSE(project.getCustomProperty("key2").empty());
}

TEST_F(ProjectTest, CustomData) {
    Project project;
    
    // Add custom binary data
    std::vector<uint8_t> data1 = {1, 2, 3, 4, 5};
    std::vector<uint8_t> data2 = {10, 20, 30};
    
    project.customData["binary1"] = data1;
    project.customData["binary2"] = data2;
    
    // Verify data
    EXPECT_EQ(project.customData.size(), 2);
    EXPECT_EQ(project.customData["binary1"], data1);
    EXPECT_EQ(project.customData["binary2"], data2);
}

TEST_F(ProjectTest, NamedSelections) {
    Project project;
    project.initializeDefaults();
    
    // Use the currentSelection which is already initialized
    ASSERT_NE(project.currentSelection, nullptr);
    
    // Create copies of the selection for named selections
    Selection::SelectionSet selection1 = *project.currentSelection;
    Selection::SelectionSet selection2 = *project.currentSelection;
    
    project.namedSelections.push_back({"Front View", selection1});
    project.namedSelections.push_back({"Important Voxels", selection2});
    
    // Verify selections
    EXPECT_EQ(project.namedSelections.size(), 2);
    EXPECT_EQ(project.namedSelections[0].first, "Front View");
    // Can't directly compare SelectionSet objects without equality operator
    // Just verify the structure is correct
    EXPECT_EQ(project.namedSelections[1].first, "Important Voxels");
}

TEST_F(ProjectTest, CalculateSize) {
    Project project;
    
    // Project with default initialization should have minimal size
    size_t emptySize = project.calculateSize();
    EXPECT_GT(emptySize, 0);  // At least metadata size
    
    // Add data
    project.metadata.name = "Large Project";
    project.metadata.description = "This is a test project with lots of data";
    
    // Add custom data
    std::vector<uint8_t> largeData(1000, 0xFF);
    project.customData["large"] = largeData;
    
    size_t fullSize = project.calculateSize();
    EXPECT_GT(fullSize, emptySize);
    EXPECT_GE(fullSize, emptySize + largeData.size());
}

TEST_F(ProjectTest, WorkspaceSettings) {
    Project project;
    
    // Check default workspace settings
    EXPECT_EQ(project.workspace.size, Math::Vector3f(5.0f, 5.0f, 5.0f));
    EXPECT_EQ(project.workspace.origin, Math::Vector3f(0, 0, 0));
    EXPECT_EQ(project.workspace.defaultResolution, VoxelData::VoxelResolution::Size_1cm);
    EXPECT_TRUE(project.workspace.gridVisible);
    EXPECT_TRUE(project.workspace.axesVisible);
    
    // Modify workspace settings
    project.workspace.size = Math::Vector3f(10.0f, 10.0f, 10.0f);
    project.workspace.defaultResolution = VoxelData::VoxelResolution::Size_4cm;
    project.workspace.gridVisible = false;
    
    // Verify changes
    EXPECT_EQ(project.workspace.size, Math::Vector3f(10.0f, 10.0f, 10.0f));
    EXPECT_EQ(project.workspace.defaultResolution, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_FALSE(project.workspace.gridVisible);
}

TEST_F(ProjectTest, SharedPointerBehavior) {
    Project project1;
    project1.initializeDefaults();
    
    // Create another project sharing some components
    Project project2;
    project2.initializeDefaults();
    project2.voxelData = project1.voxelData;  // Share voxel data
    // project2.groupData stays as initialized (different group data)
    project2.camera = project1.camera;  // Share camera
    // project2.currentSelection stays as initialized (different selection)
    
    // Verify shared components have same pointer
    EXPECT_EQ(project1.voxelData, project2.voxelData);
    EXPECT_EQ(project1.camera, project2.camera);
    
    // Verify non-shared components have different pointers
    EXPECT_NE(project1.groupData, project2.groupData);
    EXPECT_NE(project1.currentSelection, project2.currentSelection);
}

} // namespace FileIO
} // namespace VoxelEditor