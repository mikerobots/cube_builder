#include "window.h"
#include "shader.h"
#include "renderer.h"
#include <iostream>
#include <string>

const char* SIMPLE_VERTEX_SHADER = R"(
#version 120
attribute vec3 aPos;
attribute vec3 aNormal;
attribute vec3 aColor;

varying vec3 vColor;

uniform mat4 uModel;
uniform mat4 uView;
uniform mat4 uProjection;

void main() {
    vColor = aColor;
    gl_Position = uProjection * uView * uModel * vec4(aPos, 1.0);
}
)";

const char* SIMPLE_FRAGMENT_SHADER = R"(
#version 120
varying vec3 vColor;

void main() {
    gl_FragColor = vec4(vColor, 1.0);
}
)";

void setIdentityMatrix(GLint location) {
    float identity[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    glUniformMatrix4fv(location, 1, GL_FALSE, identity);
}

void setMainAppCameraMatrices(GLint modelLoc, GLint viewLoc, GLint projLoc) {
    // Exact matrices from main app debug output (Frame 0)
    
    // Model matrix (identity)
    float model[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // View matrix from main app (Camera position: 2.37214, 2.37186, 2.37214)
    float view[16] = {
        0.707107f, 0.0f, -0.707107f, -7.01345e-09f,
        -0.408204f, 0.816541f, -0.408204f, -8.49951e-05f,
        0.577382f, 0.577288f, 0.577382f, -4.10851f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // Projection matrix from main app
    float projection[16] = {
        1.34444f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.79259f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.0002f, -0.20002f,
        0.0f, 0.0f, -1.0f, 0.0f
    };
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    
    std::cout << "Set main app camera matrices" << std::endl;
}

void setFixedCameraMatrices(GLint modelLoc, GLint viewLoc, GLint projLoc) {
    // Test 5: Fix camera to view voxel at (0.64, 0.64, 0.64)
    
    // Model matrix (identity)
    float model[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // Simple view matrix: camera at (3, 3, 3) looking at (0.64, 0.64, 0.64)
    // Simplified lookAt matrix
    float view[16] = {
        0.707107f, 0.0f, -0.707107f, 0.0f,
        -0.408248f, 0.816497f, -0.408248f, 0.0f,
        0.577350f, 0.577350f, 0.577350f, -3.46410f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // Standard perspective projection (45° FOV, aspect 4:3, near=0.1, far=100)
    float projection[16] = {
        1.81066f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.41421f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.002f, -0.2002f,
        0.0f, 0.0f, -1.0f, 0.0f
    };
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    
    std::cout << "Set fixed camera matrices (camera at 3,3,3 looking at voxel)" << std::endl;
}

void setSimpleCameraMatrices(GLint modelLoc, GLint viewLoc, GLint projLoc) {
    // Test 6: Simple camera for center voxel
    
    // Model matrix (identity)  
    float model[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, 0.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // Simple view matrix: camera at (0, 0, 3) looking at origin
    float view[16] = {
        1.0f, 0.0f, 0.0f, 0.0f,
        0.0f, 1.0f, 0.0f, 0.0f,
        0.0f, 0.0f, 1.0f, -3.0f,
        0.0f, 0.0f, 0.0f, 1.0f
    };
    
    // Simple perspective projection
    float projection[16] = {
        1.5f, 0.0f, 0.0f, 0.0f,
        0.0f, 2.0f, 0.0f, 0.0f,
        0.0f, 0.0f, -1.01f, -0.201f,
        0.0f, 0.0f, -1.0f, 0.0f
    };
    
    glUniformMatrix4fv(modelLoc, 1, GL_FALSE, model);
    glUniformMatrix4fv(viewLoc, 1, GL_FALSE, view);
    glUniformMatrix4fv(projLoc, 1, GL_FALSE, projection);
    
    std::cout << "Set simple camera matrices (camera at z=3 looking at origin)" << std::endl;
}

int runTest(const std::string& testName, int testNum) {
    std::cout << "\n=== TEST " << testNum << ": " << testName << " ===\n" << std::endl;
    
    Window window;
    if (!window.create(800, 600, "Pipeline Test - " + testName)) {
        std::cerr << "Failed to create window" << std::endl;
        return 1;
    }
    
    Shader shader;
    if (!shader.createProgram(SIMPLE_VERTEX_SHADER, SIMPLE_FRAGMENT_SHADER)) {
        std::cerr << "Failed to create shader program" << std::endl;
        return 1;
    }
    
    Renderer renderer;
    bool setupSuccess = false;
    
    if (testNum == 1) {
        setupSuccess = renderer.setupSimpleTriangle();
    } else if (testNum == 2) {
        setupSuccess = renderer.setupSimpleQuad();
    } else if (testNum == 3) {
        setupSuccess = renderer.setupComplexCube();
    } else if (testNum == 4) {
        setupSuccess = renderer.setupMainAppVoxel();
    } else if (testNum == 5) {
        setupSuccess = renderer.setupMainAppVoxel();  // Same voxel, different camera
    } else if (testNum == 6) {
        setupSuccess = renderer.setupCenterVoxel();   // Center voxel, simple camera
    }
    
    if (!setupSuccess) {
        std::cerr << "Failed to setup geometry" << std::endl;
        return 1;
    }
    
    // Get uniform locations
    shader.use();
    GLint modelLoc = shader.getUniformLocation("uModel");
    GLint viewLoc = shader.getUniformLocation("uView");
    GLint projLoc = shader.getUniformLocation("uProjection");
    
    std::cout << "Uniform locations: model=" << modelLoc 
              << " view=" << viewLoc << " proj=" << projLoc << std::endl;
    
    // Set matrices based on test
    if (testNum == 4) {
        // Test 4: Use exact main app camera matrices
        setMainAppCameraMatrices(modelLoc, viewLoc, projLoc);
    } else if (testNum == 5) {
        // Test 5: Use fixed camera matrices to view voxel
        setFixedCameraMatrices(modelLoc, viewLoc, projLoc);
    } else if (testNum == 6) {
        // Test 6: Use simple camera matrices for center voxel
        setSimpleCameraMatrices(modelLoc, viewLoc, projLoc);
    } else {
        // Tests 1-3: Use identity matrices (NDC rendering)
        setIdentityMatrix(modelLoc);
        setIdentityMatrix(viewLoc);
        setIdentityMatrix(projLoc);
    }
    
    // Render loop
    for (int frame = 0; frame < 10 && !window.shouldClose(); ++frame) {
        window.pollEvents();
        
        // Clear
        glClearColor(0.2f, 0.2f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        
        // Render
        shader.use();
        if (testNum == 1) {
            renderer.renderSimpleTriangle();
        } else if (testNum == 2) {
            renderer.renderSimpleQuad();
        } else if (testNum == 3) {
            renderer.renderComplexCube();
        } else if (testNum == 4 || testNum == 5) {
            renderer.renderMainAppVoxel();
        } else if (testNum == 6) {
            renderer.renderCenterVoxel();
        }
        shader.unuse();
        
        window.swapBuffers();
        
        // Take screenshot on frame 5
        if (frame == 5) {
            std::string filename = "test" + std::to_string(testNum) + "_" + testName;
            window.saveScreenshot(filename);
        }
    }
    
    std::cout << "Test " << testNum << " completed" << std::endl;
    return 0;
}

int main(int argc, char* argv[]) {
    std::cout << "Pipeline Test Suite" << std::endl;
    std::cout << "===================" << std::endl;
    
    if (argc > 1) {
        int testNum = std::stoi(argv[1]);
        
        switch (testNum) {
            case 1:
                return runTest("SimpleTriangle", 1);
            case 2:
                return runTest("SimpleQuad", 2);
            case 3:
                return runTest("ComplexCube", 3);
            case 4:
                return runTest("MainAppVoxel", 4);
            case 5:
                return runTest("FixedCameraVoxel", 5);
            case 6:
                return runTest("SimpleCameraVoxel", 6);
            default:
                std::cerr << "Invalid test number. Use 1, 2, 3, 4, 5, or 6." << std::endl;
                return 1;
        }
    }
    
    // Run all tests
    int result = 0;
    result |= runTest("SimpleTriangle", 1);
    result |= runTest("SimpleQuad", 2);
    result |= runTest("ComplexCube", 3);
    result |= runTest("MainAppVoxel", 4);
    result |= runTest("FixedCameraVoxel", 5);
    result |= runTest("SimpleCameraVoxel", 6);
    
    std::cout << "\n=== ALL TESTS COMPLETED ===\n" << std::endl;
    return result;
}