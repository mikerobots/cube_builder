#pragma once

#include <vector>
#include <cstdint>
#include <algorithm>
#include <cmath>
#include <numeric>
#include <string>
#include <unordered_map>

namespace VoxelEditor {
namespace Testing {

/**
 * RGB Color structure for pixel analysis
 */
struct Color {
    uint8_t r, g, b;
    
    Color() : r(0), g(0), b(0) {}
    Color(uint8_t red, uint8_t green, uint8_t blue) : r(red), g(green), b(blue) {}
    
    bool operator==(const Color& other) const {
        return r == other.r && g == other.g && b == other.b;
    }
    
    // Calculate brightness (0-255)
    uint8_t brightness() const {
        return static_cast<uint8_t>((r + g + b) / 3);
    }
    
    // Calculate luminance using perceptual weights
    float luminance() const {
        return 0.299f * r + 0.587f * g + 0.114f * b;
    }
    
    // Check if color is within threshold of another color
    bool isWithinThreshold(const Color& other, uint8_t threshold) const {
        return std::abs(r - other.r) <= threshold &&
               std::abs(g - other.g) <= threshold &&
               std::abs(b - other.b) <= threshold;
    }
};

/**
 * Result structure for color distribution analysis
 */
struct ColorDistribution {
    int totalPixels;
    int backgroundPixels;
    int foregroundPixels;
    float backgroundPercentage;
    float foregroundPercentage;
    std::unordered_map<uint32_t, int> colorHistogram;
    
    // Helper to get percentage of specific color
    float getColorPercentage(const Color& color) const {
        uint32_t key = (color.r << 16) | (color.g << 8) | color.b;
        auto it = colorHistogram.find(key);
        if (it != colorHistogram.end()) {
            return (float)it->second / totalPixels * 100.0f;
        }
        return 0.0f;
    }
};

/**
 * Result structure for edge detection
 */
struct EdgeDetectionResult {
    int edgePixelCount;
    float edgePixelPercentage;
    std::vector<std::pair<int, int>> edgeCoordinates;
    bool hasDistinctEdges;
};

/**
 * Result structure for brightness analysis
 */
struct BrightnessAnalysis {
    float averageBrightness;
    float minBrightness;
    float maxBrightness;
    float brightnessVariance;
    std::vector<float> brightnessHistogram; // 256 bins
    
    // Check if lighting appears to be applied
    bool hasLightingVariation(float minVariationThreshold = 10.0f) const {
        return brightnessVariance > minVariationThreshold;
    }
};

/**
 * Result structure for color accuracy validation
 */
struct ColorAccuracyResult {
    float averageError;      // Average RGB distance from expected
    float maxError;          // Maximum RGB distance from expected
    int accuratePixels;      // Pixels within threshold
    int totalPixels;
    float accuracyPercentage;
    
    bool isAccurate(float acceptableErrorThreshold = 5.0f) const {
        return averageError <= acceptableErrorThreshold;
    }
};

/**
 * Pixel validation helper functions for shader visual testing
 */
class PixelValidationHelpers {
public:
    /**
     * Analyze color distribution in captured framebuffer
     * @param pixels RGB pixel data (width * height * 3 bytes)
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @param backgroundColor Expected background color
     * @param backgroundThreshold Threshold for considering a pixel as background
     */
    static ColorDistribution analyzeColorDistribution(
        const std::vector<uint8_t>& pixels,
        int width,
        int height,
        const Color& backgroundColor = Color(0, 0, 0),
        uint8_t backgroundThreshold = 10) {
        
        ColorDistribution result;
        result.totalPixels = width * height;
        result.backgroundPixels = 0;
        result.foregroundPixels = 0;
        
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                int idx = (y * width + x) * 3;
                Color pixel(pixels[idx], pixels[idx + 1], pixels[idx + 2]);
                
                // Add to histogram
                uint32_t colorKey = (pixel.r << 16) | (pixel.g << 8) | pixel.b;
                result.colorHistogram[colorKey]++;
                
                // Check if background
                if (pixel.isWithinThreshold(backgroundColor, backgroundThreshold)) {
                    result.backgroundPixels++;
                } else {
                    result.foregroundPixels++;
                }
            }
        }
        
