#pragma once

#include <unordered_map>
#include <string>

#include "InputTypes.h"

namespace VoxelEditor {
namespace Input {

// Input mapping configuration
struct InputMapping {
    // Mouse mappings
    std::unordered_map<uint32_t, std::string> mouseButtons; // MouseButton -> Action name
    std::unordered_map<uint32_t, std::string> mouseGestures; // MouseGesture -> Action name
    
    // Keyboard mappings
    std::unordered_map<uint32_t, std::string> keys; // KeyCode -> Action name
    std::unordered_map<uint64_t, std::string> keyCombinations; // KeyCombination hash -> Action name
    
    // Touch mappings
    std::unordered_map<uint32_t, std::string> touchGestures; // TouchGesture -> Action name
    
    // VR mappings
    std::unordered_map<uint32_t, std::string> vrGestures; // VRGesture -> Action name
    std::unordered_map<uint32_t, std::string> handPoses; // HandPose type -> Action name
    
    // Sensitivity settings
    float mouseSensitivity = 1.0f;
    float touchSensitivity = 1.0f;
    float vrSensitivity = 1.0f;
    
    // Mouse configuration
    float mouseClickTimeout = 0.3f;
    float mouseDoubleClickTimeout = 0.5f;
    float mouseDragThreshold = 5.0f;
    
    // Touch configuration
    float touchTapTimeout = 0.3f;
    float touchTapRadius = 20.0f;
    float touchPinchThreshold = 50.0f;
    float touchSwipeThreshold = 100.0f;
    
    // VR configuration
    VRComfortSettings vrComfortSettings = VRComfortSettings::Default();
    
    InputMapping() = default;
    
    // Factory methods for common configurations
    static InputMapping Default();
    static InputMapping Gaming();
    static InputMapping Accessibility();
    static InputMapping VROptimized();
    
    // Serialization
    bool saveToFile(const std::string& filename) const;
    bool loadFromFile(const std::string& filename);
    std::string toJson() const;
    bool fromJson(const std::string& json);
    
    // Helper methods
    void bindMouseButton(MouseButton button, const std::string& action, ModifierFlags modifiers = ModifierFlags::None);
    void bindKey(KeyCode key, const std::string& action, ModifierFlags modifiers = ModifierFlags::None);
    void bindKeyCombination(const KeyCombination& combo, const std::string& action);
    void bindTouchGesture(TouchGesture gesture, const std::string& action);
    void bindVRGesture(VRGesture gesture, const std::string& action);
    
    // Query methods
    std::string getMouseButtonAction(MouseButton button, ModifierFlags modifiers = ModifierFlags::None) const;
    std::string getKeyAction(KeyCode key, ModifierFlags modifiers = ModifierFlags::None) const;
    std::string getKeyCombinationAction(const KeyCombination& combo) const;
    std::string getTouchGestureAction(TouchGesture gesture) const;
    std::string getVRGestureAction(VRGesture gesture) const;
    
    // Validation
    bool isValid() const;
    std::vector<std::string> validate() const;
    
private:
    // Helper methods for key combination hashing
    uint64_t hashKeyCombination(const KeyCombination& combo) const;
    uint32_t hashMouseButton(MouseButton button, ModifierFlags modifiers) const;
    uint32_t hashKey(KeyCode key, ModifierFlags modifiers) const;
};

// Predefined action names
namespace Actions {
    // Voxel operations
    constexpr const char* PLACE_VOXEL = "place_voxel";
    constexpr const char* REMOVE_VOXEL = "remove_voxel";
    constexpr const char* PAINT_VOXEL = "paint_voxel";
    constexpr const char* SAMPLE_VOXEL = "sample_voxel";
    
    // Camera controls
    constexpr const char* ORBIT_CAMERA = "orbit_camera";
    constexpr const char* PAN_CAMERA = "pan_camera";
    constexpr const char* ZOOM_CAMERA = "zoom_camera";
    constexpr const char* RESET_CAMERA = "reset_camera";
    constexpr const char* FRAME_SELECTION = "frame_selection";
    
    // Selection
    constexpr const char* SELECT_VOXEL = "select_voxel";
    constexpr const char* SELECT_MULTIPLE = "select_multiple";
    constexpr const char* SELECT_BOX = "select_box";
    constexpr const char* DESELECT_ALL = "deselect_all";
    constexpr const char* INVERT_SELECTION = "invert_selection";
    
    // Groups
    constexpr const char* CREATE_GROUP = "create_group";
    constexpr const char* SELECT_GROUP = "select_group";
    constexpr const char* MOVE_GROUP = "move_group";
    constexpr const char* DELETE_GROUP = "delete_group";
    
    // Edit operations
    constexpr const char* UNDO = "undo";
    constexpr const char* REDO = "redo";
    constexpr const char* COPY = "copy";
    constexpr const char* PASTE = "paste";
    constexpr const char* CUT = "cut";
    constexpr const char* DELETE = "delete";
    
    // View controls
    constexpr const char* TOGGLE_WIREFRAME = "toggle_wireframe";
    constexpr const char* TOGGLE_GRID = "toggle_grid";
    constexpr const char* CYCLE_VIEW_MODE = "cycle_view_mode";
    constexpr const char* ZOOM_IN = "zoom_in";
    constexpr const char* ZOOM_OUT = "zoom_out";
    
    // File operations
    constexpr const char* NEW_FILE = "new_file";
    constexpr const char* OPEN_FILE = "open_file";
    constexpr const char* SAVE_FILE = "save_file";
    constexpr const char* SAVE_AS = "save_as";
    constexpr const char* EXPORT = "export";
    
    // Tool selection
    constexpr const char* SELECT_TOOL_PLACE = "select_tool_place";
    constexpr const char* SELECT_TOOL_REMOVE = "select_tool_remove";
    constexpr const char* SELECT_TOOL_PAINT = "select_tool_paint";
    constexpr const char* SELECT_TOOL_SELECT = "select_tool_select";
    constexpr const char* SELECT_TOOL_MOVE = "select_tool_move";
    
    // VR specific
    constexpr const char* VR_GRAB = "vr_grab";
    constexpr const char* VR_POINT = "vr_point";
    constexpr const char* VR_MENU = "vr_menu";
    constexpr const char* VR_TELEPORT = "vr_teleport";
    constexpr const char* VR_SCALE = "vr_scale";
    constexpr const char* VR_ROTATE = "vr_rotate";
}

}
}