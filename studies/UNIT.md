# UNIT.md - Unit Test Failures

## Summary
Total Tests: 100
- Passed: 90
- Failed: 10

## Failed Tests

### 1. test_unit_core_face_detector_traversal (0s)
**Failed tests:** 10
- FaceDetectorTraversalTest.BasicRaycast_ChecksFaceFront
- FaceDetectorTraversalTest.RaycastFromAbove
- FaceDetectorTraversalTest.CameraDirectionRaycast
- FaceDetectorTraversalTest.StableRaycastResults
- FaceDetectorTraversalTest.DetectFaceDirection_SimpleCase
- FaceDetectorTraversalTest.MultipleVoxelsRaycast
- FaceDetectorTraversalTest.ConsistentFaceDetection_AllCameraAngles
- FaceDetectorTraversalTest.AngledRaycast_HitsCorrectFace
- FaceDetectorTraversalTest.NegativeCoordinateRaycast
- FaceDetectorTraversalTest.DebugRaycast_VoxelAtOrigin

### 2. test_unit_core_input_placement_validation (0s)
**Failed tests:** 2
- PlacementValidationTest.ValidatePlacementBasic
- PlacementValidationTest.ValidateGroundPlaneConstraint

### 3. test_unit_core_surface_gen_dual_contouring (71s)
**Failed tests:** 6
- DualContouringTest.EstimateNormal_SingleVoxel
- DualContouringTest.SimpleVoxel
- DualContouringTest.TwoAdjacentVoxels
- DualContouringTest.EstimateHermiteData_EdgeWithNeighbor
- DualContouringTest.CornerCase_AllVerticesOccupied
- DualContouringGeneratorTest.GenerateMesh_SimpleVoxel

### 4. test_unit_core_surface_gen_generator (71s)
**Failed tests:** 6
- SurfaceGeneratorTest.EmptyVoxelData
- SurfaceGeneratorTest.SingleVoxel
- SurfaceGeneratorTest.TwoAdjacentVoxels
- SurfaceGeneratorTest.ComplexShape
- SurfaceGeneratorTest.DifferentVoxelSizes
- SurfaceGeneratorTest.SparseVoxelData

### 5. test_unit_core_surface_gen_requirements (71s)
**Failed tests:** 7
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_1cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_2cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_4cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_8cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_16cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_32cm
- SurfaceGenerationTest.REQ_9_1_1_RealTimeMeshGeneration_MultiResolution_64cm

### 6. test_unit_core_surface_gen_topology_preserver (0s)
**Failed tests:** 3
- TopologyPreserverTest.CreateMeshFromEdges_SimplePlane
- TopologyPreserverTest.CreateMeshFromEdges_ComplexMesh
- TopologyPreserverTest.DetectLoopsInTorus

### 7. test_unit_core_undo_redo_voxel_commands (0s)
**Failed tests:** 2
- VoxelCommandsTest.VoxelFillCommand_FillRegion
- VoxelCommandsTest.VoxelFillCommand_UndoRestoresPrevious

### 8. test_unit_core_visual_feedback_face_detector (3s)
**Failed tests:** 8
- FaceDetectorTest.MinimalRaycast_VoxelAtOrigin
- FaceDetectorTest.RayHit
- FaceDetectorTest.FaceDirection_AllDirections
- FaceDetectorTest.MultipleVoxelRay
- FaceDetectorTest.NonAlignedVoxelPositions_SingleVoxel
- FaceDetectorTest.NonAlignedVoxelPositions_AllFaces
- FaceDetectorTest.NonAlignedVoxelPositions_MixedAlignedAndNonAligned
- FaceDetectorTest.NonAlignedVoxelPositions_DifferentResolutions

### 9. test_unit_core_visual_feedback_requirements (0s)
**Failed tests:** 1
- VisualFeedbackRequirementTest.FaceHighlighting_REQ_2_3_1_to_2_3_2_Logic

### 10. test_unit_face_direction_accuracy (0s)
**Failed tests:** 3
- FaceDirectionAccuracyTest.PerpendicularRaysHitCorrectFaces
- FaceDirectionAccuracyTest.RaysFromInsideVoxelDetectExitFace
- FaceDirectionAccuracyTest.MultipleVoxelsCorrectFaceSelection

## Notes
- Full logs for each failed test are saved in /tmp/
- To debug individual tests: `cd build_ninja && ./bin/test_unit_<subsystem>_<name> --gtest_filter='*SpecificTest*'`
- Many failures appear to be related to face detection and coordinate system issues