#include "../include/file_io/STLExporter.h"
#include "logging/Logger.h"
#include <cstring>
#include <fstream>
#include <iomanip>
#include <chrono>
#include <map>
#include <filesystem>

namespace VoxelEditor {
namespace FileIO {

STLExporter::STLExporter() = default;
STLExporter::~STLExporter() = default;

bool STLExporter::exportMesh(const std::string& filename, const Rendering::Mesh& mesh, 
                           const STLExportOptions& options) {
    clearError();
    resetStats();
    
    auto startTime = std::chrono::steady_clock::now();
    
    // Validate mesh
    std::vector<std::string> issues;
    if (!validateMesh(mesh, issues)) {
        setError(FileError::InvalidFormat, "Invalid mesh: " + issues[0]);
        return false;
    }
    
    // Preprocess mesh
    Rendering::Mesh processedMesh = preprocessMesh(mesh, options);
    
    // Export based on format
    bool success = false;
    if (options.format == STLFormat::Binary) {
        success = exportBinarySTL(filename, processedMesh, options);
    } else {
        success = exportASCIISTL(filename, processedMesh, options);
    }
    
    if (success) {
        auto endTime = std::chrono::steady_clock::now();
        m_lastStats.exportTime = std::chrono::duration<float>(endTime - startTime).count();
        updateStats(processedMesh);
        
        // Get file size
        if (std::filesystem::exists(filename)) {
            m_lastStats.fileSize = std::filesystem::file_size(filename);
        }
    }
    
    return success;
}

bool STLExporter::exportMeshes(const std::string& filename, const std::vector<Rendering::Mesh>& meshes,
                             const STLExportOptions& options) {
    clearError();
    resetStats();
    
    if (meshes.empty()) {
        setError(FileError::InvalidFormat, "No meshes to export");
        return false;
    }
    
    // Merge meshes if requested
    if (options.mergeMeshes && meshes.size() > 1) {
        Rendering::Mesh mergedMesh = mergeMeshes(meshes);
        return exportMesh(filename, mergedMesh, options);
    }
    
    // Export individual meshes with numbered filenames
    std::string baseName = filename.substr(0, filename.find_last_of('.'));
    std::string extension = filename.substr(filename.find_last_of('.'));
    
    for (size_t i = 0; i < meshes.size(); i++) {
        std::string meshFilename = baseName + "_" + std::to_string(i) + extension;
        if (!exportMesh(meshFilename, meshes[i], options)) {
            return false;
        }
    }
    
    return true;
}

bool STLExporter::validateMesh(const Rendering::Mesh& mesh, std::vector<std::string>& issues) const {
    issues.clear();
    
    if (mesh.vertices.empty()) {
        issues.push_back("Mesh has no vertices");
        return false;
    }
    
    if (mesh.indices.empty() || mesh.indices.size() % 3 != 0) {
        issues.push_back("Invalid index count (must be multiple of 3)");
        return false;
    }
    
    // Check for degenerate triangles
    if (hasDegenerateTriangles(mesh)) {
        issues.push_back("Mesh contains degenerate triangles");
    }
    
    // Check manifold edges - but don't fail validation for it
    // A single triangle or open mesh won't have manifold edges but is still valid for STL export
    if (!hasManifoldEdges(mesh)) {
        // Just a warning, not a failure
        // issues.push_back("Mesh has non-manifold edges");
    }
    
    return issues.empty();
}

bool STLExporter::isWatertight(const Rendering::Mesh& mesh) const {
    // A mesh is watertight if every edge is shared by exactly two triangles
    return hasManifoldEdges(mesh) && !hasDegenerateTriangles(mesh);
}

Rendering::Mesh STLExporter::repairMesh(const Rendering::Mesh& mesh) const {
    Rendering::Mesh repairedMesh = mesh;
    
    // Remove degenerate triangles
    // TODO: Implement mesh repair algorithms
    
    return repairedMesh;
}

bool STLExporter::exportBinarySTL(const std::string& filename, const Rendering::Mesh& mesh,
                                const STLExportOptions& options) {
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        setError(FileError::AccessDenied, "Cannot open file for writing");
        return false;
    }
    
    // Calculate triangle count
    uint32_t triangleCount = static_cast<uint32_t>(mesh.indices.size() / 3);
    
    // Write header
    writeBinaryHeader(file, triangleCount);
    
