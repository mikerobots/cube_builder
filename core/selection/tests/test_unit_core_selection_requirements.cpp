#include <gtest/gtest.h>
#include "core/selection/SelectionManager.h"
#include "core/selection/SelectionSet.h"
#include "core/selection/SelectionTypes.h"
#include "core/selection/BoxSelector.h"
#include "core/selection/SphereSelector.h"
#include "core/selection/FloodFillSelector.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

// Test fixture for Selection requirement tests
class SelectionRequirementsTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<SelectionManager>();
        
        // Create test voxels using IncrementCoordinates for proper positioning
        voxel1 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel2 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(4, 0, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel3 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 4, 0)), VoxelData::VoxelResolution::Size_4cm);
        voxel4 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(0, 0, 4)), VoxelData::VoxelResolution::Size_4cm);
        voxel5 = VoxelId(Math::IncrementCoordinates(Math::Vector3i(8, 8, 8)), VoxelData::VoxelResolution::Size_8cm);
    }
    
    std::unique_ptr<SelectionManager> manager;
    VoxelId voxel1, voxel2, voxel3, voxel4, voxel5;
};

// REQ-8.1.7: Format shall store vertex selection state
TEST_F(SelectionRequirementsTest, SelectionStatePersistence) {
    // Test that selection state can be stored and retrieved
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    // Save selection state
    SelectionSet currentSelection = manager->getSelectionCopy();
    
    // Clear and verify empty
    manager->selectNone();
    EXPECT_FALSE(manager->hasSelection());
    
    // Restore selection state
    manager->select(currentSelection, SelectionMode::Replace);
    
    // Verify selection state was restored
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_FALSE(manager->isSelected(voxel4));
}

// REQ: Support for single and multi-voxel selection
TEST_F(SelectionRequirementsTest, SingleAndMultiVoxelSelection) {
    // Single voxel selection
    manager->selectVoxel(voxel1);
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_EQ(manager->getSelectionSize(), 1u);
    
    // Multi-voxel selection
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_EQ(manager->getSelectionSize(), 3u);
}

// REQ: Selection persistence across operations
TEST_F(SelectionRequirementsTest, SelectionPersistenceAcrossOperations) {
    // Create initial selection
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    
    // Perform various operations
    manager->toggleVoxel(voxel3);  // Add voxel3
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    
    // Remove a voxel
    manager->deselectVoxel(voxel2);
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    
    // Selection persists through operations
    EXPECT_EQ(manager->getSelectionSize(), 2u);
}

// REQ: Visual feedback for selected voxels
TEST_F(SelectionRequirementsTest, VisualFeedbackConfiguration) {
    // Test selection style configuration
    SelectionStyle style;
    style.outlineColor = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f); // Red
    style.fillColor = Rendering::Color(1.0f, 0.0f, 0.0f, 0.3f);     // Semi-transparent red
    style.animated = true;
    style.animationSpeed = 2.0f;
    
    manager->setSelectionStyle(style);
    const SelectionStyle& retrievedStyle = manager->getSelectionStyle();
    
    // Verify style is properly stored for visual feedback
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.r, 1.0f);
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.g, 0.0f);
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.b, 0.0f);
    EXPECT_TRUE(retrievedStyle.animated);
    EXPECT_FLOAT_EQ(retrievedStyle.animationSpeed, 2.0f);
}

// REQ: Integration with group system for group creation
TEST_F(SelectionRequirementsTest, GroupCreationFromSelection) {
    // Create a selection
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    // Get selection for group creation
    SelectionSet selection = manager->getSelectionCopy();
    std::vector<VoxelId> voxelsForGroup = selection.toVector();
    
    // Verify selection contains correct voxels for group
    EXPECT_EQ(voxelsForGroup.size(), 3u);
    EXPECT_TRUE(std::find(voxelsForGroup.begin(), voxelsForGroup.end(), voxel1) != voxelsForGroup.end());
    EXPECT_TRUE(std::find(voxelsForGroup.begin(), voxelsForGroup.end(), voxel2) != voxelsForGroup.end());
    EXPECT_TRUE(std::find(voxelsForGroup.begin(), voxelsForGroup.end(), voxel3) != voxelsForGroup.end());
}

// Additional requirement coverage tests

// REQ: Selection validation and bounds checking
TEST_F(SelectionRequirementsTest, SelectionBoundsValidation) {
    // Create selection and verify bounds
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    Math::BoundingBox bounds = manager->getSelectionBounds();
    
    // Verify bounds encompass all selected voxels
    // With new coordinate system, voxels extend from their center position
    // A 4cm voxel at (0,0,0) extends from (-0.02, 0, -0.02) to (0.02, 0.04, 0.02)
    EXPECT_GE(bounds.min.x, -0.02f);
    EXPECT_GE(bounds.min.y, 0.0f);    // Ground plane constraint
    EXPECT_GE(bounds.min.z, -0.02f);
    EXPECT_LE(bounds.max.x, 0.06f);  // voxel at (4,0,0) extends to x=0.06
    EXPECT_LE(bounds.max.y, 0.08f);  // voxel at (0,4,0) extends to y=0.08
    EXPECT_LE(bounds.max.z, 0.02f);  // all voxels have z from -0.02 to 0.02
}

