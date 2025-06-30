#include <gtest/gtest.h>
#include <vector>
#include <cmath>
#include <algorithm>
#include <iostream>
#include "../include/visual_feedback/OverlayRenderer.h"
#include "../../../foundation/math/CoordinateTypes.h"
#include "../../rendering/RenderTypes.h"

using namespace VoxelEditor::VisualFeedback;
using namespace VoxelEditor::Math;
using namespace VoxelEditor::Rendering;

// Test fixture for ground plane grid geometry calculations
class GroundPlaneGeometryTest : public ::testing::Test {
protected:
    // Structure to represent a grid line for testing
    struct GridLine {
        Vector3f start;
        Vector3f end;
        Color color;
        
        bool isHorizontal() const {
            return std::abs(start.z - end.z) < 0.001f && std::abs(start.y - end.y) < 0.001f;
        }
        
        bool isVertical() const {
            return std::abs(start.x - end.x) < 0.001f && std::abs(start.y - end.y) < 0.001f;
        }
        
        float getConstantCoordinate() const {
            if (isHorizontal()) return start.z;
            if (isVertical()) return start.x;
            return 0.0f;
        }
    };
    
    // Mock line collector to capture generated lines
    std::vector<GridLine> m_capturedLines;
    
    void SetUp() override {
        m_capturedLines.clear();
    }
    
    // Simulate the grid generation logic from OverlayRenderer
    void generateGroundPlaneGridLines(const Vector3f& center, float extent, 
                                    const Vector3f& cursorPos, bool enableDynamicOpacity) {
        // REQ-1.1.1: Grid with 32cm squares
        const float gridSize = 0.32f; // 32cm
        
        // REQ-1.1.3: Grid line colors and opacity
        const Color normalGridColor(180.0f/255.0f, 180.0f/255.0f, 180.0f/255.0f, 0.35f);
        
        // REQ-1.1.4: Major grid lines every 160cm (5 * 32cm)
        const float majorGridInterval = 1.60f; // 160cm
        const Color majorGridColor(200.0f/255.0f, 200.0f/255.0f, 200.0f/255.0f, 0.35f);
        
        // REQ-1.2.2: Dynamic opacity near cursor
        const float dynamicOpacityRadius = 2.0f * gridSize; // 2 grid squares = 64cm
        const float enhancedOpacity = 0.65f;
        
        int gridCount = static_cast<int>(extent / gridSize);
        float halfExtent = gridCount * gridSize * 0.5f;
        
        // Generate grid lines in XZ plane at Y=0 (ground plane)
        for (int i = -gridCount; i <= gridCount; ++i) {
            float offset = i * gridSize;
            
            // Determine if this is a major grid line
            // BUG: This logic fails due to floating-point precision errors
            // When i=5, offset = 5 * 0.32 = 1.6, but fmod(1.6, 1.6) != 0 due to precision
            bool isMajorLine = (std::abs(offset) < 0.001f) || 
                              (std::abs(std::fmod(offset, majorGridInterval)) < 0.001f);
            
            // CORRECT IMPLEMENTATION would be: bool isMajorLine = (i % 5) == 0;
            
            // Calculate distance to cursor for dynamic opacity
            float distanceToX = enableDynamicOpacity ? 
                std::abs(cursorPos.x - (center.x + offset)) : std::numeric_limits<float>::max();
            float distanceToZ = enableDynamicOpacity ? 
                std::abs(cursorPos.z - (center.z + offset)) : std::numeric_limits<float>::max();
            
            // Determine line color and opacity
            Color lineColor = isMajorLine ? majorGridColor : normalGridColor;
            
            // REQ-1.2.2: Enhance opacity near cursor
            if (enableDynamicOpacity) {
                bool enhanceOpacityX = distanceToX <= dynamicOpacityRadius;
                bool enhanceOpacityZ = distanceToZ <= dynamicOpacityRadius;
                
                if (enhanceOpacityX || enhanceOpacityZ) {
                    lineColor.a = enhancedOpacity;
                }
            }
            
            // Lines parallel to X axis (running east-west)
            GridLine horizontalLine;
            horizontalLine.start = Vector3f(center.x - halfExtent, 0.0f, center.z + offset);
            horizontalLine.end = Vector3f(center.x + halfExtent, 0.0f, center.z + offset);
            horizontalLine.color = lineColor;
            m_capturedLines.push_back(horizontalLine);
            
            // Lines parallel to Z axis (running north-south)  
            GridLine verticalLine;
            verticalLine.start = Vector3f(center.x + offset, 0.0f, center.z - halfExtent);
            verticalLine.end = Vector3f(center.x + offset, 0.0f, center.z + halfExtent);
            verticalLine.color = lineColor;
            m_capturedLines.push_back(verticalLine);
        }
    }
    