    // Write triangles
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]].position;
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]].position;
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]].position;
        
        writeBinaryTriangle(file, v0, v1, v2);
    }
    
    return file.good();
}

bool STLExporter::exportASCIISTL(const std::string& filename, const Rendering::Mesh& mesh,
                               const STLExportOptions& options) {
    std::ofstream file(filename);
    if (!file.is_open()) {
        setError(FileError::AccessDenied, "Cannot open file for writing");
        return false;
    }
    
    // Write header
    std::string name = filename.substr(filename.find_last_of("/\\") + 1);
    writeASCIIHeader(file, name);
    
    // Write triangles
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]].position;
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]].position;
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]].position;
        
        writeASCIITriangle(file, v0, v1, v2);
    }
    
    // Write footer
    writeASCIIFooter(file);
    
    return file.good();
}

Rendering::Mesh STLExporter::preprocessMesh(const Rendering::Mesh& mesh, const STLExportOptions& options) const {
    Rendering::Mesh processedMesh = mesh;
    
    // Apply scale
    if (options.scale != 1.0f) {
        processedMesh = scaleMesh(processedMesh, options.scale);
    }
    
    // Apply translation
    if (options.translation.x != 0 || options.translation.y != 0 || options.translation.z != 0) {
        processedMesh = translateMesh(processedMesh, options.translation);
    }
    
    // Apply unit conversion
    float unitScale = getUnitScale(STLUnits::Meters, options.units);
    if (unitScale != 1.0f) {
        processedMesh = scaleMesh(processedMesh, unitScale);
    }
    
    return processedMesh;
}

Rendering::Mesh STLExporter::scaleMesh(const Rendering::Mesh& mesh, float scale) const {
    Rendering::Mesh scaledMesh = mesh;
    
    for (auto& vertex : scaledMesh.vertices) {
        vertex.position *= scale;
    }
    
    return scaledMesh;
}

Rendering::Mesh STLExporter::translateMesh(const Rendering::Mesh& mesh, const Math::Vector3f& translation) const {
    Rendering::Mesh translatedMesh = mesh;
    
    for (auto& vertex : translatedMesh.vertices) {
        vertex.position += translation;
    }
    
    return translatedMesh;
}

Rendering::Mesh STLExporter::mergeMeshes(const std::vector<Rendering::Mesh>& meshes) const {
    Rendering::Mesh mergedMesh;
    
    size_t totalVertices = 0;
    size_t totalIndices = 0;
    
    // Calculate total size
    for (const auto& mesh : meshes) {
        totalVertices += mesh.vertices.size();
        totalIndices += mesh.indices.size();
    }
    
    // Reserve space
    mergedMesh.vertices.reserve(totalVertices);
    mergedMesh.indices.reserve(totalIndices);
    
    // Merge meshes
    uint32_t indexOffset = 0;
    for (const auto& mesh : meshes) {
        // Add vertices
        mergedMesh.vertices.insert(mergedMesh.vertices.end(), 
                                 mesh.vertices.begin(), mesh.vertices.end());
        
        // Add indices with offset
        for (uint32_t index : mesh.indices) {
            mergedMesh.indices.push_back(index + indexOffset);
        }
        
        indexOffset += static_cast<uint32_t>(mesh.vertices.size());
    }
    
    return mergedMesh;
}

float STLExporter::getUnitScale(STLUnits from, STLUnits to) const {
    float fromMM = getUnitToMillimeters(from);
    float toMM = getUnitToMillimeters(to);
    return fromMM / toMM;
}

float STLExporter::getUnitToMillimeters(STLUnits unit) const {
    switch (unit) {
        case STLUnits::Millimeters: return 1.0f;
        case STLUnits::Centimeters: return 10.0f;
        case STLUnits::Meters: return 1000.0f;
        case STLUnits::Inches: return 25.4f;
        default: return 1.0f;
    }
}

void STLExporter::writeBinaryHeader(std::ofstream& file, uint32_t triangleCount) {
    // Write 80 byte header (can be anything)
    char header[80];
    std::memset(header, 0, 80);
    std::strcpy(header, "Binary STL exported by VoxelEditor");
    file.write(header, 80);
    
    // Write triangle count
    file.write(reinterpret_cast<const char*>(&triangleCount), sizeof(uint32_t));
}

