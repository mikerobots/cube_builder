#pragma once

#include <stack>
#include <unordered_map>
#include <memory>

#include "SelectionTypes.h"
#include "SelectionSet.h"
#include "../voxel_data/VoxelDataManager.h"
#include "../../foundation/events/EventDispatcher.h"

namespace VoxelEditor {
namespace Selection {

class SelectionManager {
public:
    explicit SelectionManager(VoxelData::VoxelDataManager* voxelManager = nullptr,
                            ::VoxelEditor::Events::EventDispatcher* eventDispatcher = nullptr);
    ~SelectionManager() = default;
    
    // Disable copy
    SelectionManager(const SelectionManager&) = delete;
    SelectionManager& operator=(const SelectionManager&) = delete;
    
    // Basic selection operations
    void selectVoxel(const VoxelId& voxel);
    void deselectVoxel(const VoxelId& voxel);
    void toggleVoxel(const VoxelId& voxel);
    
    // Multi-selection operations
    void selectAll();
    void selectNone();
    void selectInverse();
    void selectByResolution(VoxelData::VoxelResolution resolution);
    
    // Region selection
    void selectBox(const Math::BoundingBox& box, VoxelData::VoxelResolution resolution);
    void selectSphere(const Math::Vector3f& center, float radius, VoxelData::VoxelResolution resolution);
    void selectCylinder(const Math::Vector3f& base, const Math::Vector3f& direction, 
                       float radius, float height, VoxelData::VoxelResolution resolution);
    void selectFloodFill(const VoxelId& seed, FloodFillCriteria criteria = FloodFillCriteria::Connected);
    
    // Selection with mode
    void select(const SelectionSet& selection, SelectionMode mode = SelectionMode::Replace);
    void selectVoxel(const VoxelId& voxel, SelectionMode mode);
    void selectRegion(const SelectionRegion& region, VoxelData::VoxelResolution resolution, SelectionMode mode);
    
    // Selection queries
    bool isSelected(const VoxelId& voxel) const;
    const SelectionSet& getSelection() const { return m_currentSelection; }
    SelectionSet getSelectionCopy() const { return m_currentSelection; }
    size_t getSelectionSize() const { return m_currentSelection.size(); }
    bool hasSelection() const { return !m_currentSelection.empty(); }
    Math::BoundingBox getSelectionBounds() const { return m_currentSelection.getBounds(); }
    SelectionStats getSelectionStats() const { return m_currentSelection.getStats(); }
    
    // Selection history
    void pushSelectionToHistory();
    bool canUndo() const { return !m_undoStack.empty(); }
    bool canRedo() const { return !m_redoStack.empty(); }
    void undoSelection();
    void redoSelection();
    void clearHistory();
    void setMaxHistorySize(size_t size) { m_maxHistorySize = size; }
    size_t getMaxHistorySize() const { return m_maxHistorySize; }
    
    // Selection sets (named selections)
    void saveSelectionSet(const std::string& name);
    bool loadSelectionSet(const std::string& name);
    std::vector<std::string> getSelectionSetNames() const;
    void deleteSelectionSet(const std::string& name);
    bool hasSelectionSet(const std::string& name) const;
    void clearSelectionSets();
    
    // Set operations
    void unionWith(const SelectionSet& other);
    void intersectWith(const SelectionSet& other);
    void subtractFrom(const SelectionSet& other);
    
    // Filtering
    void filterSelection(const SelectionPredicate& predicate);
    SelectionSet getFilteredSelection(const SelectionPredicate& predicate) const;
    
    // Validation
    void validateSelection();  // Remove non-existent voxels
    bool isValidSelection() const;
    
    // Configuration
    void setVoxelManager(VoxelData::VoxelDataManager* manager) { m_voxelManager = manager; }
    VoxelData::VoxelDataManager* getVoxelManager() const { return m_voxelManager; }
    void setEventDispatcher(Events::EventDispatcher* dispatcher) { m_eventDispatcher = dispatcher; }
    
    // Selection style
    void setSelectionStyle(const SelectionStyle& style) { m_selectionStyle = style; }
    const SelectionStyle& getSelectionStyle() const { return m_selectionStyle; }
    
    // Preview mode
    void setPreviewMode(bool enabled) { m_previewMode = enabled; }
    bool isPreviewMode() const { return m_previewMode; }
    void applyPreview();
    void cancelPreview();
    const SelectionSet& getPreviewSelection() const { return m_previewSelection; }
    
private:
    // Core data
    SelectionSet m_currentSelection;
    SelectionSet m_previewSelection;
    bool m_previewMode;
    
    // History
    std::stack<SelectionSet> m_undoStack;
    std::stack<SelectionSet> m_redoStack;
    size_t m_maxHistorySize;
    
    // Named selection sets
    std::unordered_map<std::string, SelectionSet> m_namedSets;
    
    // Dependencies
    VoxelData::VoxelDataManager* m_voxelManager;
    ::VoxelEditor::Events::EventDispatcher* m_eventDispatcher;
    
    // Configuration
    SelectionStyle m_selectionStyle;
    
    // Helper methods
    void applySelectionMode(const SelectionSet& newSelection, SelectionMode mode);
    void notifySelectionChanged(const SelectionSet& oldSelection, SelectionChangeType changeType);
    void trimHistory();
    bool voxelExists(const VoxelId& voxel) const;
    std::vector<VoxelId> getAllVoxels() const;
};

// Selection events
namespace Events {

struct SelectionChangedEvent : public ::VoxelEditor::Events::Event<SelectionChangedEvent> {
    SelectionSet oldSelection;
    SelectionSet newSelection;
    SelectionChangeType changeType;
    
    SelectionChangedEvent(const SelectionSet& oldSel, const SelectionSet& newSel, SelectionChangeType type)
        : oldSelection(oldSel), newSelection(newSel), changeType(type) {}
    
    const char* getEventType() const override { return "SelectionChangedEvent"; }
};

struct SelectionOperationEvent : public ::VoxelEditor::Events::Event<SelectionOperationEvent> {
    SelectionOperationType operationType;
    SelectionSet affectedVoxels;
    bool success;
    
    SelectionOperationEvent(SelectionOperationType type, const SelectionSet& voxels, bool result)
        : operationType(type), affectedVoxels(voxels), success(result) {}
    
    const char* getEventType() const override { return "SelectionOperationEvent"; }
};

}

}
}