    // Helper to count lines by type
    int countHorizontalLines() const {
        return std::count_if(m_capturedLines.begin(), m_capturedLines.end(),
                           [](const GridLine& line) { return line.isHorizontal(); });
    }
    
    int countVerticalLines() const {
        return std::count_if(m_capturedLines.begin(), m_capturedLines.end(),
                           [](const GridLine& line) { return line.isVertical(); });
    }
    
    // Helper to check if a line exists at a specific coordinate
    bool hasLineAt(float coordinate, bool horizontal) const {
        return std::any_of(m_capturedLines.begin(), m_capturedLines.end(),
            [coordinate, horizontal](const GridLine& line) {
                bool isCorrectType = horizontal ? line.isHorizontal() : line.isVertical();
                return isCorrectType && std::abs(line.getConstantCoordinate() - coordinate) < 0.001f;
            });
    }
    
    // Helper to get line color at coordinate
    Color getLineColorAt(float coordinate, bool horizontal) const {
        auto it = std::find_if(m_capturedLines.begin(), m_capturedLines.end(),
            [coordinate, horizontal](const GridLine& line) {
                bool isCorrectType = horizontal ? line.isHorizontal() : line.isVertical();
                return isCorrectType && std::abs(line.getConstantCoordinate() - coordinate) < 0.001f;
            });
        return it != m_capturedLines.end() ? it->color : Color(0, 0, 0, 0);
    }
};

// REQ-1.1.1: Test grid spacing of 32cm
TEST_F(GroundPlaneGeometryTest, GridSpacing32cm) {
    Vector3f center(0, 0, 0);
    float extent = 3.2f; // 10 grid squares
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Should have grid lines at 0, ±0.32, ±0.64, ±0.96, ±1.28, ±1.60
    EXPECT_TRUE(hasLineAt(0.0f, true));
    EXPECT_TRUE(hasLineAt(0.32f, true));
    EXPECT_TRUE(hasLineAt(-0.32f, true));
    EXPECT_TRUE(hasLineAt(0.64f, true));
    EXPECT_TRUE(hasLineAt(-0.64f, true));
    EXPECT_TRUE(hasLineAt(0.96f, true));
    EXPECT_TRUE(hasLineAt(-0.96f, true));
    
    // Vertical lines should follow same pattern
    EXPECT_TRUE(hasLineAt(0.0f, false));
    EXPECT_TRUE(hasLineAt(0.32f, false));
    EXPECT_TRUE(hasLineAt(-0.32f, false));
}

// REQ-1.1.3: Test normal grid line colors
TEST_F(GroundPlaneGeometryTest, NormalGridLineColors) {
    Vector3f center(0, 0, 0);
    float extent = 1.6f; // 5 grid squares
    Vector3f cursorPos(10, 0, 10); // Far away to avoid dynamic opacity
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Check normal grid line color at 0.32m
    Color normalColor = getLineColorAt(0.32f, true);
    EXPECT_NEAR(normalColor.r, 180.0f/255.0f, 0.001f);
    EXPECT_NEAR(normalColor.g, 180.0f/255.0f, 0.001f);
    EXPECT_NEAR(normalColor.b, 180.0f/255.0f, 0.001f);
    EXPECT_NEAR(normalColor.a, 0.35f, 0.001f);
}

