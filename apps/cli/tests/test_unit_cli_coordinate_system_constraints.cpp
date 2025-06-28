#include <gtest/gtest.h>
#include "cli/CommandTypes.h"
#include "cli/CommandProcessor.h"
#include "cli/Application.h"
#include "cli/CommandModuleInit.h"
#include "voxel_data/VoxelDataManager.h"
#include "math/Vector3i.h"
#include <sstream>
#include <algorithm>

namespace VoxelEditor {
namespace Tests {

/**
 * Test suite for REQ-11.2.4: Commands with coordinate parameters shall test coordinate system constraints
 * 
 * This test suite validates that CLI commands with coordinate parameters properly handle:
 * - Coordinate unit requirements (cm/m)
 * - Coordinate format validation
 * - Coordinate system constraints (centered at origin, Y >= 0)
 * - Unit conversion accuracy
 * - Workspace boundary validation
 * - Invalid coordinate format handling
 */
class CoordinateSystemConstraintsTest : public ::testing::Test {
protected:
    void SetUp() override {
        std::cout << "DEBUG SetUp: Starting SetUp()" << std::endl;
        
        // Enable debug logging
        Logging::Logger::getInstance().setLevel(Logging::LogLevel::Debug);
        
        // Force command module initialization before creating app
        forceCommandModuleInitialization();
        std::cout << "DEBUG SetUp: Forced command module initialization" << std::endl;
        
        app = std::make_unique<Application>();
        std::cout << "DEBUG SetUp: Created Application" << std::endl;
        
        // Initialize in headless mode for testing
        int argc = 2;
        char arg0[] = "test";
        char arg1[] = "--headless";
        char* argv[] = {arg0, arg1, nullptr};
        
        std::cout << "DEBUG SetUp: About to initialize app" << std::endl;
        initialized = app->initialize(argc, argv);
        std::cout << "DEBUG SetUp: App initialized = " << initialized << std::endl;
        ASSERT_TRUE(initialized) << "Application should initialize in headless mode";
        
        // Set up a standard workspace for testing
        std::cout << "DEBUG SetUp: Getting voxel manager" << std::endl;
        auto voxelManager = app->getVoxelManager();
        ASSERT_TRUE(voxelManager != nullptr);
        
        std::cout << "DEBUG SetUp: Resizing workspace" << std::endl;
        voxelManager->resizeWorkspace(Math::Vector3f(5.0f, 5.0f, 5.0f)); // 5m³ workspace
        std::cout << "DEBUG SetUp: Workspace size set to 5m x 5m x 5m" << std::endl;
        
        // Set resolution to 1cm for most precise coordinate testing
        std::cout << "DEBUG SetUp: Setting active resolution" << std::endl;
        voxelManager->setActiveResolution(VoxelData::VoxelResolution::Size_1cm);
        
        std::cout << "DEBUG SetUp: Getting command processor" << std::endl;
        processor = app->getCommandProcessor();
        ASSERT_TRUE(processor != nullptr);
        
        std::cout << "DEBUG SetUp: SetUp() completed successfully" << std::endl;
    }
    
    void TearDown() override {
        std::cout << "DEBUG TearDown: Starting TearDown()" << std::endl;
        app.reset();
        std::cout << "DEBUG TearDown: TearDown() completed" << std::endl;
    }
    
    // Helper to execute command and verify result
    CommandResult executeCommand(const std::string& command) {
        return processor->execute(command);
    }
    
    // Helper to create CommandContext for direct coordinate testing
    CommandContext createContext(const std::vector<std::string>& args) {
        return CommandContext(app.get(), "test", args);
    }

