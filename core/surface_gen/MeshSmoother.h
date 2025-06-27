#pragma once

#include "SurfaceTypes.h"
#include "TopologyPreserver.h"
#include <functional>
#include <atomic>

namespace VoxelEditor {
namespace SurfaceGen {

/**
 * @brief Progressive mesh smoothing system for converting blocky voxel meshes to smooth surfaces
 * 
 * Implements multiple smoothing algorithms to transform blocky voxel-based meshes into
 * smooth, organic shapes suitable for 3D printing. Supports user-controllable smoothing
 * levels (0-10+) while preserving topology (loops, holes, complex geometry).
 * 
 * @requirements REQ-10.1.8, REQ-10.1.10, REQ-10.1.12, REQ-10.1.13
 */
class MeshSmoother {
public:
    /**
     * @brief Smoothing algorithms available for different effects
     */
    enum class Algorithm {
        None,          ///< Level 0: No smoothing, raw dual contouring output
        Laplacian,     ///< Levels 1-3: Basic smoothing, removes blockiness
        Taubin,        ///< Levels 4-7: Feature-preserving smoothing
        BiLaplacian    ///< Levels 8-10+: Aggressive smoothing for organic shapes
    };

    /**
     * @brief Configuration for smoothing operations
     */
    struct SmoothingConfig {
        int smoothingLevel = 0;              ///< User-controllable level (0-10+)
        Algorithm algorithm = Algorithm::None; ///< Algorithm to use (auto-selected if not specified)
        bool preserveTopology = true;        ///< Maintain loops, holes, complex geometry
        bool preserveBoundaries = true;      ///< Keep mesh boundaries fixed
        float minFeatureSize = 1.0f;         ///< Minimum feature size in mm for 3D printing
        PreviewQuality previewQuality = PreviewQuality::Disabled; ///< Preview optimization level
        bool usePreviewQuality = false;      ///< Deprecated: use previewQuality instead
    };

    /**
     * @brief Progress callback for real-time updates during smoothing
     * @param progress Progress value between 0.0 and 1.0
     * @return True to continue, false to cancel operation
     */
    using ProgressCallback = std::function<bool(float progress)>;

    MeshSmoother();
    ~MeshSmoother();

    /**
     * @brief Apply smoothing to a mesh based on configuration
     * @param inputMesh The blocky voxel mesh to smooth
     * @param config Smoothing configuration
     * @param progressCallback Optional callback for progress updates
     * @return Smoothed mesh, or empty mesh if cancelled
     * @requirements REQ-10.1.8, REQ-10.1.10, REQ-10.1.12
     */
    Mesh smooth(const Mesh& inputMesh, const SmoothingConfig& config, 
                ProgressCallback progressCallback = nullptr);

    /**
     * @brief Get the recommended algorithm for a given smoothing level
     * @param level Smoothing level (0-10+)
     * @return Recommended algorithm
     */
    static Algorithm getAlgorithmForLevel(int level);

    /**
     * @brief Get the number of iterations for a given smoothing level
     * @param level Smoothing level (0-10+)
     * @param algorithm The algorithm being used
     * @return Number of smoothing iterations
     */
    static int getIterationsForLevel(int level, Algorithm algorithm);

    /**
     * @brief Cancel any ongoing smoothing operation
     */
    void cancelSmoothing() { m_cancelled = true; }

    /**
     * @brief Check if the last operation was cancelled
     */
    bool wasCancelled() const { return m_cancelled; }

private:
    /**
     * @brief Apply Laplacian smoothing algorithm with topology preservation
     * @param mesh Mesh to smooth (modified in place)
     * @param iterations Number of smoothing iterations
     * @param lambda Smoothing factor (0-1)
     * @param constraints Topology preservation constraints
     * @param progressCallback Progress updates
     * @return True if completed, false if cancelled
     */
    bool applyLaplacianSmoothingWithTopology(Mesh& mesh, int iterations, float lambda,
                                  const TopologyPreserver::TopologyConstraints& constraints,
                                  ProgressCallback progressCallback);

    /**
     * @brief Apply Taubin λ-μ smoothing algorithm with topology preservation
     * @param mesh Mesh to smooth (modified in place)
     * @param iterations Number of smoothing iterations
     * @param lambda Positive smoothing factor
     * @param mu Negative smoothing factor (typically -lambda * 1.02)
     * @param constraints Topology preservation constraints
     * @param progressCallback Progress updates
     * @return True if completed, false if cancelled
     */
    bool applyTaubinSmoothingWithTopology(Mesh& mesh, int iterations, float lambda, float mu,
                              const TopologyPreserver::TopologyConstraints& constraints,
                              ProgressCallback progressCallback);

    /**
     * @brief Apply BiLaplacian smoothing algorithm with topology preservation
     * @param mesh Mesh to smooth (modified in place)
     * @param iterations Number of smoothing iterations
     * @param constraints Topology preservation constraints
     * @param progressCallback Progress updates
     * @return True if completed, false if cancelled
     */
    bool applyBiLaplacianSmoothingWithTopology(Mesh& mesh, int iterations,
                                   const TopologyPreserver::TopologyConstraints& constraints,
                                   ProgressCallback progressCallback);
    
    /**
     * @brief Apply Laplacian smoothing algorithm (basic version)
     * @param mesh Mesh to smooth (modified in place)
     * @param iterations Number of smoothing iterations
     * @param lambda Smoothing factor (0-1)
     * @param preserveBoundaries Keep boundaries fixed
     * @param progressCallback Progress updates
     * @return True if completed, false if cancelled
     */
    bool applyLaplacianSmoothing(Mesh& mesh, int iterations, float lambda,
                                  bool preserveBoundaries, ProgressCallback progressCallback);

    /**
     * @brief Compute vertex neighbors for smoothing operations
     * @param mesh Input mesh
     * @return Vector of neighbor indices for each vertex
     */
    std::vector<std::vector<uint32_t>> computeVertexNeighbors(const Mesh& mesh);

    /**
     * @brief Identify boundary vertices that should not be moved
     * @param mesh Input mesh
     * @return Set of boundary vertex indices
     */
    std::unordered_set<uint32_t> identifyBoundaryVertices(const Mesh& mesh);

    /**
     * @brief Apply minimum feature size constraint
     * @param mesh Mesh to validate/fix
     * @param minFeatureSize Minimum feature size in mm
     */
    void enforceMinimumFeatureSize(Mesh& mesh, float minFeatureSize);

    std::atomic<bool> m_cancelled;
};

} // namespace SurfaceGen
} // namespace VoxelEditor