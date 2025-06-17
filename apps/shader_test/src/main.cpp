#include "shader_test/ShaderTestFramework.h"
#include "shader_test/TestMeshGenerator.h"
#include <iostream>
#include <filesystem>
#include <cstring>

void printUsage(const char* programName) {
    std::cout << "Usage: " << programName << " [options]" << std::endl;
    std::cout << "Options:" << std::endl;
    std::cout << "  --help               Show this help message" << std::endl;
    std::cout << "  --all                Run all shader tests (default)" << std::endl;
    std::cout << "  --file <shader>      Test a specific shader file" << std::endl;
    std::cout << "  --builtin            Test only built-in shaders" << std::endl;
    std::cout << "  --file-shaders       Test only file-based shaders" << std::endl;
    std::cout << "  --output <dir>       Output directory for captures (default: test_output)" << std::endl;
    std::cout << "  --windowed           Use windowed mode instead of headless" << std::endl;
    std::cout << "  --report <file>      Write test report to file" << std::endl;
    std::cout << "  --verbose            Enable verbose output" << std::endl;
}

int main(int argc, char* argv[]) {
    // Parse command line arguments
    bool runAll = true;
    bool runBuiltin = false;
    bool runFileShaders = false;
    bool windowed = false;
    bool verbose = false;
    std::string specificShader;
    std::string outputDir = "test_output";
    std::string reportFile;
    
    for (int i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "--help") == 0) {
            printUsage(argv[0]);
            return 0;
        } else if (strcmp(argv[i], "--all") == 0) {
            runAll = true;
            runBuiltin = false;
            runFileShaders = false;
        } else if (strcmp(argv[i], "--builtin") == 0) {
            runAll = false;
            runBuiltin = true;
        } else if (strcmp(argv[i], "--file-shaders") == 0) {
            runAll = false;
            runFileShaders = true;
        } else if (strcmp(argv[i], "--file") == 0 && i + 1 < argc) {
            specificShader = argv[++i];
            runAll = false;
        } else if (strcmp(argv[i], "--output") == 0 && i + 1 < argc) {
            outputDir = argv[++i];
        } else if (strcmp(argv[i], "--windowed") == 0) {
            windowed = true;
        } else if (strcmp(argv[i], "--report") == 0 && i + 1 < argc) {
            reportFile = argv[++i];
        } else if (strcmp(argv[i], "--verbose") == 0) {
            verbose = true;
        }
    }
    
    // Create output directory
    std::filesystem::create_directories(outputDir);
    
    std::cout << "=== Voxel Editor Shader Test Suite ===" << std::endl;
    std::cout << "Output directory: " << outputDir << std::endl;
    std::cout << "Mode: " << (windowed ? "Windowed" : "Headless") << std::endl;
    std::cout << std::endl;
    
    VoxelEditor::ShaderTest::ShaderTestRunner::Summary summary;
    
    if (!specificShader.empty()) {
        // Test specific shader
        std::cout << "Testing specific shader: " << specificShader << std::endl;
        
        VoxelEditor::ShaderTest::ShaderTestFramework framework;
        if (!framework.initialize(!windowed, 800, 600)) {
            std::cerr << "Failed to initialize test framework" << std::endl;
            return 1;
        }
        
        // Determine if it's a vertex or fragment shader
        bool isVertex = specificShader.find(".vert") != std::string::npos;
        std::string vertPath, fragPath;
        
        if (isVertex) {
            vertPath = specificShader;
            // Try to find matching fragment shader
            fragPath = specificShader;
            size_t pos = fragPath.find(".vert");
            if (pos != std::string::npos) {
                fragPath.replace(pos, 5, ".frag");
            }
        } else {
            fragPath = specificShader;
            // Try to find matching vertex shader
            vertPath = specificShader;
            size_t pos = vertPath.find(".frag");
            if (pos != std::string::npos) {
                vertPath.replace(pos, 5, ".vert");
            }
        }
        
        // Test with a cube mesh
        auto mesh = VoxelEditor::ShaderTest::TestMeshGenerator::createCube(1.0f);
        
        VoxelEditor::ShaderTest::ShaderUniforms uniforms;
        uniforms.projectionMatrix = VoxelEditor::Math::Matrix4f::perspective(45.0f, 800.0f/600.0f, 0.1f, 100.0f);
        uniforms.viewMatrix = VoxelEditor::Math::Matrix4f::lookAt(
            VoxelEditor::Math::Vector3f(3, 3, 3),
            VoxelEditor::Math::Vector3f(0, 0, 0),
            VoxelEditor::Math::Vector3f(0, 1, 0)
        );
        
        VoxelEditor::ShaderTest::ValidationCriteria criteria;
        criteria.captureOutput = true;
        criteria.outputPath = outputDir + "/specific_shader.ppm";
        
        auto result = framework.runCompleteTest(vertPath, fragPath, mesh, uniforms, criteria);
        
        summary.totalTests = 1;
        if (result.success) {
            summary.passedTests = 1;
            std::cout << "✓ Shader test passed" << std::endl;
        } else {
            summary.failedTests = 1;
            std::cout << "✗ Shader test failed: " << result.errorMessage << std::endl;
        }
        summary.results.push_back(result);
        
    } else if (runAll) {
        summary = VoxelEditor::ShaderTest::ShaderTestRunner::runAllTests();
    } else if (runBuiltin) {
        summary = VoxelEditor::ShaderTest::ShaderTestRunner::testBuiltInShaders();
    } else if (runFileShaders) {
        summary = VoxelEditor::ShaderTest::ShaderTestRunner::testFileShaders();
    }
    
    // Print summary
    summary.print();
    
    // Write report if requested
    if (!reportFile.empty()) {
        summary.writeToFile(reportFile);
        std::cout << "\nTest report written to: " << reportFile << std::endl;
    }
    
    // Return exit code based on test results
    return summary.failedTests > 0 ? 1 : 0;
}