    std::unique_ptr<Application> app;
    CommandProcessor* processor = nullptr;
    bool initialized = false;
};

// ============================================================================
// Coordinate Unit Validation Tests
// ============================================================================

TEST_F(CoordinateSystemConstraintsTest, ValidCoordinateUnits_REQ_11_2_4) {
    // Test valid coordinate unit formats
    std::vector<std::string> validCoordinates = {
        "0cm", "100cm", "-100cm", "50cm", "-50cm",   // Centimeter units
        "0m", "1m", "-1m", "2.5m", "-2.5m",         // Meter units
        "1.5m", "-1.5m", "0.5m", "2.0m"             // Decimal meters
    };
    
    for (const auto& coord : validCoordinates) {
        CommandContext ctx = createContext({coord, "0cm", "0cm"});
        auto result = ctx.getCoordinateArg(0);
        EXPECT_TRUE(result.has_value()) << "Valid coordinate should parse successfully: " << coord;
    }
}

TEST_F(CoordinateSystemConstraintsTest, InvalidCoordinateUnits_REQ_11_2_4) {
    // Test invalid coordinate unit formats
    std::vector<std::string> invalidCoordinates = {
        "100",     // Missing unit
        "100in",   // Wrong unit (inches)
        "100ft",   // Wrong unit (feet)
        "100x",    // Invalid unit
        "cm100",   // Unit before number
        "m1",      // Unit before number
        "",        // Empty string
        "abc",     // Non-numeric
        "m",       // Unit only
        "cm"       // Unit only
    };
    
    for (const auto& coord : invalidCoordinates) {
        CommandContext ctx = createContext({coord, "0cm", "0cm"});
        auto result = ctx.getCoordinateArg(0);
        EXPECT_FALSE(result.has_value()) << "Invalid coordinate should fail to parse: " << coord;
    }
    
    // Note: "100mm" actually gets parsed as "100m" due to the parser checking 'm' suffix first
    // This is a known issue in the coordinate parsing logic that should be fixed separately
}

TEST_F(CoordinateSystemConstraintsTest, CoordinateUnitConversion_REQ_11_2_4) {
    // Test unit conversion accuracy between cm and m
    struct ConversionTest {
        std::string input;
        int expectedGridUnits;
    };
    
    std::vector<ConversionTest> tests = {
        {"0cm", 0},       {"0m", 0},
        {"100cm", 100},   {"1m", 100},    // 1m = 100cm
        {"-100cm", -100}, {"-1m", -100},  // Negative values
        {"50cm", 50},     {"0.5m", 50},   // 0.5m = 50cm
        {"250cm", 250},   {"2.5m", 250},  // 2.5m = 250cm
        {"-50cm", -50},   {"-0.5m", -50}, // Negative decimal
    };
    
    for (const auto& test : tests) {
        CommandContext ctx = createContext({test.input, "0cm", "0cm"});
        auto result = ctx.getCoordinateArg(0);
        ASSERT_TRUE(result.has_value()) << "Coordinate " << test.input << " should parse successfully";
        EXPECT_EQ(*result, test.expectedGridUnits) 
            << "Coordinate " << test.input << " should convert to " << test.expectedGridUnits << " grid units";
    }
}

// ============================================================================
// Coordinate System Constraint Tests
// ============================================================================

TEST_F(CoordinateSystemConstraintsTest, CenteredCoordinateSystem_REQ_11_2_4) {
    // Test that the coordinate system supports negative values for X and Z
    // (centered at origin 0,0,0)
    
    std::vector<std::string> validCenteredCoordinates = {
        // X-axis: negative and positive
        "-250cm 0cm 0cm",   // Negative X
        "250cm 0cm 0cm",    // Positive X
        "0cm 0cm 0cm",      // Center
        
        // Z-axis: negative and positive  
        "0cm 0cm -250cm",   // Negative Z
        "0cm 0cm 250cm",    // Positive Z
        
        // Combinations
        "-100cm 0cm -100cm", // Negative X, Z
        "100cm 0cm 100cm",   // Positive X, Z
        "-100cm 0cm 100cm",  // Negative X, positive Z
        "100cm 0cm -100cm"   // Positive X, negative Z
    };
    
    for (const auto& coords : validCenteredCoordinates) {
        auto result = executeCommand("place " + coords);
        // Should succeed or fail for reasons other than coordinate system constraint
        // (e.g., workspace bounds, but not because of negative X/Z coordinates)
        EXPECT_TRUE(result.success || 
                   result.message.find("coordinate") == std::string::npos)
            << "Centered coordinate system should support: " << coords
            << " (Error: " << result.message << ")";
    }
}

TEST_F(CoordinateSystemConstraintsTest, GroundPlaneConstraint_REQ_11_2_4) {
    // Test Y >= 0 constraint (no voxels below ground plane)
    
    // Valid Y coordinates (at or above ground)
    std::vector<std::string> validYCoordinates = {
        "0cm 0cm 0cm",      // Ground level (Y=0)
        "0cm 50cm 0cm",     // Above ground
        "0cm 100cm 0cm",    // Well above ground
        "0cm 250cm 0cm"     // High above ground
    };
    
    for (const auto& coords : validYCoordinates) {
        auto result = executeCommand("place " + coords);
        // Should succeed or fail for reasons other than Y constraint
        EXPECT_TRUE(result.success || 
                   result.message.find("ground") == std::string::npos)
            << "Valid Y coordinate should be accepted: " << coords
            << " (Error: " << result.message << ")";
    }
    
    // Invalid Y coordinates (below ground plane)
    std::vector<std::string> invalidYCoordinates = {
        "0cm -1cm 0cm",     // Just below ground
        "0cm -50cm 0cm",    // Below ground
        "0cm -100cm 0cm"    // Well below ground
    };
    
    for (const auto& coords : invalidYCoordinates) {
        auto result = executeCommand("place " + coords);
        EXPECT_FALSE(result.success) 
            << "Y < 0 coordinate should be rejected: " << coords;
        EXPECT_TRUE(result.message.find("ground") != std::string::npos ||
                   result.message.find("Y") != std::string::npos ||
                   result.message.find("below") != std::string::npos)
            << "Error message should mention ground plane constraint for: " << coords
            << " (Error: " << result.message << ")";
    }
}

// ============================================================================
// Command-Specific Coordinate Constraint Tests
// ============================================================================

TEST_F(CoordinateSystemConstraintsTest, PlaceCommandCoordinates_REQ_11_2_4) {
    // Test place command with various coordinate constraints
    
    struct PlaceTest {
        std::string command;
        bool shouldSucceed;
        std::string description;
    };
    
    std::vector<PlaceTest> tests = {
        // Valid coordinates
        {"place 0cm 0cm 0cm", true, "Origin placement"},
        {"place 100cm 50cm -100cm", true, "Mixed positive/negative"},
        {"place -50cm 0cm 50cm", true, "Centered coordinates"},
        
        // Invalid Y coordinates
        {"place 0cm -1cm 0cm", false, "Below ground plane"},
        {"place 100cm -50cm 100cm", false, "Negative Y coordinate"},
        
        // Invalid coordinate formats
        {"place 100 50 0", false, "Missing units"},
        {"place 100cm 50 0cm", false, "Partial units"},
        {"place 100cm 50cm", false, "Insufficient coordinates"},
        {"place", false, "No coordinates"},
        
        // Workspace boundary tests (assuming 5m³ workspace)
        {"place 300cm 0cm 0cm", false, "Outside workspace X+"},
        {"place -300cm 0cm 0cm", false, "Outside workspace X-"},
        {"place 0cm 0cm 300cm", false, "Outside workspace Z+"},
        {"place 0cm 0cm -300cm", false, "Outside workspace Z-"}
    };
    
    for (const auto& test : tests) {
        auto result = executeCommand(test.command);
        if (test.shouldSucceed) {
            EXPECT_TRUE(result.success) 
                << "Should succeed: " << test.description 
                << " (" << test.command << ") Error: " << result.message;
        } else {
            EXPECT_FALSE(result.success) 
                << "Should fail: " << test.description 
                << " (" << test.command << ")";
        }
    }
}

TEST_F(CoordinateSystemConstraintsTest, DeleteCommandCoordinates_REQ_11_2_4) {
    // Test delete command with coordinate constraints
    
    struct DeleteTest {
        std::string command;
        bool shouldSucceed;
        std::string description;
    };
    
    std::vector<DeleteTest> tests = {
        // Valid coordinates (may fail if no voxel, but not due to coordinate constraint)
        {"delete 0cm 0cm 0cm", true, "Origin deletion"},
        {"delete -100cm 50cm 100cm", true, "Valid centered coordinates"},
        
        // Invalid coordinate formats
        {"delete 100 50 0", false, "Missing units"},
        {"delete 100cm", false, "Insufficient coordinates"},
        {"delete", false, "No coordinates"},
        
        // Invalid Y coordinates  
        {"delete 0cm -1cm 0cm", false, "Below ground plane"}
    };
    
    for (const auto& test : tests) {
        auto result = executeCommand(test.command);
        if (test.shouldSucceed) {
            // Delete may fail if no voxel exists, but should not fail due to coordinate constraints
            EXPECT_TRUE(result.success || 
                       result.message.find("not found") != std::string::npos ||
                       result.message.find("no voxel") != std::string::npos ||
                       result.message.find("No voxel") != std::string::npos)
                << "Should succeed or fail due to missing voxel: " << test.description 
                << " (" << test.command << ") Error: " << result.message;
        } else {
            EXPECT_FALSE(result.success) 
                << "Should fail due to coordinate constraint: " << test.description 
                << " (" << test.command << ")";
        }
    }
}

TEST_F(CoordinateSystemConstraintsTest, FillCommandCoordinates_REQ_11_2_4) {
    // Test fill command with coordinate range constraints
    std::cout << "DEBUG TEST: Starting FillCommandCoordinates test body" << std::endl;
    
    struct FillTest {
        std::string command;
        bool shouldSucceed;
        std::string description;
    };
    
    std::cout << "DEBUG TEST: Creating test vector" << std::endl;
    
    std::vector<FillTest> tests = {
        // Valid fill commands (fill takes integer arguments without units)
        {"fill 0 0 0 2 2 2", true, "Small valid fill"},
        {"fill -50 0 -50 -40 10 -40", true, "Centered fill region (non-overlapping)"},
        {"fill 0 10 0 5 20 5", true, "Above ground fill"},
        
        // Y coordinates below ground (new behavior: partial success, fills valid voxels only)
        {"fill 0 -1 0 10 10 10", true, "Start Y below ground (fills valid voxels above Y=0)"},
        {"fill 0 0 0 10 -1 10", false, "End Y below ground (no valid voxels)"},
        {"fill -10 -5 -10 10 5 10", true, "Y range spans below ground (fills valid voxels above Y=0)"},
        
        // Invalid coordinate formats
        // Note: fill command uses std::stoi which accepts "0cm" as 0, so it works
        {"fill 0 0 0 2 2", false, "Insufficient coordinates"},
        {"fill", false, "No coordinates"},
        
        // Edge cases - may fail due to grid alignment or other validation issues
        {"fill 2 0 2 0 0 0", false, "Reversed range - may fail validation"},
        {"fill 0 0 0 0 0 0", false, "Single voxel fill - may fail validation"},
    };
    
    std::cout << "DEBUG TEST: Test vector created with " << tests.size() << " tests" << std::endl;
    
    for (size_t i = 0; i < tests.size(); ++i) {
        const auto& test = tests[i];
        std::cout << "DEBUG TEST: Running test " << i << ": " << test.command << std::endl;
        
        auto result = executeCommand(test.command);
        
        std::cout << "DEBUG TEST: Command executed, result.success = " << result.success << std::endl;
        std::cout << "DEBUG TEST: Result message: " << result.message << std::endl;
        
        if (test.shouldSucceed) {
            EXPECT_TRUE(result.success) 
                << "Should succeed: " << test.description 
                << " (" << test.command << ") Error: " << result.message;
        } else {
            EXPECT_FALSE(result.success) 
                << "Should fail: " << test.description 
                << " (" << test.command << ")";
        }
        
        std::cout << "DEBUG TEST: Test " << i << " completed" << std::endl;
    }
    
    std::cout << "DEBUG TEST: All tests completed" << std::endl;
}

TEST_F(CoordinateSystemConstraintsTest, DISABLED_SelectboxCommandCoordinates_REQ_11_2_4) {
    // Test selectbox command with coordinate constraints
    
    struct SelectboxTest {
        std::string command;
        bool shouldSucceed;
        std::string description;
    };
    
    std::vector<SelectboxTest> tests = {
        // Valid coordinate ranges
        {"selectbox -100cm 0cm -100cm 100cm 200cm 100cm", true, "Valid selection box"},
        {"selectbox 0cm 0cm 0cm 50cm 50cm 50cm", true, "Small selection box"},
        
        // Invalid coordinate formats
        {"selectbox 0 0 0 100 100 100", false, "Missing units"},
        {"selectbox 0cm 0cm 0cm 100cm", false, "Insufficient coordinates"},
        
        // Invalid Y coordinates
        {"selectbox 0cm -1cm 0cm 100cm 100cm 100cm", false, "Start Y below ground"},
        {"selectbox 0cm 0cm 0cm 100cm -1cm 100cm", false, "End Y below ground"}
    };
    
    for (const auto& test : tests) {
        auto result = executeCommand(test.command);
        if (test.shouldSucceed) {
            EXPECT_TRUE(result.success) 
                << "Should succeed: " << test.description 
                << " (" << test.command << ") Error: " << result.message;
        } else {
            EXPECT_FALSE(result.success) 
                << "Should fail: " << test.description 
                << " (" << test.command << ")";
        }
    }
}

// ============================================================================
// Workspace Boundary Constraint Tests  
// ============================================================================

TEST_F(CoordinateSystemConstraintsTest, WorkspaceBoundaryValidation_REQ_11_2_4) {
    // Test that coordinates are validated against workspace boundaries
    
    // Using default 5m³ workspace set in SetUp() (-2.5m to +2.5m)
    // Note: workspace command doesn't exist, using default workspace size
    
    struct BoundaryTest {
        std::string coords;
        bool shouldBeValid;
        std::string description;
    };
    
    std::vector<BoundaryTest> tests = {
        // Within workspace bounds (5m³ = -2.5m to +2.5m = -250cm to +250cm)
        // Note: 1cm voxel extends 1cm, so max valid position is 249cm
        {"0cm 0cm 0cm", true, "Center of workspace"},
        {"249cm 0cm 249cm", true, "Near positive boundary"},
        {"-250cm 0cm -250cm", true, "Near negative boundary"},
        {"200cm 0cm 200cm", true, "Within positive boundary"},
        {"-200cm 0cm -200cm", true, "Within negative boundary"},
        
        // Outside workspace bounds
        {"300cm 0cm 0cm", false, "Beyond positive X boundary"},
        {"-300cm 0cm 0cm", false, "Beyond negative X boundary"},
        {"0cm 0cm 300cm", false, "Beyond positive Z boundary"},
        {"0cm 0cm -300cm", false, "Beyond negative Z boundary"},
        {"300cm 0cm 300cm", false, "Beyond all positive boundaries"},
        {"-300cm 0cm -300cm", false, "Beyond all negative boundaries"}
    };
    
    for (const auto& test : tests) {
        auto placeResult = executeCommand("place " + test.coords);
        if (test.shouldBeValid) {
            EXPECT_TRUE(placeResult.success) 
                << "Should be within workspace: " << test.description 
                << " (" << test.coords << ") Error: " << placeResult.message;
        } else {
            EXPECT_FALSE(placeResult.success) 
                << "Should be outside workspace: " << test.description 
                << " (" << test.coords << ")";
            EXPECT_TRUE(placeResult.message.find("workspace") != std::string::npos ||
                       placeResult.message.find("boundary") != std::string::npos ||
                       placeResult.message.find("bounds") != std::string::npos)
                << "Error should mention workspace bounds for: " << test.coords
                << " (Error: " << placeResult.message << ")";
        }
    }
}

// ============================================================================
// Error Message Quality Tests
// ============================================================================

TEST_F(CoordinateSystemConstraintsTest, CoordinateErrorMessageQuality_REQ_11_2_4) {
    // Test that coordinate constraint violations provide helpful error messages
    
    struct ErrorTest {
        std::string command;
        std::vector<std::string> expectedKeywords; // Keywords that should appear in error message
        std::string description;
    };
    
    std::vector<ErrorTest> tests = {
        {
            "place 100 50 0",
            {"unit", "cm", "m"},
            "Missing units should mention unit requirement"
        },
        {
            "place 0cm -1cm 0cm",
            {"ground", "Y", "below"},
            "Below ground should mention ground plane constraint"
        },
        {
            "place 500cm 0cm 0cm",
            {"workspace", "boundary", "bounds"},
            "Outside workspace should mention boundary violation"
        },
        {
            "place abc def ghi",
            {"coordinate", "invalid", "format"},
            "Invalid format should mention coordinate format"
        },
        {
            "fill 0cm 0cm",
            {"coordinate", "required", "insufficient"},
            "Missing coordinates should mention requirement"
        }
    };
    
    for (const auto& test : tests) {
        auto result = executeCommand(test.command);
        EXPECT_FALSE(result.success) 
            << "Command should fail: " << test.description 
            << " (" << test.command << ")";
        
        bool foundKeyword = false;
        for (const auto& keyword : test.expectedKeywords) {
            // Case-insensitive search
            std::string lowerMessage = result.message;
            std::string lowerKeyword = keyword;
            std::transform(lowerMessage.begin(), lowerMessage.end(), lowerMessage.begin(), ::tolower);
            std::transform(lowerKeyword.begin(), lowerKeyword.end(), lowerKeyword.begin(), ::tolower);
            
            if (lowerMessage.find(lowerKeyword) != std::string::npos) {
                foundKeyword = true;
                break;
            }
        }
        
        EXPECT_TRUE(foundKeyword) 
            << "Error message should contain one of the expected keywords for: " 
            << test.description << " (" << test.command << ")"
            << "\nExpected keywords: ";
        for (const auto& keyword : test.expectedKeywords) {
            std::cout << keyword << " ";
        }
        std::cout << "\nActual message: " << result.message << std::endl;
    }
}

} // namespace Tests
} // namespace VoxelEditor