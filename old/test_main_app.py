#!/usr/bin/env python3

import subprocess
import time
import signal
import os

def test_main_app():
    """Test the main app with camera fix"""
    
    # Start the CLI process
    proc = subprocess.Popen(
        ['./voxel-cli'],
        stdin=subprocess.PIPE,
        stdout=subprocess.PIPE,
        stderr=subprocess.PIPE,
        text=True,
        bufsize=1
    )
    
    try:
        # Wait for initialization
        time.sleep(2)
        
        # Send commands
        commands = [
            "voxel add 0 0 0",
            "render", 
            "screenshot test_main_fixed.ppm",
            "quit"
        ]
        
        for cmd in commands:
            print(f"Sending: {cmd}")
            proc.stdin.write(cmd + "\n")
            proc.stdin.flush()
            time.sleep(1)
        
        # Wait for completion or timeout
        try:
            proc.wait(timeout=5)
        except subprocess.TimeoutExpired:
            print("Process timeout, terminating...")
            proc.terminate()
            time.sleep(1)
            if proc.poll() is None:
                proc.kill()
        
        print("Done")
        
    except Exception as e:
        print(f"Error: {e}")
        proc.terminate()

if __name__ == "__main__":
    test_main_app()