#include <iostream>
#include <fstream>
#include "voxel_data/VoxelDataManager.h"
#include "surface_gen/SurfaceGenerator.h"
#include "events/EventDispatcher.h"
#include "math/Vector3f.h"
#include "rendering/Mesh.h"

using namespace VoxelEditor;

void printMesh(const Rendering::Mesh& mesh) {
    std::cout << "Mesh has " << mesh.vertices.size() << " vertices and " 
              << mesh.indices.size() << " indices (" << mesh.indices.size() / 3 << " triangles)" << std::endl;
    
    // Print all vertices
    std::cout << "\nVertices:\n";
    for (size_t i = 0; i < mesh.vertices.size(); i++) {
        const auto& v = mesh.vertices[i];
        std::cout << "  V" << i << ": pos=(" 
                  << v.position.value().x << ", " 
                  << v.position.value().y << ", " 
                  << v.position.value().z << ")"
                  << " normal=(" 
                  << v.normal.x << ", " 
                  << v.normal.y << ", " 
                  << v.normal.z << ")"
                  << std::endl;
    }
    
    // Print triangles
    std::cout << "\nTriangles:\n";
    for (size_t i = 0; i < mesh.indices.size(); i += 3) {
        std::cout << "  T" << (i/3) << ": " 
                  << mesh.indices[i] << ", " 
                  << mesh.indices[i+1] << ", " 
                  << mesh.indices[i+2] << std::endl;
    }
}

int main() {
    // Create event dispatcher
    Events::EventDispatcher dispatcher;
    
    // Create voxel manager
    VoxelData::VoxelDataManager voxelManager(&dispatcher);
    voxelManager.resizeWorkspace(Math::Vector3f(5.0f, 5.0f, 5.0f));
    
    // Place a single 32cm voxel at origin
    voxelManager.setActiveResolution(VoxelData::VoxelResolution::Size_32cm);
    bool placed = voxelManager.setVoxel(Math::Vector3i(0, 0, 0), VoxelData::VoxelResolution::Size_32cm, true);
    std::cout << "Voxel placed: " << (placed ? "YES" : "NO") << std::endl;
    
    // Create surface generator
    SurfaceGeneration::SurfaceGenerator surfaceGen;
    
    // Generate mesh
    std::cout << "\nGenerating mesh...\n";
    auto optMesh = surfaceGen.generateMesh(voxelManager);
    
    if (optMesh.has_value()) {
        std::cout << "Mesh generated successfully!\n";
        printMesh(optMesh.value());
        
        // Write raw vertex data to file for inspection
        std::ofstream out("mesh_vertices.txt");
        const auto& mesh = optMesh.value();
        for (size_t i = 0; i < mesh.vertices.size(); i++) {
            const auto& v = mesh.vertices[i];
            out << "Vertex " << i << ": " 
                << v.position.value().x << " " 
                << v.position.value().y << " " 
                << v.position.value().z << std::endl;
        }
        out.close();
    } else {
        std::cout << "Failed to generate mesh!\n";
    }
    
    return 0;
}