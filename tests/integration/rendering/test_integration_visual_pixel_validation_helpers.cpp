#include <gtest/gtest.h>
#include "PixelValidationHelpers.h"
#include <algorithm>
#include <random>

using namespace VoxelEditor::Testing;

class PixelValidationHelpersTest : public ::testing::Test {
protected:
    // Create a test image with known properties
    std::vector<uint8_t> createTestImage(int width, int height, const Color& bgColor, const Color& fgColor, 
                                         float fgPercentage = 25.0f) {
        std::vector<uint8_t> pixels(width * height * 3);
        
        // Fill with background
        for (int i = 0; i < width * height; ++i) {
            pixels[i * 3] = bgColor.r;
            pixels[i * 3 + 1] = bgColor.g;
            pixels[i * 3 + 2] = bgColor.b;
        }
        
        // Draw a rectangle in the center for foreground
        int fgPixels = static_cast<int>(width * height * fgPercentage / 100.0f);
        int rectSize = static_cast<int>(std::sqrt(fgPixels));
        int startX = (width - rectSize) / 2;
        int startY = (height - rectSize) / 2;
        
        for (int y = startY; y < startY + rectSize && y < height; ++y) {
            for (int x = startX; x < startX + rectSize && x < width; ++x) {
                int idx = (y * width + x) * 3;
                pixels[idx] = fgColor.r;
                pixels[idx + 1] = fgColor.g;
                pixels[idx + 2] = fgColor.b;
            }
        }
        
        return pixels;
    }
    
    // Create gradient image for edge detection testing
    std::vector<uint8_t> createGradientImage(int width, int height) {
        std::vector<uint8_t> pixels(width * height * 3);
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                // Create gradient from 0 to 254 (not 255 to avoid edge case)
                uint8_t value = static_cast<uint8_t>((x * 254) / (width - 1));
                pixels[idx] = value;
                pixels[idx + 1] = value;
                pixels[idx + 2] = value;
            }
        }
        
        return pixels;
    }
};

TEST_F(PixelValidationHelpersTest, ColorDistributionAnalysis) {
    // Create 100x100 image with 25% red foreground on black background
    Color bgColor(0, 0, 0);
    Color fgColor(255, 0, 0);
    auto pixels = createTestImage(100, 100, bgColor, fgColor, 25.0f);
    
    auto result = PixelValidationHelpers::analyzeColorDistribution(pixels, 100, 100, bgColor);
    
    // Check results
    EXPECT_EQ(result.totalPixels, 10000);
    EXPECT_NEAR(result.foregroundPercentage, 25.0f, 1.0f); // Allow 1% tolerance
    EXPECT_NEAR(result.backgroundPercentage, 75.0f, 1.0f);
    EXPECT_EQ(result.colorHistogram.size(), 2); // Only black and red
    
    // Check specific color percentages
    EXPECT_NEAR(result.getColorPercentage(bgColor), 75.0f, 1.0f);
    EXPECT_NEAR(result.getColorPercentage(fgColor), 25.0f, 1.0f);
}

TEST_F(PixelValidationHelpersTest, EdgeDetection) {
    // Create image with sharp edge (half black, half white)
    std::vector<uint8_t> pixels(100 * 100 * 3);
    for (int y = 0; y < 100; ++y) {
        for (int x = 0; x < 100; ++x) {
            int idx = (y * 100 + x) * 3;
            uint8_t value = (x < 50) ? 0 : 255;
            pixels[idx] = value;
            pixels[idx + 1] = value;
            pixels[idx + 2] = value;
        }
    }
    
    auto result = PixelValidationHelpers::detectEdges(pixels, 100, 100);
    
    // Should detect vertical edge at x=50
    EXPECT_GT(result.edgePixelCount, 50); // At least height of the edge
    EXPECT_TRUE(result.hasDistinctEdges);
    
    // Check that edge coordinates are around x=50
    int edgesNearCenter = 0;
    for (const auto& coord : result.edgeCoordinates) {
        if (std::abs(coord.first - 50) <= 2) {
            edgesNearCenter++;
        }
    }
    EXPECT_GT(edgesNearCenter, 50); // Most edges should be near the center line
}

