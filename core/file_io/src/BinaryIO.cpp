#include "../include/file_io/BinaryIO.h"
#include <cstring>

namespace VoxelEditor {
namespace FileIO {

// BinaryWriter implementation
BinaryWriter::BinaryWriter(std::ostream& stream)
    : m_stream(stream)
    , m_bytesWritten(0)
    , m_valid(true) {
}

BinaryWriter::~BinaryWriter() {
    flush();
}

void BinaryWriter::writeUInt8(uint8_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeUInt16(uint16_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeUInt32(uint32_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeUInt64(uint64_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeInt8(int8_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeInt16(int16_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeInt32(int32_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeInt64(int64_t value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeFloat(float value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeDouble(double value) {
    writeRaw(&value, sizeof(value));
}

void BinaryWriter::writeBool(bool value) {
    uint8_t byte = value ? 1 : 0;
    writeUInt8(byte);
}

void BinaryWriter::writeString(const std::string& str) {
    writeUInt32(static_cast<uint32_t>(str.length()));
    writeBytes(str.data(), str.length());
}

void BinaryWriter::writeVector3f(const Math::Vector3f& vec) {
    writeFloat(vec.x);
    writeFloat(vec.y);
    writeFloat(vec.z);
}

void BinaryWriter::writeMatrix4f(const Math::Matrix4f& mat) {
    for (int i = 0; i < 16; ++i) {
        writeFloat(mat.m[i]);
    }
}

void BinaryWriter::writeBytes(const void* data, size_t size) {
    writeRaw(data, size);
}

void BinaryWriter::flush() {
    m_stream.flush();
}

void BinaryWriter::writeRaw(const void* data, size_t size) {
    if (!m_valid) return;
    
    m_stream.write(reinterpret_cast<const char*>(data), size);
    if (m_stream.good()) {
        m_bytesWritten += size;
    } else {
        m_valid = false;
    }
}

// BinaryReader implementation
BinaryReader::BinaryReader(std::istream& stream)
    : m_stream(stream)
    , m_bytesRead(0)
    , m_valid(true) {
}

BinaryReader::~BinaryReader() = default;

uint8_t BinaryReader::readUInt8() {
    uint8_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

uint16_t BinaryReader::readUInt16() {
    uint16_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

uint32_t BinaryReader::readUInt32() {
    uint32_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

uint64_t BinaryReader::readUInt64() {
    uint64_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

int8_t BinaryReader::readInt8() {
    int8_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

int16_t BinaryReader::readInt16() {
    int16_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

int32_t BinaryReader::readInt32() {
    int32_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

int64_t BinaryReader::readInt64() {
    int64_t value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

float BinaryReader::readFloat() {
    float value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

double BinaryReader::readDouble() {
    double value = 0;
    readRaw(&value, sizeof(value));
    return value;
}

bool BinaryReader::readBool() {
    return readUInt8() != 0;
}

std::string BinaryReader::readString() {
    uint32_t length = readUInt32();
    if (!m_valid || length > 1024 * 1024) { // Sanity check: max 1MB string
        m_valid = false;
        return "";
    }
    
    std::string str(length, '\0');
    readBytes(&str[0], length);
    return str;
}

Math::Vector3f BinaryReader::readVector3f() {
    Math::Vector3f vec;
    vec.x = readFloat();
    vec.y = readFloat();
    vec.z = readFloat();
    return vec;
}

Math::Matrix4f BinaryReader::readMatrix4f() {
    Math::Matrix4f mat;
    for (int i = 0; i < 16; ++i) {
        mat.m[i] = readFloat();
    }
    return mat;
}

void BinaryReader::readBytes(void* data, size_t size) {
    readRaw(data, size);
}

std::vector<uint8_t> BinaryReader::readBytes(size_t size) {
    std::vector<uint8_t> data(size);
    readBytes(data.data(), size);
    return data;
}

bool BinaryReader::isAtEnd() const {
    return m_stream.eof();
}

void BinaryReader::skip(size_t bytes) {
    if (!m_valid) return;
    
    m_stream.seekg(bytes, std::ios::cur);
    if (m_stream.good()) {
        m_bytesRead += bytes;
    } else {
        m_valid = false;
    }
}

size_t BinaryReader::remaining() const {
    if (!m_valid) return 0;
    
    std::streampos current = m_stream.tellg();
    m_stream.seekg(0, std::ios::end);
    std::streampos end = m_stream.tellg();
    m_stream.seekg(current);
    
    return static_cast<size_t>(end - current);
}

void BinaryReader::readRaw(void* data, size_t size) {
    if (!m_valid) return;
    
    m_stream.read(reinterpret_cast<char*>(data), size);
    if (m_stream.gcount() == static_cast<std::streamsize>(size)) {
        m_bytesRead += size;
    } else {
        m_valid = false;
        // Clear the data on failure
        std::memset(data, 0, size);
    }
}

} // namespace FileIO
} // namespace VoxelEditor