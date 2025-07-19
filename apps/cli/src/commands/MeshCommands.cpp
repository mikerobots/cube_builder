#include "cli/MeshCommands.h"
#include "cli/Application.h"
#include "cli/CommandTypes.h"
#include "voxel_data/VoxelDataManager.h"
#include "surface_gen/SurfaceGenerator.h"
#include "surface_gen/MeshSmoother.h"
#include "surface_gen/MeshValidator.h"
#include "file_io/FileManager.h"
#include "file_io/STLExporter.h"
#include "rendering/RenderEngine.h"
#include "rendering/RenderTypes.h"
#include "events/EventDispatcher.h"
#include <iostream>
#include <iomanip>
#include <sstream>
#include <cstdlib>

namespace VoxelEditor {

// Register this module
REGISTER_COMMAND_MODULE(MeshCommands)

std::vector<CommandRegistration> MeshCommands::getCommands() {
    return {
        // SMOOTH command
        CommandRegistration()
            .withName(Commands::SMOOTH)
            .withDescription("Control mesh smoothing settings")
            .withCategory(CommandCategory::MESH)
            .withArg("level", "Smoothing level (0-10+) or 'preview' subcommand", "string", false, "")
            .withArg("on_off", "For 'preview': on/off", "string", false, "")
            .withArg("algorithm", "For 'algorithm': laplacian/taubin/bilaplacian", "string", false, "")
            .withHandler([this](const CommandContext& ctx) {
                if (ctx.getArgCount() == 0) {
                    // Display current smoothing settings
                    std::stringstream ss;
                    ss << "Current smoothing settings:\n";
                    ss << "  Level: " << m_app->getSmoothingLevel() << "\n";
                    
                    // Get algorithm name
                    std::string algoName;
                    switch (m_app->getSmoothingAlgorithm()) {
                        case SurfaceGen::MeshSmoother::Algorithm::None:
                            algoName = "None (raw dual contouring)";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                            algoName = "Laplacian (basic smoothing)";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                            algoName = "Taubin (feature-preserving)";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                            algoName = "BiLaplacian (aggressive smoothing)";
                            break;
                    }
                    ss << "  Algorithm: " << algoName << "\n";
                    ss << "  Preview: " << (m_app->isSmoothPreviewEnabled() ? "on" : "off") << "\n";
                    
                    return CommandResult::Success(ss.str());
                }
                
                std::string arg1 = ctx.getArg(0);
                
                // Handle subcommands
                if (arg1 == "preview") {
                    if (ctx.getArgCount() < 2) {
                        return CommandResult::Error("Usage: smooth preview on|off");
                    }
                    
                    std::string onOff = ctx.getArg(1);
                    if (onOff == "on") {
                        m_app->setSmoothPreviewEnabled(true);
                        // TODO: Trigger preview mesh generation when implemented
                        return CommandResult::Success("Smoothing preview enabled");
                    } else if (onOff == "off") {
                        m_app->setSmoothPreviewEnabled(false);
                        return CommandResult::Success("Smoothing preview disabled");
                    } else {
                        return CommandResult::Error("Invalid option. Use 'on' or 'off'");
                    }
                } else if (arg1 == "algorithm") {
                    if (ctx.getArgCount() < 2) {
                        return CommandResult::Error("Usage: smooth algorithm laplacian|taubin|bilaplacian");
                    }
                    
                    std::string algoName = ctx.getArg(1);
                    if (algoName == "laplacian") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Laplacian);
                        return CommandResult::Success("Smoothing algorithm set to Laplacian");
                    } else if (algoName == "taubin") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Taubin);
                        return CommandResult::Success("Smoothing algorithm set to Taubin");
                    } else if (algoName == "bilaplacian") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::BiLaplacian);
                        return CommandResult::Success("Smoothing algorithm set to BiLaplacian");
                    } else {
                        return CommandResult::Error("Invalid algorithm. Choose from: laplacian, taubin, bilaplacian");
                    }
                } else {
                    // Try to parse as smoothing level
                    int level = ctx.getIntArg(0, -1);
                    if (level < 0) {
                        return CommandResult::Error("Invalid smoothing level. Must be 0 or greater");
                    }
                    
                    m_app->setSmoothingLevel(level);
                    
                    // Auto-select algorithm based on level
                    m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::getAlgorithmForLevel(level));
                    
                    std::stringstream ss;
                    ss << "Smoothing level set to " << level;
                    if (level > 10) {
                        ss << " (maximum smoothing)";
                    }
                    
                    return CommandResult::Success(ss.str());
                }
            }),

        // MESH command (info, validate, etc.)
        CommandRegistration()
            .withName(Commands::MESH)
            .withDescription("Mesh validation and information")
            .withCategory(CommandCategory::MESH)
            .withAlias("mesh-info")
            .withArg("subcommand", "validate|info|repair|resolution", "string", true, "")
            .withArg("value", "For resolution: 1cm|2cm|4cm|8cm|16cm|auto", "string", false, "")
            .withHandler([this](const CommandContext& ctx) {
                std::string subcommand = ctx.getArg(0);
                
                if (subcommand == "validate" || subcommand == "mesh-validate") {
                    // Generate mesh first
                    SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher);
                    
                    // Configure surface settings with mesh resolution
                    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
                    settings.smoothingLevel = m_app->getSmoothingLevel();
                    settings.previewQuality = m_app->getMeshResolution();
                    surfaceGen.setSurfaceSettings(settings);
                    
                    auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                    
                    // Apply smoothing if enabled
                    if (m_app->getSmoothingLevel() > 0) {
                        SurfaceGen::MeshSmoother smoother;
                        SurfaceGen::MeshSmoother::SmoothingConfig config;
                        config.smoothingLevel = m_app->getSmoothingLevel();
                        config.algorithm = m_app->getSmoothingAlgorithm();
                        config.preserveTopology = true;
                        config.preserveBoundaries = true;
                        config.minFeatureSize = 1.0f;
                        config.usePreviewQuality = false;
                        
                        surfaceMesh = smoother.smooth(surfaceMesh, config);
                    }
                    
                    // Validate mesh
                    SurfaceGen::MeshValidator validator;
                    auto validation = validator.validate(surfaceMesh);
                    
                    std::stringstream ss;
                    ss << "Mesh Validation Results:\n";
                    ss << "  Watertight: " << (validation.isWatertight ? "Yes" : "No") << "\n";
                    ss << "  Manifold: " << (validation.isManifold ? "Yes" : "No") << "\n";
                    ss << "  Valid topology: " << (validation.isValid ? "Yes" : "No") << "\n";
                    
                    if (!validation.errors.empty()) {
                        ss << "\nErrors found:\n";
                        for (const auto& error : validation.errors) {
                            ss << "  - " << error << "\n";
                        }
                    }
                    
                    if (!validation.warnings.empty()) {
                        ss << "\nWarnings:\n";
                        for (const auto& warning : validation.warnings) {
                            ss << "  - " << warning << "\n";
                        }
                    }
                    
                    return CommandResult::Success(ss.str());
                    
                } else if (subcommand == "info" || subcommand == "mesh-info") {
                    // Generate mesh first
                    SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher);
                    
                    // Configure surface settings with mesh resolution
                    SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
                    settings.smoothingLevel = m_app->getSmoothingLevel();
                    settings.previewQuality = m_app->getMeshResolution();
                    surfaceGen.setSurfaceSettings(settings);
                    
                    auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                    
                    // Apply smoothing if enabled
                    if (m_app->getSmoothingLevel() > 0) {
                        SurfaceGen::MeshSmoother smoother;
                        SurfaceGen::MeshSmoother::SmoothingConfig config;
                        config.smoothingLevel = m_app->getSmoothingLevel();
                        config.algorithm = m_app->getSmoothingAlgorithm();
                        config.preserveTopology = true;
                        config.preserveBoundaries = true;
                        config.minFeatureSize = 1.0f;
                        config.usePreviewQuality = false;
                        
                        surfaceMesh = smoother.smooth(surfaceMesh, config);
                    }
                    
                    std::stringstream ss;
                    ss << "Mesh Information:\n";
                    ss << "  Vertices: " << surfaceMesh.getVertexCount() << "\n";
                    ss << "  Triangles: " << surfaceMesh.getTriangleCount() << "\n";
                    ss << "  Memory usage: " << (surfaceMesh.getMemoryUsage() / 1024) << " KB\n";
                    
                    // Calculate bounds
                    surfaceMesh.calculateBounds();
                    auto bounds = surfaceMesh.bounds;
                    ss << "\nBounding box:\n";
                    ss << "  Min: (" << bounds.min.x << ", " << bounds.min.y << ", " << bounds.min.z << ")\n";
                    ss << "  Max: (" << bounds.max.x << ", " << bounds.max.y << ", " << bounds.max.z << ")\n";
                    ss << "  Size: (" << (bounds.max.x - bounds.min.x) << ", " 
                       << (bounds.max.y - bounds.min.y) << ", " 
                       << (bounds.max.z - bounds.min.z) << ")\n";
                    
                    if (m_app->getSmoothingLevel() > 0) {
                        ss << "\nSmoothing applied:\n";
                        ss << "  Level: " << m_app->getSmoothingLevel() << "\n";
                        ss << "  Algorithm: ";
                        switch (m_app->getSmoothingAlgorithm()) {
                            case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                                ss << "Laplacian\n";
                                break;
                            case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                                ss << "Taubin\n";
                                break;
                            case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                                ss << "BiLaplacian\n";
                                break;
                            default:
                                ss << "None\n";
                        }
                    }
                    
                    return CommandResult::Success(ss.str());
                    
                } else if (subcommand == "repair") {
                    // Note: MeshBuilder repair functions need to be implemented
                    return CommandResult::Success("Mesh repair functionality is pending implementation in MeshBuilder");
                } else if (subcommand == "resolution") {
                    // Handle mesh resolution setting
                    if (ctx.getArgCount() == 1) {
                        // Show current resolution
                        auto currentRes = m_app->getMeshResolution();
                        std::string resStr;
                        switch (currentRes) {
                            case SurfaceGen::PreviewQuality::Fast:
                                resStr = "16cm (fast preview)";
                                break;
                            case SurfaceGen::PreviewQuality::Balanced:
                                resStr = "8cm (balanced)";
                                break;
                            case SurfaceGen::PreviewQuality::HighQuality:
                                resStr = "4cm (high quality)";
                                break;
                            case SurfaceGen::PreviewQuality::Disabled:
                            default:
                                resStr = "8cm (default)";
                                break;
                        }
                        return CommandResult::Success("Current mesh resolution: " + resStr + "\nNote: Only affects mesh generation when smoothing level is 0");
                    } else {
                        // Set resolution
                        std::string value = ctx.getArg(1);
                        if (value == "1cm") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::HighQuality);
                            return CommandResult::Success("Mesh resolution set to 1cm (not available in SimpleMesher, using 4cm)");
                        } else if (value == "2cm") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::HighQuality);
                            return CommandResult::Success("Mesh resolution set to 2cm (not available in SimpleMesher, using 4cm)");
                        } else if (value == "4cm") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::HighQuality);
                            return CommandResult::Success("Mesh resolution set to 4cm (high quality)");
                        } else if (value == "8cm") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::Balanced);
                            return CommandResult::Success("Mesh resolution set to 8cm (balanced)");
                        } else if (value == "16cm") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::Fast);
                            return CommandResult::Success("Mesh resolution set to 16cm (fast preview)");
                        } else if (value == "auto") {
                            m_app->setMeshResolution(SurfaceGen::PreviewQuality::Disabled);
                            return CommandResult::Success("Mesh resolution set to auto (8cm default)");
                        } else {
                            return CommandResult::Error("Invalid resolution. Use: 1cm, 2cm, 4cm, 8cm, 16cm, or auto");
                        }
                    }
                } else {
                    return CommandResult::Error("Invalid subcommand. Use: validate, info, repair, or resolution");
                }
            }),

        // SURFACE-EXPORT command
        CommandRegistration()
            .withName("surface-export")
            .withDescription("Export surface mesh to various formats")
            .withCategory(CommandCategory::MESH)
            .withArg("filename", "Output filename with extension (.stl, .obj)", "string", true, "")
            .withArg("format", "Export format (auto-detect from extension if not specified)", "string", false, "")
            .withHandler([this](const CommandContext& ctx) {
                std::string filename = ctx.getArg(0);
                if (filename.empty()) {
                    return CommandResult::Error("Filename required");
                }
                
                // Generate surface mesh
                SurfaceGen::SurfaceGenerator surfaceGen(m_eventDispatcher);
                
                // Configure surface settings with mesh resolution
                SurfaceGen::SurfaceSettings settings = SurfaceGen::SurfaceSettings::Default();
                settings.smoothingLevel = m_app->getSmoothingLevel();
                settings.previewQuality = m_app->getMeshResolution();
                surfaceGen.setSurfaceSettings(settings);
                
                auto surfaceMesh = surfaceGen.generateMultiResMesh(*m_voxelManager, m_voxelManager->getActiveResolution());
                
                // Apply smoothing if enabled
                if (m_app->getSmoothingLevel() > 0) {
                    SurfaceGen::MeshSmoother smoother;
                    SurfaceGen::MeshSmoother::SmoothingConfig config;
                    config.smoothingLevel = m_app->getSmoothingLevel();
                    config.algorithm = m_app->getSmoothingAlgorithm();
                    config.preserveTopology = true;
                    config.preserveBoundaries = true;
                    config.minFeatureSize = 1.0f; // 1mm minimum feature size for 3D printing
                    config.usePreviewQuality = false; // Full quality for export
                    
                    // Apply smoothing with progress callback
                    std::cout << "Applying smoothing (level " << m_app->getSmoothingLevel() << ")..." << std::flush;
                    surfaceMesh = smoother.smooth(surfaceMesh, config, 
                        [](float progress) {
                            // Update progress display
                            std::cout << "\rApplying smoothing... " 
                                      << std::fixed << std::setprecision(0)
                                      << (progress * 100.0f) << "%" << std::flush;
                            return true; // Continue processing
                        });
                    std::cout << "\rApplying smoothing... Done!    " << std::endl;
                    
                    if (surfaceMesh.vertices.empty()) {
                        return CommandResult::Error("Smoothing operation failed or was cancelled");
                    }
                }
                
                // Convert to Rendering::Mesh for STL export
                Rendering::Mesh renderMesh;
                renderMesh.vertices.reserve(surfaceMesh.vertices.size());
                for (size_t i = 0; i < surfaceMesh.vertices.size(); ++i) {
                    Rendering::Vertex vertex;
                    vertex.position = surfaceMesh.vertices[i];
                    if (i < surfaceMesh.normals.size()) {
                        vertex.normal = surfaceMesh.normals[i];
                    }
                    renderMesh.vertices.push_back(vertex);
                }
                renderMesh.indices = surfaceMesh.indices;
                
                // Determine format from filename or explicit format
                std::string format = ctx.getArg(1);
                if (format.empty()) {
                    // Auto-detect from extension
                    if (filename.find(".stl") != std::string::npos || filename.find(".STL") != std::string::npos) {
                        format = "stl";
                    } else if (filename.find(".obj") != std::string::npos || filename.find(".OBJ") != std::string::npos) {
                        format = "obj";
                    } else {
                        return CommandResult::Error("Unknown file format. Please use .stl or .obj extension");
                    }
                }
                
                if (format == "stl") {
                    FileIO::STLExportOptions options;
                    options.format = FileIO::STLFormat::Binary;
                    options.validateWatertight = false; // Disable validation for now
                    
                    auto result = m_fileManager->exportSTL(filename, renderMesh, options);
                    if (result.success) {
                        return CommandResult::Success("Exported to: " + filename);
                    }
                    return CommandResult::Error("Failed to export STL: " + result.message);
                } else if (format == "obj") {
                    // TODO: Implement OBJ export when available
                    return CommandResult::Error("OBJ export not yet implemented");
                } else {
                    return CommandResult::Error("Unsupported format: " + format);
                }
            }),

        // SURFACE-PREVIEW command
        CommandRegistration()
            .withName("surface-preview")
            .withDescription("Preview surface generation with current settings")
            .withCategory(CommandCategory::MESH)
            .withArg("enable", "on/off to enable/disable preview mode", "string", false, "on")
            .withHandler([this](const CommandContext& ctx) {
                std::string enable = ctx.getArg(0, "on");
                
                if (enable == "on") {
                    m_app->setSmoothPreviewEnabled(true);
                    requestMeshUpdate(); // Trigger mesh regeneration
                    return CommandResult::Success("Surface preview enabled. Mesh will update in real-time.");
                } else if (enable == "off") {
                    m_app->setSmoothPreviewEnabled(false);
                    requestMeshUpdate(); // Trigger mesh regeneration
                    return CommandResult::Success("Surface preview disabled. Using standard voxel rendering.");
                } else {
                    return CommandResult::Error("Invalid option. Use 'on' or 'off'");
                }
            }),

        // SURFACE-SETTINGS command
        CommandRegistration()
            .withName("surface-settings")
            .withDescription("Configure surface generation settings")
            .withCategory(CommandCategory::MESH)
            .withArg("setting", "Setting name (quality, algorithm, topology)", "string", false, "")
            .withArg("value", "Setting value", "string", false, "")
            .withHandler([this](const CommandContext& ctx) {
                if (ctx.getArgCount() == 0) {
                    // Display all current settings
                    std::stringstream ss;
                    ss << "Surface Generation Settings:\n";
                    ss << "\nSmoothing:\n";
                    ss << "  Level: " << m_app->getSmoothingLevel() << "\n";
                    ss << "  Algorithm: ";
                    switch (m_app->getSmoothingAlgorithm()) {
                        case SurfaceGen::MeshSmoother::Algorithm::None:
                            ss << "None\n";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::Laplacian:
                            ss << "Laplacian\n";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::Taubin:
                            ss << "Taubin\n";
                            break;
                        case SurfaceGen::MeshSmoother::Algorithm::BiLaplacian:
                            ss << "BiLaplacian\n";
                            break;
                    }
                    ss << "  Preview Mode: " << (m_app->isSmoothPreviewEnabled() ? "on" : "off") << "\n";
                    
                    ss << "\nQuality Settings:\n";
                    ss << "  Topology Preservation: enabled\n";
                    ss << "  Boundary Preservation: enabled\n";
                    ss << "  Min Feature Size: 1.0mm\n";
                    
                    return CommandResult::Success(ss.str());
                }
                
                std::string setting = ctx.getArg(0);
                std::string value = ctx.getArg(1);
                
                if (setting == "quality") {
                    if (value == "low") {
                        m_app->setSmoothingLevel(2);
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Laplacian);
                        return CommandResult::Success("Surface quality set to low (fast generation)");
                    } else if (value == "medium") {
                        m_app->setSmoothingLevel(5);
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Taubin);
                        return CommandResult::Success("Surface quality set to medium (balanced)");
                    } else if (value == "high") {
                        m_app->setSmoothingLevel(10);
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::BiLaplacian);
                        return CommandResult::Success("Surface quality set to high (best quality)");
                    } else {
                        return CommandResult::Error("Invalid quality level. Use: low, medium, or high");
                    }
                } else if (setting == "algorithm") {
                    if (value == "none") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::None);
                        return CommandResult::Success("Surface algorithm set to none (raw dual contouring)");
                    } else if (value == "laplacian") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Laplacian);
                        return CommandResult::Success("Surface algorithm set to Laplacian");
                    } else if (value == "taubin") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::Taubin);
                        return CommandResult::Success("Surface algorithm set to Taubin");
                    } else if (value == "bilaplacian") {
                        m_app->setSmoothingAlgorithm(SurfaceGen::MeshSmoother::Algorithm::BiLaplacian);
                        return CommandResult::Success("Surface algorithm set to BiLaplacian");
                    } else {
                        return CommandResult::Error("Invalid algorithm. Use: none, laplacian, taubin, or bilaplacian");
                    }
                } else if (setting == "topology") {
                    // TODO: When topology settings are exposed in MeshSmoother config
                    return CommandResult::Success("Topology preservation settings will be available in a future update");
                } else {
                    return CommandResult::Error("Unknown setting. Available settings: quality, algorithm, topology");
                }
            })
    };
}

} // namespace VoxelEditor