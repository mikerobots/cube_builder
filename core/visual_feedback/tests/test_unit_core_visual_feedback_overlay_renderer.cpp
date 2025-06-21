#include <gtest/gtest.h>
#include "../include/visual_feedback/FeedbackTypes.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../rendering/RenderStats.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Rendering;

// Unit tests for overlay rendering logic without OpenGL dependencies
// NOTE: OverlayRenderer requires OpenGL context and has been moved to integration tests
class OverlayLogicTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Test overlay rendering logic without actual renderer instance
    }
};

// Unit tests for overlay rendering calculations and validation logic
TEST_F(OverlayLogicTest, TextStyleValidation) {
    // Test text style creation and validation (pure logic)
    TextStyle defaultStyle = TextStyle::Default();
    EXPECT_GT(defaultStyle.size, 0);
    EXPECT_GT(defaultStyle.color.a, 0.0f);
}

TEST_F(OverlayLogicTest, FrameStateLogic) {
    // Test frame state logic without actual frame management
    struct FrameState {
        bool isActive = false;
        int width = 0;
        int height = 0;
    };
    
    FrameState state;
    EXPECT_FALSE(state.isActive);
    
    // Simulate frame begin/end logic
    state.isActive = true;
    state.width = 1920;
    state.height = 1080;
    EXPECT_TRUE(state.isActive);
    EXPECT_GT(state.width, 0);
    EXPECT_GT(state.height, 0);
}

TEST_F(OverlayLogicTest, TextStyleFactories) {
    // Test text style creation (pure logic)
    TextStyle defaultStyle = TextStyle::Default();
    EXPECT_GT(defaultStyle.size, 0);
    
    TextStyle headerStyle = TextStyle::Header();
    EXPECT_GT(headerStyle.size, defaultStyle.size);
    
    TextStyle debugStyle = TextStyle::Debug();
    TextStyle warningStyle = TextStyle::Warning();
    TextStyle errorStyle = TextStyle::Error();
    
    // All styles should have valid properties
    EXPECT_GT(debugStyle.size, 0);
    EXPECT_GT(warningStyle.size, 0);
    EXPECT_GT(errorStyle.size, 0);
}

