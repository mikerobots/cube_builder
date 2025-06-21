#include <gtest/gtest.h>
#include <sstream>
#include <cmath>
#include "file_io/BinaryIO.h"
#include "math/Vector3f.h"
#include "math/Matrix4f.h"

namespace VoxelEditor {
namespace FileIO {

class BinaryIOTest : public ::testing::Test {
protected:
    std::stringstream m_stream;
    
    void SetUp() override {
        m_stream.clear();
        m_stream.str("");
    }
    
    void TearDown() override {
    }
};

// Test basic type round-trip
TEST_F(BinaryIOTest, UInt8RoundTrip) {
    BinaryWriter writer(m_stream);
    uint8_t writeValue = 255;
    writer.writeUInt8(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    uint8_t readValue = reader.readUInt8();
    
    EXPECT_EQ(writeValue, readValue);
    EXPECT_TRUE(writer.isValid());
    EXPECT_TRUE(reader.isValid());
}

TEST_F(BinaryIOTest, UInt16RoundTrip) {
    BinaryWriter writer(m_stream);
    uint16_t writeValue = 65535;
    writer.writeUInt16(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    uint16_t readValue = reader.readUInt16();
    
    EXPECT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, UInt32RoundTrip) {
    BinaryWriter writer(m_stream);
    uint32_t writeValue = 4294967295U;
    writer.writeUInt32(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    uint32_t readValue = reader.readUInt32();
    
    EXPECT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, UInt64RoundTrip) {
    BinaryWriter writer(m_stream);
    uint64_t writeValue = 18446744073709551615ULL;
    writer.writeUInt64(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    uint64_t readValue = reader.readUInt64();
    
    EXPECT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, FloatRoundTrip) {
    BinaryWriter writer(m_stream);
    float writeValue = 3.14159f;
    writer.writeFloat(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    float readValue = reader.readFloat();
    
    EXPECT_FLOAT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, DoubleRoundTrip) {
    BinaryWriter writer(m_stream);
    double writeValue = 3.141592653589793;
    writer.writeDouble(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    double readValue = reader.readDouble();
    
    EXPECT_DOUBLE_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, StringRoundTrip) {
    BinaryWriter writer(m_stream);
    std::string writeValue = "Hello, Binary World!";
    writer.writeString(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    std::string readValue = reader.readString();
    
    EXPECT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, EmptyStringRoundTrip) {
    BinaryWriter writer(m_stream);
    std::string writeValue = "";
    writer.writeString(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    std::string readValue = reader.readString();
    
    EXPECT_EQ(writeValue, readValue);
}

TEST_F(BinaryIOTest, Vector3fRoundTrip) {
    BinaryWriter writer(m_stream);
    Math::Vector3f writeValue(1.0f, 2.0f, 3.0f);
    writer.writeVector3f(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    Math::Vector3f readValue = reader.readVector3f();
    
    EXPECT_FLOAT_EQ(writeValue.x, readValue.x);
    EXPECT_FLOAT_EQ(writeValue.y, readValue.y);
    EXPECT_FLOAT_EQ(writeValue.z, readValue.z);
}

TEST_F(BinaryIOTest, Matrix4fRoundTrip) {
    BinaryWriter writer(m_stream);
    Math::Matrix4f writeValue;
    writeValue.identity();
    writeValue[0 * 4 + 1] = 2.0f;  // Set element at row 0, col 1
    writeValue[1 * 4 + 2] = 3.0f;  // Set element at row 1, col 2
    writeValue[2 * 4 + 3] = 4.0f;  // Set element at row 2, col 3
    writer.writeMatrix4f(writeValue);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    Math::Matrix4f readValue = reader.readMatrix4f();
    
    for (int i = 0; i < 16; ++i) {
        EXPECT_FLOAT_EQ(writeValue[i], readValue[i]);
    }
}

TEST_F(BinaryIOTest, BytesRoundTrip) {
    BinaryWriter writer(m_stream);
    std::vector<uint8_t> writeData = {1, 2, 3, 4, 5, 6, 7, 8, 9, 10};
    writer.writeBytes(writeData.data(), writeData.size());
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    std::vector<uint8_t> readData(10);
    reader.readBytes(readData.data(), readData.size());
    
    EXPECT_EQ(writeData, readData);
}

TEST_F(BinaryIOTest, TemplateSpecializationWrite) {
    BinaryWriter writer(m_stream);
    
    // Test template specializations
    writer.write<uint8_t>(42);
    writer.write<uint16_t>(1234);
    writer.write<uint32_t>(567890);
    writer.write<float>(3.14f);
    writer.write<std::string>("template test");
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    
    EXPECT_EQ(reader.read<uint8_t>(), 42);
    EXPECT_EQ(reader.read<uint16_t>(), 1234);
    EXPECT_EQ(reader.read<uint32_t>(), 567890);
    EXPECT_FLOAT_EQ(reader.read<float>(), 3.14f);
    EXPECT_EQ(reader.read<std::string>(), "template test");
}

TEST_F(BinaryIOTest, ArrayRoundTrip) {
    BinaryWriter writer(m_stream);
    std::vector<uint32_t> writeArray = {10, 20, 30, 40, 50};
    writer.writeArray(writeArray);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    std::vector<uint32_t> readArray = reader.readArray<uint32_t>();
    
    EXPECT_EQ(writeArray, readArray);
}

TEST_F(BinaryIOTest, EmptyArrayRoundTrip) {
    BinaryWriter writer(m_stream);
    std::vector<uint32_t> writeArray;
    writer.writeArray(writeArray);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    std::vector<uint32_t> readArray = reader.readArray<uint32_t>();
    
    EXPECT_TRUE(readArray.empty());
}

TEST_F(BinaryIOTest, ComplexDataRoundTrip) {
    BinaryWriter writer(m_stream);
    
    // Write complex data structure
    writer.writeUInt32(0x12345678);  // Magic number
    writer.writeString("Project Name");
    writer.writeFloat(1.5f);  // Version
    
    std::vector<Math::Vector3f> positions = {
        Math::Vector3f(0, 0, 0),
        Math::Vector3f(1, 1, 1),
        Math::Vector3f(2, 2, 2)
    };
    writer.writeArray(positions);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    
    EXPECT_EQ(reader.readUInt32(), 0x12345678);
    EXPECT_EQ(reader.readString(), "Project Name");
    EXPECT_FLOAT_EQ(reader.readFloat(), 1.5f);
    
    auto readPositions = reader.readArray<Math::Vector3f>();
    EXPECT_EQ(readPositions.size(), positions.size());
    for (size_t i = 0; i < positions.size(); ++i) {
        EXPECT_FLOAT_EQ(readPositions[i].x, positions[i].x);
        EXPECT_FLOAT_EQ(readPositions[i].y, positions[i].y);
        EXPECT_FLOAT_EQ(readPositions[i].z, positions[i].z);
    }
}

TEST_F(BinaryIOTest, ByteCountTracking) {
    BinaryWriter writer(m_stream);
    
    EXPECT_EQ(writer.getBytesWritten(), 0);
    
    writer.writeUInt8(42);
    EXPECT_EQ(writer.getBytesWritten(), 1);
    
    writer.writeUInt16(1234);
    EXPECT_EQ(writer.getBytesWritten(), 3);
    
    writer.writeUInt32(567890);
    EXPECT_EQ(writer.getBytesWritten(), 7);
    
    writer.writeString("test");  // 4 bytes length + 4 characters
    EXPECT_EQ(writer.getBytesWritten(), 15);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    
    EXPECT_EQ(reader.getBytesRead(), 0);
    
    reader.readUInt8();
    EXPECT_EQ(reader.getBytesRead(), 1);
    
    reader.readUInt16();
    EXPECT_EQ(reader.getBytesRead(), 3);
    
    reader.readUInt32();
    EXPECT_EQ(reader.getBytesRead(), 7);
    
    reader.readString();
    EXPECT_EQ(reader.getBytesRead(), 15);
}

TEST_F(BinaryIOTest, ErrorHandling) {
    BinaryReader reader(m_stream);  // Empty stream
    
    // Should handle reading from empty stream
    EXPECT_TRUE(reader.isValid());
    uint32_t value = reader.readUInt32();
    EXPECT_FALSE(reader.isValid());
    
    // After error, subsequent reads should also fail
    std::string str = reader.readString();
    EXPECT_TRUE(str.empty());
    EXPECT_FALSE(reader.isValid());
}

TEST_F(BinaryIOTest, IsAtEnd) {
    BinaryWriter writer(m_stream);
    writer.writeUInt32(42);
    
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    
    EXPECT_FALSE(reader.isAtEnd());
    reader.readUInt32();
    EXPECT_TRUE(reader.isAtEnd());
}

TEST_F(BinaryIOTest, Flush) {
    BinaryWriter writer(m_stream);
    writer.writeUInt32(42);
    writer.flush();
    
    // Verify data is written after flush
    m_stream.seekg(0);
    BinaryReader reader(m_stream);
    EXPECT_EQ(reader.readUInt32(), 42);
}

} // namespace FileIO
} // namespace VoxelEditor