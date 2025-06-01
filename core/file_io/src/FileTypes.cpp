#include "../include/file_io/FileTypes.h"
#include <sstream>
#include <iomanip>

namespace VoxelEditor {
namespace FileIO {

// FileVersion implementation
std::string FileVersion::toString() const {
    std::stringstream ss;
    ss << major << "." << minor << "." << patch << "." << build;
    return ss.str();
}

FileVersion FileVersion::fromString(const std::string& str) {
    FileVersion version;
    std::stringstream ss(str);
    char dot;
    
    ss >> version.major >> dot >> version.minor >> dot >> version.patch >> dot >> version.build;
    
    return version;
}

FileVersion FileVersion::Current() {
    return FileVersion{1, 0, 0, 0}; // Current version of the file format
}

} // namespace FileIO
} // namespace VoxelEditor