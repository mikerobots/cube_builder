#!/bin/bash
# Test if voxels are actually being stored

echo "=== Testing Voxel Storage ==="
echo ""

# Test immediate save after place
echo "Test: Place and Save"
cat > /tmp/storage_test.txt << EOF
workspace 5 5 5
resolution 64cm
place 200 0 200
save /tmp/test_voxels.vox
quit
EOF

rm -f /tmp/test_voxels.vox
timeout 10s ../../voxel-cli --headless < /tmp/storage_test.txt 2>&1 | grep -E "placed|saved"

if [ -f "/tmp/test_voxels.vox" ]; then
    SIZE=$(ls -la /tmp/test_voxels.vox | awk '{print $5}')
    echo "✅ Save file created, size: $SIZE bytes"
    
    # Try to load it back
    echo ""
    echo "Test: Load saved file"
    cat > /tmp/load_test.txt << EOF
load /tmp/test_voxels.vox
status
quit
EOF
    
    timeout 10s ../../voxel-cli --headless < /tmp/load_test.txt 2>&1 | grep -E "loaded|Voxels:"
else
    echo "❌ Save file not created"
fi

# Test with export
echo ""
echo "Test: Export to STL"
cat > /tmp/export_test.txt << EOF
workspace 5 5 5  
resolution 64cm
place 200 0 200
place 264 0 200
place 200 0 264
export /tmp/test_export.stl
quit
EOF

rm -f /tmp/test_export.stl
OUTPUT=$(timeout 10s ../../voxel-cli --headless < /tmp/export_test.txt 2>&1)
echo "$OUTPUT" | grep -E "placed|Exported"

if [ -f "/tmp/test_export.stl" ]; then
    SIZE=$(ls -la /tmp/test_export.stl | awk '{print $5}')
    echo "✅ STL file created, size: $SIZE bytes"
else
    echo "❌ STL file not created"
fi

echo ""
echo "=== Analysis ==="
echo "Voxels are being placed via commands but may not be"
echo "stored in the octree data structure correctly."