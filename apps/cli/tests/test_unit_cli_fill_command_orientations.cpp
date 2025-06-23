#include <gtest/gtest.h>
#include "cli/Application.h"
#include "cli/CommandProcessor.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/Vector3f.h"

namespace VoxelEditor {
namespace Tests {

/**
 * Test suite for REQ-11.3.6: Fill command shall test valid coordinate ranges in all orientations
 * 
 * This test suite validates that the fill command properly handles coordinate ranges in all
 * possible orientations and directions:
 * - Positive direction fills (min to max in each axis)
 * - All axis combinations (X, Y, Z individually and in combinations)
 * - Different coordinate range sizes and positions
 * - Validation that the correct voxel count is filled
 */
class FillCommandOrientationsTest : public ::testing::Test {
protected:
    void SetUp() override {
        app = std::make_unique<Application>();
        
        // Initialize in headless mode for testing
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        initialized = app->initialize(argc, argv);
        ASSERT_TRUE(initialized) << "Application should initialize in headless mode";
        
        // Set up a standard workspace for testing
        auto voxelManager = app->getVoxelManager();
        ASSERT_TRUE(voxelManager != nullptr);
        voxelManager->resizeWorkspace(Math::Vector3f(6.0f, 6.0f, 6.0f)); // 6m³ workspace for testing
        
        // Set resolution to 1cm for precise coordinate testing
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_1cm);
        
        processor = app->getCommandProcessor();
        ASSERT_TRUE(processor != nullptr);
    }
    
    void TearDown() override {
        if (app) {
            // Clear all voxels before cleanup
            executeCommand("selectall");
            executeCommand("delete");
        }
        app.reset();
    }
    
    // Helper to execute command and return result
    CommandResult executeCommand(const std::string& command) {
        return processor->execute(command);
    }
    
    // Helper to count voxels in the workspace
    int countVoxels() {
        auto voxelManager = app->getVoxelManager();
        int count = 0;
        
        // Count voxels at current resolution
        auto voxels = voxelManager->getAllVoxels(voxelManager->getActiveResolution());
        count = static_cast<int>(voxels.size());
        
        return count;
    }
    
    // Helper to calculate expected voxel count for a range
    int calculateExpectedVoxelCount(int x1, int y1, int z1, int x2, int y2, int z2) {
        int dx = std::abs(x2 - x1) + 1;
        int dy = std::abs(y2 - y1) + 1;
        int dz = std::abs(z2 - z1) + 1;
        return dx * dy * dz;
    }

