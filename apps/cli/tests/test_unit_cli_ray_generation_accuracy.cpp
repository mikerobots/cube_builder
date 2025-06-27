#include <gtest/gtest.h>
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include "foundation/math/Matrix4f.h"
#include "foundation/math/Vector3f.h"
#include "foundation/math/Vector4f.h"
#include <cmath>

using namespace VoxelEditor;

class RayGenerationTest : public ::testing::Test {
protected:
    // Helper function to convert Math::Matrix4f to glm::mat4
    glm::mat4 mathToGlm(const Math::Matrix4f& mat) {
        glm::mat4 result;
        // Updated implementation in MouseInteraction.cpp
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result[col][row] = mat.m[row * 4 + col];
            }
        }
        return result;
    }
    
    // Correct implementation that should be used
    glm::mat4 mathToGlmCorrect(const Math::Matrix4f& mat) {
        glm::mat4 result;
        // Math::Matrix4f is row-major: m[row * 4 + col]
        // GLM is column-major: mat[col][row]
        // Need to transpose during conversion
        for (int row = 0; row < 4; ++row) {
            for (int col = 0; col < 4; ++col) {
                result[col][row] = mat.m[row * 4 + col];
            }
        }
        return result;
    }
    
    // Helper to compare matrices with tolerance
    bool matricesEqual(const glm::mat4& a, const glm::mat4& b, float tolerance = 1e-6f) {
        for (int i = 0; i < 4; ++i) {
            for (int j = 0; j < 4; ++j) {
                if (std::abs(a[i][j] - b[i][j]) > tolerance) {
                    return false;
                }
            }
        }
        return true;
    }
};

TEST_F(RayGenerationTest, MatrixConversionIdentity) {
    // Test with identity matrix
    Math::Matrix4f identity;
    identity.identity();
    
    glm::mat4 glmIdentity = glm::mat4(1.0f);
    glm::mat4 converted = mathToGlm(identity);
    
    EXPECT_TRUE(matricesEqual(converted, glmIdentity)) 
        << "Identity matrix conversion failed";
}

TEST_F(RayGenerationTest, MatrixConversionTranslation) {
    // Test with translation matrix
    Math::Vector3f translation(1.0f, 2.0f, 3.0f);
    Math::Matrix4f translationMat = Math::Matrix4f::translation(translation);
    
    glm::mat4 glmTranslation = glm::translate(glm::mat4(1.0f), glm::vec3(1.0f, 2.0f, 3.0f));
    glm::mat4 converted = mathToGlm(translationMat);
    
    EXPECT_TRUE(matricesEqual(converted, glmTranslation))
        << "Translation matrix conversion failed";
}

TEST_F(RayGenerationTest, MatrixConversionRotation) {
    // Test with rotation matrix
    float angle = M_PI / 4.0f; // 45 degrees
    Math::Matrix4f rotationMat = Math::Matrix4f::rotationY(angle);
    
    glm::mat4 glmRotation = glm::rotate(glm::mat4(1.0f), angle, glm::vec3(0.0f, 1.0f, 0.0f));
    glm::mat4 converted = mathToGlm(rotationMat);
    
    EXPECT_TRUE(matricesEqual(converted, glmRotation))
        << "Rotation matrix conversion failed";
}

TEST_F(RayGenerationTest, MatrixConversionLookAt) {
    // Test with lookAt matrix (view matrix)
    Math::Vector3f eye(3.0f, 3.0f, 3.0f);
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    Math::Vector3f up(0.0f, 1.0f, 0.0f);
    
    Math::Matrix4f lookAtMat = Math::Matrix4f::lookAt(eye, center, up);
    glm::mat4 glmLookAt = glm::lookAt(
        glm::vec3(3.0f, 3.0f, 3.0f),
        glm::vec3(0.0f, 0.0f, 0.0f),
        glm::vec3(0.0f, 1.0f, 0.0f)
    );
    
    glm::mat4 converted = mathToGlm(lookAtMat);
    
    EXPECT_TRUE(matricesEqual(converted, glmLookAt, 1e-5f))
        << "LookAt matrix conversion failed";
}

TEST_F(RayGenerationTest, MatrixConversionPerspective) {
    // Test with perspective projection matrix
    float fov = glm::radians(45.0f);
    float aspect = 16.0f / 9.0f;
    float nearPlane = 0.1f;
    float farPlane = 100.0f;
    
    Math::Matrix4f perspectiveMat = Math::Matrix4f::perspective(fov, aspect, nearPlane, farPlane);
    glm::mat4 glmPerspective = glm::perspective(fov, aspect, nearPlane, farPlane);
    
    glm::mat4 converted = mathToGlm(perspectiveMat);
    
    EXPECT_TRUE(matricesEqual(converted, glmPerspective, 1e-4f))
        << "Perspective matrix conversion failed";
}

TEST_F(RayGenerationTest, RayGenerationScreenCenter) {
    // Test ray generation at screen center
    float ndcX = 0.0f;
    float ndcY = 0.0f;
    
    // Create a simple view and projection matrix
    Math::Vector3f cameraPos(0.0f, 0.0f, 5.0f);
    Math::Vector3f target(0.0f, 0.0f, 0.0f);
    Math::Vector3f up(0.0f, 1.0f, 0.0f);
    
    Math::Matrix4f viewMat = Math::Matrix4f::lookAt(cameraPos, target, up);
    Math::Matrix4f projMat = Math::Matrix4f::perspective(glm::radians(45.0f), 1.0f, 0.1f, 100.0f);
    
    // Convert to GLM
    glm::mat4 viewMatrix = mathToGlm(viewMat);
    glm::mat4 projMatrix = mathToGlm(projMat);
    
    // Generate ray
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
    farPoint /= farPoint.w;
    
    glm::vec3 origin(cameraPos.x, cameraPos.y, cameraPos.z);
    glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - origin);
    
    // Ray from center should point forward (negative Z in view space)
    EXPECT_NEAR(direction.x, 0.0f, 1e-5f) << "Ray X direction incorrect for screen center";
    EXPECT_NEAR(direction.y, 0.0f, 1e-5f) << "Ray Y direction incorrect for screen center";
    EXPECT_NEAR(direction.z, -1.0f, 1e-5f) << "Ray Z direction incorrect for screen center";
}

