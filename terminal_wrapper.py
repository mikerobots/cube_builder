#!/usr/bin/env python3
"""
Terminal Application Wrapper

A Python wrapper that runs terminal applications and automatically responds
to "Do you want to proceed?" prompts after 10 seconds of inactivity.
"""

import sys
import time
import threading
import signal
import argparse
import re
from typing import Optional, List
import pexpect
from pexpect import pxssh


class TerminalWrapper:
    def __init__(self, command: List[str], timeout: int = 10):
        """
        Initialize the terminal wrapper.
        
        Args:
            command: The command and arguments to execute
            timeout: Seconds to wait for inactivity before checking for prompts
        """
        self.command = command
        self.timeout = timeout
        self.process: Optional[pexpect.spawn] = None
        self.last_output_time = time.time()
        self.screen_buffer = ""
        self.running = True
        self.monitor_thread: Optional[threading.Thread] = None
        
        # Patterns to detect prompts
        self.proceed_patterns = [
            r"Do you want to proceed\?",
            r"Continue\?",
            r"Proceed\?",
            r"\[Y/n\]",
            r"\[y/N\]",
            r"\(y/n\)",
        ]
        
    def start(self):
        """Start the wrapped terminal application."""
        try:
            # Start the process with xterm-256color terminal
            self.process = pexpect.spawn(
                ' '.join(self.command),
                encoding='utf-8',
                timeout=1
            )
            
            # Set terminal type and ensure full output capture
            self.process.setenv('TERM', 'xterm-256color')
            self.process.logfile_read = sys.stdout  # Mirror all output to stdout
            
            print(f"Starting: {' '.join(self.command)}")
            print("=" * 50)
            
            # Start monitoring thread
            self.monitor_thread = threading.Thread(target=self._monitor_inactivity, daemon=True)
            self.monitor_thread.start()
            
            # Main loop to handle I/O
            self._handle_io()
            
        except Exception as e:
            print(f"Error starting process: {e}")
            sys.exit(1)
            
    def _handle_io(self):
        """Handle input/output between user and wrapped application."""
        try:
            while self.running and self.process.isalive():
                try:
                    # Check for output from the process - read all available data
                    output = self.process.read_nonblocking(size=8192, timeout=0.1)
                    if output:
                        self.last_output_time = time.time()
                        self.screen_buffer += output
                        
                        # Keep only last 8KB of screen buffer for prompt detection
                        if len(self.screen_buffer) > 8192:
                            self.screen_buffer = self.screen_buffer[-8192:]
                        
                        # Print ALL output to user immediately - preserve formatting
                        sys.stdout.write(output)
                        sys.stdout.flush()
                        
                except pexpect.TIMEOUT:
                    # No output available, continue
                    pass
                except pexpect.EOF:
                    # Process ended
                    break
                    
                # Check for user input (non-blocking)
                import select
                if select.select([sys.stdin], [], [], 0)[0]:
                    user_input = sys.stdin.read(1)
                    if user_input:
                        self.process.send(user_input)
                        
        except KeyboardInterrupt:
            print("\nReceived interrupt signal")
            self.stop()
            
    def _monitor_inactivity(self):
        """Monitor for inactivity and auto-respond to prompts."""
        while self.running:
            time.sleep(1)
            
            # Check if we've been inactive for the timeout period
            if time.time() - self.last_output_time >= self.timeout:
                if self._check_for_proceed_prompt():
                    print("\n[AUTO] Detected proceed prompt, sending Enter...")
                    self.process.send('\r\n')
                    self.last_output_time = time.time()  # Reset timer
                    
    def _check_for_proceed_prompt(self) -> bool:
        """
        Check if the current screen contains a proceed prompt.
        
        Returns:
            True if a proceed prompt is detected
        """
        # Get the last few lines of the screen buffer
        recent_lines = self.screen_buffer.split('\n')[-5:]
        recent_text = '\n'.join(recent_lines)
        
        # Check against all proceed patterns
        for pattern in self.proceed_patterns:
            if re.search(pattern, recent_text, re.IGNORECASE):
                return True
                
        return False
        
    def stop(self):
        """Stop the wrapper and clean up."""
        self.running = False
        if self.process and self.process.isalive():
            self.process.terminate()
            
    def wait(self):
        """Wait for the wrapped process to complete."""
        if self.process:
            self.process.wait()
            return self.process.exitstatus
        return 0


def main():
    parser = argparse.ArgumentParser(
        description="Wrap a terminal application with auto-response to proceed prompts"
    )
    parser.add_argument(
        'command', 
        nargs='+', 
        help='Command and arguments to execute'
    )
    parser.add_argument(
        '--timeout', 
        type=int, 
        default=10,
        help='Seconds to wait for inactivity before checking prompts (default: 10)'
    )
    parser.add_argument(
        '--verbose', 
        action='store_true',
        help='Enable verbose logging'
    )
    
    args = parser.parse_args()
    
    # Setup signal handlers
    wrapper = TerminalWrapper(args.command, args.timeout)
    
    def signal_handler(signum, frame):
        wrapper.stop()
        sys.exit(0)
        
    signal.signal(signal.SIGINT, signal_handler)
    signal.signal(signal.SIGTERM, signal_handler)
    
    try:
        wrapper.start()
        exit_code = wrapper.wait()
        sys.exit(exit_code)
    except Exception as e:
        print(f"Error: {e}")
        sys.exit(1)


if __name__ == '__main__':
    main()