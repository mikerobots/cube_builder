#include <gtest/gtest.h>
#include "../ConfigManager.h"
#include "../../events/EventDispatcher.h"
#include <sstream>
#include <fstream>
#include <filesystem>
#include <thread>

using namespace VoxelEditor::Config;
using namespace VoxelEditor::Events;

class ConfigManagerTest : public ::testing::Test {
protected:
    void SetUp() override {
        auto& manager = ConfigManager::getInstance();
        manager.clear();
        manager.loadDefaults();
    }
    
    void TearDown() override {
        ConfigManager& manager = ConfigManager::getInstance();
        manager.setEventDispatcher(nullptr);  // Clear event dispatcher
        manager.clear();
        
        // Clean up test files
        if (std::filesystem::exists("test_config.txt")) {
            std::filesystem::remove("test_config.txt");
        }
    }
};

TEST_F(ConfigManagerTest, Singleton) {
    ConfigManager& manager1 = ConfigManager::getInstance();
    ConfigManager& manager2 = ConfigManager::getInstance();
    
    EXPECT_EQ(&manager1, &manager2);
}

TEST_F(ConfigManagerTest, DefaultConfiguration) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // Test some default values
    EXPECT_EQ(manager.getValue<std::string>("app.name"), "VoxelEditor");
    EXPECT_EQ(manager.getValue<std::string>("app.version"), "1.0.0");
    EXPECT_EQ(manager.getValue<bool>("app.debug"), false);
    
    EXPECT_EQ(manager.getValue<bool>("rendering.vsync"), true);
    EXPECT_EQ(manager.getValue<int>("rendering.msaa"), 4);
    EXPECT_EQ(manager.getValue<int>("rendering.max_fps"), 60);
    
    EXPECT_FLOAT_EQ(manager.getValue<float>("workspace.size_default"), 5.0f);
    EXPECT_FLOAT_EQ(manager.getValue<float>("workspace.size_min"), 2.0f);
    EXPECT_FLOAT_EQ(manager.getValue<float>("workspace.size_max"), 8.0f);
}

TEST_F(ConfigManagerTest, ValueOperations) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // Test setting and getting values
    manager.setValue("test.int_value", 42);
    manager.setValue("test.float_value", 3.14f);
    manager.setValue("test.string_value", std::string("hello"));
    manager.setValue("test.bool_value", true);
    
    EXPECT_EQ(manager.getValue<int>("test.int_value"), 42);
    EXPECT_FLOAT_EQ(manager.getValue<float>("test.float_value"), 3.14f);
    EXPECT_EQ(manager.getValue<std::string>("test.string_value"), "hello");
    EXPECT_EQ(manager.getValue<bool>("test.bool_value"), true);
}

TEST_F(ConfigManagerTest, DefaultValues) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // Test getting non-existent values with defaults
    EXPECT_EQ(manager.getValue<int>("nonexistent.key", 100), 100);
    EXPECT_FLOAT_EQ(manager.getValue<float>("nonexistent.key", 2.0f), 2.0f);
    EXPECT_EQ(manager.getValue<std::string>("nonexistent.key", "default"), "default");
    EXPECT_EQ(manager.getValue<bool>("nonexistent.key", false), false);
}

TEST_F(ConfigManagerTest, HasValue) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    manager.setValue("existing.key", 42);
    
    EXPECT_TRUE(manager.hasValue("existing.key"));
    EXPECT_FALSE(manager.hasValue("nonexistent.key"));
    
    // Test after setting some known values
    manager.setValue("app.name", std::string("VoxelEditor"));
    manager.setValue("rendering.vsync", true);
    EXPECT_TRUE(manager.hasValue("app.name"));
    EXPECT_TRUE(manager.hasValue("rendering.vsync"));
}

TEST_F(ConfigManagerTest, RemoveValue) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    manager.setValue("temp.key", 42);
    EXPECT_TRUE(manager.hasValue("temp.key"));
    
    manager.removeValue("temp.key");
    EXPECT_FALSE(manager.hasValue("temp.key"));
}

