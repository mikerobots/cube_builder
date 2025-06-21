# Requirements Management System

Thread-safe requirements management with auto-generated IDs, categories, test tracking, and markdown export.

## Quick Usage (Claude Code LLM)

### Categories
```bash
./req add-category "Grid System" --description "Ground plane requirements"
./req list-categories
```

### Groups and Subgroups (For Migration)
```bash
# Add groups with specific IDs
./req add-group "1" "Ground Plane Grid System" "Grid System" --description "Grid requirements"
./req add-subgroup "1.1" "Grid Specifications" "1" --description "Visual specs"

# List groups and subgroups
./req list-groups --category "Grid System"
./req list-subgroups --group-id "1"
```

### Requirements (Auto-generated or Custom IDs)
```bash
# Add requirement (ID auto-generated)
./req add-requirement "Grid shall display 32cm squares" "Grid System" \
  --subsystems "Rendering" "Visual Feedback" \
  --description "Detailed description"

# Add with custom ID and group assignment (for migration)
./req add-requirement "Grid positioned at Y=0" "Grid System" \
  --id "REQ-1.1.2" --group-id "1" --subgroup-id "1.1" \
  --subsystems "Rendering" "Camera"

# Add without testing
./req add-requirement "Documentation only" "Docs" --no-test
```

### List and Read
```bash
./req list-all
./req list-by-category "Grid System"
./req read REQ-1.1.1
./req list-without-tests          # Find requirements missing test files
```

### Independent Test Management
```bash
# Add independent tests
./req add-test "TEST-GRID-001" "Grid Display Test" "test_grid_display.cpp" \
  --description "Tests 32cm grid squares" --type "unit"

# List tests (with optional filters)
./req list-tests                           # All tests
./req list-tests --type unit              # Only unit tests
./req list-tests --status passed          # Only passed tests

# Update test status
./req update-test-status TEST-GRID-001 passed
./req update-test-status TEST-GRID-001 failed

# Assign tests to requirements
./req assign-test REQ-1.1.1 TEST-GRID-001
./req unassign-test REQ-1.1.1 TEST-GRID-001

# Check requirement validation (based on assigned tests)
./req check-validation REQ-1.1.1

# Clear all test statuses (resets requirement validation)
./req clear-all-tests
```


### Export
```bash
./req export-markdown --output requirements.md
```

## Test Status Values (Independent Tests)
- `pending` - Test needs to be run (default for new tests)
- `in_progress` - Test being written or fixed
- `passed` - Test exists and is passing
- `failed` - Test exists but is failing

## Requirement Validation Status
- `no_tests` - No tests assigned to requirement
- `incomplete` - Has tests but some are pending/in_progress
- `failed` - At least one assigned test has failed
- `validated` - All assigned tests have passed

## Features
- **Auto-generated unique IDs** (REQ-X.Y.Z format)
- **Custom ID assignment** for migration from existing documents
- **Groups and subgroups** with specific IDs (e.g., "1", "1.1", "1.2")
- **Multi-process safe** with robust file locking and stale lock detection
- **No requirement deletion** (stable IDs)
- **Independent test management** with validation status tracking
- **Test-to-requirement assignment** for flexible validation workflows
- **Category organization** and subsystem impact tracking
- **Hierarchical markdown export** with proper group structure