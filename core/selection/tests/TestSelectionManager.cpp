#include <gtest/gtest.h>
#include "core/selection/SelectionManager.h"

using namespace VoxelEditor;
using namespace VoxelEditor::Selection;

class SelectionManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        manager = std::make_unique<SelectionManager>();
        
        // Create test voxels
        voxel1 = VoxelId(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_4cm);
        voxel2 = VoxelId(Math::Vector3i(1, 0, 0), VoxelData::VoxelResolution::Size_4cm);
        voxel3 = VoxelId(Math::Vector3i(0, 1, 0), VoxelData::VoxelResolution::Size_4cm);
        voxel4 = VoxelId(Math::Vector3i(0, 0, 1), VoxelData::VoxelResolution::Size_4cm);
        voxel5 = VoxelId(Math::Vector3i(1, 1, 1), VoxelData::VoxelResolution::Size_8cm);
    }
    
    std::unique_ptr<SelectionManager> manager;
    VoxelId voxel1, voxel2, voxel3, voxel4, voxel5;
};

// Basic Selection Tests
TEST_F(SelectionManagerTest, InitialState) {
    EXPECT_FALSE(manager->hasSelection());
    EXPECT_EQ(manager->getSelectionSize(), 0u);
    EXPECT_TRUE(manager->getSelection().empty());
}

TEST_F(SelectionManagerTest, SelectVoxel) {
    manager->selectVoxel(voxel1);
    
    EXPECT_TRUE(manager->hasSelection());
    EXPECT_EQ(manager->getSelectionSize(), 1u);
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
}

TEST_F(SelectionManagerTest, DeselectVoxel) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->deselectVoxel(voxel1);
    
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_EQ(manager->getSelectionSize(), 1u);
}

TEST_F(SelectionManagerTest, ToggleVoxel) {
    EXPECT_FALSE(manager->isSelected(voxel1));
    
    manager->toggleVoxel(voxel1);
    EXPECT_TRUE(manager->isSelected(voxel1));
    
    manager->toggleVoxel(voxel1);
    EXPECT_FALSE(manager->isSelected(voxel1));
}

// Multi-Selection Operations Tests
TEST_F(SelectionManagerTest, SelectNone) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    manager->selectNone();
    EXPECT_FALSE(manager->hasSelection());
    EXPECT_EQ(manager->getSelectionSize(), 0u);
}

TEST_F(SelectionManagerTest, SelectAll) {
    // Without voxel manager, selectAll should just log warning
    manager->selectAll();
    EXPECT_FALSE(manager->hasSelection());
}

TEST_F(SelectionManagerTest, SelectInverse) {
    // Without voxel manager, selectInverse should just log warning
    manager->selectVoxel(voxel1);
    manager->selectInverse();
    EXPECT_TRUE(manager->isSelected(voxel1)); // Should remain unchanged
}

// Selection Mode Tests
TEST_F(SelectionManagerTest, SelectWithReplaceMode) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    
    SelectionSet newSelection = {voxel3, voxel4};
    manager->select(newSelection, SelectionMode::Replace);
    
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_TRUE(manager->isSelected(voxel4));
    EXPECT_EQ(manager->getSelectionSize(), 2u);
}

TEST_F(SelectionManagerTest, SelectWithAddMode) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    
    SelectionSet newSelection = {voxel3, voxel4};
    manager->select(newSelection, SelectionMode::Add);
    
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_TRUE(manager->isSelected(voxel4));
    EXPECT_EQ(manager->getSelectionSize(), 4u);
}

TEST_F(SelectionManagerTest, SelectWithSubtractMode) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    SelectionSet toRemove = {voxel2, voxel3};
    manager->select(toRemove, SelectionMode::Subtract);
    
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_FALSE(manager->isSelected(voxel3));
    EXPECT_EQ(manager->getSelectionSize(), 1u);
}

TEST_F(SelectionManagerTest, SelectWithIntersectMode) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    SelectionSet toIntersect = {voxel2, voxel3, voxel4};
    manager->select(toIntersect, SelectionMode::Intersect);
    
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_FALSE(manager->isSelected(voxel4));
    EXPECT_EQ(manager->getSelectionSize(), 2u);
}

