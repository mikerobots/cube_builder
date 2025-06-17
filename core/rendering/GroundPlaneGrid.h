#pragma once

#include "RenderTypes.h"
#include "../../foundation/math/Vector3f.h"
#include "../../foundation/math/Vector2f.h"
#include "../../foundation/math/Matrix4f.h"
#include <memory>

namespace VoxelEditor {
namespace Rendering {

// Forward declarations
class ShaderManager;
class OpenGLRenderer;

/**
 * @brief Renders a ground plane grid for voxel placement
 * 
 * This class handles rendering of the ground plane grid at Y=0 with:
 * - 32cm x 32cm grid squares
 * - Major grid lines every 160cm (5 squares)
 * - Dynamic opacity based on cursor proximity
 * - Center at workspace origin (0,0,0)
 */
class GroundPlaneGrid {
public:
    /**
     * @brief Constructor
     * @param shaderManager Shader manager for loading grid shaders
     * @param glRenderer OpenGL renderer for GPU operations
     */
    GroundPlaneGrid(ShaderManager* shaderManager, OpenGLRenderer* glRenderer);
    ~GroundPlaneGrid();
    
    /**
     * @brief Initialize the grid renderer
     * @return true if initialization successful
     */
    bool initialize();
    
    /**
     * @brief Update grid mesh for given workspace size
     * @param workspaceSize Size of the workspace in meters
     */
    void updateGridMesh(const Math::Vector3f& workspaceSize);
    
    /**
     * @brief Update grid state with smooth transitions
     * @param deltaTime Time since last update in seconds
     */
    void update(float deltaTime);
    
    /**
     * @brief Set the cursor position for opacity calculations
     * @param cursorWorldPos Cursor position in world space
     */
    void setCursorPosition(const Math::Vector3f& cursorWorldPos);
    
    /**
     * @brief Render the ground plane grid
     * @param viewMatrix Camera view matrix
     * @param projMatrix Camera projection matrix
     */
    void render(const Math::Matrix4f& viewMatrix, 
               const Math::Matrix4f& projMatrix);
    
    /**
     * @brief Set opacity parameters
     * @param baseOpacity Base opacity when cursor is far (default 0.35)
     * @param nearOpacity Opacity when cursor is near (default 0.65)
     * @param transitionSpeed Speed of opacity transitions (default 5.0)
     */
    void setOpacityParameters(float baseOpacity = 0.35f,
                             float nearOpacity = 0.65f,
                             float transitionSpeed = 5.0f);
    
    /**
     * @brief Set whether grid should be rendered
     * @param visible true to show grid, false to hide
     */
    void setVisible(bool visible) { m_visible = visible; }
    
    /**
     * @brief Check if grid is visible
     * @return true if grid is being rendered
     */
    bool isVisible() const { return m_visible; }
    
    /**
     * @brief Get the grid cell size (32cm)
     * @return Grid cell size in meters
     */
    static constexpr float getGridCellSize() { return 0.32f; } // 32cm
    
    /**
     * @brief Get the major grid line interval (160cm = 5 cells)
     * @return Major grid line interval in meters
     */
    static constexpr float getMajorLineInterval() { return 1.6f; } // 160cm
    
private:
    // Grid rendering data
    struct GridVertex {
        Math::Vector3f position;
        float isMajorLine; // 0.0 for minor, 1.0 for major
        
        GridVertex(const Math::Vector3f& pos, bool major) 
            : position(pos), isMajorLine(major ? 1.0f : 0.0f) {}
    };
    
    // GPU resources
    VertexArrayId m_vao;
    VertexBufferId m_vbo;
    ShaderId m_shader;
    size_t m_lineCount;
    
    // Dependencies
    ShaderManager* m_shaderManager;
    OpenGLRenderer* m_glRenderer;
    
    // State
    bool m_initialized;
    bool m_visible;
    Math::Vector3f m_currentWorkspaceSize;
    
    // Opacity parameters
    float m_baseOpacity;
    float m_nearOpacity;
    float m_transitionSpeed;
    
    // Dynamic state
    Math::Vector3f m_cursorPosition;
    Math::Vector3f m_smoothedCursorPosition;
    float m_currentOpacity;
    float m_targetOpacity;
    
    // Grid generation
    void generateGridMesh(const Math::Vector3f& workspaceSize);
    bool loadShader();
    void cleanup();
    
    // Grid visual constants
    static constexpr float MinorLineColorValue = 180.0f / 255.0f;
    static constexpr float MajorLineColorValue = 200.0f / 255.0f;
    static constexpr float CursorSmoothingFactor = 10.0f;
    static constexpr float DefaultLineWidth = 1.0f;
    static constexpr int MajorLineInterval = 5; // Major line every 5 minor lines
    static constexpr float MajorLineVisibilityMultiplier = 1.2f;
    
    // Colors for grid lines
    static Math::Vector3f MinorLineColor() { 
        return Math::Vector3f(MinorLineColorValue, MinorLineColorValue, MinorLineColorValue); 
    }
    
    static Math::Vector3f MajorLineColor() { 
        return Math::Vector3f(MajorLineColorValue, MajorLineColorValue, MajorLineColorValue); 
    }
    
    // Proximity fade parameters
    static constexpr float ProximityRadius = 2.0f; // 2 grid squares = 64cm
};

} // namespace Rendering
} // namespace VoxelEditor