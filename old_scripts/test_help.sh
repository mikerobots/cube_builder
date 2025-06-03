#!/bin/bash

# Test to see available commands

echo "help" | ./voxel-cli 2>&1 | grep -A50 "Available commands" | head -100