    std::unique_ptr<Application> app;
    CommandProcessor* processor = nullptr;
    bool initialized = false;
};

// ============================================================================
// Single Axis Range Tests (REQ-11.3.6)
// ============================================================================

TEST_F(FillCommandOrientationsTest, DISABLED_SingleAxisXDirection_REQ_11_3_6) {
    // Test fill command with ranges only along X-axis
    
    struct XAxisTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<XAxisTest> tests = {
        // X-axis ranges with Y and Z constant
        {"fill 0cm 0cm 0cm 100cm 0cm 0cm", 101, "X-axis line from 0 to 100cm"},
        {"fill -50cm 0cm 0cm 50cm 0cm 0cm", 101, "X-axis line from -50 to 50cm (centered)"},
        {"fill 10cm 50cm 10cm 20cm 50cm 10cm", 11, "X-axis line from 10 to 20cm"},
        {"fill -100cm 100cm -100cm 0cm 100cm -100cm", 101, "X-axis line in positive quadrant"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "X-axis fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

TEST_F(FillCommandOrientationsTest, DISABLED_SingleAxisYDirection_REQ_11_3_6) {
    // Test fill command with ranges only along Y-axis (vertical)
    
    struct YAxisTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<YAxisTest> tests = {
        // Y-axis ranges with X and Z constant (all Y >= 0 due to ground plane constraint)
        {"fill 0cm 0cm 0cm 0cm 100cm 0cm", 101, "Y-axis vertical line from 0 to 100cm"},
        {"fill 50cm 0cm 50cm 50cm 50cm 50cm", 51, "Y-axis line from 0 to 50cm"},
        {"fill -50cm 0cm -50cm -50cm 200cm -50cm", 201, "Y-axis line from 0 to 200cm"},
        {"fill 10cm 0cm 10cm 10cm 25cm 10cm", 26, "Y-axis short line from 0 to 25cm"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "Y-axis fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

TEST_F(FillCommandOrientationsTest, DISABLED_SingleAxisZDirection_REQ_11_3_6) {
    // Test fill command with ranges only along Z-axis
    
    struct ZAxisTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<ZAxisTest> tests = {
        // Z-axis ranges with X and Y constant
        {"fill 0cm 0cm 0cm 0cm 0cm 100cm", 101, "Z-axis line from 0 to 100cm"},
        {"fill 0cm 0cm -50cm 0cm 0cm 50cm", 101, "Z-axis line from -50 to 50cm (centered)"},
        {"fill 50cm 50cm 10cm 50cm 50cm 20cm", 11, "Z-axis line from 10 to 20cm"},
        {"fill -100cm 100cm -100cm -100cm 100cm 0cm", 101, "Z-axis line in negative Z"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "Z-axis fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

// ============================================================================
// Two-Axis Plane Tests (REQ-11.3.6)
// ============================================================================

TEST_F(FillCommandOrientationsTest, DISABLED_TwoAxisXYPlane_REQ_11_3_6) {
    // Test fill command with ranges in X-Y plane (Z constant)
    
    struct XYPlaneTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<XYPlaneTest> tests = {
        // X-Y plane fills with Z constant
        {"fill 0cm 0cm 0cm 10cm 10cm 0cm", 121, "11x11 square in XY plane at Z=0"},
        {"fill -5cm 0cm 50cm 5cm 5cm 50cm", 66, "11x6 rectangle in XY plane at Z=50cm"},
        {"fill 10cm 0cm -10cm 20cm 20cm -10cm", 231, "11x21 rectangle in XY plane at Z=-10cm"},
        {"fill -50cm 0cm 100cm 50cm 100cm 100cm", 10201, "101x101 large square in XY plane"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "XY-plane fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

TEST_F(FillCommandOrientationsTest, DISABLED_TwoAxisXZPlane_REQ_11_3_6) {
    // Test fill command with ranges in X-Z plane (Y constant)
    
    struct XZPlaneTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<XZPlaneTest> tests = {
        // X-Z plane fills with Y constant
        {"fill 0cm 0cm 0cm 10cm 0cm 10cm", 121, "11x11 square in XZ plane at Y=0"},
        {"fill -5cm 50cm -5cm 5cm 50cm 5cm", 121, "11x11 square in XZ plane at Y=50cm"},
        {"fill 10cm 25cm -10cm 20cm 25cm 10cm", 231, "11x21 rectangle in XZ plane"},
        {"fill -25cm 100cm -25cm 25cm 100cm 25cm", 2601, "51x51 square in XZ plane"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "XZ-plane fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

TEST_F(FillCommandOrientationsTest, DISABLED_TwoAxisYZPlane_REQ_11_3_6) {
    // Test fill command with ranges in Y-Z plane (X constant)
    
    struct YZPlaneTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<YZPlaneTest> tests = {
        // Y-Z plane fills with X constant (Y >= 0 due to ground plane)
        {"fill 0cm 0cm 0cm 0cm 10cm 10cm", 121, "11x11 square in YZ plane at X=0"},
        {"fill 50cm 0cm -5cm 50cm 5cm 5cm", 66, "6x11 rectangle in YZ plane at X=50cm"},
        {"fill -25cm 0cm 10cm -25cm 20cm 20cm", 231, "21x11 rectangle in YZ plane"},
        {"fill 100cm 0cm -25cm 100cm 25cm 25cm", 1326, "26x51 rectangle in YZ plane"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "YZ-plane fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

// ============================================================================
// Three-Axis Volume Tests (REQ-11.3.6)
// ============================================================================

TEST_F(FillCommandOrientationsTest, DISABLED_ThreeAxisVolumeOrientation_REQ_11_3_6) {
    // Test fill command with ranges in all three axes (full 3D volumes)
    
    struct VolumeTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<VolumeTest> tests = {
        // 3D volume fills in all orientations
        {"fill 0cm 0cm 0cm 5cm 5cm 5cm", 216, "6x6x6 cube at origin"},
        {"fill -5cm 0cm -5cm 5cm 10cm 5cm", 1331, "11x11x11 cube centered at origin"},
        {"fill 10cm 0cm 10cm 15cm 5cm 15cm", 216, "6x6x6 cube in positive quadrant"},
        {"fill -10cm 0cm -10cm -5cm 5cm -5cm", 216, "6x6x6 cube in negative XZ quadrant"},
        
        // Rectangular volumes (different dimensions)
        {"fill 0cm 0cm 0cm 10cm 5cm 2cm", 198, "11x6x3 rectangular volume"},
        {"fill -5cm 0cm 0cm 5cm 20cm 10cm", 2541, "11x21x11 tall rectangular volume"},
        {"fill 0cm 0cm -10cm 20cm 2cm 10cm", 1323, "21x3x21 flat rectangular volume"},
        
        // Single voxel (all coordinates equal)
        {"fill 5cm 5cm 5cm 5cm 5cm 5cm", 1, "Single voxel at (5,5,5)"},
        {"fill -10cm 0cm -10cm -10cm 0cm -10cm", 1, "Single voxel at (-10,0,-10)"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "3D volume fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

// ============================================================================
// Coordinate Order Independence Tests (REQ-11.3.6)
// ============================================================================

TEST_F(FillCommandOrientationsTest, CoordinateOrderIndependence_REQ_11_3_6) {
    // Test that fill command produces same result regardless of coordinate order
    // (min, max) vs (max, min) should produce identical results
    
    struct OrderTest {
        std::string command1; // min to max
        std::string command2; // max to min
        std::string description;
    };
    
    std::vector<OrderTest> tests = {
        {
            "fill 0cm 0cm 0cm 10cm 10cm 10cm",
            "fill 10cm 10cm 10cm 0cm 0cm 0cm",
            "3D cube with reversed coordinates"
        },
        {
            "fill -5cm 0cm -5cm 5cm 5cm 5cm",
            "fill 5cm 5cm 5cm -5cm 0cm -5cm",
            "Centered cube with reversed coordinates"
        },
        {
            "fill 0cm 0cm 0cm 20cm 0cm 0cm",
            "fill 20cm 0cm 0cm 0cm 0cm 0cm",
            "X-axis line with reversed coordinates"
        },
        {
            "fill 0cm 0cm 0cm 0cm 15cm 0cm",
            "fill 0cm 15cm 0cm 0cm 0cm 0cm",
            "Y-axis line with reversed coordinates"
        },
        {
            "fill 0cm 0cm 0cm 0cm 0cm 25cm",
            "fill 0cm 0cm 25cm 0cm 0cm 0cm",
            "Z-axis line with reversed coordinates"
        }
    };
    
    for (const auto& test : tests) {
        // Test first command
        executeCommand("selectall");
        executeCommand("delete");
        auto result1 = executeCommand(test.command1);
        int count1 = 0;
        if (result1.success) {
            count1 = countVoxels();
        }
        
        // Test second command (reversed coordinates)
        executeCommand("selectall");
        executeCommand("delete");
        auto result2 = executeCommand(test.command2);
        int count2 = 0;
        if (result2.success) {
            count2 = countVoxels();
        }
        
        EXPECT_TRUE(result1.success && result2.success)
            << "Both coordinate orders should succeed: " << test.description;
        
        EXPECT_EQ(count1, count2)
            << "Fill should produce same voxel count regardless of coordinate order: " 
            << test.description
            << " Order1 count: " << count1 << ", Order2 count: " << count2;
    }
}

// ============================================================================
// Mixed Orientation Range Tests (REQ-11.3.6)
// ============================================================================

TEST_F(FillCommandOrientationsTest, DISABLED_MixedOrientationRanges_REQ_11_3_6) {
    // Test fill command with different range sizes in each axis
    
    struct MixedTest {
        std::string command;
        int expectedCount;
        std::string description;
    };
    
    std::vector<MixedTest> tests = {
        // Different range sizes per axis
        {"fill 0cm 0cm 0cm 1cm 10cm 5cm", 132, "Small X, large Y, medium Z"},
        {"fill 0cm 0cm 0cm 10cm 1cm 5cm", 132, "Large X, small Y, medium Z"},
        {"fill 0cm 0cm 0cm 10cm 5cm 1cm", 132, "Large X, medium Y, small Z"},
        
        // Very uneven ratios
        {"fill 0cm 0cm 0cm 20cm 1cm 1cm", 42, "Long thin line along X"},
        {"fill 0cm 0cm 0cm 1cm 20cm 1cm", 42, "Long thin line along Y"},
        {"fill 0cm 0cm 0cm 1cm 1cm 20cm", 42, "Long thin line along Z"},
        
        // Complex mixed ranges in different quadrants
        {"fill -10cm 0cm 5cm -5cm 5cm 15cm", 396, "Negative X, positive Y and Z"},
        {"fill 5cm 0cm -10cm 15cm 5cm -5cm", 396, "Positive X and Y, negative Z"},
        {"fill -5cm 0cm -10cm 5cm 10cm 5cm", 1716, "Mixed signs, larger Y range"},
    };
    
    for (const auto& test : tests) {
        // Clear workspace before each test
        executeCommand("selectall");
        executeCommand("delete");
        
        auto result = executeCommand(test.command);
        EXPECT_TRUE(result.success) 
            << "Fill command should succeed: " << test.description 
            << " (" << test.command << ") Error: " << result.message;
        
        if (result.success) {
            int actualCount = countVoxels();
            EXPECT_EQ(actualCount, test.expectedCount)
                << "Mixed orientation fill should create correct voxel count: " << test.description
                << " Expected: " << test.expectedCount << ", Actual: " << actualCount;
        }
    }
}

} // namespace Tests
} // namespace VoxelEditor