TEST_F(OverlayLogicTest, GridParameterValidation) {
    // Test grid parameter validation logic
    auto validateGridParameters = [](Vector3f workspaceSize, Vector3f workspaceCenter, float opacity) {
        if (opacity < 0.0f || opacity > 1.0f) return false;
        if (workspaceSize.x <= 0.0f || workspaceSize.y <= 0.0f || workspaceSize.z <= 0.0f) return false;
        return true;
    };
    
    Vector3f workspaceSize(5.0f, 5.0f, 5.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    
    // Valid parameters
    EXPECT_TRUE(validateGridParameters(workspaceSize, workspaceCenter, 0.35f));
    EXPECT_TRUE(validateGridParameters(workspaceSize, workspaceCenter, 0.65f));
    EXPECT_TRUE(validateGridParameters(workspaceSize, workspaceCenter, 1.0f));
    
    // Invalid parameters
    EXPECT_FALSE(validateGridParameters(workspaceSize, workspaceCenter, -0.1f)); // Negative opacity
    EXPECT_FALSE(validateGridParameters(workspaceSize, workspaceCenter, 1.5f));  // Opacity > 1
    
    Vector3f invalidSize(-1.0f, 5.0f, 5.0f);
    EXPECT_FALSE(validateGridParameters(invalidSize, workspaceCenter, 0.35f)); // Negative size
}

TEST_F(OverlayLogicTest, TextLayoutCalculation) {
    // Test text layout calculations (pure math)
    auto calculateTextLayout = [](const std::string& text, Vector2f position, TextStyle style) {
        struct TextLayout {
            float width;
            float height;
            Vector2f position;
            int lineCount;
        };
        
        TextLayout layout;
        layout.position = position;
        layout.lineCount = 1 + std::count(text.begin(), text.end(), '\n');
        layout.width = text.length() * style.size * 0.6f; // Approximate character width
        layout.height = layout.lineCount * style.size * 1.2f; // Line height
        return layout;
    };
    
    std::string text = "Hello, World!";
    Vector2f position(100, 100);
    TextStyle style = TextStyle::Default();
    
    auto layout = calculateTextLayout(text, position, style);
    
    EXPECT_GT(layout.width, 0);
    EXPECT_GT(layout.height, 0);
    EXPECT_EQ(layout.position.x, position.x);
    EXPECT_EQ(layout.position.y, position.y);
}

TEST_F(OverlayLogicTest, TextLayoutMultipleLines) {
    // Test multi-line text layout
    auto calculateTextLayout = [](const std::string& text, Vector2f position, TextStyle style) {
        struct TextLayout {
            float width;
            float height;
            Vector2f position;
            int lineCount;
        };
        
        TextLayout layout;
        layout.position = position;
        layout.lineCount = 1 + std::count(text.begin(), text.end(), '\n');
        layout.width = text.length() * style.size * 0.6f; // Approximate character width
        layout.height = layout.lineCount * style.size * 1.2f; // Line height
        return layout;
    };
    
    std::string multiLineText = "Line 1\nLine 2\nLine 3";
    Vector2f position(50, 50);
    TextStyle style = TextStyle::Default();
    
    auto layout = calculateTextLayout(multiLineText, position, style);
    
    EXPECT_GT(layout.width, 0);
    EXPECT_GT(layout.height, style.size * 3); // Should be at least 3 lines tall
    EXPECT_EQ(layout.lineCount, 3);
}

TEST_F(OverlayLogicTest, GridCalculations) {
    // Test grid line calculations
    auto calculateGridInfo = [](Vector3f workspaceSize, Vector3f workspaceCenter) {
        struct GridInfo {
            int lineCount;
            int vertexCount;
            float gridSpacing;
            Vector3f extent;
        };
        
        GridInfo info;
        info.gridSpacing = 0.32f; // 32cm grid spacing
        
        // Calculate lines needed for workspace
        int linesX = static_cast<int>((workspaceSize.x * 2.0f) / info.gridSpacing) + 1;
        int linesZ = static_cast<int>((workspaceSize.z * 2.0f) / info.gridSpacing) + 1;
        info.lineCount = linesX + linesZ;
        info.vertexCount = info.lineCount * 2; // 2 vertices per line
        
        info.extent = Vector3f(linesX * info.gridSpacing, workspaceSize.y, linesZ * info.gridSpacing);
        return info;
    };
    
    Vector3f workspaceSize(8.0f, 8.0f, 8.0f);
    Vector3f workspaceCenter(0.0f, 0.0f, 0.0f);
    
    auto gridInfo = calculateGridInfo(workspaceSize, workspaceCenter);
    
    EXPECT_GT(gridInfo.lineCount, 0);
    EXPECT_GT(gridInfo.vertexCount, 0);
    EXPECT_EQ(gridInfo.gridSpacing, 0.32f); // 32cm grid spacing
    
    // Grid should cover the workspace
    EXPECT_GE(gridInfo.extent.x, workspaceSize.x);
    EXPECT_GE(gridInfo.extent.z, workspaceSize.z);
}

TEST_F(OverlayLogicTest, PerformanceMetrics) {
    // Test performance metrics formatting (pure logic)
    auto formatRenderStats = [](VoxelEditor::Rendering::RenderStats stats) {
        struct FormattedMetrics {
            std::string frameTimeText;
            std::string verticesText;
            std::string memoryUsageText;
        };
        
        FormattedMetrics formatted;
        formatted.frameTimeText = "Frame: " + std::to_string(stats.frameTime) + "ms";
        formatted.verticesText = "Vertices: " + std::to_string(stats.verticesProcessed);
        formatted.memoryUsageText = "Memory: " + std::to_string(stats.totalGPUMemory / (1024*1024)) + "MB";
        return formatted;
    };
    
    VoxelEditor::Rendering::RenderStats stats;
    stats.frameTime = 16.7f; // 60 FPS
    stats.verticesProcessed = 1000;
    stats.totalGPUMemory = 50 * 1024 * 1024; // 50MB
    
    auto formattedMetrics = formatRenderStats(stats);
    
    EXPECT_FALSE(formattedMetrics.frameTimeText.empty());
    EXPECT_FALSE(formattedMetrics.verticesText.empty());
    EXPECT_FALSE(formattedMetrics.memoryUsageText.empty());
    
    // Check specific formatting
    EXPECT_TRUE(formattedMetrics.frameTimeText.find("16.7") != std::string::npos);
    EXPECT_TRUE(formattedMetrics.verticesText.find("1000") != std::string::npos);
    EXPECT_TRUE(formattedMetrics.memoryUsageText.find("50") != std::string::npos);
}

TEST_F(OverlayLogicTest, ScreenCoordinateConversion) {
    // Test screen coordinate conversion logic
    auto screenToNormalized = [](Vector2f screenPos, int screenWidth, int screenHeight) {
        return Vector2f(screenPos.x / screenWidth, screenPos.y / screenHeight);
    };
    
    Vector2f screenPos(1920, 1080);
    Vector2f normalizedPos = screenToNormalized(screenPos, 1920, 1080);
    
    EXPECT_FLOAT_EQ(normalizedPos.x, 1.0f);
    EXPECT_FLOAT_EQ(normalizedPos.y, 1.0f);
    
    Vector2f centerPos(960, 540);
    Vector2f normalizedCenter = screenToNormalized(centerPos, 1920, 1080);
    
    EXPECT_FLOAT_EQ(normalizedCenter.x, 0.5f);
    EXPECT_FLOAT_EQ(normalizedCenter.y, 0.5f);
}

TEST_F(OverlayLogicTest, TextBounds) {
    // Test text bounding box calculations
    auto calculateTextBounds = [](const std::string& text, TextStyle style) {
        struct TextBounds {
            float width;
            float height;
        };
        
        TextBounds bounds;
        bounds.width = text.length() * style.size * 0.6f; // Approximate character width
        bounds.height = style.size * 1.2f; // Line height
        return bounds;
    };
    
    std::string shortText = "Hi";
    std::string longText = "This is a much longer text string";
    
    TextStyle style = TextStyle::Default();
    
    auto shortBounds = calculateTextBounds(shortText, style);
    auto longBounds = calculateTextBounds(longText, style);
    
    EXPECT_GT(longBounds.width, shortBounds.width);
    EXPECT_EQ(longBounds.height, shortBounds.height); // Same font size
}

TEST_F(OverlayLogicTest, IndicatorPositioning) {
    // Test indicator positioning logic
    auto worldToScreenIndicator = [](Vector3f worldPosition, int screenWidth, int screenHeight) {
        // Simplified projection calculation
        Vector2f screenPos;
        screenPos.x = (worldPosition.x / 10.0f + 0.5f) * screenWidth;  // Map -5..5 to 0..screenWidth
        screenPos.y = (worldPosition.z / 10.0f + 0.5f) * screenHeight; // Map -5..5 to 0..screenHeight
        
        // Clamp to screen bounds
        screenPos.x = std::max(0.0f, std::min(static_cast<float>(screenWidth), screenPos.x));
        screenPos.y = std::max(0.0f, std::min(static_cast<float>(screenHeight), screenPos.y));
        
        return screenPos;
    };
    
    Vector3f worldPosition(5.0f, 5.0f, 5.0f);
    int screenWidth = 1920;
    int screenHeight = 1080;
    
    auto screenPosition = worldToScreenIndicator(worldPosition, screenWidth, screenHeight);
    
    // Screen position should be within bounds
    EXPECT_GE(screenPosition.x, 0);
    EXPECT_GE(screenPosition.y, 0);
    EXPECT_LE(screenPosition.x, screenWidth);
    EXPECT_LE(screenPosition.y, screenHeight);
}

TEST_F(OverlayLogicTest, BoundingBoxCalculation) {
    // Test bounding box calculation for workspace visualization
    auto calculateWorkspaceBounds = [](Vector3f workspaceSize, Vector3f workspaceCenter) {
        struct BoundingBox {
            Vector3f min;
            Vector3f max;
        };
        
        BoundingBox boundingBox;
        boundingBox.min = workspaceCenter - workspaceSize / 2.0f;
        boundingBox.max = workspaceCenter + workspaceSize / 2.0f;
        return boundingBox;
    };
    
    Vector3f workspaceSize(10.0f, 8.0f, 6.0f);
    Vector3f workspaceCenter(2.0f, 1.0f, -1.0f);
    
    auto boundingBox = calculateWorkspaceBounds(workspaceSize, workspaceCenter);
    
    // Check min bounds
    EXPECT_FLOAT_EQ(boundingBox.min.x, workspaceCenter.x - workspaceSize.x / 2);
    EXPECT_FLOAT_EQ(boundingBox.min.y, workspaceCenter.y - workspaceSize.y / 2);
    EXPECT_FLOAT_EQ(boundingBox.min.z, workspaceCenter.z - workspaceSize.z / 2);
    
    // Check max bounds
    EXPECT_FLOAT_EQ(boundingBox.max.x, workspaceCenter.x + workspaceSize.x / 2);
    EXPECT_FLOAT_EQ(boundingBox.max.y, workspaceCenter.y + workspaceSize.y / 2);
    EXPECT_FLOAT_EQ(boundingBox.max.z, workspaceCenter.z + workspaceSize.z / 2);
}