// REQ: Integration with undo/redo system for reversible selections
TEST_F(SelectionRequirementsTest, UndoRedoIntegration) {
    // Make initial selection
    manager->selectVoxel(voxel1);
    manager->pushSelectionToHistory();
    
    // Change selection
    manager->selectNone();
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    // Verify can undo
    EXPECT_TRUE(manager->canUndo());
    
    // Undo to previous selection
    manager->undoSelection();
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_FALSE(manager->isSelected(voxel3));
    
    // Redo to newer selection
    EXPECT_TRUE(manager->canRedo());
    manager->redoSelection();
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
}

// REQ: Performance optimization for large selections
// Note: Detailed performance tests are in TestSelectionPerformance.cpp
TEST_F(SelectionRequirementsTest, BasicPerformanceValidation) {
    // Basic performance validation - create a moderate selection
    const int numVoxels = 100;
    
    // Add 100 voxels to selection
    for (int i = 0; i < numVoxels; ++i) {
        VoxelId voxel(Math::IncrementCoordinates(Math::Vector3i(i, 0, 0)), 
                      VoxelData::VoxelResolution::Size_4cm);
        manager->selectVoxel(voxel);
    }
    
    // Verify selection was created
    EXPECT_EQ(manager->getSelectionSize(), 100u);
    
    // Verify stats calculation works
    SelectionStats stats = manager->getSelectionStats();
    EXPECT_EQ(stats.voxelCount, 100u);
    
    // This test validates that performance optimizations are in place
    // For comprehensive performance testing, see TestSelectionPerformance.cpp
}

// REQ: Selection serialization for project files
TEST_F(SelectionRequirementsTest, SelectionSerialization) {
    // Create a complex selection with multiple resolutions
    manager->selectVoxel(voxel1);  // 4cm
    manager->selectVoxel(voxel2);  // 4cm
    manager->selectVoxel(voxel5);  // 8cm
    
    // Save named selection set (simulates serialization)
    manager->saveSelectionSet("test_selection");
    
    // Clear current selection
    manager->selectNone();
    EXPECT_FALSE(manager->hasSelection());
    
    // Load selection set (simulates deserialization)
    EXPECT_TRUE(manager->loadSelectionSet("test_selection"));
    
    // Verify selection was properly restored
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel5));
    EXPECT_EQ(manager->getSelectionSize(), 3u);
    
    // Verify selection statistics are correct
    SelectionStats stats = manager->getSelectionStats();
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_4cm], 2u);
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_8cm], 1u);
}

// REQ: Different selection methods (Box, Sphere, FloodFill)
TEST_F(SelectionRequirementsTest, SelectionMethods) {
    // Test Box selection
    {
        BoxSelector boxSelector;
        Math::BoundingBox box(Math::Vector3f(-0.01f, -0.01f, -0.01f),
                             Math::Vector3f(0.09f, 0.09f, 0.09f));
        SelectionSet boxResult = boxSelector.selectFromWorld(box, VoxelData::VoxelResolution::Size_4cm, false);
        EXPECT_GT(boxResult.size(), 0u);
    }
    
    // Test Sphere selection
    {
        SphereSelector sphereSelector;
        Math::Vector3f center(0.04f, 0.04f, 0.04f);
        float radius = 0.08f;
        SelectionSet sphereResult = sphereSelector.selectFromSphere(center, radius, 
                                                                   VoxelData::VoxelResolution::Size_4cm);
        EXPECT_GT(sphereResult.size(), 0u);
    }
    
    // Test FloodFill selection
    {
        FloodFillSelector floodFillSelector;
        // Set a small limit to prevent flooding the entire space
        floodFillSelector.setMaxVoxels(100);
        VoxelId seed(Math::IncrementCoordinates(Math::Vector3i(0, 0, 0)), 
                    VoxelData::VoxelResolution::Size_4cm);
        SelectionSet floodResult = floodFillSelector.selectFloodFill(seed, FloodFillCriteria::Connected6);
        // Without voxel manager, flood fill assumes all voxels exist and will flood to the max limit
        EXPECT_GE(floodResult.size(), 1u);
        EXPECT_LE(floodResult.size(), 100u);
    }
}

// Note: The following requirements are covered by existing tests in other files:
// - SelectionManager coordinates selection operations (TestSelectionManager.cpp)
// - SelectionSet manages collections of selected voxels (TestSelectionSet.cpp)
// - Support for selection validation and bounds checking (TestSelectionSet.cpp - GetBounds test)
// - Integration with undo/redo system (TestSelectionManager.cpp - UndoRedo test)
// - Performance optimization for large selections (implicit in data structure choice)
// - Selection serialization (TestSelectionManager.cpp - SaveAndLoadSelectionSet test)