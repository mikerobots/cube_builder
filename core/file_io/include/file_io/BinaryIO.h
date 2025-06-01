#pragma once

#include <iostream>
#include <string>
#include <vector>
#include "Vector3f.h"
#include "Matrix4f.h"

namespace VoxelEditor {
namespace FileIO {

// Binary writer for serialization
class BinaryWriter {
public:
    explicit BinaryWriter(std::ostream& stream);
    ~BinaryWriter();
    
    // Basic types
    void writeUInt8(uint8_t value);
    void writeUInt16(uint16_t value);
    void writeUInt32(uint32_t value);
    void writeUInt64(uint64_t value);
    void writeInt8(int8_t value);
    void writeInt16(int16_t value);
    void writeInt32(int32_t value);
    void writeInt64(int64_t value);
    void writeFloat(float value);
    void writeDouble(double value);
    void writeBool(bool value);
    
    // Complex types
    void writeString(const std::string& str);
    void writeVector3f(const Math::Vector3f& vec);
    void writeMatrix4f(const Math::Matrix4f& mat);
    void writeBytes(const void* data, size_t size);
    
    // Array writing
    template<typename T>
    void writeArray(const std::vector<T>& array) {
        writeUInt32(static_cast<uint32_t>(array.size()));
        for (const auto& item : array) {
            write(item);
        }
    }
    
    // Generic write (requires specialization)
    template<typename T>
    void write(const T& value);
    
    // Stream management
    size_t getBytesWritten() const { return m_bytesWritten; }
    bool isValid() const { return m_valid && m_stream.good(); }
    void flush();
    
private:
    std::ostream& m_stream;
    size_t m_bytesWritten;
    bool m_valid;
    
    void writeRaw(const void* data, size_t size);
};

// Binary reader for deserialization
class BinaryReader {
public:
    explicit BinaryReader(std::istream& stream);
    ~BinaryReader();
    
    // Basic types
    uint8_t readUInt8();
    uint16_t readUInt16();
    uint32_t readUInt32();
    uint64_t readUInt64();
    int8_t readInt8();
    int16_t readInt16();
    int32_t readInt32();
    int64_t readInt64();
    float readFloat();
    double readDouble();
    bool readBool();
    
    // Complex types
    std::string readString();
    Math::Vector3f readVector3f();
    Math::Matrix4f readMatrix4f();
    void readBytes(void* data, size_t size);
    std::vector<uint8_t> readBytes(size_t size);
    
    // Array reading
    template<typename T>
    std::vector<T> readArray() {
        uint32_t size = readUInt32();
        std::vector<T> array;
        array.reserve(size);
        for (uint32_t i = 0; i < size; ++i) {
            array.push_back(read<T>());
        }
        return array;
    }
    
    // Generic read (requires specialization)
    template<typename T>
    T read();
    
    // Stream management
    size_t getBytesRead() const { return m_bytesRead; }
    bool isValid() const { return m_valid && m_stream.good(); }
    bool isAtEnd() const;
    void skip(size_t bytes);
    size_t remaining() const;
    
private:
    std::istream& m_stream;
    size_t m_bytesRead;
    bool m_valid;
    
    void readRaw(void* data, size_t size);
};

// Template specializations for common types
template<> inline void BinaryWriter::write(const uint8_t& value) { writeUInt8(value); }
template<> inline void BinaryWriter::write(const uint16_t& value) { writeUInt16(value); }
template<> inline void BinaryWriter::write(const uint32_t& value) { writeUInt32(value); }
template<> inline void BinaryWriter::write(const uint64_t& value) { writeUInt64(value); }
template<> inline void BinaryWriter::write(const int8_t& value) { writeInt8(value); }
template<> inline void BinaryWriter::write(const int16_t& value) { writeInt16(value); }
template<> inline void BinaryWriter::write(const int32_t& value) { writeInt32(value); }
template<> inline void BinaryWriter::write(const int64_t& value) { writeInt64(value); }
template<> inline void BinaryWriter::write(const float& value) { writeFloat(value); }
template<> inline void BinaryWriter::write(const double& value) { writeDouble(value); }
template<> inline void BinaryWriter::write(const bool& value) { writeBool(value); }
template<> inline void BinaryWriter::write(const std::string& value) { writeString(value); }
template<> inline void BinaryWriter::write(const Math::Vector3f& value) { writeVector3f(value); }
template<> inline void BinaryWriter::write(const Math::Matrix4f& value) { writeMatrix4f(value); }

template<> inline uint8_t BinaryReader::read<uint8_t>() { return readUInt8(); }
template<> inline uint16_t BinaryReader::read<uint16_t>() { return readUInt16(); }
template<> inline uint32_t BinaryReader::read<uint32_t>() { return readUInt32(); }
template<> inline uint64_t BinaryReader::read<uint64_t>() { return readUInt64(); }
template<> inline int8_t BinaryReader::read<int8_t>() { return readInt8(); }
template<> inline int16_t BinaryReader::read<int16_t>() { return readInt16(); }
template<> inline int32_t BinaryReader::read<int32_t>() { return readInt32(); }
template<> inline int64_t BinaryReader::read<int64_t>() { return readInt64(); }
template<> inline float BinaryReader::read<float>() { return readFloat(); }
template<> inline double BinaryReader::read<double>() { return readDouble(); }
template<> inline bool BinaryReader::read<bool>() { return readBool(); }
template<> inline std::string BinaryReader::read<std::string>() { return readString(); }
template<> inline Math::Vector3f BinaryReader::read<Math::Vector3f>() { return readVector3f(); }
template<> inline Math::Matrix4f BinaryReader::read<Math::Matrix4f>() { return readMatrix4f(); }

} // namespace FileIO
} // namespace VoxelEditor