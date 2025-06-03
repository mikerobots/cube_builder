/*
 @licstart  The following is the entire license notice for the JavaScript code in this file.

 The MIT License (MIT)

 Copyright (C) 1997-2020 by Dimitri van Heesch

 Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 and associated documentation files (the "Software"), to deal in the Software without restriction,
 including without limitation the rights to use, copy, modify, merge, publish, distribute,
 sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is
 furnished to do so, subject to the following conditions:

 The above copyright notice and this permission notice shall be included in all copies or
 substantial portions of the Software.

 THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

 @licend  The above is the entire license notice for the JavaScript code in this file
*/
var NAVTREE =
[
  [ "Voxel Editor", "index.html", [
    [ "Voxel Editor CLI Guide", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html", [
      [ "Quick Start", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md1", null ],
      [ "Basic Workflow", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md2", [
        [ "Starting a New Project", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md3", null ],
        [ "Building with Voxels", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md4", null ],
        [ "Using the Mouse", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md5", null ],
        [ "Working with Groups", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md6", null ],
        [ "Camera Control", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md7", null ],
        [ "File Operations", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md8", null ]
      ] ],
      [ "Advanced Tips", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md9", [
        [ "Efficient Building", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md10", null ],
        [ "Keyboard Shortcuts", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md11", null ],
        [ "Selection Techniques", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md12", null ],
        [ "Undo/Redo", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md13", null ]
      ] ],
      [ "Command Reference", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md14", [
        [ "Essential Commands", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md15", null ],
        [ "View Presets", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md16", null ],
        [ "Resolution Sizes", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md17", null ]
      ] ],
      [ "Troubleshooting", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md18", [
        [ "Can't see my voxels?", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md19", null ],
        [ "Mouse not working?", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md20", null ],
        [ "Performance issues?", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md21", null ]
      ] ],
      [ "Example Projects", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md22", [
        [ "Simple House", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md23", null ],
        [ "Detailed Sculpture", "md_apps_2cli_2_c_l_i___g_u_i_d_e.html#autotoc_md24", null ]
      ] ]
    ] ],
    [ "Voxel Editor Architecture", "md__a_r_c_h_i_t_e_c_t_u_r_e.html", [
      [ "Overview", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md26", null ],
      [ "System Architecture", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md27", null ],
      [ "Core Library Subsystems", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md28", [
        [ "1. Voxel Data Management (<span class=\"tt\">VoxelDataManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md29", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md30", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md31", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md32", null ]
        ] ],
        [ "2. Group Management System (<span class=\"tt\">GroupManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md33", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md34", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md35", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md36", null ]
        ] ],
        [ "3. Surface Generation (<span class=\"tt\">SurfaceGenerator</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md37", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md38", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md39", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md40", null ]
        ] ],
        [ "4. Selection System (<span class=\"tt\">SelectionManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md41", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md42", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md43", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md44", null ]
        ] ],
        [ "5. Camera System (<span class=\"tt\">CameraManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md45", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md46", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md47", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md48", null ]
        ] ],
        [ "6. Rendering Engine (<span class=\"tt\">RenderEngine</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md49", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md50", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md51", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md52", null ]
        ] ],
        [ "7. Visual Feedback System (<span class=\"tt\">FeedbackRenderer</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md53", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md54", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md55", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md56", null ]
        ] ],
        [ "8. Input System (<span class=\"tt\">InputManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md57", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md58", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md59", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md60", null ]
        ] ],
        [ "9. File I/O System (<span class=\"tt\">FileManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md61", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md62", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md63", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md64", null ]
        ] ],
        [ "10. Undo/Redo System (<span class=\"tt\">HistoryManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md65", [
          [ "Components:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md66", null ],
          [ "Key Classes:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md67", null ],
          [ "Tests:", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md68", null ]
        ] ]
      ] ],
      [ "Foundation Layer", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md69", [
        [ "1. Math Utilities (<span class=\"tt\">MathUtils</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md70", null ],
        [ "2. Memory Pool (<span class=\"tt\">MemoryManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md71", null ],
        [ "3. Event System (<span class=\"tt\">EventDispatcher</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md72", null ],
        [ "4. Configuration (<span class=\"tt\">ConfigManager</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md73", null ],
        [ "5. Logging (<span class=\"tt\">Logger</span>)", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md74", null ]
      ] ],
      [ "Application Layer", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md75", [
        [ "Command Line Application", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md76", null ],
        [ "Qt Desktop Application", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md77", null ],
        [ "VR Application", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md78", null ]
      ] ],
      [ "Testing Strategy", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md79", null ],
      [ "Dependencies and Build", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md80", [
        [ "External Dependencies", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md81", null ],
        [ "Internal Dependencies", "md__a_r_c_h_i_t_e_c_t_u_r_e.html#autotoc_md82", null ]
      ] ]
    ] ],
    [ "Build Instructions for Voxel Editor", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html", [
      [ "Prerequisites Check", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md84", [
        [ "On macOS:", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md85", null ],
        [ "On Ubuntu/Debian:", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md86", null ],
        [ "On Windows:", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md87", null ]
      ] ],
      [ "Build Steps", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md88", [
        [ "Option 1: Using the Build Script (Recommended)", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md89", null ],
        [ "Option 2: Manual Build", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md90", null ]
      ] ],
      [ "Troubleshooting", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md91", [
        [ "Issue: CMake version too old", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md92", null ],
        [ "Issue: C++20 not supported", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md93", null ],
        [ "Issue: OpenGL not found", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md94", null ],
        [ "Issue: GLFW download fails", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md95", null ],
        [ "Issue: Missing dependencies", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md96", null ]
      ] ],
      [ "Verify Build", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md97", null ],
      [ "Quick Test", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md98", null ],
      [ "Common Build Configurations", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md99", [
        [ "Debug Build", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md100", null ],
        [ "Release with Debug Info", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md101", null ],
        [ "Minimal Build (CLI only, no tests)", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md102", null ],
        [ "Full Build (all components)", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md103", null ]
      ] ],
      [ "Platform-Specific Notes", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md104", [
        [ "macOS", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md105", null ],
        [ "Linux", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md106", null ],
        [ "Windows", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md107", null ]
      ] ],
      [ "Next Steps", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md108", null ],
      [ "Getting Help", "md__b_u_i_l_d___i_n_s_t_r_u_c_t_i_o_n_s.html#autotoc_md109", null ]
    ] ],
    [ "CLAUDE", "md__c_l_a_u_d_e.html", null ],
    [ "Camera System Subsystem", "md_core_2camera_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md112", null ],
      [ "Key Components", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md113", [
        [ "CameraManager", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md114", null ],
        [ "OrbitCamera", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md115", null ],
        [ "ViewPresets", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md116", null ],
        [ "CameraAnimator", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md117", null ]
      ] ],
      [ "Interface Design", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md118", null ],
      [ "Data Structures", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md119", [
        [ "ViewType", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md120", null ],
        [ "ProjectionType", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md121", null ],
        [ "CameraState", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md122", null ],
        [ "CameraConstraints", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md123", null ]
      ] ],
      [ "Camera Implementation", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md124", [
        [ "OrbitCamera", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md125", null ],
        [ "ViewPresets", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md126", null ],
        [ "CameraAnimator", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md127", null ]
      ] ],
      [ "Easing Functions", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md128", null ],
      [ "Input Integration", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md129", [
        [ "Mouse Controls", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md130", null ],
        [ "Keyboard Shortcuts", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md131", null ],
        [ "Touch Controls (Qt)", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md132", null ],
        [ "VR Controls", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md133", null ]
      ] ],
      [ "Performance Considerations", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md134", [
        [ "Matrix Caching", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md135", null ],
        [ "Animation Optimization", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md136", null ],
        [ "Constraint Validation", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md137", null ]
      ] ],
      [ "Events", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md138", [
        [ "CameraChanged Event", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md139", null ],
        [ "ViewChanged Event", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md140", null ]
      ] ],
      [ "Testing Strategy", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md141", [
        [ "Unit Tests", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md142", null ],
        [ "Integration Tests", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md143", null ],
        [ "Visual Tests", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md144", null ],
        [ "Performance Tests", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md145", null ]
      ] ],
      [ "Dependencies", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md146", null ],
      [ "Platform Considerations", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md147", [
        [ "Desktop", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md148", null ],
        [ "Mobile/Touch", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md149", null ],
        [ "VR", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md150", null ]
      ] ],
      [ "File I/O Integration", "md_core_2camera_2_d_e_s_i_g_n.html#autotoc_md151", null ]
    ] ],
    [ "File I/O System Subsystem", "md_core_2file__io_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md153", null ],
      [ "Key Components", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md154", [
        [ "FileManager", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md155", null ],
        [ "BinaryFormat", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md156", null ],
        [ "STLExporter", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md157", null ],
        [ "FileVersioning", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md158", null ],
        [ "Compression", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md159", null ]
      ] ],
      [ "Interface Design", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md160", null ],
      [ "Data Structures", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md161", [
        [ "Project Structure", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md162", null ],
        [ "File Options", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md163", null ]
      ] ],
      [ "Binary Format Implementation", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md164", [
        [ "File Structure", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md165", null ],
        [ "BinaryFormat", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md166", null ],
        [ "Binary I/O Utilities", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md167", null ]
      ] ],
      [ "STL Export Implementation", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md168", [
        [ "STLExporter", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md169", null ]
      ] ],
      [ "File Versioning", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md170", [
        [ "FileVersioning", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md171", null ]
      ] ],
      [ "Compression Implementation", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md172", [
        [ "Compression", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md173", null ]
      ] ],
      [ "Auto-Save and Backup", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md174", [
        [ "AutoSave", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md175", null ]
      ] ],
      [ "Error Handling and Validation", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md176", [
        [ "File Validation", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md177", null ]
      ] ],
      [ "Testing Strategy", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md178", [
        [ "Unit Tests", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md179", null ],
        [ "Integration Tests", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md180", null ],
        [ "Performance Tests", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md181", null ],
        [ "Stress Tests", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md182", null ]
      ] ],
      [ "Dependencies", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md183", null ],
      [ "Platform Considerations", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md184", [
        [ "File System Integration", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md185", null ],
        [ "Performance Optimization", "md_core_2file__io_2_d_e_s_i_g_n.html#autotoc_md186", null ]
      ] ]
    ] ],
    [ "Group Management Subsystem", "md_core_2groups_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md188", null ],
      [ "Key Components", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md189", [
        [ "GroupManager", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md190", null ],
        [ "VoxelGroup", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md191", null ],
        [ "GroupHierarchy", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md192", null ],
        [ "GroupOperations", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md193", null ]
      ] ],
      [ "Interface Design", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md194", null ],
      [ "Data Structures", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md195", [
        [ "GroupId", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md196", null ],
        [ "VoxelId", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md197", null ],
        [ "GroupInfo", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md198", null ],
        [ "GroupMetadata", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md199", null ]
      ] ],
      [ "Group Operations", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md200", [
        [ "Move Operation", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md201", null ],
        [ "Copy Operation", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md202", null ]
      ] ],
      [ "Visual Properties", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md203", [
        [ "Group Colors", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md204", null ],
        [ "Group Indicators", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md205", null ]
      ] ],
      [ "Hierarchy Management", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md206", [
        [ "Rules", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md207", null ],
        [ "Hierarchy Operations", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md208", null ]
      ] ],
      [ "Events", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md209", [
        [ "GroupCreated Event", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md210", null ],
        [ "GroupModified Event", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md211", null ],
        [ "GroupDeleted Event", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md212", null ]
      ] ],
      [ "Performance Considerations", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md213", [
        [ "Memory Optimization", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md214", null ],
        [ "Operation Efficiency", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md215", null ],
        [ "Rendering Integration", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md216", null ]
      ] ],
      [ "Testing Strategy", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md217", [
        [ "Unit Tests", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md218", null ],
        [ "Integration Tests", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md219", null ],
        [ "Performance Tests", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md220", null ]
      ] ],
      [ "Dependencies", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md221", null ],
      [ "File I/O Integration", "md_core_2groups_2_d_e_s_i_g_n.html#autotoc_md222", null ]
    ] ],
    [ "Input System Subsystem", "md_core_2input_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md224", null ],
      [ "Key Components", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md225", [
        [ "InputManager", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md226", null ],
        [ "MouseHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md227", null ],
        [ "KeyboardHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md228", null ],
        [ "TouchHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md229", null ],
        [ "VRInputHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md230", null ]
      ] ],
      [ "Interface Design", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md231", null ],
      [ "Data Structures", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md232", [
        [ "Input Events", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md233", null ],
        [ "Input Mapping", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md234", null ]
      ] ],
      [ "Mouse Input Processing", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md235", [
        [ "MouseHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md236", null ],
        [ "Mouse Actions", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md237", null ]
      ] ],
      [ "Keyboard Input Processing", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md238", [
        [ "KeyboardHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md239", null ],
        [ "Key Combinations", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md240", null ]
      ] ],
      [ "Touch Input Processing", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md241", [
        [ "TouchHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md242", null ]
      ] ],
      [ "VR Input Processing", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md243", [
        [ "VRInputHandler", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md244", null ]
      ] ],
      [ "Action System", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md245", [
        [ "Action Processing", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md246", null ]
      ] ],
      [ "Platform Integration", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md247", [
        [ "Desktop Integration", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md248", null ],
        [ "VR Integration", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md249", null ]
      ] ],
      [ "Testing Strategy", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md250", [
        [ "Unit Tests", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md251", null ],
        [ "Integration Tests", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md252", null ],
        [ "Performance Tests", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md253", null ]
      ] ],
      [ "Dependencies", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md254", null ],
      [ "Configuration", "md_core_2input_2_d_e_s_i_g_n.html#autotoc_md255", null ]
    ] ],
    [ "Rendering Engine Subsystem", "md_core_2rendering_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md257", null ],
      [ "Key Components", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md258", [
        [ "RenderEngine", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md259", null ],
        [ "OpenGLRenderer", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md260", null ],
        [ "ShaderManager", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md261", null ],
        [ "RenderState", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md262", null ]
      ] ],
      [ "Interface Design", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md263", null ],
      [ "Data Structures", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md264", [
        [ "RenderMode", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md265", null ],
        [ "RenderConfig", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md266", null ],
        [ "Material", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md267", null ],
        [ "RenderStats", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md268", null ]
      ] ],
      [ "OpenGL Abstraction", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md269", [
        [ "OpenGLRenderer", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md270", null ],
        [ "ShaderManager", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md271", null ]
      ] ],
      [ "Rendering Pipeline", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md272", [
        [ "Frame Rendering", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md273", null ],
        [ "Voxel Rendering", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md274", null ]
      ] ],
      [ "Performance Optimization", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md275", [
        [ "State Management", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md276", null ],
        [ "Instanced Rendering", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md277", null ]
      ] ],
      [ "Debug and Profiling", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md278", [
        [ "Debug Rendering", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md279", null ],
        [ "GPU Profiling", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md280", null ]
      ] ],
      [ "Testing Strategy", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md281", [
        [ "Unit Tests", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md282", null ],
        [ "Integration Tests", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md283", null ],
        [ "Visual Tests", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md284", null ]
      ] ],
      [ "Dependencies", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md285", null ],
      [ "Platform Considerations", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md286", [
        [ "Desktop OpenGL", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md287", null ],
        [ "Mobile OpenGL ES", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md288", null ],
        [ "VR Rendering", "md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md289", null ]
      ] ]
    ] ],
    [ "Selection System Subsystem", "md_core_2selection_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md291", null ],
      [ "Key Components", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md292", [
        [ "SelectionManager", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md293", null ],
        [ "SelectionSet", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md294", null ],
        [ "SelectionOperations", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md295", null ],
        [ "SelectionRenderer", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md296", null ]
      ] ],
      [ "Interface Design", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md297", null ],
      [ "Data Structures", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md298", [
        [ "VoxelId", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md299", null ],
        [ "SelectionSet", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md300", null ],
        [ "SelectionMode", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md301", null ]
      ] ],
      [ "Selection Algorithms", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md302", [
        [ "Flood Fill Selection", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md303", null ],
        [ "Box Selection", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md304", null ],
        [ "Sphere Selection", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md305", null ]
      ] ],
      [ "Visual Feedback", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md306", [
        [ "Selection Rendering", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md307", null ],
        [ "Visual Indicators", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md308", null ]
      ] ],
      [ "Selection Operations", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md309", [
        [ "Move Operation", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md310", null ],
        [ "Copy Operation", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md311", null ],
        [ "Delete Operation", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md312", null ]
      ] ],
      [ "Performance Optimization", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md313", [
        [ "Spatial Indexing", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md314", null ],
        [ "Memory Management", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md315", null ],
        [ "Rendering Optimization", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md316", null ]
      ] ],
      [ "Events", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md317", [
        [ "SelectionChanged Event", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md318", null ],
        [ "SelectionOperationEvent", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md319", null ]
      ] ],
      [ "Testing Strategy", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md320", [
        [ "Unit Tests", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md321", null ],
        [ "Integration Tests", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md322", null ],
        [ "Visual Tests", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md323", null ],
        [ "Performance Tests", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md324", null ]
      ] ],
      [ "Dependencies", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md325", null ],
      [ "Input Integration", "md_core_2selection_2_d_e_s_i_g_n.html#autotoc_md326", null ]
    ] ],
    [ "Surface Generation Subsystem", "md_core_2surface__gen_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md328", null ],
      [ "Key Components", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md329", [
        [ "SurfaceGenerator", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md330", null ],
        [ "DualContouring", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md331", null ],
        [ "MeshBuilder", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md332", null ],
        [ "LODManager", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md333", null ],
        [ "MeshCache", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md334", null ]
      ] ],
      [ "Interface Design", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md335", null ],
      [ "Data Structures", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md336", [
        [ "Mesh", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md337", null ],
        [ "SurfaceSettings", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md338", null ],
        [ "ExportQuality", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md339", null ],
        [ "HermiteData", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md340", null ]
      ] ],
      [ "Dual Contouring Algorithm", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md341", [
        [ "Core Implementation", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md342", null ],
        [ "Sharp Feature Preservation", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md343", null ]
      ] ],
      [ "Level of Detail (LOD)", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md344", [
        [ "LOD Levels", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md345", null ],
        [ "LOD Manager Implementation", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md346", null ]
      ] ],
      [ "Performance Optimization", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md347", [
        [ "Mesh Caching", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md348", null ],
        [ "Background Generation", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md349", null ],
        [ "Memory Management", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md350", null ]
      ] ],
      [ "Real-time vs Export Quality", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md351", [
        [ "Real-time Generation", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md352", null ],
        [ "Export Quality", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md353", null ]
      ] ],
      [ "Integration Points", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md354", [
        [ "Voxel Data Integration", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md355", null ],
        [ "Rendering Integration", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md356", null ]
      ] ],
      [ "Testing Strategy", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md357", [
        [ "Unit Tests", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md358", null ],
        [ "Visual Tests", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md359", null ],
        [ "Performance Tests", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md360", null ],
        [ "Integration Tests", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md361", null ]
      ] ],
      [ "Dependencies", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md362", null ],
      [ "Export Integration", "md_core_2surface__gen_2_d_e_s_i_g_n.html#autotoc_md363", null ]
    ] ],
    [ "Undo/Redo System Subsystem", "md_core_2undo__redo_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md365", null ],
      [ "Key Components", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md366", [
        [ "HistoryManager", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md367", null ],
        [ "Command", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md368", null ],
        [ "StateSnapshot", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md369", null ],
        [ "CommandComposer", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md370", null ]
      ] ],
      [ "Interface Design", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md371", null ],
      [ "Command Pattern Implementation", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md372", [
        [ "Base Command Interface", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md373", null ],
        [ "Voxel Commands", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md374", null ],
        [ "Selection Commands", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md375", null ],
        [ "Group Commands", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md376", null ],
        [ "Camera Commands", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md377", null ]
      ] ],
      [ "Composite Commands", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md378", [
        [ "CompositeCommand", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md379", null ],
        [ "Transaction Support", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md380", null ]
      ] ],
      [ "State Snapshots", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md381", [
        [ "StateSnapshot", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md382", null ],
        [ "Incremental Snapshots", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md383", null ]
      ] ],
      [ "Memory Management", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md384", [
        [ "Memory Optimization", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md385", null ],
        [ "Command Compression", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md386", null ]
      ] ],
      [ "Performance Considerations", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md387", [
        [ "Fast Execution", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md388", null ],
        [ "Memory Efficiency", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md389", null ],
        [ "Responsiveness", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md390", null ]
      ] ],
      [ "Events and Callbacks", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md391", [
        [ "Event Types", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md392", null ]
      ] ],
      [ "Testing Strategy", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md393", [
        [ "Unit Tests", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md394", null ],
        [ "Integration Tests", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md395", null ],
        [ "Stress Tests", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md396", null ],
        [ "Performance Tests", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md397", null ]
      ] ],
      [ "Dependencies", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md398", null ],
      [ "Platform Considerations", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md399", [
        [ "Memory Constraints", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md400", null ],
        [ "Performance Optimization", "md_core_2undo__redo_2_d_e_s_i_g_n.html#autotoc_md401", null ]
      ] ]
    ] ],
    [ "Visual Feedback Subsystem", "md_core_2visual__feedback_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md403", null ],
      [ "Key Components", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md404", [
        [ "FeedbackRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md405", null ],
        [ "HighlightRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md406", null ],
        [ "OutlineRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md407", null ],
        [ "OverlayRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md408", null ]
      ] ],
      [ "Interface Design", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md409", null ],
      [ "Data Structures", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md410", [
        [ "Face", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md411", null ],
        [ "HighlightStyle", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md412", null ],
        [ "OutlineStyle", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md413", null ]
      ] ],
      [ "Highlight Rendering", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md414", [
        [ "HighlightRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md415", null ],
        [ "Face Detection", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md416", null ]
      ] ],
      [ "Outline Rendering", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md417", [
        [ "OutlineRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md418", null ],
        [ "Voxel Outline Generation", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md419", null ]
      ] ],
      [ "Overlay Rendering", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md420", [
        [ "OverlayRenderer", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md421", null ],
        [ "Text Rendering", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md422", null ]
      ] ],
      [ "Animation System", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md423", [
        [ "Animation Controllers", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md424", null ]
      ] ],
      [ "Performance Optimization", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md425", [
        [ "Instanced Rendering", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md426", null ],
        [ "Level of Detail", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md427", null ],
        [ "Memory Management", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md428", null ]
      ] ],
      [ "Testing Strategy", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md429", [
        [ "Unit Tests", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md430", null ],
        [ "Visual Tests", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md431", null ],
        [ "Integration Tests", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md432", null ],
        [ "Performance Tests", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md433", null ]
      ] ],
      [ "Dependencies", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md434", null ],
      [ "Platform Considerations", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md435", [
        [ "Desktop", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md436", null ],
        [ "Mobile/Touch", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md437", null ],
        [ "VR", "md_core_2visual__feedback_2_d_e_s_i_g_n.html#autotoc_md438", null ]
      ] ]
    ] ],
    [ "Voxel Data Management Subsystem", "md_core_2voxel__data_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md440", null ],
      [ "Key Components", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md441", [
        [ "VoxelDataManager", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md442", null ],
        [ "MultiResolutionGrid", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md443", null ],
        [ "VoxelGrid", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md444", null ],
        [ "SparseOctree", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md445", null ],
        [ "WorkspaceManager", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md446", null ]
      ] ],
      [ "Interface Design", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md447", null ],
      [ "Data Structures", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md448", [
        [ "Voxel Resolution Enum", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md449", null ],
        [ "Voxel Position", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md450", null ]
      ] ],
      [ "Memory Management", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md451", [
        [ "Sparse Storage Strategy", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md452", null ],
        [ "Memory Pressure Handling", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md453", null ],
        [ "Performance Targets", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md454", null ]
      ] ],
      [ "Events", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md455", [
        [ "VoxelChanged Event", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md456", null ],
        [ "ResolutionChanged Event", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md457", null ],
        [ "WorkspaceResized Event", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md458", null ]
      ] ],
      [ "Testing Strategy", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md459", [
        [ "Unit Tests", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md460", null ],
        [ "Performance Tests", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md461", null ],
        [ "Stress Tests", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md462", null ]
      ] ],
      [ "Dependencies", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md463", null ],
      [ "File I/O Integration", "md_core_2voxel__data_2_d_e_s_i_g_n.html#autotoc_md464", null ]
    ] ],
    [ "Voxel Editor Design Document", "md__d_e_s_i_g_n.html", [
      [ "Project Overview", "md__d_e_s_i_g_n.html#autotoc_md466", null ],
      [ "Core Architecture", "md__d_e_s_i_g_n.html#autotoc_md467", [
        [ "Shared Library (Core Engine)", "md__d_e_s_i_g_n.html#autotoc_md468", null ],
        [ "Application Layer", "md__d_e_s_i_g_n.html#autotoc_md469", null ]
      ] ],
      [ "Technical Constraints", "md__d_e_s_i_g_n.html#autotoc_md470", [
        [ "Performance Requirements", "md__d_e_s_i_g_n.html#autotoc_md471", null ],
        [ "Platform Constraints", "md__d_e_s_i_g_n.html#autotoc_md472", null ],
        [ "Build System", "md__d_e_s_i_g_n.html#autotoc_md473", null ],
        [ "Development Constraints", "md__d_e_s_i_g_n.html#autotoc_md474", null ],
        [ "Development Priority", "md__d_e_s_i_g_n.html#autotoc_md475", null ]
      ] ],
      [ "Core Components", "md__d_e_s_i_g_n.html#autotoc_md476", [
        [ "1. Voxel Data Management", "md__d_e_s_i_g_n.html#autotoc_md477", null ],
        [ "2. Vertex Selection System", "md__d_e_s_i_g_n.html#autotoc_md478", null ],
        [ "3. Voxel Grouping System", "md__d_e_s_i_g_n.html#autotoc_md479", null ],
        [ "4. Surface Generation", "md__d_e_s_i_g_n.html#autotoc_md480", null ],
        [ "5. Rendering Engine", "md__d_e_s_i_g_n.html#autotoc_md481", null ],
        [ "6. Camera System", "md__d_e_s_i_g_n.html#autotoc_md482", null ],
        [ "7. Visual Feedback System", "md__d_e_s_i_g_n.html#autotoc_md483", null ],
        [ "8. File I/O System", "md__d_e_s_i_g_n.html#autotoc_md484", null ],
        [ "9. Input Abstraction", "md__d_e_s_i_g_n.html#autotoc_md485", null ]
      ] ],
      [ "Command Line Tool Interface", "md__d_e_s_i_g_n.html#autotoc_md486", [
        [ "Interactive Commands", "md__d_e_s_i_g_n.html#autotoc_md487", null ],
        [ "Auto-completion", "md__d_e_s_i_g_n.html#autotoc_md488", null ],
        [ "Mouse Interaction", "md__d_e_s_i_g_n.html#autotoc_md489", null ]
      ] ],
      [ "Design Complete", "md__d_e_s_i_g_n.html#autotoc_md490", null ]
    ] ],
    [ "Configuration Management Foundation", "md_foundation_2config_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md492", null ],
      [ "Key Components", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md493", [
        [ "ConfigManager", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md494", null ],
        [ "ConfigValue", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md495", null ],
        [ "ConfigSchema", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md496", null ]
      ] ],
      [ "Interface Design", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md497", null ],
      [ "Dependencies", "md_foundation_2config_2_d_e_s_i_g_n.html#autotoc_md498", null ]
    ] ],
    [ "Event System Foundation", "md_foundation_2events_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md500", null ],
      [ "Key Components", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md501", [
        [ "EventDispatcher", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md502", null ],
        [ "Event Types", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md503", null ],
        [ "EventHandler", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md504", null ]
      ] ],
      [ "Interface Design", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md505", null ],
      [ "Dependencies", "md_foundation_2events_2_d_e_s_i_g_n.html#autotoc_md506", null ]
    ] ],
    [ "Foundation/Events TODO", "md_foundation_2events_2_t_o_d_o.html", [
      [ "Status: Starting Implementation", "md_foundation_2events_2_t_o_d_o.html#autotoc_md508", [
        [ "✅ Completed", "md_foundation_2events_2_t_o_d_o.html#autotoc_md509", null ],
        [ "🎉 Status: FOUNDATION/EVENTS COMPLETE", "md_foundation_2events_2_t_o_d_o.html#autotoc_md510", null ],
        [ "Next Steps", "md_foundation_2events_2_t_o_d_o.html#autotoc_md511", null ]
      ] ]
    ] ],
    [ "Logging System Foundation", "md_foundation_2logging_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md513", null ],
      [ "Key Components", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md514", [
        [ "Logger", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md515", null ],
        [ "LogFormatter", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md516", null ],
        [ "PerformanceProfiler", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md517", null ]
      ] ],
      [ "Interface Design", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md518", null ],
      [ "Dependencies", "md_foundation_2logging_2_d_e_s_i_g_n.html#autotoc_md519", null ]
    ] ],
    [ "Math Utilities Foundation", "md_foundation_2math_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md521", null ],
      [ "Key Components", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md522", [
        [ "Vector Operations", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md523", null ],
        [ "Matrix Operations", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md524", null ],
        [ "Geometric Utilities", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md525", null ],
        [ "Spatial Mathematics", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md526", null ]
      ] ],
      [ "Interface Design", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md527", null ],
      [ "Dependencies", "md_foundation_2math_2_d_e_s_i_g_n.html#autotoc_md528", null ]
    ] ],
    [ "Foundation/Math TODO", "md_foundation_2math_2_t_o_d_o.html", [
      [ "Status: Starting Implementation", "md_foundation_2math_2_t_o_d_o.html#autotoc_md530", [
        [ "✅ Completed", "md_foundation_2math_2_t_o_d_o.html#autotoc_md531", null ],
        [ "🎉 Status: FOUNDATION/MATH COMPLETE", "md_foundation_2math_2_t_o_d_o.html#autotoc_md532", null ],
        [ "📋 TODO", "md_foundation_2math_2_t_o_d_o.html#autotoc_md533", null ],
        [ "Next Steps", "md_foundation_2math_2_t_o_d_o.html#autotoc_md534", null ]
      ] ]
    ] ],
    [ "Memory Management Foundation", "md_foundation_2memory_2_d_e_s_i_g_n.html", [
      [ "Purpose", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md536", null ],
      [ "Key Components", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md537", [
        [ "MemoryPool", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md538", null ],
        [ "MemoryTracker", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md539", null ],
        [ "MemoryOptimizer", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md540", null ]
      ] ],
      [ "Interface Design", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md541", null ],
      [ "Dependencies", "md_foundation_2memory_2_d_e_s_i_g_n.html#autotoc_md542", null ]
    ] ],
    [ "Foundation/Memory TODO", "md_foundation_2memory_2_t_o_d_o.html", [
      [ "Status: Starting Implementation", "md_foundation_2memory_2_t_o_d_o.html#autotoc_md544", [
        [ "✅ Completed", "md_foundation_2memory_2_t_o_d_o.html#autotoc_md545", null ],
        [ "🚧 Current Task: Memory Management Implementation", "md_foundation_2memory_2_t_o_d_o.html#autotoc_md546", null ],
        [ "📋 TODO", "md_foundation_2memory_2_t_o_d_o.html#autotoc_md547", null ],
        [ "Current Focus", "md_foundation_2memory_2_t_o_d_o.html#autotoc_md548", null ]
      ] ]
    ] ],
    [ "INTENT", "md__i_n_t_e_n_t.html", null ],
    [ "Interactive Voxel Editor CLI Session", "md_interactive__session.html", [
      [ "Starting the CLI", "md_interactive__session.html#autotoc_md550", null ],
      [ "Session 1: Create a Simple Cube", "md_interactive__session.html#autotoc_md551", null ],
      [ "Session 2: Using Fill Command", "md_interactive__session.html#autotoc_md552", null ],
      [ "Session 3: Working with Groups", "md_interactive__session.html#autotoc_md553", null ],
      [ "Session 4: Camera Controls", "md_interactive__session.html#autotoc_md554", null ],
      [ "Session 5: Undo/Redo Operations", "md_interactive__session.html#autotoc_md555", null ],
      [ "Tips:", "md_interactive__session.html#autotoc_md556", null ]
    ] ],
    [ "Voxel Editor Project Summary", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html", [
      [ "Project Overview", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md558", null ],
      [ "Implementation Status", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md559", [
        [ "✅ Foundation Layer (5 systems, 142+ tests)", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md560", null ],
        [ "✅ Core Systems (12 systems, 400+ tests)", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md561", null ],
        [ "✅ CLI Application", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md562", null ]
      ] ],
      [ "File Statistics", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md563", null ],
      [ "Key Features", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md564", [
        [ "Multi-Resolution System", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md565", null ],
        [ "Advanced Editing", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md566", null ],
        [ "File Support", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md567", null ],
        [ "Performance", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md568", null ]
      ] ],
      [ "Testing Infrastructure", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md569", [
        [ "Unit Tests", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md570", null ],
        [ "Integration Tests", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md571", null ]
      ] ],
      [ "Build System", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md572", null ],
      [ "Documentation", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md573", null ],
      [ "Next Steps", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md574", [
        [ "Immediate", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md575", null ],
        [ "Future Development", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md576", null ]
      ] ],
      [ "Command Quick Reference", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md577", null ],
      [ "Architecture Highlights", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md578", [
        [ "Modular Design", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md579", null ],
        [ "Performance Optimized", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md580", null ],
        [ "Future-Proof", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md581", null ]
      ] ],
      [ "Conclusion", "md__p_r_o_j_e_c_t___s_u_m_m_a_r_y.html#autotoc_md582", null ]
    ] ],
    [ "Voxel Editor", "md__r_e_a_d_m_e.html", [
      [ "Features", "md__r_e_a_d_m_e.html#autotoc_md584", null ],
      [ "Project Structure", "md__r_e_a_d_m_e.html#autotoc_md585", null ],
      [ "Building", "md__r_e_a_d_m_e.html#autotoc_md586", [
        [ "Prerequisites", "md__r_e_a_d_m_e.html#autotoc_md587", null ],
        [ "Quick Build", "md__r_e_a_d_m_e.html#autotoc_md588", null ],
        [ "Build Options", "md__r_e_a_d_m_e.html#autotoc_md589", null ]
      ] ],
      [ "Running the CLI Application", "md__r_e_a_d_m_e.html#autotoc_md590", null ],
      [ "CLI Commands", "md__r_e_a_d_m_e.html#autotoc_md591", [
        [ "File Operations", "md__r_e_a_d_m_e.html#autotoc_md592", null ],
        [ "Edit Operations", "md__r_e_a_d_m_e.html#autotoc_md593", null ],
        [ "View Controls", "md__r_e_a_d_m_e.html#autotoc_md594", null ],
        [ "Selection", "md__r_e_a_d_m_e.html#autotoc_md595", null ],
        [ "Groups", "md__r_e_a_d_m_e.html#autotoc_md596", null ],
        [ "System", "md__r_e_a_d_m_e.html#autotoc_md597", null ]
      ] ],
      [ "Mouse Controls", "md__r_e_a_d_m_e.html#autotoc_md598", null ],
      [ "Keyboard Shortcuts", "md__r_e_a_d_m_e.html#autotoc_md599", null ],
      [ "Architecture", "md__r_e_a_d_m_e.html#autotoc_md600", [
        [ "Foundation Layer", "md__r_e_a_d_m_e.html#autotoc_md601", null ],
        [ "Core Systems", "md__r_e_a_d_m_e.html#autotoc_md602", null ]
      ] ],
      [ "Testing", "md__r_e_a_d_m_e.html#autotoc_md603", null ],
      [ "Performance", "md__r_e_a_d_m_e.html#autotoc_md604", null ],
      [ "Future Development", "md__r_e_a_d_m_e.html#autotoc_md605", null ],
      [ "License", "md__r_e_a_d_m_e.html#autotoc_md606", null ],
      [ "Contributing", "md__r_e_a_d_m_e.html#autotoc_md607", null ]
    ] ],
    [ "Voxel Editor Development TODO", "md__t_o_d_o.html", [
      [ "Project Status: Core/VoxelData Complete! 🎉", "md__t_o_d_o.html#autotoc_md609", [
        [ "✅ Completed", "md__t_o_d_o.html#autotoc_md610", null ],
        [ "🎉 Current Phase: Build System Integration", "md__t_o_d_o.html#autotoc_md611", null ],
        [ "✅ Apps/CLI Complete Implementation", "md__t_o_d_o.html#autotoc_md612", null ],
        [ "🔨 CLI Feature Summary", "md__t_o_d_o.html#autotoc_md613", null ],
        [ "✅ Final Steps Complete", "md__t_o_d_o.html#autotoc_md614", null ],
        [ "🔧 Current Build Status", "md__t_o_d_o.html#autotoc_md615", null ],
        [ "📋 TODO - Future Phases", "md__t_o_d_o.html#autotoc_md616", null ]
      ] ],
      [ "Development Guidelines", "md__t_o_d_o.html#autotoc_md617", null ],
      [ "Current Focus", "md__t_o_d_o.html#autotoc_md618", [
        [ "🔧 Current Build Status", "md__t_o_d_o.html#autotoc_md619", null ]
      ] ],
      [ "Quick Start Commands", "md__t_o_d_o.html#autotoc_md620", null ],
      [ "Build Status", "md__t_o_d_o.html#autotoc_md621", null ]
    ] ],
    [ "Namespaces", "namespaces.html", [
      [ "Namespace List", "namespaces.html", "namespaces_dup" ],
      [ "Namespace Members", "namespacemembers.html", [
        [ "All", "namespacemembers.html", "namespacemembers_dup" ],
        [ "Functions", "namespacemembers_func.html", null ],
        [ "Variables", "namespacemembers_vars.html", null ],
        [ "Typedefs", "namespacemembers_type.html", null ],
        [ "Enumerations", "namespacemembers_enum.html", null ]
      ] ]
    ] ],
    [ "Classes", "annotated.html", [
      [ "Class List", "annotated.html", "annotated_dup" ],
      [ "Class Index", "classes.html", null ],
      [ "Class Hierarchy", "hierarchy.html", "hierarchy" ],
      [ "Class Members", "functions.html", [
        [ "All", "functions.html", "functions_dup" ],
        [ "Functions", "functions_func.html", "functions_func" ],
        [ "Variables", "functions_vars.html", "functions_vars" ],
        [ "Typedefs", "functions_type.html", null ],
        [ "Enumerations", "functions_enum.html", null ],
        [ "Enumerator", "functions_eval.html", null ],
        [ "Related Symbols", "functions_rela.html", null ]
      ] ]
    ] ],
    [ "Files", "files.html", [
      [ "File List", "files.html", "files_dup" ],
      [ "File Members", "globals.html", [
        [ "All", "globals.html", "globals_dup" ],
        [ "Functions", "globals_func.html", "globals_func" ],
        [ "Typedefs", "globals_type.html", null ],
        [ "Macros", "globals_defs.html", null ]
      ] ]
    ] ]
  ] ]
];

var NAVTREEINDEX =
[
"_application_8cpp.html",
"_test_dual_contouring_8cpp.html#a84b37ddf361cb075b0fd5ed1bbc34f01",
"_test_selection_set_8cpp.html#af14b37bc90b1d670c37fe22e51b2a8c4",
"class_mock_voxel_data_manager.html#ae1afb1a576085d0ea89b7edd2814b981",
"class_voxel_editor_1_1_camera_1_1_camera.html#a4de920cbef2cb6a11991cdeb98959a39",
"class_voxel_editor_1_1_command_processor.html#a01e3df73ea2f69e166a3dd3c69749199",
"class_voxel_editor_1_1_events_1_1_handler_entry.html#a34461a6f7c6323699d364812c6864daf",
"class_voxel_editor_1_1_file_i_o_1_1_compression.html#a3437639f3f8d7312a064081f458fcdb2",
"class_voxel_editor_1_1_groups_1_1_group_manager.html#a04ca2997fdabe6d53e134826c9239788",
"class_voxel_editor_1_1_groups_1_1_voxel_group.html#afa8e6d45c7cbf9f38cf6e037db2a2768",
"class_voxel_editor_1_1_input_1_1_keyboard_handler.html#abbdb4d80125f5e887e70d3d325d64f84",
"class_voxel_editor_1_1_line_editor.html#a60dd22ba5be6f3131adf0b471ed4779f",
"class_voxel_editor_1_1_math_1_1_quaternion.html#a1b0831405b24063ba9332ca3989bca47",
"class_voxel_editor_1_1_math_1_1_vector3i.html",
"class_voxel_editor_1_1_memory_1_1_memory_tracker.html#af74c4d5803165d304d0d055132cc0889",
"class_voxel_editor_1_1_rendering_1_1_render_engine.html#a26f3793cdb778505b283ca0bb2a3e1b1",
"class_voxel_editor_1_1_rendering_1_1_shader_manager.html#ade139a43a6f526dc861b963490ff0eb1",
"class_voxel_editor_1_1_selection_1_1_selection_renderer.html#a7145ed26458edd64132be594bfbe201c",
"class_voxel_editor_1_1_surface_gen_1_1_mesh_simplifier.html#a60af0768d5143264b91c4f060efa1ebe",
"class_voxel_editor_1_1_undo_redo_1_1_modify_selection_command.html#a923ddca69fd263fa4887ddf46b7ea990",
"class_voxel_editor_1_1_visual_feedback_1_1_feedback_renderer.html#a4074820cd63329f44f1fe2e04027d68d",
"class_voxel_editor_1_1_voxel_data_1_1_sparse_octree.html#a9f99dec1f4a26cdea5ad112950caa8ef",
"dir_28d1e4f04e8118b19af886cdc04c0396.html",
"md__d_e_s_i_g_n.html#autotoc_md490",
"md_core_2rendering_2_d_e_s_i_g_n.html#autotoc_md269",
"md_foundation_2memory_2_t_o_d_o.html#autotoc_md545",
"namespace_voxel_editor_1_1_input.html#a8b2a9d37fd35de3bfe892807b743a530a88183b946cc5f0e8c96b2e66e1c74a7e",
"namespace_voxel_editor_1_1_rendering.html#ae7396e93f4d60e03702133999cb032faae3e73a4b6e7cfd12008a35f6a051b319",
"struct_voxel_editor_1_1_events_1_1_v_r_grab_event.html",
"struct_voxel_editor_1_1_file_i_o_1_1_project_metadata.html#a85cdeddb553178c23e40f110d2606dab",
"struct_voxel_editor_1_1_input_1_1_events_1_1_key_action_event.html#a398474022f274d3270b8e69b98e0201e",
"struct_voxel_editor_1_1_input_1_1_input_mapping.html#a87f6af2fa0c16f552be81d3bad00d65c",
"struct_voxel_editor_1_1_logging_1_1_performance_profiler_1_1_memory_stats.html#af89bcd6fc5df6a7f6092760fd2d8494b",
"struct_voxel_editor_1_1_rendering_1_1_render_stats.html#a21e8fcdd538d4362315f2fbb1127d26e",
"struct_voxel_editor_1_1_rendering_1_1_vertex_layout_1_1_attribute.html#a106cd07d3e33dfc69171cfbbeba66122",
"struct_voxel_editor_1_1_surface_gen_1_1_simplification_settings.html#ae6e078fc2c36b3dadbd8dfb67038e541",
"test___camera_8cpp.html#add65e4f11b4d1a4e06c06143f53fc45b",
"test___render_stats_8cpp.html"
];

var SYNCONMSG = 'click to disable panel synchronization';
var SYNCOFFMSG = 'click to enable panel synchronization';
var LISTOFALLMEMBERS = 'List of all members';