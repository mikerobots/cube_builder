#include <gtest/gtest.h>
#include "../ConfigSection.h"

using namespace VoxelEditor::Config;

class ConfigSectionTest : public ::testing::Test {
protected:
    void SetUp() override {
        section = std::make_unique<ConfigSection>("TestSection");
    }
    
    void TearDown() override {
        section.reset();
    }
    
    std::unique_ptr<ConfigSection> section;
};

TEST_F(ConfigSectionTest, BasicValueOperations) {
    // Test setting and getting values
    section->setValue("intKey", 42);
    section->setValue("floatKey", 3.14f);
    section->setValue("stringKey", std::string("hello"));
    section->setValue("boolKey", true);
    
    EXPECT_EQ(section->getValue<int>("intKey"), 42);
    EXPECT_FLOAT_EQ(section->getValue<float>("floatKey"), 3.14f);
    EXPECT_EQ(section->getValue<std::string>("stringKey"), "hello");
    EXPECT_EQ(section->getValue<bool>("boolKey"), true);
}

TEST_F(ConfigSectionTest, DefaultValues) {
    // Test getting non-existent keys with defaults
    EXPECT_EQ(section->getValue<int>("nonexistent", 100), 100);
    EXPECT_FLOAT_EQ(section->getValue<float>("nonexistent", 2.0f), 2.0f);
    EXPECT_EQ(section->getValue<std::string>("nonexistent", "default"), "default");
    EXPECT_EQ(section->getValue<bool>("nonexistent", false), false);
}

TEST_F(ConfigSectionTest, HasValue) {
    section->setValue("existingKey", 42);
    
    EXPECT_TRUE(section->hasValue("existingKey"));
    EXPECT_FALSE(section->hasValue("nonexistentKey"));
}

TEST_F(ConfigSectionTest, RemoveValue) {
    section->setValue("tempKey", 42);
    EXPECT_TRUE(section->hasValue("tempKey"));
    
    section->removeValue("tempKey");
    EXPECT_FALSE(section->hasValue("tempKey"));
    
    // Removing non-existent key should not crash
    section->removeValue("nonexistentKey");
}

TEST_F(ConfigSectionTest, GetKeys) {
    section->setValue("key1", 1);
    section->setValue("key2", 2);
    section->setValue("key3", 3);
    
    auto keys = section->getKeys();
    EXPECT_EQ(keys.size(), 3);
    
    // Check that all keys are present (order may vary)
    std::sort(keys.begin(), keys.end());
    EXPECT_EQ(keys[0], "key1");
    EXPECT_EQ(keys[1], "key2");
    EXPECT_EQ(keys[2], "key3");
}

TEST_F(ConfigSectionTest, NestedSections) {
    // Create nested sections
    auto child1 = section->getSection("child1");
    auto child2 = section->getSection("child2");
    
    EXPECT_NE(child1, nullptr);
    EXPECT_NE(child2, nullptr);
    EXPECT_NE(child1, child2);
    
    // Test that getting the same section returns the same object
    auto child1Again = section->getSection("child1");
    EXPECT_EQ(child1, child1Again);
}

TEST_F(ConfigSectionTest, SectionQueries) {
    auto child = section->getSection("child");
    
    EXPECT_TRUE(section->hasSection("child"));
    EXPECT_FALSE(section->hasSection("nonexistent"));
    
    auto sectionNames = section->getSectionNames();
    EXPECT_EQ(sectionNames.size(), 1);
    EXPECT_EQ(sectionNames[0], "child");
}

TEST_F(ConfigSectionTest, RemoveSection) {
    auto child = section->getSection("child");
    EXPECT_TRUE(section->hasSection("child"));
    
    section->removeSection("child");
    EXPECT_FALSE(section->hasSection("child"));
}