TEST_F(PixelValidationHelpersTest, BrightnessAnalysis) {
    // Create image with gradient from dark to bright
    auto pixels = createGradientImage(100, 100);
    
    auto result = PixelValidationHelpers::analyzeBrightness(pixels, 100, 100, false);
    
    // Check brightness properties
    EXPECT_NEAR(result.averageBrightness, 127.5f, 5.0f); // Middle of 0-255
    EXPECT_NEAR(result.minBrightness, 0.0f, 1.0f);
    EXPECT_NEAR(result.maxBrightness, 254.0f, 5.0f); // Max is 254 due to gradient calculation
    EXPECT_GT(result.brightnessVariance, 1000.0f); // Should have high variance
    EXPECT_TRUE(result.hasLightingVariation()); // Should detect variation
    
    // Check histogram has values across range
    int nonZeroBins = 0;
    for (float count : result.brightnessHistogram) {
        if (count > 0) nonZeroBins++;
    }
    EXPECT_GT(nonZeroBins, 90); // Gradient creates ~100 different brightness levels
}

TEST_F(PixelValidationHelpersTest, BrightnessAnalysisIgnoreBackground) {
    // Create image with black background and bright square
    Color bgColor(0, 0, 0);
    Color fgColor(200, 200, 200);
    auto pixels = createTestImage(100, 100, bgColor, fgColor, 25.0f);
    
    // Analyze with background ignored
    auto result = PixelValidationHelpers::analyzeBrightness(pixels, 100, 100, true);
    
    // Should only analyze the bright square
    EXPECT_NEAR(result.averageBrightness, 200.0f, 5.0f);
    EXPECT_NEAR(result.minBrightness, 200.0f, 5.0f);
    EXPECT_NEAR(result.maxBrightness, 200.0f, 5.0f);
    EXPECT_LT(result.brightnessVariance, 1.0f); // Should have low variance (all same color)
}

TEST_F(PixelValidationHelpersTest, ColorAccuracyValidation) {
    // Create expected and actual images with slight differences
    Color color1(100, 150, 200);
    Color color2(102, 148, 203); // Slightly different
    
    auto expected = createTestImage(50, 50, Color(0, 0, 0), color1, 100.0f);
    auto actual = createTestImage(50, 50, Color(0, 0, 0), color2, 100.0f);
    
    auto result = PixelValidationHelpers::validateColorAccuracy(
        actual, expected, 50, 50, 5, false);
    
    // Should be accurate within threshold
    EXPECT_TRUE(result.isAccurate(5.0f));
    EXPECT_LT(result.averageError, 5.0f);
    EXPECT_GT(result.accuracyPercentage, 95.0f);
    EXPECT_EQ(result.totalPixels, 2500);
}

TEST_F(PixelValidationHelpersTest, ColorAccuracyFailure) {
    // Create expected and actual with large differences
    Color color1(100, 150, 200);
    Color color2(200, 50, 100); // Very different
    
    auto expected = createTestImage(50, 50, Color(0, 0, 0), color1, 100.0f);
    auto actual = createTestImage(50, 50, Color(0, 0, 0), color2, 100.0f);
    
    auto result = PixelValidationHelpers::validateColorAccuracy(
        actual, expected, 50, 50, 5, false);
    
    // Should fail accuracy test
    EXPECT_FALSE(result.isAccurate(5.0f));
    EXPECT_GT(result.averageError, 50.0f);
    EXPECT_LT(result.accuracyPercentage, 10.0f);
}

TEST_F(PixelValidationHelpersTest, RegionValidation) {
    // Create image with specific colored region
    std::vector<uint8_t> pixels(100 * 100 * 3, 0); // All black
    
    // Draw red square at (10, 10) with size 20x20
    Color red(255, 0, 0);
    for (int y = 10; y < 30; ++y) {
        for (int x = 10; x < 30; ++x) {
            int idx = (y * 100 + x) * 3;
            pixels[idx] = red.r;
            pixels[idx + 1] = red.g;
            pixels[idx + 2] = red.b;
        }
    }
    
    // Validate the red region
    EXPECT_TRUE(PixelValidationHelpers::validateRegion(
        pixels, 100, 10, 10, 20, 20, red, 95.0f));
    
    // Validate a region that should be black
    EXPECT_TRUE(PixelValidationHelpers::validateRegion(
        pixels, 100, 50, 50, 20, 20, Color(0, 0, 0), 95.0f));
    
    // Validate a partially covered region should fail
    EXPECT_FALSE(PixelValidationHelpers::validateRegion(
        pixels, 100, 20, 20, 20, 20, red, 95.0f));
}

