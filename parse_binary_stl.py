#!/usr/bin/env python3

import struct
import sys

def parse_binary_stl(filename):
    """Parse and display contents of a binary STL file"""
    
    with open(filename, 'rb') as f:
        # Read 80-byte header
        header = f.read(80)
        print(f"Header: {header.decode('ascii', errors='ignore').strip()}")
        
        # Read number of triangles
        num_triangles_data = f.read(4)
        if len(num_triangles_data) < 4:
            print("Error: File too short to contain triangle count")
            return
            
        num_triangles = struct.unpack('<I', num_triangles_data)[0]
        print(f"\nNumber of triangles: {num_triangles}")
        
        # Read each triangle
        for i in range(num_triangles):
            # Read normal vector (3 floats)
            normal_data = f.read(12)
            if len(normal_data) < 12:
                print(f"Error: Incomplete triangle {i}")
                break
                
            normal = struct.unpack('<fff', normal_data)
            
            # Read 3 vertices (9 floats total)
            vertices = []
            for j in range(3):
                vertex_data = f.read(12)
                if len(vertex_data) < 12:
                    print(f"Error: Incomplete vertex {j} of triangle {i}")
                    break
                vertex = struct.unpack('<fff', vertex_data)
                vertices.append(vertex)
            
            # Read attribute byte count (should be 0)
            attr_data = f.read(2)
            if len(attr_data) < 2:
                print(f"Error: Missing attribute data for triangle {i}")
                break
            attr = struct.unpack('<H', attr_data)[0]
            
            print(f"\nTriangle {i + 1}:")
            print(f"  Normal: ({normal[0]:.6f}, {normal[1]:.6f}, {normal[2]:.6f})")
            for j, vertex in enumerate(vertices):
                print(f"  Vertex {j + 1}: ({vertex[0]:.6f}, {vertex[1]:.6f}, {vertex[2]:.6f})")
            if attr != 0:
                print(f"  Attributes: {attr}")
        
        # Check if there's extra data
        remaining = f.read()
        if remaining:
            print(f"\nWarning: {len(remaining)} extra bytes at end of file")
            
        # Verify file size
        expected_size = 80 + 4 + (num_triangles * 50)
        actual_size = 80 + 4 + len(num_triangles_data) - 4 + f.tell() - 84
        print(f"\nExpected file size: {expected_size} bytes")
        print(f"Actual position: {f.tell()} bytes")

if __name__ == "__main__":
    if len(sys.argv) > 1:
        parse_binary_stl(sys.argv[1])
    else:
        parse_binary_stl("single_cube.stl")