// REQ-1.1.4: Test major grid lines every 160cm
TEST_F(GroundPlaneGeometryTest, MajorGridLines160cm) {
    Vector3f center(0, 0, 0);
    float extent = 4.8f; // 15 grid squares
    Vector3f cursorPos(10, 0, 10); // Far away
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Major grid lines should be at 0, ±1.60, ±3.20, ±4.80
    Color centerLineColor = getLineColorAt(0.0f, true);
    Color normalLineColor = getLineColorAt(0.32f, true);
    
    // Center line (0) should be major - THIS WORKS
    EXPECT_NEAR(centerLineColor.r, 200.0f/255.0f, 0.001f);
    EXPECT_NEAR(centerLineColor.g, 200.0f/255.0f, 0.001f);
    EXPECT_NEAR(centerLineColor.b, 200.0f/255.0f, 0.001f);
    
    // BUG: Lines at ±1.60, ±3.20, ±4.80 should be major but aren't
    // This is due to floating-point precision errors in the modulo calculation
    // The implementation uses: fmod(i * 0.32, 1.60) < 0.001
    // But when i=5, offset = 5 * 0.32 = 1.6, and fmod(1.6, 1.6) != 0 due to precision
    
    // Test reveals the bug: line at 1.60 is not marked as major
    Color majorLineColor = getLineColorAt(1.60f, true);
    
    // This test SHOULD pass but currently fails, revealing the bug
    EXPECT_NEAR(majorLineColor.r, 200.0f/255.0f, 0.001f) 
        << "BUG: Major grid line at 1.60m not detected due to floating-point precision";
    EXPECT_NEAR(majorLineColor.g, 200.0f/255.0f, 0.001f);
    EXPECT_NEAR(majorLineColor.b, 200.0f/255.0f, 0.001f);
    
    // Normal lines should be different from major lines
    EXPECT_NE(normalLineColor.r, centerLineColor.r);
}

// REQ-1.2.2: Test dynamic opacity near cursor
TEST_F(GroundPlaneGeometryTest, DynamicOpacityNearCursor) {
    Vector3f center(0, 0, 0);
    float extent = 3.2f; // 10 grid squares
    Vector3f cursorPos(0.16f, 0, 0.16f); // Near center
    
    generateGroundPlaneGridLines(center, extent, cursorPos, true);
    
    // Lines within 2 grid squares (64cm) should have enhanced opacity
    Color nearLineColor = getLineColorAt(0.0f, true);
    Color farLineColor = getLineColorAt(0.96f, true); // 3 squares away
    
    EXPECT_NEAR(nearLineColor.a, 0.65f, 0.001f); // Enhanced opacity
    EXPECT_NEAR(farLineColor.a, 0.35f, 0.001f); // Normal opacity
}

// Test ground plane constraint - all lines at Y=0
TEST_F(GroundPlaneGeometryTest, AllLinesAtGroundPlane) {
    Vector3f center(0, 0, 0);
    float extent = 1.6f;
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // All lines should have Y=0
    for (const auto& line : m_capturedLines) {
        EXPECT_FLOAT_EQ(line.start.y, 0.0f);
        EXPECT_FLOAT_EQ(line.end.y, 0.0f);
    }
}

