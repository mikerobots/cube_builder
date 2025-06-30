#include <gtest/gtest.h>
#include "cli/CommandRegistry.h"
#include "cli/CommandProcessor.h"
#include "cli/Application.h"
#include "cli/EditCommands.h"
#include "cli/FileCommands.h"
#include "cli/ViewCommands.h"
#include "cli/SelectCommands.h"
#include "cli/SystemCommands.h"
#include "cli/MeshCommands.h"
#include <memory>

namespace VoxelEditor {
namespace {

class CommandRegistryTest : public ::testing::Test {
protected:
    void SetUp() override {
        // Create a mock application with minimal dependencies
        m_app = std::make_unique<Application>();
        m_processor = std::make_unique<CommandProcessor>(m_app.get());
    }

    void TearDown() override {
        m_processor.reset();
        m_app.reset();
    }

    std::unique_ptr<Application> m_app;
    std::unique_ptr<CommandProcessor> m_processor;
};

TEST_F(CommandRegistryTest, RegistryExists) {
    // Test that we can get the CommandRegistry instance
    auto& registry = CommandRegistry::getInstance();
    EXPECT_NO_THROW(registry.registerAllCommands(m_app.get(), m_processor.get()));
}

TEST_F(CommandRegistryTest, ManualModuleRegistration) {
    // Test manual registration of command modules
    auto& registry = CommandRegistry::getInstance();
    
    // Manually create and register each module
    auto editModule = std::make_unique<EditCommands>();
    editModule->setApplication(m_app.get());
    auto editCommands = editModule->getCommands();
    EXPECT_GT(editCommands.size(), 0) << "EditCommands should provide commands";
    
    auto fileModule = std::make_unique<FileCommands>();
    fileModule->setApplication(m_app.get());
    auto fileCommands = fileModule->getCommands();
    EXPECT_GT(fileCommands.size(), 0) << "FileCommands should provide commands";
    
    auto viewModule = std::make_unique<ViewCommands>();
    viewModule->setApplication(m_app.get());
    auto viewCommands = viewModule->getCommands();
    EXPECT_GT(viewCommands.size(), 0) << "ViewCommands should provide commands";
    
    auto selectModule = std::make_unique<SelectCommands>();
    selectModule->setApplication(m_app.get());
    auto selectCommands = selectModule->getCommands();
    EXPECT_GT(selectCommands.size(), 0) << "SelectCommands should provide commands";
    
    auto systemModule = std::make_unique<SystemCommands>();
    systemModule->setApplication(m_app.get());
    auto systemCommands = systemModule->getCommands();
    EXPECT_GT(systemCommands.size(), 0) << "SystemCommands should provide commands";
    
    auto meshModule = std::make_unique<MeshCommands>();
    meshModule->setApplication(m_app.get());
    auto meshCommands = meshModule->getCommands();
    EXPECT_GT(meshCommands.size(), 0) << "MeshCommands should provide commands";
}

TEST_F(CommandRegistryTest, CommandProcessorRegistration) {
    // Test that commands can be registered with the processor
    auto editModule = std::make_unique<EditCommands>();
    editModule->setApplication(m_app.get());
    auto commands = editModule->getCommands();
    
    // Register commands with processor
    for (const auto& cmdReg : commands) {
        CommandDefinition cmdDef;
        cmdDef.name = cmdReg.name;
        cmdDef.description = cmdReg.description;
        cmdDef.category = cmdReg.category;
        cmdDef.aliases = cmdReg.aliases;
        
        for (const auto& arg : cmdReg.args) {
            CommandArgument cmdArg;
            cmdArg.name = arg.name;
            cmdArg.description = arg.description;
            cmdArg.type = arg.type;
            cmdArg.required = arg.required;
            cmdArg.defaultValue = arg.defaultValue;
            cmdDef.arguments.push_back(cmdArg);
        }
        
        cmdDef.handler = [handler = cmdReg.handler](const CommandContext& ctx) {
            return handler(ctx);
        };
        
        EXPECT_NO_THROW(m_processor->registerCommand(cmdDef)) 
            << "Should be able to register command: " << cmdDef.name;
    }
    
    // Verify commands are registered
    auto help = m_processor->getHelp();
    EXPECT_FALSE(help.empty()) << "Help should show registered commands";
}

TEST_F(CommandRegistryTest, StaticInitializationCheck) {
    // Check if static initialization has occurred
    // This helps diagnose the initialization order issue
    
    // Force static initialization by creating instances
    // This simulates what the static initializers would do
    EditCommands editCmd;
    FileCommands fileCmd;
    ViewCommands viewCmd;
    SelectCommands selectCmd;
    SystemCommands systemCmd;
    MeshCommands meshCmd;
    
    // Now check if factories were registered
    auto& registry = CommandRegistry::getInstance();
    
    // Since we can't directly access the factory count, we'll try registration
    // and check the output (we added debug output in registerAllCommands)
    testing::internal::CaptureStdout();
    registry.registerAllCommands(m_app.get(), m_processor.get());
    std::string output = testing::internal::GetCapturedStdout();
    
    // Check if any modules were registered
    EXPECT_TRUE(output.find("0 factories") == std::string::npos) 
        << "Static initialization should have registered factories. Output: " << output;
}

TEST_F(CommandRegistryTest, DirectModuleRegistration) {
    // Test direct module registration without factories
    // This simulates what should happen with proper static initialization
    
    // Create a fresh registry instance for this test
    CommandRegistry testRegistry;
    
    // Directly register modules
    testRegistry.registerModule(std::make_unique<EditCommands>());
    testRegistry.registerModule(std::make_unique<FileCommands>());
    testRegistry.registerModule(std::make_unique<ViewCommands>());
    testRegistry.registerModule(std::make_unique<SelectCommands>());
    testRegistry.registerModule(std::make_unique<SystemCommands>());
    testRegistry.registerModule(std::make_unique<MeshCommands>());
    
    // Create a fresh processor for testing
    CommandProcessor testProcessor(m_app.get());
    
    // Register all commands
    testing::internal::CaptureStdout();
    testRegistry.registerAllCommands(m_app.get(), &testProcessor);
    std::string output = testing::internal::GetCapturedStdout();
    
    // Verify registration worked - expecting 51 commands now
    EXPECT_TRUE(output.find("51 commands") != std::string::npos) 
        << "Should have registered many commands. Output: " << output;
    EXPECT_TRUE(output.find("6 modules") != std::string::npos) 
        << "Should have 6 modules. Output: " << output;
    
    // Verify specific commands exist
    auto help = testProcessor.getHelp();
    EXPECT_TRUE(help.find("place") != std::string::npos) << "Should have place command";
    EXPECT_TRUE(help.find("delete") != std::string::npos) << "Should have delete command";
    EXPECT_TRUE(help.find("save") != std::string::npos) << "Should have save command";
    EXPECT_TRUE(help.find("open") != std::string::npos) << "Should have open command";
    EXPECT_TRUE(help.find("camera") != std::string::npos) << "Should have camera command";
    EXPECT_TRUE(help.find("select") != std::string::npos) << "Should have select command";
    EXPECT_TRUE(help.find("help") != std::string::npos) << "Should have help command";
    EXPECT_TRUE(help.find("quit") != std::string::npos) << "Should have quit command";
    EXPECT_TRUE(help.find("smooth") != std::string::npos) << "Should have smooth command";
}

TEST_F(CommandRegistryTest, ForceStaticInitialization) {
    // Force static initialization and verify it works
    // This is what the actual application needs to do
    
    // Create command module instances to force their static initializers to run
    volatile auto* edit = new EditCommands();
    volatile auto* file = new FileCommands();
    volatile auto* view = new ViewCommands();
    volatile auto* select = new SelectCommands();
    volatile auto* system = new SystemCommands();
    volatile auto* mesh = new MeshCommands();
    
    // Clean up
    delete edit;
    delete file;
    delete view;
    delete select;
    delete system;
    delete mesh;
    
    // Now the static initializers should have run
    // Test with a fresh Application and CommandProcessor
    Application testApp;
    CommandProcessor testProcessor(&testApp);
    
    testing::internal::CaptureStdout();
    CommandRegistry::getInstance().registerAllCommands(&testApp, &testProcessor);
    std::string output = testing::internal::GetCapturedStdout();
    
    // Check if factories were registered
    EXPECT_TRUE(output.find("factories") != std::string::npos) 
        << "Should show factory count. Output: " << output;
    
    // If static init worked, we should see modules
    if (output.find("0 factories") == std::string::npos) {
        EXPECT_TRUE(output.find("6 modules") != std::string::npos) 
            << "Should have 6 modules if static init worked. Output: " << output;
    }
}

} // namespace
} // namespace VoxelEditor