        result.backgroundPercentage = (float)result.backgroundPixels / result.totalPixels * 100.0f;
        result.foregroundPercentage = (float)result.foregroundPixels / result.totalPixels * 100.0f;
        
        return result;
    }
    
    /**
     * Detect edges in the rendered image using simple gradient detection
     * @param pixels RGB pixel data
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @param edgeThreshold Minimum gradient to consider as edge
     */
    static EdgeDetectionResult detectEdges(
        const std::vector<uint8_t>& pixels,
        int width,
        int height,
        float edgeThreshold = 30.0f) {
        
        EdgeDetectionResult result;
        result.edgePixelCount = 0;
        result.hasDistinctEdges = false;
        
        // Simple Sobel edge detection
        for (int y = 1; y < height - 1; ++y) {
            for (int x = 1; x < width - 1; ++x) {
                // Calculate gradients
                float gx = 0, gy = 0;
                
                // Sobel X kernel: [-1 0 1; -2 0 2; -1 0 1]
                // Sobel Y kernel: [-1 -2 -1; 0 0 0; 1 2 1]
                for (int dy = -1; dy <= 1; ++dy) {
                    for (int dx = -1; dx <= 1; ++dx) {
                        int idx = ((y + dy) * width + (x + dx)) * 3;
                        float luminance = 0.299f * pixels[idx] + 
                                        0.587f * pixels[idx + 1] + 
                                        0.114f * pixels[idx + 2];
                        
                        // Sobel X
                        if (dx != 0) {
                            gx += luminance * dx * (dy == 0 ? 2 : 1);
                        }
                        
                        // Sobel Y
                        if (dy != 0) {
                            gy += luminance * dy * (dx == 0 ? 2 : 1);
                        }
                    }
                }
                
                float gradient = std::sqrt(gx * gx + gy * gy);
                if (gradient > edgeThreshold) {
                    result.edgePixelCount++;
                    result.edgeCoordinates.push_back({x, y});
                }
            }
        }
        
        result.edgePixelPercentage = (float)result.edgePixelCount / (width * height) * 100.0f;
        result.hasDistinctEdges = result.edgePixelCount > (width * height * 0.001f); // At least 0.1% edge pixels
        
        return result;
    }
    
    /**
     * Analyze brightness distribution to validate lighting
     * @param pixels RGB pixel data
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @param ignoreBackground If true, skip black pixels in analysis
     */
    static BrightnessAnalysis analyzeBrightness(
        const std::vector<uint8_t>& pixels,
        int width,
        int height,
        bool ignoreBackground = true) {
        
        BrightnessAnalysis result;
        result.brightnessHistogram.resize(256, 0);
        
        std::vector<float> brightnessValues;
        float totalBrightness = 0;
        result.minBrightness = 255.0f;
        result.maxBrightness = 0.0f;
        
        for (int i = 0; i < width * height; ++i) {
            int idx = i * 3;
            Color pixel(pixels[idx], pixels[idx + 1], pixels[idx + 2]);
            
            // Skip background pixels if requested
            if (ignoreBackground && pixel.brightness() < 5) {
                continue;
            }
            
            float brightness = pixel.luminance();
            brightnessValues.push_back(brightness);
            totalBrightness += brightness;
            
            result.minBrightness = std::min(result.minBrightness, brightness);
            result.maxBrightness = std::max(result.maxBrightness, brightness);
            
            int bin = std::min(255, (int)brightness);
            result.brightnessHistogram[bin]++;
        }
        
        if (!brightnessValues.empty()) {
            result.averageBrightness = totalBrightness / brightnessValues.size();
            
            // Calculate variance
            float variance = 0;
            for (float brightness : brightnessValues) {
                float diff = brightness - result.averageBrightness;
                variance += diff * diff;
            }
            result.brightnessVariance = variance / brightnessValues.size();
        } else {
            result.averageBrightness = 0;
            result.brightnessVariance = 0;
        }
        
        return result;
    }
    