// Region Selection Tests
TEST_F(SelectionManagerTest, SelectBox) {
    Math::BoundingBox box(Math::Vector3f(0.0f, 0.0f, 0.0f), 
                          Math::Vector3f(0.1f, 0.1f, 0.1f));
    
    manager->selectBox(box, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_GT(manager->getSelectionSize(), 0u);
}

TEST_F(SelectionManagerTest, SelectSphere) {
    Math::Vector3f center(0.05f, 0.05f, 0.05f);
    float radius = 0.1f;
    
    manager->selectSphere(center, radius, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_GT(manager->getSelectionSize(), 0u);
}

TEST_F(SelectionManagerTest, SelectCylinder) {
    Math::Vector3f base(0.0f, 0.0f, 0.0f);
    Math::Vector3f direction(0.0f, 1.0f, 0.0f);
    float radius = 0.1f;
    float height = 0.2f;
    
    manager->selectCylinder(base, direction, radius, height, VoxelData::VoxelResolution::Size_4cm);
    EXPECT_GT(manager->getSelectionSize(), 0u);
}

// Selection History Tests
TEST_F(SelectionManagerTest, UndoRedo) {
    // Initial state
    EXPECT_FALSE(manager->canUndo());
    EXPECT_FALSE(manager->canRedo());
    
    // Make first selection and save to history
    manager->selectVoxel(voxel1);
    manager->pushSelectionToHistory();
    
    // Replace with second selection
    manager->selectNone();
    manager->selectVoxel(voxel2);
    
    EXPECT_TRUE(manager->canUndo());
    EXPECT_FALSE(manager->canRedo());
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_FALSE(manager->isSelected(voxel1));
    
    // Undo - should restore to previous state (only voxel1 selected)
    manager->undoSelection();
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_FALSE(manager->canUndo()); // Should be at beginning of history
    EXPECT_TRUE(manager->canRedo());
    
    // Redo - should restore to only voxel2 selected
    manager->redoSelection();
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->canUndo());
    EXPECT_FALSE(manager->canRedo());
}

TEST_F(SelectionManagerTest, ClearHistory) {
    manager->selectVoxel(voxel1);
    manager->pushSelectionToHistory();
    manager->selectVoxel(voxel2);
    manager->pushSelectionToHistory();
    
    EXPECT_TRUE(manager->canUndo());
    
    manager->clearHistory();
    EXPECT_FALSE(manager->canUndo());
    EXPECT_FALSE(manager->canRedo());
}

TEST_F(SelectionManagerTest, MaxHistorySize) {
    manager->setMaxHistorySize(3);
    EXPECT_EQ(manager->getMaxHistorySize(), 3u);
    
    // Add more than max history
    for (int i = 0; i < 5; ++i) {
        manager->selectVoxel(voxel1);
        manager->pushSelectionToHistory();
        manager->selectNone();
    }
    
    // Should only be able to undo 3 times
    int undoCount = 0;
    while (manager->canUndo() && undoCount < 10) {
        manager->undoSelection();
        undoCount++;
    }
    EXPECT_EQ(undoCount, 3);
}

// Named Selection Sets Tests
TEST_F(SelectionManagerTest, SaveAndLoadSelectionSet) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    manager->saveSelectionSet("test_set");
    EXPECT_TRUE(manager->hasSelectionSet("test_set"));
    
    // Clear and load
    manager->selectNone();
    EXPECT_FALSE(manager->hasSelection());
    
    EXPECT_TRUE(manager->loadSelectionSet("test_set"));
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
}

TEST_F(SelectionManagerTest, GetSelectionSetNames) {
    manager->selectVoxel(voxel1);
    manager->saveSelectionSet("set1");
    
    manager->selectVoxel(voxel2);
    manager->saveSelectionSet("set2");
    
    std::vector<std::string> names = manager->getSelectionSetNames();
    EXPECT_EQ(names.size(), 2u);
    EXPECT_TRUE(std::find(names.begin(), names.end(), "set1") != names.end());
    EXPECT_TRUE(std::find(names.begin(), names.end(), "set2") != names.end());
}

TEST_F(SelectionManagerTest, DeleteSelectionSet) {
    manager->selectVoxel(voxel1);
    manager->saveSelectionSet("test_set");
    EXPECT_TRUE(manager->hasSelectionSet("test_set"));
    
    manager->deleteSelectionSet("test_set");
    EXPECT_FALSE(manager->hasSelectionSet("test_set"));
    EXPECT_FALSE(manager->loadSelectionSet("test_set"));
}

TEST_F(SelectionManagerTest, ClearSelectionSets) {
    manager->selectVoxel(voxel1);
    manager->saveSelectionSet("set1");
    manager->saveSelectionSet("set2");
    
    manager->clearSelectionSets();
    EXPECT_FALSE(manager->hasSelectionSet("set1"));
    EXPECT_FALSE(manager->hasSelectionSet("set2"));
}

// Set Operations Tests
TEST_F(SelectionManagerTest, UnionWith) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    
    SelectionSet other = {voxel2, voxel3};
    manager->unionWith(other);
    
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_EQ(manager->getSelectionSize(), 3u);
}

TEST_F(SelectionManagerTest, IntersectWith) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    SelectionSet other = {voxel2, voxel3, voxel4};
    manager->intersectWith(other);
    
    EXPECT_FALSE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_FALSE(manager->isSelected(voxel4));
    EXPECT_EQ(manager->getSelectionSize(), 2u);
}

