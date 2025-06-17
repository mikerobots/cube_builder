#include "../include/file_io/Project.h"
#include "events/EventDispatcher.h"

namespace VoxelEditor {
namespace FileIO {

// Project implementation
Project::Project() {
    // Don't automatically initialize - let user call initializeDefaults if needed
}

Project::~Project() = default;

void Project::initializeDefaults() {
    // Initialize metadata
    metadata.created = std::chrono::system_clock::now();
    metadata.modified = metadata.created;
    metadata.version = FileVersion::Current();
    metadata.applicationVersion = "1.0.0";
    
    // Create default components
    auto eventDispatcher = std::make_shared<Events::EventDispatcher>();
    
    voxelData = std::make_shared<VoxelData::VoxelDataManager>(eventDispatcher.get());
    groupData = std::make_shared<Groups::GroupManager>(voxelData.get(), eventDispatcher.get());
    camera = std::make_shared<Camera::OrbitCamera>();
    currentSelection = std::make_shared<Selection::SelectionSet>();
    
    // Set default workspace
    workspace.size = Math::Vector3f(5.0f, 5.0f, 5.0f);
    workspace.defaultResolution = VoxelData::VoxelResolution::Size_1cm;
}

bool Project::isValid() const {
    if (!voxelData || !groupData || !camera || !currentSelection) {
        return false;
    }
    
    // Additional validation can be added here
    return true;
}

size_t Project::calculateSize() const {
    size_t size = sizeof(ProjectMetadata);
    size += sizeof(WorkspaceSettings);
    
    if (voxelData) {
        // Estimate voxel data size
        size += voxelData->getTotalVoxelCount() * (sizeof(Math::Vector3i) + sizeof(Rendering::Color));
    }
    
    if (groupData) {
        // Estimate group data size
        size += groupData->getGroupCount() * 256; // Rough estimate per group
    }
    
    // Add selection and custom data sizes
    size += namedSelections.size() * 1024; // Rough estimate
    for (const auto& [key, data] : customData) {
        size += key.size() + data.size();
    }
    
    return size;
}

void Project::clear() {
    voxelData.reset();
    groupData.reset();
    camera.reset();
    currentSelection.reset();
    namedSelections.clear();
    customData.clear();
    m_hasUnsavedChanges = false;
    
    // Reset metadata
    metadata = ProjectMetadata();
    
    initializeDefaults();
}

void Project::updateMetadata() {
    metadata.modified = std::chrono::system_clock::now();
    m_hasUnsavedChanges = true;
}

void Project::setName(const std::string& name) {
    metadata.name = name;
    updateMetadata();
}

void Project::setDescription(const std::string& description) {
    metadata.description = description;
    updateMetadata();
}

void Project::setAuthor(const std::string& author) {
    metadata.author = author;
    updateMetadata();
}

void Project::setCustomProperty(const std::string& key, const std::string& value) {
    metadata.customProperties[key] = value;
    updateMetadata();
}

std::string Project::getCustomProperty(const std::string& key) const {
    auto it = metadata.customProperties.find(key);
    return (it != metadata.customProperties.end()) ? it->second : "";
}

void Project::saveCurrentSelection(const std::string& name) {
    if (currentSelection) {
        namedSelections.emplace_back(name, *currentSelection);
        updateMetadata();
    }
}

bool Project::loadNamedSelection(const std::string& name) {
    for (const auto& [selName, selection] : namedSelections) {
        if (selName == name) {
            *currentSelection = selection;
            return true;
        }
    }
    return false;
}

void Project::deleteNamedSelection(const std::string& name) {
    auto it = std::remove_if(namedSelections.begin(), namedSelections.end(),
        [&name](const auto& pair) { return pair.first == name; });
    
    if (it != namedSelections.end()) {
        namedSelections.erase(it, namedSelections.end());
        updateMetadata();
    }
}

std::vector<std::string> Project::getNamedSelectionList() const {
    std::vector<std::string> names;
    names.reserve(namedSelections.size());
    
    for (const auto& [name, selection] : namedSelections) {
        names.push_back(name);
    }
    
    return names;
}

void Project::setCustomData(const std::string& key, const std::vector<uint8_t>& data) {
    customData[key] = data;
    updateMetadata();
}

bool Project::getCustomData(const std::string& key, std::vector<uint8_t>& data) const {
    auto it = customData.find(key);
    if (it != customData.end()) {
        data = it->second;
        return true;
    }
    return false;
}

void Project::removeCustomData(const std::string& key) {
    if (customData.erase(key) > 0) {
        updateMetadata();
    }
}

std::vector<std::string> Project::getCustomDataKeys() const {
    std::vector<std::string> keys;
    keys.reserve(customData.size());
    
    for (const auto& [key, data] : customData) {
        keys.push_back(key);
    }
    
    return keys;
}

bool Project::hasUnsavedChanges() const {
    return m_hasUnsavedChanges;
}

std::chrono::system_clock::time_point Project::getLastModified() const {
    return metadata.modified;
}

size_t Project::getVoxelCount() const {
    return voxelData ? voxelData->getTotalVoxelCount() : 0;
}

size_t Project::getGroupCount() const {
    return groupData ? groupData->getGroupCount() : 0;
}

Math::BoundingBox Project::getContentBounds() const {
    if (voxelData) {
        // TODO: Implement bounds calculation
        return Math::BoundingBox();
    }
    return Math::BoundingBox();
}

// ProjectFactory implementation
std::unique_ptr<Project> ProjectFactory::createNewProject(const std::string& name,
                                                         const Math::Vector3f& workspaceSize) {
    auto project = std::make_unique<Project>();
    
    project->setName(name);
    project->workspace.size = workspaceSize;
    
    // Set up workspace in voxel data manager
    if (project->voxelData) {
        // The voxel data manager would need to support workspace size setting
        // This is a placeholder for the actual implementation
    }
    
    return project;
}

std::unique_ptr<Project> ProjectFactory::createFromTemplate(const std::string& templateName) {
    auto project = createNewProject("From Template");
    applyTemplate(*project, templateName);
    return project;
}

std::vector<std::string> ProjectFactory::getAvailableTemplates() {
    return {
        "Empty",
        "Small Workspace (2m)",
        "Medium Workspace (5m)",
        "Large Workspace (8m)",
        "Grid Pattern",
        "Test Scene"
    };
}

void ProjectFactory::applyTemplate(Project& project, const std::string& templateName) {
    if (templateName == "Small Workspace (2m)") {
        project.workspace.size = Math::Vector3f(2.0f, 2.0f, 2.0f);
    } else if (templateName == "Medium Workspace (5m)") {
        project.workspace.size = Math::Vector3f(5.0f, 5.0f, 5.0f);
    } else if (templateName == "Large Workspace (8m)") {
        project.workspace.size = Math::Vector3f(8.0f, 8.0f, 8.0f);
    } else if (templateName == "Grid Pattern") {
        // Add a grid of voxels
        if (project.voxelData) {
            for (int x = -5; x <= 5; x += 2) {
                for (int z = -5; z <= 5; z += 2) {
                    project.voxelData->setVoxel(
                        Math::Vector3i(x, 0, z),
                        VoxelData::VoxelResolution::Size_32cm,
                        true
                    );
                }
            }
        }
    } else if (templateName == "Test Scene") {
        // Add various test voxels
        if (project.voxelData) {
            // Add a cube
            for (int x = 0; x < 3; ++x) {
                for (int y = 0; y < 3; ++y) {
                    for (int z = 0; z < 3; ++z) {
                        project.voxelData->setVoxel(
                            Math::Vector3i(x, y, z),
                            VoxelData::VoxelResolution::Size_16cm,
                            true
                        );
                    }
                }
            }
        }
    }
}

// ProjectValidator implementation
bool ProjectValidator::validate(const Project& project, std::vector<std::string>& errors) const {
    errors.clear();
    
    if (!validateMetadata(project.metadata, errors)) {
        return false;
    }
    
    if (!project.voxelData) {
        errors.push_back("Missing voxel data");
        return false;
    }
    
    if (!project.groupData) {
        errors.push_back("Missing group data");
        return false;
    }
    
    if (!project.camera) {
        errors.push_back("Missing camera");
        return false;
    }
    
    if (!validateVoxelData(*project.voxelData, errors)) {
        return false;
    }
    
    if (!validateGroupData(*project.groupData, errors)) {
        return false;
    }
    
    if (!validateSelections(project, errors)) {
        return false;
    }
    
    if (!validateWorkspace(project.workspace, errors)) {
        return false;
    }
    
    return errors.empty();
}

bool ProjectValidator::repair(Project& project, std::vector<std::string>& repairLog) {
    repairLog.clear();
    
    // Ensure all components exist
    if (!project.voxelData) {
        // Project will initialize defaults in constructor
        repairLog.push_back("Recreated missing components");
    }
    
    if (project.voxelData) {
        repairVoxelData(*project.voxelData, repairLog);
    }
    
    if (project.groupData) {
        repairGroupData(*project.groupData, repairLog);
    }
    
    repairSelections(project, repairLog);
    
    return true;
}

bool ProjectValidator::validateMetadata(const ProjectMetadata& metadata, std::vector<std::string>& errors) const {
    if (metadata.name.empty()) {
        errors.push_back("Project name is empty");
    }
    
    if (metadata.created > metadata.modified) {
        errors.push_back("Creation time is after modification time");
    }
    
    return true;
}

bool ProjectValidator::validateVoxelData(const VoxelData::VoxelDataManager& voxelData, std::vector<std::string>& errors) const {
    // Validate voxel data integrity
    // This would check for things like:
    // - Voxels within workspace bounds
    // - Valid resolution values
    // - Color values within range
    
    return true;
}

bool ProjectValidator::validateGroupData(const Groups::GroupManager& groupData, std::vector<std::string>& errors) const {
    // Validate group data
    if (!groupData.validateGroups()) {
        errors.push_back("Group data validation failed");
        return false;
    }
    
    return true;
}

bool ProjectValidator::validateSelections(const Project& project, std::vector<std::string>& errors) const {
    // Validate selections contain valid voxels
    // This would check that selected voxels actually exist in the voxel data
    
    return true;
}

bool ProjectValidator::validateWorkspace(const WorkspaceSettings& workspace, std::vector<std::string>& errors) const {
    if (workspace.size.x <= 0 || workspace.size.y <= 0 || workspace.size.z <= 0) {
        errors.push_back("Invalid workspace size");
        return false;
    }
    
    if (workspace.size.x > 10.0f || workspace.size.y > 10.0f || workspace.size.z > 10.0f) {
        errors.push_back("Workspace size exceeds maximum (10m)");
        return false;
    }
    
    return true;
}

bool ProjectValidator::repairVoxelData(VoxelData::VoxelDataManager& voxelData, std::vector<std::string>& repairLog) {
    // Remove voxels outside workspace bounds
    // Fix invalid resolution values
    // etc.
    
    return true;
}

bool ProjectValidator::repairGroupData(Groups::GroupManager& groupData, std::vector<std::string>& repairLog) {
    // Clean up empty groups
    groupData.cleanupEmptyGroups();
    repairLog.push_back("Cleaned up empty groups");
    
    return true;
}

bool ProjectValidator::repairSelections(Project& project, std::vector<std::string>& repairLog) {
    // Remove selections with invalid voxels
    // Ensure current selection exists
    
    if (!project.currentSelection) {
        project.currentSelection = std::make_shared<Selection::SelectionSet>();
        repairLog.push_back("Created missing current selection");
    }
    
    return true;
}

} // namespace FileIO
} // namespace VoxelEditor