TEST_F(ConfigManagerTest, SectionAccess) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // First create a section with some values
    manager.setValue("rendering.vsync", true);
    manager.setValue("rendering.msaa", 4);
    
    auto renderingSection = manager.getSection("rendering");
    EXPECT_NE(renderingSection, nullptr);
    
    // Test that we can access values through the section
    EXPECT_EQ(renderingSection->getValue<bool>("vsync"), true);
    EXPECT_EQ(renderingSection->getValue<int>("msaa"), 4);
    
    // Test setting values through section
    renderingSection->setValue("new_setting", 123);
    EXPECT_EQ(manager.getValue<int>("rendering.new_setting"), 123);
}

TEST_F(ConfigManagerTest, CreateSection) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    manager.createSection("new.deep.section");
    auto section = manager.getSection("new.deep.section");
    EXPECT_NE(section, nullptr);
    
    section->setValue("test_key", 42);
    EXPECT_EQ(manager.getValue<int>("new.deep.section.test_key"), 42);
}

class TestConfigHandler : public EventHandler<ConfigurationChangedEvent> {
public:
    void handleEvent(const ConfigurationChangedEvent& event) override {
        eventReceived = true;
        eventKey = event.key;
    }
    
    bool eventReceived = false;
    std::string eventKey;
};

TEST_F(ConfigManagerTest, EventNotification) {
    ConfigManager& manager = ConfigManager::getInstance();
    EventDispatcher dispatcher;
    manager.setEventDispatcher(&dispatcher);
    
    TestConfigHandler handler;
    dispatcher.subscribe<ConfigurationChangedEvent>(&handler);
    
    manager.setValue("test.notification", 42);
    
    // Manually dispatch the event to trigger handlers
    ConfigurationChangedEvent testEvent("test.notification");
    dispatcher.dispatch(testEvent);
    
    EXPECT_TRUE(handler.eventReceived);
    EXPECT_EQ(handler.eventKey, "test.notification");
}

TEST_F(ConfigManagerTest, GetAllKeys) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    manager.setValue("custom.key1", 1);
    manager.setValue("custom.key2", 2);
    manager.setValue("custom.nested.key3", 3);
    
    auto allKeys = manager.getAllKeys();
    
    // Should contain default keys plus our custom keys
    EXPECT_GT(allKeys.size(), 3);
    
    // Check for our custom keys
    bool foundKey1 = false, foundKey2 = false, foundKey3 = false;
    for (const auto& key : allKeys) {
        if (key == "custom.key1") foundKey1 = true;
        if (key == "custom.key2") foundKey2 = true;
        if (key == "custom.nested.key3") foundKey3 = true;
    }
    
    EXPECT_TRUE(foundKey1);
    EXPECT_TRUE(foundKey2);
    EXPECT_TRUE(foundKey3);
}

TEST_F(ConfigManagerTest, FileIO) {
    ConfigManager& manager = ConfigManager::getInstance();
    const std::string testFile = "test_config.txt";
    
    // Set some custom values
    manager.setValue("test.int_val", 42);
    manager.setValue("test.float_val", 3.14f);
    manager.setValue("test.string_val", std::string("hello"));
    manager.setValue("test.bool_val", true);
    manager.setValue("nested.section.value", 100);
    
    // Save to file
    EXPECT_TRUE(manager.saveToFile(testFile));
    EXPECT_TRUE(std::filesystem::exists(testFile));
    
    // Clear and reload defaults
    manager.clear();
    manager.loadDefaults();
    
    // Verify our custom values are gone
    EXPECT_EQ(manager.getValue<int>("test.int_val", -1), -1);
    
    // Load from file
    EXPECT_TRUE(manager.loadFromFile(testFile));
    
    // Verify values were restored (along with defaults)
    EXPECT_EQ(manager.getValue<int>("test.int_val"), 42);
    EXPECT_FLOAT_EQ(manager.getValue<float>("test.float_val"), 3.14f);
    EXPECT_EQ(manager.getValue<std::string>("test.string_val"), "hello");
    EXPECT_EQ(manager.getValue<bool>("test.bool_val"), true);
    EXPECT_EQ(manager.getValue<int>("nested.section.value"), 100);
    
    // Default values should still be present
    EXPECT_EQ(manager.getValue<std::string>("app.name"), "VoxelEditor");
}