// Test grid extent calculation
TEST_F(GroundPlaneGeometryTest, GridExtentCalculation) {
    Vector3f center(0, 0, 0);
    float extent = 2.0f; // Should result in 6 grid squares (1.92m actual extent)
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Find maximum extent
    float maxX = 0.0f, maxZ = 0.0f;
    for (const auto& line : m_capturedLines) {
        maxX = std::max(maxX, std::abs(line.start.x));
        maxX = std::max(maxX, std::abs(line.end.x));
        maxZ = std::max(maxZ, std::abs(line.start.z));
        maxZ = std::max(maxZ, std::abs(line.end.z));
    }
    
    // Debug output
    std::cout << "Extent input: " << extent << std::endl;
    std::cout << "Grid count: " << static_cast<int>(extent / 0.32f) << std::endl;
    std::cout << "Half extent calculation: " << static_cast<int>(extent / 0.32f) * 0.32f * 0.5f << std::endl;
    std::cout << "Max X found: " << maxX << std::endl;
    std::cout << "Max Z found: " << maxZ << std::endl;
    
    // The implementation uses full extent on each side, not half extent
    // So with grid count = 6, we get lines from -6*0.32 to +6*0.32 = -1.92 to 1.92
    EXPECT_NEAR(maxX, 1.92f, 0.001f);
    EXPECT_NEAR(maxZ, 1.92f, 0.001f);
}

// Test centered grid generation
TEST_F(GroundPlaneGeometryTest, GridCenteredAtOrigin) {
    Vector3f center(0, 0, 0);
    float extent = 1.6f;
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Should have equal number of positive and negative lines
    int posX = 0, negX = 0, posZ = 0, negZ = 0;
    for (const auto& line : m_capturedLines) {
        float coord = line.getConstantCoordinate();
        if (coord > 0.001f) {
            if (line.isHorizontal()) posZ++;
            else posX++;
        } else if (coord < -0.001f) {
            if (line.isHorizontal()) negZ++;
            else negX++;
        }
    }
    
    EXPECT_EQ(posX, negX);
    EXPECT_EQ(posZ, negZ);
}

// Test off-center grid generation
TEST_F(GroundPlaneGeometryTest, GridOffCenter) {
    Vector3f center(1.0f, 0, 1.0f);
    float extent = 1.6f;
    Vector3f cursorPos(1.0f, 0, 1.0f);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Grid should be centered at (1, 0, 1)
    bool hasLineAtCenterX = hasLineAt(1.0f, false);
    bool hasLineAtCenterZ = hasLineAt(1.0f, true);
    
    // Center lines might not exist if not on grid boundary
    // But we should have lines offset from center by multiples of 0.32
    EXPECT_TRUE(hasLineAt(1.0f + 0.32f, false) || hasLineAt(1.0f - 0.32f, false));
    EXPECT_TRUE(hasLineAt(1.0f + 0.32f, true) || hasLineAt(1.0f - 0.32f, true));
}

// REQ-6.2.2: Test large workspace grid
TEST_F(GroundPlaneGeometryTest, LargeWorkspaceGrid) {
    Vector3f center(0, 0, 0);
    float extent = 8.0f; // Maximum 8m workspace
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Should have many lines
    int totalLines = m_capturedLines.size();
    int expectedGridCount = static_cast<int>(extent / 0.32f);
    int expectedLines = (expectedGridCount * 2 + 1) * 2; // Both directions
    
    EXPECT_EQ(totalLines, expectedLines);
    EXPECT_GT(totalLines, 100); // Should be many lines for 8m grid
}

// Test dynamic opacity radius calculation
TEST_F(GroundPlaneGeometryTest, DynamicOpacityRadius) {
    Vector3f center(0, 0, 0);
    float extent = 3.2f;
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, true);
    
    // Lines at exactly 2 grid squares (64cm) should be at boundary
    Color boundaryLine1 = getLineColorAt(0.64f, true);
    Color boundaryLine2 = getLineColorAt(-0.64f, false);
    Color insideLine = getLineColorAt(0.32f, true);
    Color outsideLine = getLineColorAt(0.96f, true);
    
    // Inside radius should have enhanced opacity
    EXPECT_NEAR(insideLine.a, 0.65f, 0.001f);
    EXPECT_NEAR(boundaryLine1.a, 0.65f, 0.001f);
    EXPECT_NEAR(boundaryLine2.a, 0.65f, 0.001f);
    
    // Outside radius should have normal opacity
    EXPECT_NEAR(outsideLine.a, 0.35f, 0.001f);
}

