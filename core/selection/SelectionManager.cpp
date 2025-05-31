#include "SelectionManager.h"
#include "../../foundation/logging/Logger.h"
#include <algorithm>

namespace VoxelEditor {
namespace Selection {

SelectionManager::SelectionManager(VoxelData::VoxelDataManager* voxelManager,
                                 ::VoxelEditor::Events::EventDispatcher* eventDispatcher)
    : m_previewMode(false)
    , m_maxHistorySize(100)
    , m_voxelManager(voxelManager)
    , m_eventDispatcher(eventDispatcher) {
}

void SelectionManager::selectVoxel(const VoxelId& voxel) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.add(voxel);
    notifySelectionChanged(oldSelection, SelectionChangeType::Added);
}

void SelectionManager::deselectVoxel(const VoxelId& voxel) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.remove(voxel);
    notifySelectionChanged(oldSelection, SelectionChangeType::Removed);
}

void SelectionManager::toggleVoxel(const VoxelId& voxel) {
    if (m_currentSelection.contains(voxel)) {
        deselectVoxel(voxel);
    } else {
        selectVoxel(voxel);
    }
}

void SelectionManager::selectAll() {
    if (!m_voxelManager) {
        Logging::Logger::getInstance().warning("SelectionManager::selectAll: No voxel manager set");
        return;
    }
    
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.clear();
    
    // Get all voxels from the voxel manager
    std::vector<VoxelId> allVoxels = getAllVoxels();
    m_currentSelection.addRange(allVoxels);
    
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::selectNone() {
    if (m_currentSelection.empty()) return;
    
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.clear();
    notifySelectionChanged(oldSelection, SelectionChangeType::Cleared);
}

void SelectionManager::selectInverse() {
    if (!m_voxelManager) {
        Logging::Logger::getInstance().warning("SelectionManager::selectInverse: No voxel manager set");
        return;
    }
    
    SelectionSet oldSelection = m_currentSelection;
    SelectionSet newSelection;
    
    // Get all voxels and add those not in current selection
    std::vector<VoxelId> allVoxels = getAllVoxels();
    for (const auto& voxel : allVoxels) {
        if (!m_currentSelection.contains(voxel)) {
            newSelection.add(voxel);
        }
    }
    
    m_currentSelection = std::move(newSelection);
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::selectByResolution(VoxelData::VoxelResolution resolution) {
    if (!m_voxelManager) {
        Logging::Logger::getInstance().warning("SelectionManager::selectByResolution: No voxel manager set");
        return;
    }
    
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.clear();
    
    // Select all voxels with the given resolution
    std::vector<VoxelId> allVoxels = getAllVoxels();
    for (const auto& voxel : allVoxels) {
        if (voxel.resolution == resolution) {
            m_currentSelection.add(voxel);
        }
    }
    
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::selectBox(const Math::BoundingBox& box, VoxelData::VoxelResolution resolution) {
    SelectionSet boxSelection = makeBoxSelection(box, resolution);
    select(boxSelection, SelectionMode::Replace);
}

void SelectionManager::selectSphere(const Math::Vector3f& center, float radius, VoxelData::VoxelResolution resolution) {
    SelectionSet sphereSelection = makeSphereSelection(center, radius, resolution);
    select(sphereSelection, SelectionMode::Replace);
}

void SelectionManager::selectCylinder(const Math::Vector3f& base, const Math::Vector3f& direction, 
                                     float radius, float height, VoxelData::VoxelResolution resolution) {
    SelectionSet cylinderSelection = makeCylinderSelection(base, direction, radius, height, resolution);
    select(cylinderSelection, SelectionMode::Replace);
}

void SelectionManager::selectFloodFill(const VoxelId& seed, FloodFillCriteria criteria) {
    if (!m_voxelManager || !voxelExists(seed)) {
        return;
    }
    
    SelectionSet oldSelection = m_currentSelection;
    SelectionSet floodSelection;
    
    // TODO: Implement flood fill algorithm
    // For now, just select the seed
    floodSelection.add(seed);
    
    m_currentSelection = std::move(floodSelection);
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::select(const SelectionSet& selection, SelectionMode mode) {
    SelectionSet oldSelection = m_currentSelection;
    applySelectionMode(selection, mode);
    notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
}

void SelectionManager::selectVoxel(const VoxelId& voxel, SelectionMode mode) {
    SelectionSet selection;
    selection.add(voxel);
    select(selection, mode);
}

void SelectionManager::selectRegion(const SelectionRegion& region, VoxelData::VoxelResolution resolution, SelectionMode mode) {
    SelectionSet regionSelection;
    
    switch (region.type) {
        case SelectionRegion::Box:
            regionSelection = makeBoxSelection(region.box, resolution);
            break;
        case SelectionRegion::Sphere:
            regionSelection = makeSphereSelection(region.center, region.radius, resolution);
            break;
        case SelectionRegion::Cylinder:
            regionSelection = makeCylinderSelection(region.center, region.direction, region.radius, region.height, resolution);
            break;
        default:
            Logging::Logger::getInstance().warning("SelectionManager::selectRegion: Unsupported region type");
            return;
    }
    
    select(regionSelection, mode);
}

bool SelectionManager::isSelected(const VoxelId& voxel) const {
    return m_currentSelection.contains(voxel);
}

void SelectionManager::pushSelectionToHistory() {
    m_undoStack.push(m_currentSelection);
    // Clear redo stack when new action is performed
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }
    trimHistory();
}

void SelectionManager::undoSelection() {
    if (m_undoStack.empty()) return;
    
    SelectionSet oldSelection = m_currentSelection;
    m_redoStack.push(m_currentSelection);
    m_currentSelection = m_undoStack.top();
    m_undoStack.pop();
    
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::redoSelection() {
    if (m_redoStack.empty()) return;
    
    SelectionSet oldSelection = m_currentSelection;
    m_undoStack.push(m_currentSelection);
    m_currentSelection = m_redoStack.top();
    m_redoStack.pop();
    
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::clearHistory() {
    while (!m_undoStack.empty()) {
        m_undoStack.pop();
    }
    while (!m_redoStack.empty()) {
        m_redoStack.pop();
    }
}

void SelectionManager::saveSelectionSet(const std::string& name) {
    m_namedSets[name] = m_currentSelection;
}

bool SelectionManager::loadSelectionSet(const std::string& name) {
    auto it = m_namedSets.find(name);
    if (it == m_namedSets.end()) {
        return false;
    }
    
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection = it->second;
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
    return true;
}

std::vector<std::string> SelectionManager::getSelectionSetNames() const {
    std::vector<std::string> names;
    names.reserve(m_namedSets.size());
    for (const auto& pair : m_namedSets) {
        names.push_back(pair.first);
    }
    return names;
}

void SelectionManager::deleteSelectionSet(const std::string& name) {
    m_namedSets.erase(name);
}

bool SelectionManager::hasSelectionSet(const std::string& name) const {
    return m_namedSets.find(name) != m_namedSets.end();
}

void SelectionManager::clearSelectionSets() {
    m_namedSets.clear();
}

void SelectionManager::unionWith(const SelectionSet& other) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.unite(other);
    notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
}

void SelectionManager::intersectWith(const SelectionSet& other) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.intersect(other);
    notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
}

void SelectionManager::subtractFrom(const SelectionSet& other) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.subtractFrom(other);
    notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
}

void SelectionManager::filterSelection(const SelectionPredicate& predicate) {
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection.filterInPlace(predicate);
    notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
}

SelectionSet SelectionManager::getFilteredSelection(const SelectionPredicate& predicate) const {
    return m_currentSelection.filter(predicate);
}

void SelectionManager::validateSelection() {
    if (!m_voxelManager) return;
    
    SelectionSet oldSelection = m_currentSelection;
    
    // Remove non-existent voxels
    m_currentSelection.filterInPlace([this](const VoxelId& voxel) {
        return voxelExists(voxel);
    });
    
    if (oldSelection != m_currentSelection) {
        notifySelectionChanged(oldSelection, SelectionChangeType::Modified);
    }
}

bool SelectionManager::isValidSelection() const {
    if (!m_voxelManager) return true;
    
    for (const auto& voxel : m_currentSelection) {
        if (!voxelExists(voxel)) {
            return false;
        }
    }
    return true;
}

void SelectionManager::applyPreview() {
    if (!m_previewMode) return;
    
    SelectionSet oldSelection = m_currentSelection;
    m_currentSelection = m_previewSelection;
    m_previewMode = false;
    m_previewSelection.clear();
    
    notifySelectionChanged(oldSelection, SelectionChangeType::Replaced);
}

void SelectionManager::cancelPreview() {
    if (!m_previewMode) return;
    
    m_previewMode = false;
    m_previewSelection.clear();
}

void SelectionManager::applySelectionMode(const SelectionSet& newSelection, SelectionMode mode) {
    switch (mode) {
        case SelectionMode::Replace:
            m_currentSelection = newSelection;
            break;
        case SelectionMode::Add:
            m_currentSelection.unite(newSelection);
            break;
        case SelectionMode::Subtract:
            m_currentSelection.subtractFrom(newSelection);
            break;
        case SelectionMode::Intersect:
            m_currentSelection.intersect(newSelection);
            break;
    }
}

void SelectionManager::notifySelectionChanged(const SelectionSet& oldSelection, SelectionChangeType changeType) {
    if (!m_eventDispatcher) return;
    
    Events::SelectionChangedEvent event(oldSelection, m_currentSelection, changeType);
    m_eventDispatcher->dispatch(event);
}

void SelectionManager::trimHistory() {
    while (m_undoStack.size() > m_maxHistorySize) {
        // Remove oldest item (bottom of stack)
        std::stack<SelectionSet> tempStack;
        while (m_undoStack.size() > 1) {
            tempStack.push(m_undoStack.top());
            m_undoStack.pop();
        }
        m_undoStack.pop(); // Remove the oldest
        while (!tempStack.empty()) {
            m_undoStack.push(tempStack.top());
            tempStack.pop();
        }
    }
}

bool SelectionManager::voxelExists(const VoxelId& voxel) const {
    if (!m_voxelManager) return false;
    
    // TODO: Implement actual voxel existence check using VoxelDataManager
    // For now, assume all voxels exist
    return true;
}

std::vector<VoxelId> SelectionManager::getAllVoxels() const {
    std::vector<VoxelId> allVoxels;
    
    if (!m_voxelManager) {
        return allVoxels;
    }
    
    // TODO: Implement getting all voxels from VoxelDataManager
    // For now, return empty vector
    return allVoxels;
}

}
}