TEST_F(SelectionManagerTest, SubtractFrom) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    SelectionSet other = {voxel2, voxel3, voxel4};
    manager->subtractFrom(other);
    
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_FALSE(manager->isSelected(voxel2));
    EXPECT_FALSE(manager->isSelected(voxel3));
    EXPECT_EQ(manager->getSelectionSize(), 1u);
}

// Filtering Tests
TEST_F(SelectionManagerTest, FilterSelection) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    manager->selectVoxel(voxel4);
    manager->selectVoxel(voxel5);
    
    // Filter for 4cm resolution only
    auto predicate = [](const VoxelId& v) { 
        return v.resolution == VoxelData::VoxelResolution::Size_4cm; 
    };
    
    manager->filterSelection(predicate);
    
    EXPECT_TRUE(manager->isSelected(voxel1));
    EXPECT_TRUE(manager->isSelected(voxel2));
    EXPECT_TRUE(manager->isSelected(voxel3));
    EXPECT_TRUE(manager->isSelected(voxel4));
    EXPECT_FALSE(manager->isSelected(voxel5));
    EXPECT_EQ(manager->getSelectionSize(), 4u);
}

TEST_F(SelectionManagerTest, GetFilteredSelection) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    manager->selectVoxel(voxel4);
    manager->selectVoxel(voxel5);
    
    // Filter for positive x coordinates
    auto predicate = [](const VoxelId& v) { 
        return v.position.x() > 0; 
    };
    
    SelectionSet filtered = manager->getFilteredSelection(predicate);
    
    // Original selection unchanged
    EXPECT_EQ(manager->getSelectionSize(), 5u);
    
    // Filtered set has only positive x
    EXPECT_EQ(filtered.size(), 2u);
    EXPECT_TRUE(filtered.contains(voxel2));
    EXPECT_TRUE(filtered.contains(voxel5));
}

// Style and Configuration Tests
TEST_F(SelectionManagerTest, SelectionStyle) {
    SelectionStyle style;
    style.outlineColor = Rendering::Color(1.0f, 0.0f, 0.0f, 1.0f);
    style.animated = false;
    
    manager->setSelectionStyle(style);
    const SelectionStyle& retrievedStyle = manager->getSelectionStyle();
    
    // Check color components individually since Color doesn't have operator==
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.r, style.outlineColor.r);
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.g, style.outlineColor.g);
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.b, style.outlineColor.b);
    EXPECT_FLOAT_EQ(retrievedStyle.outlineColor.a, style.outlineColor.a);
    EXPECT_EQ(retrievedStyle.animated, style.animated);
}

TEST_F(SelectionManagerTest, PreviewMode) {
    EXPECT_FALSE(manager->isPreviewMode());
    
    manager->setPreviewMode(true);
    EXPECT_TRUE(manager->isPreviewMode());
    
    // Preview selection should be empty initially
    EXPECT_TRUE(manager->getPreviewSelection().empty());
    
    // Apply preview (should do nothing with empty preview)
    manager->applyPreview();
    EXPECT_FALSE(manager->isPreviewMode());
    
    // Cancel preview
    manager->setPreviewMode(true);
    manager->cancelPreview();
    EXPECT_FALSE(manager->isPreviewMode());
}

// Bounds and Stats Tests
TEST_F(SelectionManagerTest, GetSelectionBounds) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    
    Math::BoundingBox bounds = manager->getSelectionBounds();
    EXPECT_EQ(bounds.min, Math::Vector3f(0.0f, 0.0f, 0.0f));
    EXPECT_EQ(bounds.max, Math::Vector3f(0.08f, 0.08f, 0.04f));
}

TEST_F(SelectionManagerTest, GetSelectionStats) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    manager->selectVoxel(voxel3);
    manager->selectVoxel(voxel4);
    manager->selectVoxel(voxel5);
    
    SelectionStats stats = manager->getSelectionStats();
    EXPECT_EQ(stats.voxelCount, 5u);
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_4cm], 4u);
    EXPECT_EQ(stats.countByResolution[VoxelData::VoxelResolution::Size_8cm], 1u);
}

// Copy Test
TEST_F(SelectionManagerTest, GetSelectionCopy) {
    manager->selectVoxel(voxel1);
    manager->selectVoxel(voxel2);
    
    SelectionSet copy = manager->getSelectionCopy();
    EXPECT_EQ(copy.size(), 2u);
    EXPECT_TRUE(copy.contains(voxel1));
    EXPECT_TRUE(copy.contains(voxel2));
    
    // Modifying copy should not affect manager
    copy.add(voxel3);
    EXPECT_FALSE(manager->isSelected(voxel3));
}