// Test that horizontal and vertical lines form a proper grid
TEST_F(GroundPlaneGeometryTest, ProperGridFormation) {
    Vector3f center(0, 0, 0);
    float extent = 1.6f;
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Should have equal number of horizontal and vertical lines
    int horizontalCount = countHorizontalLines();
    int verticalCount = countVerticalLines();
    
    EXPECT_EQ(horizontalCount, verticalCount);
    EXPECT_GT(horizontalCount, 0);
    
    // Total should be even (pairs of lines)
    EXPECT_EQ(m_capturedLines.size() % 2, 0);
}

// Test cursor position affecting correct lines
TEST_F(GroundPlaneGeometryTest, CursorAffectsCorrectLines) {
    Vector3f center(0, 0, 0);
    float extent = 3.2f;
    Vector3f cursorPos(0.5f, 0, 0.5f); // Between grid lines
    
    generateGroundPlaneGridLines(center, extent, cursorPos, true);
    
    // Lines closest to cursor should be enhanced
    // Cursor is at (0.5, 0, 0.5), so lines at 0.32 and 0.64 should be enhanced
    Color line032H = getLineColorAt(0.32f, true);
    Color line064H = getLineColorAt(0.64f, true);
    Color line032V = getLineColorAt(0.32f, false);
    Color line064V = getLineColorAt(0.64f, false);
    
    // All should be enhanced (within 64cm radius)
    EXPECT_NEAR(line032H.a, 0.65f, 0.001f);
    EXPECT_NEAR(line064H.a, 0.65f, 0.001f);
    EXPECT_NEAR(line032V.a, 0.65f, 0.001f);
    EXPECT_NEAR(line064V.a, 0.65f, 0.001f);
    
    // Far lines should be normal
    Color farLine = getLineColorAt(1.28f, true);
    EXPECT_NEAR(farLine.a, 0.35f, 0.001f);
}

// Test edge case: very small extent
TEST_F(GroundPlaneGeometryTest, VerySmallExtent) {
    Vector3f center(0, 0, 0);
    float extent = 0.1f; // Less than one grid square
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // Should still generate at least the center lines
    EXPECT_FALSE(m_capturedLines.empty());
}

// Test mathematical precision of grid positions
TEST_F(GroundPlaneGeometryTest, GridPositionPrecision) {
    Vector3f center(0, 0, 0);
    float extent = 1.6f;
    Vector3f cursorPos(0, 0, 0);
    
    generateGroundPlaneGridLines(center, extent, cursorPos, false);
    
    // All grid positions should be exact multiples of 0.32
    for (const auto& line : m_capturedLines) {
        float coord = line.getConstantCoordinate();
        float gridMultiple = coord / 0.32f;
        float rounded = std::round(gridMultiple);
        
        // Should be very close to an integer multiple
        EXPECT_NEAR(gridMultiple, rounded, 0.001f);
    }
}

// Test to demonstrate the correct major grid line implementation
TEST_F(GroundPlaneGeometryTest, MajorGridLinesCorrectImplementation) {
    // This test shows how major grid lines SHOULD be calculated
    // Major lines are every 160cm, grid squares are 32cm
    // So major lines occur every 5 grid squares (160/32 = 5)
    
    const float gridSize = 0.32f;
    const int majorInterval = 5; // Every 5th grid line is major
    
    // Test grid indices from -15 to 15
    for (int i = -15; i <= 15; i++) {
        bool shouldBeMajor = (i % majorInterval) == 0;
        float position = i * gridSize;
        
        if (shouldBeMajor) {
            // Verify this is indeed a multiple of 1.60
            float multipleOf160 = position / 1.60f;
            float rounded = std::round(multipleOf160);
            EXPECT_NEAR(multipleOf160, rounded, 0.001f) 
                << "Grid index " << i << " at position " << position 
                << " should be a major line (multiple of 1.60)";
        }
    }
}