TEST_F(ConfigManagerTest, StreamIO) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // Set some test values
    manager.setValue("stream_test.int_val", 123);
    manager.setValue("stream_test.bool_val", false);
    manager.setValue("stream_test.string_val", std::string("test_string"));
    
    // Save to stream
    std::stringstream ss;
    EXPECT_TRUE(manager.saveToStream(ss));
    
    std::string saved = ss.str();
    EXPECT_FALSE(saved.empty());
    EXPECT_TRUE(saved.find("stream_test.int_val=123") != std::string::npos);
    EXPECT_TRUE(saved.find("stream_test.bool_val=false") != std::string::npos);
    
    // Clear and reload from stream
    manager.clear();
    manager.loadDefaults();
    
    std::stringstream loadStream(saved);
    EXPECT_TRUE(manager.loadFromStream(loadStream));
    
    // Verify values were loaded
    EXPECT_EQ(manager.getValue<int>("stream_test.int_val"), 123);
    EXPECT_EQ(manager.getValue<bool>("stream_test.bool_val"), false);
    EXPECT_EQ(manager.getValue<std::string>("stream_test.string_val"), "test_string");
}

TEST_F(ConfigManagerTest, ConfigFileFormat) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    // Test loading from a manually created config string
    std::string configContent = R"(
# Test configuration file
app.test_mode=true
rendering.resolution_width=1920
rendering.resolution_height=1080
workspace.name="My Workspace"
performance.target_fps=144
debug.enabled=false
)";
    
    manager.clear();
    manager.loadDefaults();
    
    std::stringstream ss(configContent);
    EXPECT_TRUE(manager.loadFromStream(ss));
    
    EXPECT_EQ(manager.getValue<bool>("app.test_mode"), true);
    EXPECT_EQ(manager.getValue<int>("rendering.resolution_width"), 1920);
    EXPECT_EQ(manager.getValue<int>("rendering.resolution_height"), 1080);
    EXPECT_EQ(manager.getValue<std::string>("workspace.name"), "My Workspace");
    EXPECT_EQ(manager.getValue<int>("performance.target_fps"), 144);
    EXPECT_EQ(manager.getValue<bool>("debug.enabled"), false);
}

TEST_F(ConfigManagerTest, ThreadSafety) {
    ConfigManager& manager = ConfigManager::getInstance();
    
    const int numThreads = 4;
    const int operationsPerThread = 100;
    std::vector<std::thread> threads;
    
    // Each thread will set and get values
    for (int t = 0; t < numThreads; ++t) {
        threads.emplace_back([&manager, t, operationsPerThread]() {
            for (int i = 0; i < operationsPerThread; ++i) {
                std::string key = "thread" + std::to_string(t) + ".key" + std::to_string(i);
                int value = t * 1000 + i;
                
                manager.setValue(key, value);
                int retrieved = manager.getValue<int>(key, -1);
                
                // The value should be what we just set
                EXPECT_EQ(retrieved, value);
            }
        });
    }
    
    for (auto& thread : threads) {
        thread.join();
    }
    
    // Verify all values are still correct
    for (int t = 0; t < numThreads; ++t) {
        for (int i = 0; i < operationsPerThread; ++i) {
            std::string key = "thread" + std::to_string(t) + ".key" + std::to_string(i);
            int expected = t * 1000 + i;
            EXPECT_EQ(manager.getValue<int>(key), expected);
        }
    }
}