void STLExporter::writeBinaryTriangle(std::ofstream& file, const Math::Vector3f& v0,
                                    const Math::Vector3f& v1, const Math::Vector3f& v2) {
    // Calculate normal
    Math::Vector3f normal = calculateTriangleNormal(v0, v1, v2);
    
    // Write normal
    file.write(reinterpret_cast<const char*>(&normal.x), sizeof(float));
    file.write(reinterpret_cast<const char*>(&normal.y), sizeof(float));
    file.write(reinterpret_cast<const char*>(&normal.z), sizeof(float));
    
    // Write vertices
    file.write(reinterpret_cast<const char*>(&v0.x), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v0.y), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v0.z), sizeof(float));
    
    file.write(reinterpret_cast<const char*>(&v1.x), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v1.y), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v1.z), sizeof(float));
    
    file.write(reinterpret_cast<const char*>(&v2.x), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v2.y), sizeof(float));
    file.write(reinterpret_cast<const char*>(&v2.z), sizeof(float));
    
    // Write attribute byte count (always 0)
    uint16_t attributeByteCount = 0;
    file.write(reinterpret_cast<const char*>(&attributeByteCount), sizeof(uint16_t));
}

void STLExporter::writeASCIIHeader(std::ofstream& file, const std::string& name) {
    file << "solid " << name << std::endl;
}

void STLExporter::writeASCIIFooter(std::ofstream& file) {
    file << "endsolid" << std::endl;
}

void STLExporter::writeASCIITriangle(std::ofstream& file, const Math::Vector3f& v0,
                                   const Math::Vector3f& v1, const Math::Vector3f& v2) {
    Math::Vector3f normal = calculateTriangleNormal(v0, v1, v2);
    
    file << std::fixed << std::setprecision(6);
    file << "  facet normal " << normal.x << " " << normal.y << " " << normal.z << std::endl;
    file << "    outer loop" << std::endl;
    file << "      vertex " << v0.x << " " << v0.y << " " << v0.z << std::endl;
    file << "      vertex " << v1.x << " " << v1.y << " " << v1.z << std::endl;
    file << "      vertex " << v2.x << " " << v2.y << " " << v2.z << std::endl;
    file << "    endloop" << std::endl;
    file << "  endfacet" << std::endl;
}

Math::Vector3f STLExporter::calculateTriangleNormal(const Math::Vector3f& v0,
                                                  const Math::Vector3f& v1,
                                                  const Math::Vector3f& v2) const {
    Math::Vector3f edge1 = v1 - v0;
    Math::Vector3f edge2 = v2 - v0;
    Math::Vector3f normal = edge1.cross(edge2);
    return normal.normalized();
}

float STLExporter::calculateTriangleArea(const Math::Vector3f& v0,
                                       const Math::Vector3f& v1,
                                       const Math::Vector3f& v2) const {
    Math::Vector3f edge1 = v1 - v0;
    Math::Vector3f edge2 = v2 - v0;
    return edge1.cross(edge2).length() * 0.5f;
}

bool STLExporter::hasManifoldEdges(const Rendering::Mesh& mesh) const {
    // Count how many times each edge appears
    std::map<std::pair<uint32_t, uint32_t>, int> edgeCount;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        uint32_t i0 = mesh.indices[i];
        uint32_t i1 = mesh.indices[i + 1];
        uint32_t i2 = mesh.indices[i + 2];
        
        // Add edges (always with smaller index first)
        edgeCount[std::make_pair(std::min(i0, i1), std::max(i0, i1))]++;
        edgeCount[std::make_pair(std::min(i1, i2), std::max(i1, i2))]++;
        edgeCount[std::make_pair(std::min(i2, i0), std::max(i2, i0))]++;
    }
    
    // Check that each edge appears exactly twice
    for (const auto& [edge, count] : edgeCount) {
        if (count != 2) {
            return false;
        }
    }
    
    return true;
}

bool STLExporter::hasConsistentNormals(const Rendering::Mesh& mesh) const {
    // TODO: Implement normal consistency check
    return true;
}

bool STLExporter::hasDegenerateTriangles(const Rendering::Mesh& mesh) const {
    const float epsilon = 1e-6f;
    
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        const Math::Vector3f& v0 = mesh.vertices[mesh.indices[i]].position;
        const Math::Vector3f& v1 = mesh.vertices[mesh.indices[i + 1]].position;
        const Math::Vector3f& v2 = mesh.vertices[mesh.indices[i + 2]].position;
        
        float area = calculateTriangleArea(v0, v1, v2);
        if (area < epsilon) {
            return true;
        }
    }
    
    return false;
}