TEST_F(PixelValidationHelpersTest, DebugReportGeneration) {
    // Create test image
    Color bgColor(0, 0, 0);
    Color fgColor(255, 128, 64);
    auto pixels = createTestImage(100, 100, bgColor, fgColor, 30.0f);
    
    // Run all analyses
    auto colorDist = PixelValidationHelpers::analyzeColorDistribution(pixels, 100, 100, bgColor);
    auto edges = PixelValidationHelpers::detectEdges(pixels, 100, 100);
    auto brightness = PixelValidationHelpers::analyzeBrightness(pixels, 100, 100, true);
    
    // Generate report
    std::string report = PixelValidationHelpers::generateDebugReport(colorDist, edges, brightness);
    
    // Check report contains expected sections
    EXPECT_NE(report.find("Color Distribution:"), std::string::npos);
    EXPECT_NE(report.find("Edge Detection:"), std::string::npos);
    EXPECT_NE(report.find("Brightness Analysis:"), std::string::npos);
    EXPECT_NE(report.find("Background:"), std::string::npos);
    EXPECT_NE(report.find("Foreground:"), std::string::npos);
    EXPECT_NE(report.find("Has lighting variation:"), std::string::npos);
}

TEST_F(PixelValidationHelpersTest, ColorMethods) {
    Color color(100, 150, 200);
    
    // Test brightness calculation
    EXPECT_EQ(color.brightness(), 150); // (100+150+200)/3 = 150
    
    // Test luminance calculation
    float expectedLuminance = 0.299f * 100 + 0.587f * 150 + 0.114f * 200;
    EXPECT_NEAR(color.luminance(), expectedLuminance, 0.1f);
    
    // Test threshold comparison
    Color similar(102, 148, 197);
    Color different(200, 50, 100);
    
    EXPECT_TRUE(color.isWithinThreshold(similar, 5));
    EXPECT_FALSE(color.isWithinThreshold(different, 5));
    EXPECT_TRUE(color.isWithinThreshold(different, 150));
}

// Test with realistic shader output simulation
TEST_F(PixelValidationHelpersTest, RealisticShaderOutput) {
    // Simulate a lit cube on dark background
    std::vector<uint8_t> pixels(200 * 200 * 3, 0); // Black background
    
    // Draw a "cube" with different face brightnesses (simulating lighting)
    // Front face (brightest)
    for (int y = 70; y < 130; ++y) {
        for (int x = 70; x < 130; ++x) {
            int idx = (y * 200 + x) * 3;
            pixels[idx] = 200;     // R
            pixels[idx + 1] = 100; // G
            pixels[idx + 2] = 100; // B
        }
    }
    
    // Top face (medium brightness) - offset to simulate 3D
    for (int y = 50; y < 70; ++y) {
        for (int x = 80; x < 140; ++x) {
            int idx = (y * 200 + x) * 3;
            pixels[idx] = 150;     // R
            pixels[idx + 1] = 75;  // G
            pixels[idx + 2] = 75;  // B
        }
    }
    
    // Analyze the rendered cube
    auto colorDist = PixelValidationHelpers::analyzeColorDistribution(pixels, 200, 200);
    auto edges = PixelValidationHelpers::detectEdges(pixels, 200, 200);
    auto brightness = PixelValidationHelpers::analyzeBrightness(pixels, 200, 200, true);
    
    // Validate realistic rendering properties
    EXPECT_GT(colorDist.foregroundPercentage, 5.0f);  // Cube visible
    EXPECT_LT(colorDist.foregroundPercentage, 20.0f); // Not filling screen
    EXPECT_TRUE(edges.hasDistinctEdges);              // Cube edges detected
    EXPECT_TRUE(brightness.hasLightingVariation());    // Different face brightnesses
    EXPECT_GT(brightness.maxBrightness, brightness.minBrightness + 20); // Clear lighting difference
}