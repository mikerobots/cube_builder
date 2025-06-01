#pragma once

#include "FileTypes.h"
#include "Mesh.h"
#include <string>
#include <vector>
#include <fstream>

namespace VoxelEditor {
namespace FileIO {

// STL exporter for 3D printing and CAD applications
class STLExporter {
public:
    STLExporter();
    ~STLExporter();
    
    // Export single mesh
    bool exportMesh(const std::string& filename, const Rendering::Mesh& mesh, 
                   const STLExportOptions& options = STLExportOptions::Default());
    
    // Export multiple meshes
    bool exportMeshes(const std::string& filename, const std::vector<Rendering::Mesh>& meshes,
                     const STLExportOptions& options = STLExportOptions::Default());
    
    // Validation
    bool validateMesh(const Rendering::Mesh& mesh, std::vector<std::string>& issues) const;
    bool isWatertight(const Rendering::Mesh& mesh) const;
    
    // Mesh repair
    Rendering::Mesh repairMesh(const Rendering::Mesh& mesh) const;
    
    // Statistics
    STLExportStats getLastExportStats() const { return m_lastStats; }
    
    // Error handling
    FileError getLastError() const { return m_lastError; }
    std::string getLastErrorMessage() const { return m_lastErrorMessage; }
    
private:
    STLExportStats m_lastStats;
    FileError m_lastError = FileError::None;
    std::string m_lastErrorMessage;
    
    // Format-specific export
    bool exportBinarySTL(const std::string& filename, const Rendering::Mesh& mesh,
                        const STLExportOptions& options);
    bool exportASCIISTL(const std::string& filename, const Rendering::Mesh& mesh,
                       const STLExportOptions& options);
    
    // Mesh preprocessing
    Rendering::Mesh preprocessMesh(const Rendering::Mesh& mesh, const STLExportOptions& options) const;
    Rendering::Mesh scaleMesh(const Rendering::Mesh& mesh, float scale) const;
    Rendering::Mesh translateMesh(const Rendering::Mesh& mesh, const Math::Vector3f& translation) const;
    Rendering::Mesh mergeMeshes(const std::vector<Rendering::Mesh>& meshes) const;
    
    // Unit conversion
    float getUnitScale(STLUnits from, STLUnits to) const;
    float getUnitToMillimeters(STLUnits unit) const;
    
    // Binary STL writing
    void writeBinaryHeader(std::ofstream& file, uint32_t triangleCount);
    void writeBinaryTriangle(std::ofstream& file, const Math::Vector3f& v0,
                           const Math::Vector3f& v1, const Math::Vector3f& v2);
    
    // ASCII STL writing
    void writeASCIIHeader(std::ofstream& file, const std::string& name);
    void writeASCIIFooter(std::ofstream& file);
    void writeASCIITriangle(std::ofstream& file, const Math::Vector3f& v0,
                          const Math::Vector3f& v1, const Math::Vector3f& v2);
    
    // Triangle operations
    Math::Vector3f calculateTriangleNormal(const Math::Vector3f& v0,
                                         const Math::Vector3f& v1,
                                         const Math::Vector3f& v2) const;
    float calculateTriangleArea(const Math::Vector3f& v0,
                              const Math::Vector3f& v1,
                              const Math::Vector3f& v2) const;
    
    // Validation helpers
    bool hasManifoldEdges(const Rendering::Mesh& mesh) const;
    bool hasConsistentNormals(const Rendering::Mesh& mesh) const;
    bool hasDegenerateTriangles(const Rendering::Mesh& mesh) const;
    
    // Error handling
    void setError(FileError error, const std::string& message);
    void clearError();
    
    // Statistics tracking
    void resetStats();
    void updateStats(const Rendering::Mesh& mesh);
};

// STL file reader (for validation and testing)
class STLImporter {
public:
    STLImporter();
    ~STLImporter();
    
    // Import STL file
    bool importMesh(const std::string& filename, Rendering::Mesh& mesh);
    
    // Format detection
    bool isBinarySTL(const std::string& filename) const;
    
    // Error handling
    FileError getLastError() const { return m_lastError; }
    std::string getLastErrorMessage() const { return m_lastErrorMessage; }
    
private:
    FileError m_lastError = FileError::None;
    std::string m_lastErrorMessage;
    
    // Format-specific import
    bool importBinarySTL(const std::string& filename, Rendering::Mesh& mesh);
    bool importASCIISTL(const std::string& filename, Rendering::Mesh& mesh);
    
    // Binary STL reading
    bool readBinaryHeader(std::ifstream& file, uint32_t& triangleCount);
    bool readBinaryTriangle(std::ifstream& file, Math::Vector3f& v0,
                          Math::Vector3f& v1, Math::Vector3f& v2,
                          Math::Vector3f& normal);
    
    // ASCII STL parsing
    bool parseASCIISTL(std::ifstream& file, Rendering::Mesh& mesh);
    bool parseVertex(const std::string& line, Math::Vector3f& vertex);
    bool parseNormal(const std::string& line, Math::Vector3f& normal);
    
    // Error handling
    void setError(FileError error, const std::string& message);
    void clearError();
};

} // namespace FileIO
} // namespace VoxelEditor