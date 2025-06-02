#pragma once

#include "FileTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "groups/GroupManager.h"
#include "selection/SelectionSet.h"
#include "camera/OrbitCamera.h"
#include <memory>
#include <vector>

namespace VoxelEditor {
namespace FileIO {

// Forward declarations
class FileValidator;

// Project data structure containing all saveable state
struct Project {
    // Metadata
    ProjectMetadata metadata;
    
    // Core data
    std::shared_ptr<VoxelData::VoxelDataManager> voxelData;
    std::shared_ptr<Groups::GroupManager> groupData;
    
    // View state
    std::shared_ptr<Camera::OrbitCamera> camera;
    
    // Selection state
    std::shared_ptr<Selection::SelectionSet> currentSelection;
    std::vector<std::pair<std::string, Selection::SelectionSet>> namedSelections;
    
    // Settings
    WorkspaceSettings workspace;
    
    // Additional data
    std::unordered_map<std::string, std::vector<uint8_t>> customData;
    
    // Constructors
    Project();
    ~Project();
    
    // Validation
    bool isValid() const;
    size_t calculateSize() const;
    void clear();
    
    // Metadata management
    void updateMetadata();
    void setName(const std::string& name);
    void setDescription(const std::string& description);
    void setAuthor(const std::string& author);
    void setCustomProperty(const std::string& key, const std::string& value);
    std::string getCustomProperty(const std::string& key) const;
    
    // Selection management
    void saveCurrentSelection(const std::string& name);
    bool loadNamedSelection(const std::string& name);
    void deleteNamedSelection(const std::string& name);
    std::vector<std::string> getNamedSelectionList() const;
    
    // Custom data management
    void setCustomData(const std::string& key, const std::vector<uint8_t>& data);
    bool getCustomData(const std::string& key, std::vector<uint8_t>& data) const;
    void removeCustomData(const std::string& key);
    std::vector<std::string> getCustomDataKeys() const;
    
    // State queries
    bool hasUnsavedChanges() const;
    std::chrono::system_clock::time_point getLastModified() const;
    size_t getVoxelCount() const;
    size_t getGroupCount() const;
    Math::BoundingBox getContentBounds() const;
    
private:
    mutable bool m_hasUnsavedChanges = false;
    
    // Initialize default components
    void initializeDefaults();
};

// Project factory for creating new projects
class ProjectFactory {
public:
    static std::unique_ptr<Project> createNewProject(
        const std::string& name = "Untitled",
        const Math::Vector3f& workspaceSize = Math::Vector3f(5.0f, 5.0f, 5.0f));
    
    static std::unique_ptr<Project> createFromTemplate(
        const std::string& templateName);
    
    static std::vector<std::string> getAvailableTemplates();
    
private:
    static void applyTemplate(Project& project, const std::string& templateName);
};

// Project validation
class ProjectValidator {
public:
    bool validate(const Project& project, std::vector<std::string>& errors) const;
    bool repair(Project& project, std::vector<std::string>& repairLog);
    
private:
    bool validateMetadata(const ProjectMetadata& metadata, std::vector<std::string>& errors) const;
    bool validateVoxelData(const VoxelData::VoxelDataManager& voxelData, std::vector<std::string>& errors) const;
    bool validateGroupData(const Groups::GroupManager& groupData, std::vector<std::string>& errors) const;
    bool validateSelections(const Project& project, std::vector<std::string>& errors) const;
    bool validateWorkspace(const WorkspaceSettings& workspace, std::vector<std::string>& errors) const;
    
    bool repairVoxelData(VoxelData::VoxelDataManager& voxelData, std::vector<std::string>& repairLog);
    bool repairGroupData(Groups::GroupManager& groupData, std::vector<std::string>& repairLog);
    bool repairSelections(Project& project, std::vector<std::string>& repairLog);
};

} // namespace FileIO
} // namespace VoxelEditor