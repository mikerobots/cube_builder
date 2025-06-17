#pragma once

#include "rendering/RenderTypes.h"
#include "math/Vector3f.h"
#include "math/Vector2f.h"
#include <vector>

namespace VoxelEditor {
namespace ShaderTest {

/**
 * @brief Generates standard test meshes for shader validation
 * 
 * Provides a variety of mesh primitives commonly used in the voxel editor
 * to ensure shaders work correctly with typical geometry.
 */
class TestMeshGenerator {
public:
    /**
     * @brief Create a unit cube mesh
     * @param size Cube size (default 1.0)
     * @param color Vertex color for all vertices
     * @return Mesh with position, normal, and color attributes
     */
    static Rendering::Mesh createCube(float size = 1.0f, 
                                     const Math::Vector3f& color = Math::Vector3f(0.7f, 0.7f, 0.7f));
    
    /**
     * @brief Create a grid of lines on the XZ plane
     * @param divisions Number of grid divisions
     * @param spacing Distance between grid lines
     * @param majorInterval Every Nth line is a major line
     * @return Mesh with position attributes and line primitives
     */
    static Rendering::Mesh createGrid(int divisions = 10, 
                                     float spacing = 0.32f,
                                     int majorInterval = 5);
    
    /**
     * @brief Create wireframe cube using line primitives
     * @param size Cube size
     * @param color Line color
     * @return Mesh with position and color attributes, line primitives
     */
    static Rendering::Mesh createWireframeCube(float size = 1.0f,
                                              const Math::Vector3f& color = Math::Vector3f(0.1f, 0.1f, 0.1f));
    
    /**
     * @brief Create a single quad (for face highlighting)
     * @param size Quad size
     * @param normal Face normal direction
     * @return Mesh with position and normal attributes
     */
    static Rendering::Mesh createQuad(float size = 1.0f,
                                     const Math::Vector3f& normal = Math::Vector3f(0.0f, 1.0f, 0.0f));
    
    /**
     * @brief Create a cluster of voxels for performance testing
     * @param xCount Voxels in X direction
     * @param yCount Voxels in Y direction  
     * @param zCount Voxels in Z direction
     * @param voxelSize Size of each voxel
     * @param spacing Gap between voxels
     * @return Mesh containing all voxel geometry
     */
    static Rendering::Mesh createVoxelCluster(int xCount, int yCount, int zCount,
                                             float voxelSize = 0.32f,
                                             float spacing = 0.0f);
    
    /**
     * @brief Create a sphere for testing curved surfaces
     * @param radius Sphere radius
     * @param segments Number of segments (latitude)
     * @param rings Number of rings (longitude)
     * @return Mesh with smooth normals
     */
    static Rendering::Mesh createSphere(float radius = 1.0f,
                                       int segments = 16,
                                       int rings = 16);
    
    /**
     * @brief Create test geometry for highlight effects
     * @param innerSize Inner box size
     * @param outerSize Outer box size for outline
     * @return Mesh suitable for transparency testing
     */
    static Rendering::Mesh createHighlightBox(float innerSize = 0.9f,
                                             float outerSize = 1.1f);
    
    /**
     * @brief Determine primitive type for a mesh based on its name/purpose
     * @param meshType Type hint (e.g., "grid", "wireframe", "cube")
     * @return OpenGL primitive type constant
     */
    static unsigned int getPrimitiveType(const std::string& meshType);

private:
    // Helper to add a cube face to vertex/index arrays
    static void addCubeFace(std::vector<Rendering::Vertex>& vertices,
                           std::vector<unsigned int>& indices,
                           const Math::Vector3f& center,
                           const Math::Vector3f& normal,
                           const Math::Vector3f& up,
                           const Math::Vector3f& right,
                           float size,
                           const Math::Vector3f& color);
};

} // namespace ShaderTest
} // namespace VoxelEditor