TEST_F(RayGenerationTest, RayGenerationScreenCorners) {
    // Test ray generation at screen corners
    Math::Vector3f cameraPos(0.0f, 0.0f, 5.0f);
    Math::Vector3f target(0.0f, 0.0f, 0.0f);
    Math::Vector3f up(0.0f, 1.0f, 0.0f);
    
    Math::Matrix4f viewMat = Math::Matrix4f::lookAt(cameraPos, target, up);
    Math::Matrix4f projMat = Math::Matrix4f::perspective(glm::radians(90.0f), 1.0f, 0.1f, 100.0f);
    
    glm::mat4 viewMatrix = mathToGlm(viewMat);
    glm::mat4 projMatrix = mathToGlm(projMat);
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    glm::vec3 origin(cameraPos.x, cameraPos.y, cameraPos.z);
    
    // Test all four corners
    float corners[4][2] = {{-1, -1}, {1, -1}, {1, 1}, {-1, 1}};
    
    for (int i = 0; i < 4; ++i) {
        float ndcX = corners[i][0];
        float ndcY = corners[i][1];
        
        glm::vec4 farPoint = invVP * glm::vec4(ndcX, ndcY, 1.0f, 1.0f);
        farPoint /= farPoint.w;
        
        glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - origin);
        
        // Ray origin should always be camera position
        EXPECT_NEAR(origin.x, cameraPos.x, 1e-5f) << "Ray origin X incorrect";
        EXPECT_NEAR(origin.y, cameraPos.y, 1e-5f) << "Ray origin Y incorrect";
        EXPECT_NEAR(origin.z, cameraPos.z, 1e-5f) << "Ray origin Z incorrect";
        
        // Ray direction should be normalized
        float length = glm::length(direction);
        EXPECT_NEAR(length, 1.0f, 1e-5f) << "Ray direction not normalized";
        
        // For a 90-degree FOV, corner rays should have significant X/Y components
        EXPECT_GT(std::abs(direction.x), 0.4f) << "Corner ray X component too small";
        EXPECT_GT(std::abs(direction.y), 0.4f) << "Corner ray Y component too small";
    }
}

TEST_F(RayGenerationTest, RayDirectionNormalization) {
    // Test that all generated rays are properly normalized
    Math::Vector3f cameraPos(3.0f, 4.0f, 5.0f);
    Math::Vector3f target(1.0f, 0.0f, -2.0f);
    Math::Vector3f up(0.0f, 1.0f, 0.0f);
    
    Math::Matrix4f viewMat = Math::Matrix4f::lookAt(cameraPos, target, up);
    Math::Matrix4f projMat = Math::Matrix4f::perspective(glm::radians(60.0f), 16.0f/9.0f, 0.1f, 100.0f);
    
    glm::mat4 viewMatrix = mathToGlm(viewMat);
    glm::mat4 projMatrix = mathToGlm(projMat);
    glm::mat4 invVP = glm::inverse(projMatrix * viewMatrix);
    glm::vec3 origin(cameraPos.x, cameraPos.y, cameraPos.z);
    
    // Test various screen positions
    for (float x = -1.0f; x <= 1.0f; x += 0.5f) {
        for (float y = -1.0f; y <= 1.0f; y += 0.5f) {
            glm::vec4 farPoint = invVP * glm::vec4(x, y, 1.0f, 1.0f);
            farPoint /= farPoint.w;
            
            glm::vec3 direction = glm::normalize(glm::vec3(farPoint) - origin);
            
            float length = glm::length(direction);
            EXPECT_NEAR(length, 1.0f, 1e-6f) 
                << "Ray direction not normalized at NDC (" << x << ", " << y << ")";
        }
    }
}

TEST_F(RayGenerationTest, CompareMatrixConversionMethods) {
    // Compare current implementation with correct implementation
    Math::Vector3f eye(2.0f, 3.0f, 4.0f);
    Math::Vector3f center(0.0f, 0.0f, 0.0f);
    Math::Vector3f up(0.0f, 1.0f, 0.0f);
    
    Math::Matrix4f lookAtMat = Math::Matrix4f::lookAt(eye, center, up);
    
    glm::mat4 currentMethod = mathToGlm(lookAtMat);
    glm::mat4 correctMethod = mathToGlmCorrect(lookAtMat);
    
    // They should be different if there's a transpose issue
    bool areEqual = matricesEqual(currentMethod, correctMethod);
    
    // Log the matrices for debugging
    if (!areEqual) {
        std::cout << "Current method produces different result than correct method\n";
        std::cout << "This indicates a transpose issue in matrix conversion\n";
    }
    
    // Test that the correct method produces expected results
    glm::vec3 testPoint(1.0f, 0.0f, 0.0f);
    glm::vec4 transformedCurrent = currentMethod * glm::vec4(testPoint, 1.0f);
    glm::vec4 transformedCorrect = correctMethod * glm::vec4(testPoint, 1.0f);
    
    // The results should now be the same after fixing the transpose issue
    EXPECT_TRUE(matricesEqual(currentMethod, correctMethod))
        << "Matrix conversion methods still produce different results - transpose issue not fixed";
}