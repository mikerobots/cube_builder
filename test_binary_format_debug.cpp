#include <iostream>
#include <sstream>
#include "core/file_io/include/file_io/BinaryFormat.h"
#include "core/file_io/include/file_io/BinaryIO.h"
#include "core/file_io/include/file_io/Project.h"
#include "core/file_io/include/file_io/FileManager.h"

using namespace VoxelEditor::FileIO;

int main() {
    // Create a simple project
    Project project;
    project.initializeDefaults();
    project.metadata.name = "Debug Test";
    
    // Save without compression
    SaveOptions saveOptions = SaveOptions::Default();
    saveOptions.compress = false;
    
    std::stringstream stream;
    BinaryWriter writer(stream);
    BinaryFormat format;
    
    std::cout << "Writing project..." << std::endl;
    bool writeResult = format.writeProject(writer, project, saveOptions);
    std::cout << "Write result: " << writeResult << std::endl;
    std::cout << "Stream size: " << stream.tellp() << std::endl;
    
    // Try to read it back
    stream.seekg(0);
    BinaryReader reader(stream);
    Project loadedProject;
    LoadOptions loadOptions = LoadOptions::Default();
    
    std::cout << "\nReading project..." << std::endl;
    bool readResult = format.readProject(reader, loadedProject, loadOptions);
    std::cout << "Read result: " << readResult << std::endl;
    
    if (!readResult) {
        std::cout << "Error: " << static_cast<int>(format.getLastError()) 
                  << " - " << format.getLastErrorMessage() << std::endl;
    }
    
    return readResult ? 0 : 1;
}