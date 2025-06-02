# Build Instructions for Voxel Editor

## Prerequisites Check

Before building, ensure you have:
- CMake 3.16 or higher
- C++ compiler with C++20 support (GCC 10+, Clang 10+, or MSVC 2019+)
- Git
- OpenGL development libraries

### On macOS:
```bash
# Install Xcode Command Line Tools (if not already installed)
xcode-select --install

# Install CMake via Homebrew (if needed)
brew install cmake
```

### On Ubuntu/Debian:
```bash
sudo apt update
sudo apt install build-essential cmake git libgl1-mesa-dev libglu1-mesa-dev
```

### On Windows:
- Install Visual Studio 2019 or later with C++ desktop development
- Install CMake from https://cmake.org/download/

## Build Steps

### Option 1: Using the Build Script (Recommended)

```bash
# Clean build (removes existing build directory)
./build.sh

# The script will:
# 1. Create/clean build directory
# 2. Configure with CMake
# 3. Build with all available cores
# 4. Report success/failure
```

### Option 2: Manual Build

```bash
# 1. Clean existing build (optional)
rm -rf build

# 2. Create build directory
mkdir build
cd build

# 3. Configure with CMake
cmake .. -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=ON -DBUILD_CLI=ON

# 4. Build
# On Unix-like systems:
make -j$(nproc)

# On macOS:
make -j$(sysctl -n hw.ncpu)

# On Windows with Visual Studio:
cmake --build . --config Release
```

## Troubleshooting

### Issue: CMake version too old
```bash
# Check CMake version
cmake --version

# If < 3.16, update CMake:
# macOS: brew upgrade cmake
# Linux: Follow instructions at https://cmake.org/download/
```

### Issue: C++20 not supported
```bash
# Check compiler version
g++ --version  # For GCC
clang++ --version  # For Clang

# Update compiler if needed
# macOS: Update Xcode
# Linux: sudo apt install g++-10
```

### Issue: OpenGL not found
```bash
# macOS: Should be included with Xcode
# Linux: sudo apt install libgl1-mesa-dev
# Windows: Should be included with Visual Studio
```

### Issue: GLFW download fails
The project uses FetchContent to download GLFW. If this fails:
1. Check internet connection
2. Try manual build with system GLFW:
   ```bash
   # Install system GLFW
   # macOS: brew install glfw
   # Linux: sudo apt install libglfw3-dev
   
   # Then modify CMakeLists.txt to use find_package(glfw3) instead
   ```

### Issue: Missing dependencies
```bash
# The project will automatically download:
# - GLFW (window management)
# - GLM (math library)
# - Google Test (if BUILD_TESTS=ON)

# If downloads fail, check:
# 1. Internet connectivity
# 2. Firewall/proxy settings
# 3. Git configuration
```

## Verify Build

After successful build:

```bash
# Check if executable exists
ls -la build/bin/VoxelEditor_CLI

# Run the CLI
./build/bin/VoxelEditor_CLI

# Run tests
cd build
ctest --verbose

# Or run specific test
./foundation/math/tests/VoxelEditor_Math_Tests
```

## Quick Test

```bash
# Run integration test
./integration_test.sh

# Run validation
./validate_project.sh
```

## Common Build Configurations

### Debug Build
```bash
cmake .. -DCMAKE_BUILD_TYPE=Debug
```

### Release with Debug Info
```bash
cmake .. -DCMAKE_BUILD_TYPE=RelWithDebInfo
```

### Minimal Build (CLI only, no tests)
```bash
cmake .. -DBUILD_TESTS=OFF -DBUILD_CLI=ON
```

### Full Build (all components)
```bash
cmake .. -DBUILD_TESTS=ON -DBUILD_CLI=ON -DBUILD_QT_DESKTOP=OFF -DBUILD_VR=OFF
```

## Platform-Specific Notes

### macOS
- Requires macOS 10.15 or later
- OpenGL is deprecated but still works
- May see OpenGL deprecation warnings (safe to ignore)

### Linux
- Tested on Ubuntu 20.04+, Debian 10+
- Requires X11 for windowing
- Wayland support through XWayland

### Windows
- Use Visual Studio 2019 or later
- Open "x64 Native Tools Command Prompt"
- Use cmake --build instead of make

## Next Steps

After successful build:
1. Read `CLI_GUIDE.md` for usage instructions
2. Try the example commands in the guide
3. Run performance tests to verify system capabilities
4. Start building your voxel creations!

## Getting Help

If build fails:
1. Check the error message carefully
2. Verify all prerequisites are installed
3. Try a clean build (delete build directory)
4. Check if the issue is listed in troubleshooting above
5. Review the CMake output for missing dependencies