TEST_F(ConfigSectionTest, PathBasedAccess) {
    // Set values using path notation
    section->setValueByPath("level1.level2.key1", 42);
    section->setValueByPath("level1.level2.key2", std::string("hello"));
    section->setValueByPath("level1.key3", 3.14f);
    section->setValueByPath("directKey", true);
    
    // Get values using path notation
    EXPECT_EQ(section->getValueByPath<int>("level1.level2.key1"), 42);
    EXPECT_EQ(section->getValueByPath<std::string>("level1.level2.key2"), "hello");
    EXPECT_FLOAT_EQ(section->getValueByPath<float>("level1.key3"), 3.14f);
    EXPECT_EQ(section->getValueByPath<bool>("directKey"), true);
    
    // Test defaults for non-existent paths
    EXPECT_EQ(section->getValueByPath<int>("nonexistent.path", 100), 100);
}

TEST_F(ConfigSectionTest, PathBasedHasValue) {
    section->setValueByPath("level1.level2.key", 42);
    
    EXPECT_TRUE(section->hasValueByPath("level1.level2.key"));
    EXPECT_FALSE(section->hasValueByPath("level1.level2.nonexistent"));
    EXPECT_FALSE(section->hasValueByPath("nonexistent.path"));
}

TEST_F(ConfigSectionTest, ChangeNotifications) {
    bool notificationReceived = false;
    std::string notifiedKey;
    ConfigValue notifiedOldValue, notifiedNewValue;
    
    section->subscribe("testKey", [&](const std::string& key, const ConfigValue& oldValue, const ConfigValue& newValue) {
        notificationReceived = true;
        notifiedKey = key;
        notifiedOldValue = oldValue;
        notifiedNewValue = newValue;
    });
    
    section->setValue("testKey", 42);
    
    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ(notifiedKey, "testKey");
    EXPECT_FALSE(notifiedOldValue.isValid());
    EXPECT_TRUE(notifiedNewValue.isValid());
    EXPECT_EQ(notifiedNewValue.get<int>(), 42);
    
    // Test value change notification
    notificationReceived = false;
    section->setValue("testKey", 100);
    
    EXPECT_TRUE(notificationReceived);
    EXPECT_EQ(notifiedOldValue.get<int>(), 42);
    EXPECT_EQ(notifiedNewValue.get<int>(), 100);
}

TEST_F(ConfigSectionTest, Unsubscribe) {
    bool notificationReceived = false;
    
    section->subscribe("testKey", [&](const std::string&, const ConfigValue&, const ConfigValue&) {
        notificationReceived = true;
    });
    
    section->setValue("testKey", 42);
    EXPECT_TRUE(notificationReceived);
    
    notificationReceived = false;
    section->unsubscribe("testKey");
    section->setValue("testKey", 100);
    EXPECT_FALSE(notificationReceived);
}

TEST_F(ConfigSectionTest, SectionUtilities) {
    EXPECT_TRUE(section->isEmpty());
    EXPECT_EQ(section->size(), 0);
    EXPECT_EQ(section->sectionCount(), 0);
    
    section->setValue("key1", 42);
    auto child = section->getSection("child");
    
    EXPECT_FALSE(section->isEmpty());
    EXPECT_EQ(section->size(), 1);
    EXPECT_EQ(section->sectionCount(), 1);
    
    section->clear();
    
    EXPECT_TRUE(section->isEmpty());
    EXPECT_EQ(section->size(), 0);
    EXPECT_EQ(section->sectionCount(), 0);
}

TEST_F(ConfigSectionTest, SectionName) {
    EXPECT_EQ(section->getName(), "TestSection");
    
    auto child = section->getSection("ChildSection");
    EXPECT_EQ(child->getName(), "ChildSection");
}

TEST_F(ConfigSectionTest, ConstSectionAccess) {
    // Set up some data
    section->setValueByPath("level1.key", 42);
    
    // Test const access
    const ConfigSection* constSection = section.get();
    
    auto constChild = constSection->getSection("level1");
    EXPECT_NE(constChild, nullptr);
    
    // Non-existent section should return nullptr
    auto nonExistent = constSection->getSection("nonexistent");
    EXPECT_EQ(nonExistent, nullptr);
}