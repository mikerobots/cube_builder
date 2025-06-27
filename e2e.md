/# TODO.md - Voxel Placement Requirements Update

This TODO.md list is shared by multiple agents. If you pick up a task,
mark it as "In Progress, [Your Name]". Update it as you are
working. Don't pick up any work that is "In progress" by other
agents. If you have not picked a random name, please pick one. Once
you finish a task, pick another one. Do not disable tests unless
specifically approved.

---

We are fixing integration tests. We made some changes to the placement
of voxels. Voxels are now placed on the ground plane vs the middle of
the voxels being at the ground plane. We also decreased the number of
grid resolutions.

We need to fix the following tests. 

[ ] test_integration_cli_application
[ ] test_integration_cli_command_sequences
[ ] test_integration_cli_command_validation
[ ] test_integration_cli_undo_redo_chains


