add_test([=[VoxelDataManagerPerfTest.CollisionCheck10000Voxels]=]  /Users/michaelhalloran/cube_edit/build_debug/bin/test_uperf_core_voxel_data_manager [==[--gtest_filter=VoxelDataManagerPerfTest.CollisionCheck10000Voxels]==] --gtest_also_run_disabled_tests)
set_tests_properties([=[VoxelDataManagerPerfTest.CollisionCheck10000Voxels]=]  PROPERTIES WORKING_DIRECTORY /Users/michaelhalloran/cube_edit/build_debug/core/voxel_data/tests SKIP_REGULAR_EXPRESSION [==[\[  SKIPPED \]]==] TIMEOUT 300)
set(  test_uperf_core_voxel_data_manager_TESTS VoxelDataManagerPerfTest.CollisionCheck10000Voxels)