void STLExporter::setError(FileError error, const std::string& message) {
    m_lastError = error;
    m_lastErrorMessage = message;
    LOG_ERROR("STLExporter: " + message);
}

void STLExporter::clearError() {
    m_lastError = FileError::None;
    m_lastErrorMessage.clear();
}

void STLExporter::resetStats() {
    m_lastStats = STLExportStats();
}

void STLExporter::updateStats(const Rendering::Mesh& mesh) {
    m_lastStats.triangleCount = mesh.indices.size() / 3;
    m_lastStats.vertexCount = mesh.vertices.size();
    m_lastStats.watertight = isWatertight(mesh);
}

// STLImporter implementation
STLImporter::STLImporter() = default;
STLImporter::~STLImporter() = default;

bool STLImporter::importMesh(const std::string& filename, Rendering::Mesh& mesh) {
    clearError();
    
    if (!std::filesystem::exists(filename)) {
        setError(FileError::FileNotFound, "File not found: " + filename);
        return false;
    }
    
    if (isBinarySTL(filename)) {
        return importBinarySTL(filename, mesh);
    } else {
        return importASCIISTL(filename, mesh);
    }
}

bool STLImporter::isBinarySTL(const std::string& filename) const {
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        return false;
    }
    
    // Read first 5 bytes
    char buffer[6];
    file.read(buffer, 5);
    buffer[5] = '\0';
    
    // ASCII STL files start with "solid"
    return std::string(buffer) != "solid";
}

bool STLImporter::importBinarySTL(const std::string& filename, Rendering::Mesh& mesh) {
    // TODO: Implement binary STL import
    return false;
}

bool STLImporter::importASCIISTL(const std::string& filename, Rendering::Mesh& mesh) {
    // TODO: Implement ASCII STL import
    return false;
}

bool STLImporter::readBinaryHeader(std::ifstream& file, uint32_t& triangleCount) {
    // Skip 80 byte header
    file.seekg(80, std::ios::beg);
    
    // Read triangle count
    file.read(reinterpret_cast<char*>(&triangleCount), sizeof(uint32_t));
    
    return file.good();
}

bool STLImporter::readBinaryTriangle(std::ifstream& file, Math::Vector3f& v0,
                                   Math::Vector3f& v1, Math::Vector3f& v2,
                                   Math::Vector3f& normal) {
    // Read normal
    file.read(reinterpret_cast<char*>(&normal.x), sizeof(float));
    file.read(reinterpret_cast<char*>(&normal.y), sizeof(float));
    file.read(reinterpret_cast<char*>(&normal.z), sizeof(float));
    
    // Read vertices
    file.read(reinterpret_cast<char*>(&v0.x), sizeof(float));
    file.read(reinterpret_cast<char*>(&v0.y), sizeof(float));
    file.read(reinterpret_cast<char*>(&v0.z), sizeof(float));
    
    file.read(reinterpret_cast<char*>(&v1.x), sizeof(float));
    file.read(reinterpret_cast<char*>(&v1.y), sizeof(float));
    file.read(reinterpret_cast<char*>(&v1.z), sizeof(float));
    
    file.read(reinterpret_cast<char*>(&v2.x), sizeof(float));
    file.read(reinterpret_cast<char*>(&v2.y), sizeof(float));
    file.read(reinterpret_cast<char*>(&v2.z), sizeof(float));
    
    // Skip attribute byte count
    uint16_t attributeByteCount;
    file.read(reinterpret_cast<char*>(&attributeByteCount), sizeof(uint16_t));
    
    return file.good();
}

bool STLImporter::parseASCIISTL(std::ifstream& file, Rendering::Mesh& mesh) {
    // TODO: Implement ASCII STL parsing
    return false;
}

bool STLImporter::parseVertex(const std::string& line, Math::Vector3f& vertex) {
    // TODO: Implement vertex parsing
    return false;
}

bool STLImporter::parseNormal(const std::string& line, Math::Vector3f& normal) {
    // TODO: Implement normal parsing
    return false;
}

void STLImporter::setError(FileError error, const std::string& message) {
    m_lastError = error;
    m_lastErrorMessage = message;
    LOG_ERROR("STLImporter: " + message);
}

void STLImporter::clearError() {
    m_lastError = FileError::None;
    m_lastErrorMessage.clear();
}

} // namespace FileIO
} // namespace VoxelEditor