    /**
     * Validate color accuracy against expected colors
     * @param pixels RGB pixel data
     * @param expectedPixels Expected RGB pixel data
     * @param width Framebuffer width
     * @param height Framebuffer height
     * @param accuracyThreshold Maximum allowed error per pixel
     * @param ignoreBackground If true, skip comparison for black pixels
     */
    static ColorAccuracyResult validateColorAccuracy(
        const std::vector<uint8_t>& pixels,
        const std::vector<uint8_t>& expectedPixels,
        int width,
        int height,
        uint8_t accuracyThreshold = 5,
        bool ignoreBackground = true) {
        
        ColorAccuracyResult result;
        result.averageError = 0;
        result.maxError = 0;
        result.accuratePixels = 0;
        result.totalPixels = 0;
        
        float totalError = 0;
        
        for (int i = 0; i < width * height; ++i) {
            int idx = i * 3;
            Color actual(pixels[idx], pixels[idx + 1], pixels[idx + 2]);
            Color expected(expectedPixels[idx], expectedPixels[idx + 1], expectedPixels[idx + 2]);
            
            // Skip background pixels if requested
            if (ignoreBackground && expected.brightness() < 5) {
                continue;
            }
            
            result.totalPixels++;
            
            // Calculate RGB distance
            float error = std::sqrt(
                std::pow(actual.r - expected.r, 2) +
                std::pow(actual.g - expected.g, 2) +
                std::pow(actual.b - expected.b, 2)
            );
            
            totalError += error;
            result.maxError = std::max(result.maxError, error);
            
            if (error <= accuracyThreshold) {
                result.accuratePixels++;
            }
        }
        
        if (result.totalPixels > 0) {
            result.averageError = totalError / result.totalPixels;
            result.accuracyPercentage = (float)result.accuratePixels / result.totalPixels * 100.0f;
        }
        
        return result;
    }
    
    /**
     * Check if a rectangular region contains expected content
     * @param pixels RGB pixel data
     * @param width Framebuffer width
     * @param x, y, w, h Region to check
     * @param expectedColor Expected color in the region
     * @param coverageThreshold Minimum percentage of pixels that should match
     */
    static bool validateRegion(
        const std::vector<uint8_t>& pixels,
        int width,
        int x, int y, int w, int h,
        const Color& expectedColor,
        float coverageThreshold = 90.0f,
        uint8_t colorThreshold = 10) {
        
        int matchingPixels = 0;
        int totalPixels = w * h;
        
        for (int dy = 0; dy < h; ++dy) {
            for (int dx = 0; dx < w; ++dx) {
                int px = x + dx;
                int py = y + dy;
                int idx = (py * width + px) * 3;
                
                Color pixel(pixels[idx], pixels[idx + 1], pixels[idx + 2]);
                if (pixel.isWithinThreshold(expectedColor, colorThreshold)) {
                    matchingPixels++;
                }
            }
        }
        
        float coverage = (float)matchingPixels / totalPixels * 100.0f;
        return coverage >= coverageThreshold;
    }
    
    /**
     * Generate a debug string describing the validation results
     */
    static std::string generateDebugReport(
        const ColorDistribution& colorDist,
        const EdgeDetectionResult& edges,
        const BrightnessAnalysis& brightness) {
        
        std::string report = "=== Pixel Validation Report ===\n";
        
        // Color distribution
        report += "Color Distribution:\n";
        report += "  Background: " + std::to_string(colorDist.backgroundPercentage) + "%\n";
        report += "  Foreground: " + std::to_string(colorDist.foregroundPercentage) + "%\n";
        report += "  Unique colors: " + std::to_string(colorDist.colorHistogram.size()) + "\n";
        
        // Edge detection
        report += "\nEdge Detection:\n";
        report += "  Edge pixels: " + std::to_string(edges.edgePixelCount) + 
                  " (" + std::to_string(edges.edgePixelPercentage) + "%)\n";
        report += "  Has distinct edges: " + std::string(edges.hasDistinctEdges ? "Yes" : "No") + "\n";
        
        // Brightness analysis
        report += "\nBrightness Analysis:\n";
        report += "  Average: " + std::to_string(brightness.averageBrightness) + "\n";
        report += "  Min: " + std::to_string(brightness.minBrightness) + "\n";
        report += "  Max: " + std::to_string(brightness.maxBrightness) + "\n";
        report += "  Variance: " + std::to_string(brightness.brightnessVariance) + "\n";
        report += "  Has lighting variation: " + 
                  std::string(brightness.hasLightingVariation() ? "Yes" : "No") + "\n";
        
        return report;
    }
};

} // namespace Testing
} // namespace VoxelEditor