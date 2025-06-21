#include <gtest/gtest.h>
#include "../ConfigValue.h"

using namespace VoxelEditor::Config;

class ConfigValueTest : public ::testing::Test {
protected:
    void SetUp() override {}
    void TearDown() override {}
};

TEST_F(ConfigValueTest, BasicConstruction) {
    ConfigValue intValue(42);
    ConfigValue floatValue(3.14f);
    ConfigValue stringValue(std::string("hello"));
    ConfigValue boolValue(true);
    
    EXPECT_TRUE(intValue.isValid());
    EXPECT_TRUE(floatValue.isValid());
    EXPECT_TRUE(stringValue.isValid());
    EXPECT_TRUE(boolValue.isValid());
    
    EXPECT_FALSE(intValue.isEmpty());
    EXPECT_FALSE(floatValue.isEmpty());
    EXPECT_FALSE(stringValue.isEmpty());
    EXPECT_FALSE(boolValue.isEmpty());
}

TEST_F(ConfigValueTest, DefaultConstruction) {
    ConfigValue emptyValue;
    
    EXPECT_FALSE(emptyValue.isValid());
    EXPECT_TRUE(emptyValue.isEmpty());
    EXPECT_EQ(emptyValue.getTypeName(), "void");
}

TEST_F(ConfigValueTest, TypedRetrieval) {
    ConfigValue intValue(42);
    ConfigValue floatValue(3.14f);
    ConfigValue stringValue(std::string("hello"));
    ConfigValue boolValue(true);
    
    EXPECT_EQ(intValue.get<int>(), 42);
    EXPECT_FLOAT_EQ(floatValue.get<float>(), 3.14f);
    EXPECT_EQ(stringValue.get<std::string>(), "hello");
    EXPECT_EQ(boolValue.get<bool>(), true);
}

TEST_F(ConfigValueTest, TypeMismatch) {
    ConfigValue intValue(42);
    
    EXPECT_THROW(intValue.get<std::string>(), std::bad_cast);
    EXPECT_THROW(intValue.get<float>(), std::bad_cast);
}

TEST_F(ConfigValueTest, TypeNames) {
    ConfigValue intValue(42);
    ConfigValue floatValue(3.14f);
    ConfigValue stringValue(std::string("hello"));
    ConfigValue boolValue(true);
    ConfigValue doubleValue(2.718);
    
    EXPECT_EQ(intValue.getTypeName(), "int");
    EXPECT_EQ(floatValue.getTypeName(), "float");
    EXPECT_EQ(stringValue.getTypeName(), "string");
    EXPECT_EQ(boolValue.getTypeName(), "bool");
    EXPECT_EQ(doubleValue.getTypeName(), "double");
}

TEST_F(ConfigValueTest, StringConversion) {
    ConfigValue intValue(42);
    ConfigValue floatValue(3.14f);
    ConfigValue stringValue(std::string("hello"));
    ConfigValue boolValue(true);
    
    EXPECT_EQ(intValue.toString(), "42");
    EXPECT_EQ(floatValue.toString(), "3.140000");
    EXPECT_EQ(stringValue.toString(), "hello");
    EXPECT_EQ(boolValue.toString(), "true");
    
    ConfigValue falseValue(false);
    EXPECT_EQ(falseValue.toString(), "false");
}

TEST_F(ConfigValueTest, FromString) {
    ConfigValue value;
    
    // Test boolean conversion
    value.fromString("true", std::type_index(typeid(bool)));
    EXPECT_EQ(value.get<bool>(), true);
    
    value.fromString("false", std::type_index(typeid(bool)));
    EXPECT_EQ(value.get<bool>(), false);
    
    value.fromString("1", std::type_index(typeid(bool)));
    EXPECT_EQ(value.get<bool>(), true);
    
    value.fromString("0", std::type_index(typeid(bool)));
    EXPECT_EQ(value.get<bool>(), false);
    
    // Test integer conversion
    value.fromString("123", std::type_index(typeid(int)));
    EXPECT_EQ(value.get<int>(), 123);
    
    value.fromString("-456", std::type_index(typeid(int)));
    EXPECT_EQ(value.get<int>(), -456);
    
    // Test float conversion
    value.fromString("3.14", std::type_index(typeid(float)));
    EXPECT_FLOAT_EQ(value.get<float>(), 3.14f);
    
    // Test double conversion
    value.fromString("2.718", std::type_index(typeid(double)));
    EXPECT_DOUBLE_EQ(value.get<double>(), 2.718);
    
    // Test string conversion
    value.fromString("hello world", std::type_index(typeid(std::string)));
    EXPECT_EQ(value.get<std::string>(), "hello world");
}

TEST_F(ConfigValueTest, ValueUpdate) {
    ConfigValue value(42);
    
    EXPECT_EQ(value.get<int>(), 42);
    EXPECT_EQ(value.getTypeName(), "int");
    
    value.set(std::string("hello"));
    EXPECT_EQ(value.get<std::string>(), "hello");
    EXPECT_EQ(value.getTypeName(), "string");
    
    value.set(3.14f);
    EXPECT_FLOAT_EQ(value.get<float>(), 3.14f);
    EXPECT_EQ(value.getTypeName(), "float");
}

TEST_F(ConfigValueTest, TypeChecking) {
    ConfigValue intValue(42);
    ConfigValue stringValue(std::string("hello"));
    
    EXPECT_TRUE(intValue.canConvertTo<int>());
    EXPECT_FALSE(intValue.canConvertTo<std::string>());
    EXPECT_FALSE(intValue.canConvertTo<float>());
    
    EXPECT_TRUE(stringValue.canConvertTo<std::string>());
    EXPECT_FALSE(stringValue.canConvertTo<int>());
    EXPECT_FALSE(stringValue.canConvertTo<float>());
}

TEST_F(ConfigValueTest, GetValueOrHelper) {
    ConfigValue intValue(42);
    ConfigValue emptyValue;
    
    EXPECT_EQ(getValueOr(intValue, 100), 42);
    EXPECT_EQ(getValueOr(emptyValue, 100), 100);
    
    // Test type mismatch fallback
    EXPECT_EQ(getValueOr<std::string>(intValue, "default"), "default");
}

TEST_F(ConfigValueTest, InvalidStringConversion) {
    ConfigValue value;
    
    // Test invalid integer
    EXPECT_THROW(value.fromString("not_a_number", std::type_index(typeid(int))), std::exception);
    
    // Test invalid float
    EXPECT_THROW(value.fromString("not_a_float", std::type_index(typeid(float))), std::exception);
    
    // Test unsupported type
    EXPECT_THROW(value.fromString("test", std::type_index(typeid(void*))), std::runtime_error);
}