# Install script for directory: /Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "/usr/local")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Debug")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

# Set default install directory permissions.
if(NOT DEFINED CMAKE_OBJDUMP)
  set(CMAKE_OBJDUMP "/Library/Developer/CommandLineTools/usr/bin/objdump")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE FILE FILES
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/AdolcForward"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/AlignedVector3"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/ArpackSupport"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/AutoDiff"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/BVH"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/EulerAngles"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/FFT"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/IterativeSolvers"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/KroneckerProduct"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/LevenbergMarquardt"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/MatrixFunctions"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/MoreVectorization"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/MPRealSupport"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/NonLinearOptimization"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/NumericalDiff"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/OpenGLSupport"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/Polynomials"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/Skyline"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/SparseExtra"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/SpecialFunctions"
    "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/Splines"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Devel" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/eigen3/unsupported/Eigen" TYPE DIRECTORY FILES "/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-src/unsupported/Eigen/src" FILES_MATCHING REGEX "/[^/]*\\.h$")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for each subdirectory.
  include("/Users/michaelhalloran/cube_edit/build_test/_deps/eigen3-build/unsupported/Eigen/CXX11/cmake_install.cmake")

endif()

