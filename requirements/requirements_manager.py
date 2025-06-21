#!/usr/bin/env python3
"""
Requirements Management System for Voxel Editor
Manages requirements with stable IDs, categories, and subsystems.
Supports concurrent access from multiple processes.
"""

import json
import os
import sys
import argparse
import fcntl
import time
import atexit
import signal
from typing import Dict, List, Optional, Any
from datetime import datetime


class RequirementsManager:
    """Thread-safe requirements management with file locking."""
    
    def __init__(self, json_file: str = "requirements.json"):
        self.json_file = os.path.abspath(json_file)
        self.lock_file = self.json_file + ".lock"
        self.current_lock_fd = None
        self._ensure_file_exists()
        
        # Register cleanup handlers
        atexit.register(self._cleanup_on_exit)
        signal.signal(signal.SIGTERM, self._signal_handler)
        signal.signal(signal.SIGINT, self._signal_handler)
    
    def _ensure_file_exists(self):
        """Create empty requirements file if it doesn't exist."""
        if not os.path.exists(self.json_file):
            initial_data = {
                "metadata": {
                    "created": datetime.now().isoformat(),
                    "last_modified": datetime.now().isoformat(),
                    "version": "1.0"
                },
                "categories": {},
                "requirements": {},
                "tests": {},  # Independent test registry
                "test_assignments": {}  # Maps requirement_id -> [test_ids]
            }
            with open(self.json_file, 'w') as f:
                json.dump(initial_data, f, indent=2)
    
    def _cleanup_on_exit(self):
        """Clean up lock file on normal exit."""
        if self.current_lock_fd is not None:
            self._release_lock(self.current_lock_fd)
            self.current_lock_fd = None
    
    def _signal_handler(self, signum, frame):
        """Handle signals by cleaning up and exiting."""
        self._cleanup_on_exit()
        sys.exit(0)
    
    def _is_process_running(self, pid: int) -> bool:
        """Check if a process with given PID is still running."""
        try:
            os.kill(pid, 0)  # Signal 0 doesn't kill, just checks if process exists
            return True
        except (OSError, ProcessLookupError):
            return False
    
    def _acquire_lock(self, timeout: int = 10) -> Any:
        """Acquire file lock with timeout and stale lock detection."""
        start_time = time.time()
        
        while time.time() - start_time < timeout:
            try:
                # Try to create lock file exclusively
                lock_fd = os.open(self.lock_file, os.O_CREAT | os.O_EXCL | os.O_RDWR)
                
                # Write our PID to the lock file
                pid_data = f"{os.getpid()}\n{time.time()}\n"
                os.write(lock_fd, pid_data.encode())
                os.fsync(lock_fd)
                
                # Apply fcntl lock as additional safety
                fcntl.flock(lock_fd, fcntl.LOCK_EX | fcntl.LOCK_NB)
                
                # Track the current lock
                self.current_lock_fd = lock_fd
                return lock_fd
                
            except FileExistsError:
                # Lock file exists, check if holder is still alive
                try:
                    with open(self.lock_file, 'r') as f:
                        lines = f.readlines()
                        if len(lines) >= 2:
                            old_pid = int(lines[0].strip())
                            lock_time = float(lines[1].strip())
                            
                            # Check if the process is still running
                            if not self._is_process_running(old_pid):
                                print(f"Removing stale lock from dead process {old_pid}")
                                os.unlink(self.lock_file)
                                continue  # Try again
                            
                            # Check if lock is very old (>5 minutes = potential deadlock)
                            if time.time() - lock_time > 300:
                                print(f"Removing very old lock from process {old_pid} (>5 minutes)")
                                os.unlink(self.lock_file)
                                continue  # Try again
                                
                except (ValueError, FileNotFoundError, IOError):
                    # Corrupted lock file, remove it
                    try:
                        os.unlink(self.lock_file)
                        continue
                    except FileNotFoundError:
                        pass
                
                time.sleep(0.1)
                
            except (OSError, IOError) as e:
                # Some other error, wait and retry
                time.sleep(0.1)
                
        raise TimeoutError(f"Could not acquire lock within {timeout} seconds")
    
    def _release_lock(self, lock_fd: Any):
        """Release file lock."""
        try:
            fcntl.flock(lock_fd, fcntl.LOCK_UN)
            os.close(lock_fd)
            os.unlink(self.lock_file)
        except Exception as e:
            # Still try to close the file descriptor
            try:
                os.close(lock_fd)
            except:
                pass
        finally:
            # Clear the current lock tracking
            if self.current_lock_fd == lock_fd:
                self.current_lock_fd = None
    
    def _load_data(self) -> Dict:
        """Load requirements data from JSON file."""
        with open(self.json_file, 'r') as f:
            return json.load(f)
    
    def _save_data(self, data: Dict):
        """Save requirements data to JSON file."""
        data["metadata"]["last_modified"] = datetime.now().isoformat()
        with open(self.json_file, 'w') as f:
            json.dump(data, f, indent=2)
    
    def add_category(self, category_name: str, description: str = "") -> bool:
        """Add a new category."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            if category_name in data["categories"]:
                print(f"Category '{category_name}' already exists")
                return False
            
            data["categories"][category_name] = {
                "description": description,
                "created": datetime.now().isoformat(),
                "requirement_count": 0
            }
            self._save_data(data)
            print(f"Added category: {category_name}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def list_categories(self) -> List[Dict]:
        """List all categories."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            categories = []
            for name, info in data["categories"].items():
                req_count = len([r for r in data["requirements"].values() 
                               if r.get("category") == name])
                categories.append({
                    "name": name,
                    "description": info.get("description", ""),
                    "requirement_count": req_count,
                    "created": info.get("created", "")
                })
            return sorted(categories, key=lambda x: x["name"])
        finally:
            self._release_lock(lock_fd)
    
    def _generate_next_req_id(self, data: Dict, category: str) -> str:
        """Generate the next available requirement ID for a category."""
        # Get category index (1-based)
        categories = sorted(data["categories"].keys())
        try:
            category_index = categories.index(category) + 1
        except ValueError:
            # If category not found, use next available index
            category_index = len(categories) + 1
        
        # Find existing requirements for this category
        category_reqs = []
        for req_id in data["requirements"].keys():
            if data["requirements"][req_id]["category"] == category:
                # Extract section number from REQ-X.Y.Z format
                try:
                    parts = req_id.split("-")[1].split(".")
                    if len(parts) >= 2 and int(parts[0]) == category_index:
                        category_reqs.append(int(parts[1]))
                except (IndexError, ValueError):
                    continue
        
        # Find next available section number
        next_section = 1
        if category_reqs:
            next_section = max(category_reqs) + 1
        
        return f"REQ-{category_index}.{next_section}.1"
    
    def add_group(self, group_id: str, group_name: str, category: str, description: str = "") -> bool:
        """Add a group with specific ID to a category."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            # Validate category exists
            if category not in data["categories"]:
                print(f"Category '{category}' does not exist. Add it first.")
                return False
            
            # Initialize groups structure if needed
            if "groups" not in data:
                data["groups"] = {}
            
            if group_id in data["groups"]:
                print(f"Group ID '{group_id}' already exists")
                return False
            
            data["groups"][group_id] = {
                "name": group_name,
                "description": description,
                "category": category,
                "subgroups": {},
                "created": datetime.now().isoformat(),
                "last_modified": datetime.now().isoformat()
            }
            
            self._save_data(data)
            print(f"Added group: {group_id} - {group_name}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def add_subgroup(self, subgroup_id: str, subgroup_name: str, group_id: str, description: str = "") -> bool:
        """Add a subgroup with specific ID to a group."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            # Initialize groups structure if needed
            if "groups" not in data:
                data["groups"] = {}
            
            # Validate group exists
            if group_id not in data["groups"]:
                print(f"Group '{group_id}' does not exist. Add it first.")
                return False
            
            if subgroup_id in data["groups"][group_id]["subgroups"]:
                print(f"Subgroup ID '{subgroup_id}' already exists in group '{group_id}'")
                return False
            
            data["groups"][group_id]["subgroups"][subgroup_id] = {
                "name": subgroup_name,
                "description": description,
                "created": datetime.now().isoformat(),
                "last_modified": datetime.now().isoformat()
            }
            
            data["groups"][group_id]["last_modified"] = datetime.now().isoformat()
            self._save_data(data)
            print(f"Added subgroup: {subgroup_id} - {subgroup_name} (to group {group_id})")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def add_requirement(self, req_id: str = None, title: str = "", category: str = "", 
                       subsystems: List[str] = None, description: str = "", 
                       test_needed: bool = True, test_files: List[str] = None,
                       group_id: str = None, subgroup_id: str = None) -> bool:
        """Add a new requirement with unique ID."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            # Validate required fields
            if not title:
                print("Title is required")
                return False
            if not category:
                print("Category is required")
                return False
            
            # Validate category exists
            if category not in data["categories"]:
                print(f"Category '{category}' does not exist. Add it first.")
                return False
            
            # Generate ID if not provided
            if not req_id:
                req_id = self._generate_next_req_id(data, category)
                print(f"Generated requirement ID: {req_id}")
            else:
                # Check if provided ID already exists
                if req_id in data["requirements"]:
                    print(f"Requirement ID '{req_id}' already exists")
                    return False
            
            data["requirements"][req_id] = {
                "title": title,
                "description": description,
                "category": category,
                "subsystems": subsystems or [],
                "test_needed": test_needed,
                "group_id": group_id,
                "subgroup_id": subgroup_id,
                "created": datetime.now().isoformat(),
                "last_modified": datetime.now().isoformat(),
                "status": "active"
            }
            
            self._save_data(data)
            print(f"Added requirement: {req_id}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def list_all_requirements(self) -> List[Dict]:
        """List all requirements."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            requirements = []
            for req_id, req_data in data["requirements"].items():
                req_info = dict(req_data)
                req_info["id"] = req_id
                requirements.append(req_info)
            return sorted(requirements, key=lambda x: x["id"])
        finally:
            self._release_lock(lock_fd)
    
    def list_requirements_by_category(self, category: str) -> List[Dict]:
        """List requirements for a specific category."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            requirements = []
            for req_id, req_data in data["requirements"].items():
                if req_data.get("category") == category:
                    req_info = dict(req_data)
                    req_info["id"] = req_id
                    requirements.append(req_info)
            return sorted(requirements, key=lambda x: x["id"])
        finally:
            self._release_lock(lock_fd)
    
    def get_requirement(self, req_id: str) -> Optional[Dict]:
        """Get a specific requirement by ID."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            if req_id in data["requirements"]:
                req_data = dict(data["requirements"][req_id])
                req_data["id"] = req_id
                return req_data
            return None
        finally:
            self._release_lock(lock_fd)
    
    
    def add_test(self, test_id: str, test_name: str, test_file: str, 
                 description: str = "", test_type: str = "unit") -> bool:
        """Add an independent test to the test registry."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            # Initialize tests structure if needed
            if "tests" not in data:
                data["tests"] = {}
            if "test_assignments" not in data:
                data["test_assignments"] = {}
            
            if test_id in data["tests"]:
                print(f"Test ID '{test_id}' already exists")
                return False
            
            data["tests"][test_id] = {
                "name": test_name,
                "file": test_file,
                "description": description,
                "type": test_type,  # unit, integration, e2e, etc.
                "status": "pending",
                "created": datetime.now().isoformat(),
                "last_modified": datetime.now().isoformat()
            }
            
            self._save_data(data)
            print(f"Added test: {test_id} - {test_name}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def update_test_status(self, test_id: str, status: str) -> bool:
        """Update the status of an independent test."""
        valid_statuses = ["pending", "in_progress", "passed", "failed"]
        if status not in valid_statuses:
            print(f"Invalid test status. Valid options: {', '.join(valid_statuses)}")
            return False
            
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            if "tests" not in data:
                data["tests"] = {}
            
            if test_id not in data["tests"]:
                print(f"Test '{test_id}' not found")
                return False
            
            data["tests"][test_id]["status"] = status
            data["tests"][test_id]["last_modified"] = datetime.now().isoformat()
            
            self._save_data(data)
            print(f"Updated test status for {test_id}: {status}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def assign_test_to_requirement(self, req_id: str, test_id: str) -> bool:
        """Assign an existing test to a requirement."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            # Initialize structures if needed
            if "tests" not in data:
                data["tests"] = {}
            if "test_assignments" not in data:
                data["test_assignments"] = {}
            
            # Validate requirement and test exist
            if req_id not in data["requirements"]:
                print(f"Requirement '{req_id}' not found")
                return False
            
            if test_id not in data["tests"]:
                print(f"Test '{test_id}' not found")
                return False
            
            # Initialize assignment list for requirement if needed
            if req_id not in data["test_assignments"]:
                data["test_assignments"][req_id] = []
            
            # Check if already assigned
            if test_id in data["test_assignments"][req_id]:
                print(f"Test '{test_id}' already assigned to {req_id}")
                return False
            
            # Assign test to requirement
            data["test_assignments"][req_id].append(test_id)
            data["requirements"][req_id]["last_modified"] = datetime.now().isoformat()
            
            self._save_data(data)
            print(f"Assigned test '{test_id}' to requirement {req_id}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def unassign_test_from_requirement(self, req_id: str, test_id: str) -> bool:
        """Unassign a test from a requirement."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            if "test_assignments" not in data:
                data["test_assignments"] = {}
            
            if req_id not in data["test_assignments"]:
                print(f"No tests assigned to requirement '{req_id}'")
                return False
            
            if test_id not in data["test_assignments"][req_id]:
                print(f"Test '{test_id}' not assigned to {req_id}")
                return False
            
            data["test_assignments"][req_id].remove(test_id)
            data["requirements"][req_id]["last_modified"] = datetime.now().isoformat()
            
            self._save_data(data)
            print(f"Unassigned test '{test_id}' from requirement {req_id}")
            return True
        finally:
            self._release_lock(lock_fd)
    
    def list_tests(self, test_type: str = None, status: str = None) -> List[Dict]:
        """List all tests, optionally filtered by type or status."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            tests = []
            
            if "tests" not in data:
                return tests
            
            for test_id, test_data in data["tests"].items():
                if test_type and test_data.get("type") != test_type:
                    continue
                if status and test_data.get("status") != status:
                    continue
                
                test_info = dict(test_data)
                test_info["id"] = test_id
                tests.append(test_info)
            
            return sorted(tests, key=lambda x: x["id"])
        finally:
            self._release_lock(lock_fd)
    
    def get_requirement_validation_status(self, req_id: str) -> Dict:
        """Get validation status for a requirement based on assigned tests."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            if req_id not in data["requirements"]:
                return {"error": f"Requirement '{req_id}' not found"}
            
            if "test_assignments" not in data or req_id not in data["test_assignments"]:
                return {
                    "requirement_id": req_id,
                    "validation_status": "no_tests",
                    "assigned_tests": [],
                    "test_summary": {"total": 0, "passed": 0, "failed": 0, "pending": 0, "in_progress": 0}
                }
            
            assigned_test_ids = data["test_assignments"][req_id]
            test_summary = {"total": 0, "passed": 0, "failed": 0, "pending": 0, "in_progress": 0}
            test_details = []
            
            for test_id in assigned_test_ids:
                if test_id in data.get("tests", {}):
                    test_data = data["tests"][test_id]
                    status = test_data.get("status", "pending")
                    test_summary["total"] += 1
                    test_summary[status] += 1
                    
                    test_details.append({
                        "id": test_id,
                        "name": test_data.get("name", ""),
                        "file": test_data.get("file", ""),
                        "status": status,
                        "type": test_data.get("type", "unit")
                    })
            
            # Determine overall validation status
            if test_summary["total"] == 0:
                validation_status = "no_tests"
            elif test_summary["failed"] > 0:
                validation_status = "failed"
            elif test_summary["pending"] > 0 or test_summary["in_progress"] > 0:
                validation_status = "incomplete"
            elif test_summary["passed"] == test_summary["total"]:
                validation_status = "validated"
            else:
                validation_status = "unknown"
            
            return {
                "requirement_id": req_id,
                "validation_status": validation_status,
                "assigned_tests": test_details,
                "test_summary": test_summary
            }
        finally:
            self._release_lock(lock_fd)
    
    def clear_all_test_statuses(self) -> bool:
        """Clear all test statuses (reset to pending)."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            if "tests" not in data:
                return True
            
            cleared_count = 0
            for test_id in data["tests"]:
                if data["tests"][test_id].get("status") != "pending":
                    data["tests"][test_id]["status"] = "pending"
                    data["tests"][test_id]["last_modified"] = datetime.now().isoformat()
                    cleared_count += 1
            
            self._save_data(data)
            print(f"Cleared status for {cleared_count} tests (reset to pending)")
            return True
        finally:
            self._release_lock(lock_fd)
    
    
    def list_groups(self, category: str = None) -> List[Dict]:
        """List all groups or groups in a specific category."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            groups = []
            
            if "groups" not in data:
                return groups
            
            for group_id, group_data in data["groups"].items():
                if category is None or group_data.get("category") == category:
                    group_info = dict(group_data)
                    group_info["id"] = group_id
                    group_info["subgroup_count"] = len(group_data.get("subgroups", {}))
                    groups.append(group_info)
            
            return sorted(groups, key=lambda x: x["id"])
        finally:
            self._release_lock(lock_fd)
    
    def list_subgroups(self, group_id: str = None) -> Dict:
        """List subgroups for a specific group or all subgroups."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            result = {}
            
            if "groups" not in data:
                return result
            
            if group_id:
                if group_id in data["groups"]:
                    result[group_id] = data["groups"][group_id].get("subgroups", {})
                else:
                    print(f"Group '{group_id}' not found")
                    return {}
            else:
                for gid, group_data in data["groups"].items():
                    subgroups = group_data.get("subgroups", {})
                    if subgroups:
                        result[gid] = subgroups
            
            return result
        finally:
            self._release_lock(lock_fd)
    
    
    def list_requirements_without_tests(self) -> List[Dict]:
        """List requirements that need testing but have no tests assigned."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            requirements = []
            
            if "test_assignments" not in data:
                data["test_assignments"] = {}
            
            for req_id, req_data in data["requirements"].items():
                test_needed = req_data.get("test_needed", True)
                assigned_tests = data["test_assignments"].get(req_id, [])
                
                if test_needed and not assigned_tests:
                    req_info = dict(req_data)
                    req_info["id"] = req_id
                    requirements.append(req_info)
            
            return sorted(requirements, key=lambda x: x["id"])
        finally:
            self._release_lock(lock_fd)
    
    def _write_requirement_to_markdown(self, f, req, data):
        """Helper method to write a requirement to markdown file."""
        f.write(f"- **{req['id']}**: {req['title']}\n")
        
        if req.get('description'):
            f.write(f"  \n  {req['description']}\n")
        
        if req.get('subsystems'):
            subsystems_str = ', '.join([f"**{s}**" for s in req['subsystems']])
            f.write(f"  \n  *Subsystems: {subsystems_str}*\n")
        
        # Test information - get validation status from assigned tests
        test_needed = req.get('test_needed', True)
        
        if test_needed:
            # Get assigned tests for this requirement
            req_id = req['id']
            assigned_test_ids = data.get("test_assignments", {}).get(req_id, [])
            
            if assigned_test_ids:
                f.write("  \n  *Assigned Tests:*\n")
                for test_id in assigned_test_ids:
                    if test_id in data.get("tests", {}):
                        test_data = data["tests"][test_id]
                        status = test_data.get("status", "pending")
                        test_name = test_data.get("name", "")
                        f.write(f"  - `{test_id}`: {test_name} - Status: **{status}**\n")
                    else:
                        f.write(f"  - `{test_id}`: (Test not found) - Status: **unknown**\n")
            else:
                f.write("  \n  *Assigned Tests: None*\n")
        else:
            f.write("  \n  *Testing: Not Required*\n")
        
        f.write(f"  \n  *Status: {req.get('status', 'active')}*\n\n")
    
    def export_markdown(self, output_file: str) -> bool:
        """Export all requirements to a markdown file."""
        lock_fd = self._acquire_lock()
        try:
            data = self._load_data()
            
            with open(output_file, 'w') as f:
                # Header
                f.write("# Requirements Specification\n\n")
                f.write(f"*Generated: {datetime.now().strftime('%B %d, %Y')}*\n\n")
                f.write(f"**Total Requirements**: {len(data['requirements'])}\n\n")
                f.write(f"**Categories**: {len(data['categories'])}\n\n")
                
                f.write("**IMPORTANT: Requirement IDs (e.g., REQ-1.1.1) must remain stable and should NEVER be changed when updating this document. New requirements should be added with new IDs, and obsolete requirements should be marked as deprecated rather than removed or renumbered.**\n\n")
                
                # Table of Contents - use data directly to avoid deadlock
                f.write("## Table of Contents\n\n")
                categories = []
                for name, info in data["categories"].items():
                    req_count = len([r for r in data["requirements"].values() 
                                   if r.get("category") == name])
                    categories.append({
                        "name": name,
                        "description": info.get("description", ""),
                        "requirement_count": req_count,
                        "created": info.get("created", "")
                    })
                categories = sorted(categories, key=lambda x: x["name"])
                
                for i, cat in enumerate(categories, 1):
                    f.write(f"{i}. [{cat['name']}](#{cat['name'].lower().replace(' ', '-')})\n")
                f.write("\n---\n\n")
                
                # Requirements by category, organized by groups and subgroups
                f.write("## Requirements\n\n")
                
                for cat in categories:
                    cat_name = cat['name']
                    f.write(f"### {cat_name}\n\n")
                    
                    if cat['description']:
                        f.write(f"*{cat['description']}*\n\n")
                    
                    # Get groups for this category (use data directly to avoid deadlock)
                    groups = []
                    if "groups" in data:
                        for group_id, group_data in data["groups"].items():
                            if group_data.get("category") == cat_name:
                                group_info = dict(group_data)
                                group_info["id"] = group_id
                                group_info["subgroup_count"] = len(group_data.get("subgroups", {}))
                                groups.append(group_info)
                        groups = sorted(groups, key=lambda x: x["id"])
                    
                    # Get requirements for this category (use data directly to avoid deadlock)
                    requirements = []
                    for req_id, req_data in data["requirements"].items():
                        if req_data.get("category") == cat_name:
                            req_info = dict(req_data)
                            req_info["id"] = req_id
                            requirements.append(req_info)
                    requirements = sorted(requirements, key=lambda x: x["id"])
                    
                    # Group requirements by group_id and subgroup_id
                    grouped_reqs = {}
                    ungrouped_reqs = []
                    
                    for req in requirements:
                        group_id = req.get('group_id')
                        subgroup_id = req.get('subgroup_id')
                        
                        if group_id:
                            if group_id not in grouped_reqs:
                                grouped_reqs[group_id] = {'grouped': [], 'subgroups': {}}
                            
                            if subgroup_id:
                                if subgroup_id not in grouped_reqs[group_id]['subgroups']:
                                    grouped_reqs[group_id]['subgroups'][subgroup_id] = []
                                grouped_reqs[group_id]['subgroups'][subgroup_id].append(req)
                            else:
                                grouped_reqs[group_id]['grouped'].append(req)
                        else:
                            ungrouped_reqs.append(req)
                    
                    # Write grouped requirements
                    for group in groups:
                        group_id = group['id']
                        f.write(f"#### {group_id}. {group['name']}\n\n")
                        if group.get('description'):
                            f.write(f"*{group['description']}*\n\n")
                        
                        if group_id in grouped_reqs:
                            # Write direct group requirements
                            for req in grouped_reqs[group_id]['grouped']:
                                self._write_requirement_to_markdown(f, req, data)
                            
                            # Write subgroup requirements
                            subgroups_data = data.get("groups", {}).get(group_id, {}).get("subgroups", {})
                            for subgroup_id, subgroup_info in subgroups_data.items():
                                if subgroup_id in grouped_reqs[group_id]['subgroups']:
                                    f.write(f"##### {subgroup_id}. {subgroup_info['name']}\n\n")
                                    if subgroup_info.get('description'):
                                        f.write(f"*{subgroup_info['description']}*\n\n")
                                    
                                    for req in grouped_reqs[group_id]['subgroups'][subgroup_id]:
                                        self._write_requirement_to_markdown(f, req, data)
                    
                    # Write ungrouped requirements
                    if ungrouped_reqs:
                        f.write("#### Other Requirements\n\n")
                        for req in ungrouped_reqs:
                            self._write_requirement_to_markdown(f, req, data)
                    
                    if not requirements:
                        f.write("*No requirements in this category*\n\n")
                
                # Summary section
                f.write("## Summary\n\n")
                f.write("### Requirements by Category\n\n")
                f.write("| Category | Count | Description |\n")
                f.write("|----------|-------|-------------|\n")
                for cat in categories:
                    req_count = cat['requirement_count']
                    desc = cat['description'] or "No description"
                    f.write(f"| {cat['name']} | {req_count} | {desc} |\n")
                f.write("\n")
                
                # Subsystem impact
                f.write("### Subsystem Impact\n\n")
                subsystem_count = {}
                for req_data in data['requirements'].values():
                    for subsystem in req_data.get('subsystems', []):
                        subsystem_count[subsystem] = subsystem_count.get(subsystem, 0) + 1
                
                if subsystem_count:
                    sorted_subsystems = sorted(subsystem_count.items(), key=lambda x: x[1], reverse=True)
                    f.write("| Subsystem | Requirement Count |\n")
                    f.write("|-----------|------------------|\n")
                    for subsystem, count in sorted_subsystems:
                        f.write(f"| **{subsystem}** | {count} |\n")
                    f.write("\n")
                
                f.write(f"*Document generated on {datetime.now().isoformat()}*\n")
            
            print(f"Markdown file exported to: {output_file}")
            return True
            
        except Exception as e:
            print(f"Error exporting markdown: {e}")
            return False
        finally:
            self._release_lock(lock_fd)


def main():
    """Command line interface for requirements management."""
    parser = argparse.ArgumentParser(description="Requirements Management System")
    parser.add_argument("--file", default="requirements.json", 
                       help="JSON file to store requirements")
    
    subparsers = parser.add_subparsers(dest="command", help="Available commands")
    
    # Add category command
    add_cat_parser = subparsers.add_parser("add-category", help="Add a new category")
    add_cat_parser.add_argument("name", help="Category name")
    add_cat_parser.add_argument("--description", default="", help="Category description")
    
    # List categories command
    subparsers.add_parser("list-categories", help="List all categories")
    
    # Group management commands
    add_group_parser = subparsers.add_parser("add-group", help="Add a group with specific ID")
    add_group_parser.add_argument("group_id", help="Group ID (e.g., '1')")
    add_group_parser.add_argument("group_name", help="Group name")
    add_group_parser.add_argument("category", help="Category name")
    add_group_parser.add_argument("--description", default="", help="Group description")
    
    add_subgroup_parser = subparsers.add_parser("add-subgroup", help="Add a subgroup with specific ID")
    add_subgroup_parser.add_argument("subgroup_id", help="Subgroup ID (e.g., '1.1')")
    add_subgroup_parser.add_argument("subgroup_name", help="Subgroup name")
    add_subgroup_parser.add_argument("group_id", help="Parent group ID")
    add_subgroup_parser.add_argument("--description", default="", help="Subgroup description")
    
    list_groups_parser = subparsers.add_parser("list-groups", help="List groups")
    list_groups_parser.add_argument("--category", help="Filter by category")
    
    list_subgroups_parser = subparsers.add_parser("list-subgroups", help="List subgroups")
    list_subgroups_parser.add_argument("--group-id", help="Filter by group ID")
    
    # Add requirement command
    add_req_parser = subparsers.add_parser("add-requirement", help="Add a new requirement")
    add_req_parser.add_argument("title", help="Requirement title")
    add_req_parser.add_argument("category", help="Category name")
    add_req_parser.add_argument("--id", help="Custom requirement ID (auto-generated if not provided)")
    add_req_parser.add_argument("--subsystems", nargs="*", default=[], 
                               help="List of subsystems")
    add_req_parser.add_argument("--description", default="", help="Requirement description")
    add_req_parser.add_argument("--no-test", action="store_true", 
                               help="Mark requirement as not needing testing")
    add_req_parser.add_argument("--test-files", nargs="*", default=[], 
                               help="List of test files")
    add_req_parser.add_argument("--group-id", help="Group ID to assign requirement to")
    add_req_parser.add_argument("--subgroup-id", help="Subgroup ID to assign requirement to")
    
    # List requirements commands
    subparsers.add_parser("list-all", help="List all requirements")
    
    list_cat_parser = subparsers.add_parser("list-by-category", 
                                           help="List requirements by category")
    list_cat_parser.add_argument("category", help="Category name")
    
    # Read requirement command
    read_parser = subparsers.add_parser("read", help="Read a specific requirement")
    read_parser.add_argument("id", help="Requirement ID")
    
    # Export markdown command
    export_parser = subparsers.add_parser("export-markdown", help="Export requirements to markdown file")
    export_parser.add_argument("--output", default="requirements_generated.md", 
                              help="Output markdown file name (default: requirements_generated.md)")
    
    # List requirements without tests
    subparsers.add_parser("list-without-tests", help="List requirements needing tests but without assigned tests")
    
    # Independent test management commands
    add_test_parser = subparsers.add_parser("add-test", help="Add an independent test")
    add_test_parser.add_argument("test_id", help="Test ID (e.g., 'TEST-GRID-001')")
    add_test_parser.add_argument("test_name", help="Test name")
    add_test_parser.add_argument("test_file", help="Test file path")
    add_test_parser.add_argument("--description", default="", help="Test description")
    add_test_parser.add_argument("--type", default="unit", choices=["unit", "integration", "e2e", "performance"],
                                help="Test type")
    
    update_test_parser = subparsers.add_parser("update-test-status", help="Update status of an independent test")
    update_test_parser.add_argument("test_id", help="Test ID")
    update_test_parser.add_argument("status", choices=["pending", "in_progress", "passed", "failed"],
                                   help="Test status")
    
    list_tests_parser = subparsers.add_parser("list-tests", help="List independent tests")
    list_tests_parser.add_argument("--type", choices=["unit", "integration", "e2e", "performance"],
                                  help="Filter by test type")
    list_tests_parser.add_argument("--status", choices=["pending", "in_progress", "passed", "failed"],
                                  help="Filter by test status")
    
    # Test assignment commands
    assign_test_parser = subparsers.add_parser("assign-test", help="Assign test to requirement")
    assign_test_parser.add_argument("requirement_id", help="Requirement ID")
    assign_test_parser.add_argument("test_id", help="Test ID")
    
    unassign_test_parser = subparsers.add_parser("unassign-test", help="Unassign test from requirement")
    unassign_test_parser.add_argument("requirement_id", help="Requirement ID")
    unassign_test_parser.add_argument("test_id", help="Test ID")
    
    # Validation status commands
    validation_parser = subparsers.add_parser("check-validation", help="Check validation status of requirement")
    validation_parser.add_argument("requirement_id", help="Requirement ID")
    
    clear_tests_parser = subparsers.add_parser("clear-all-tests", help="Clear all test statuses to pending")
    
    args = parser.parse_args()
    
    if not args.command:
        parser.print_help()
        return
    
    manager = RequirementsManager(args.file)
    
    try:
        if args.command == "add-category":
            manager.add_category(args.name, args.description)
        
        elif args.command == "list-categories":
            categories = manager.list_categories()
            if categories:
                print("\nCategories:")
                print("-" * 60)
                for cat in categories:
                    print(f"{cat['name']:<20} | {cat['requirement_count']:<5} reqs | {cat['description']}")
            else:
                print("No categories found")
        
        elif args.command == "add-group":
            manager.add_group(args.group_id, args.group_name, args.category, args.description)
        
        elif args.command == "add-subgroup":
            manager.add_subgroup(args.subgroup_id, args.subgroup_name, args.group_id, args.description)
        
        elif args.command == "list-groups":
            groups = manager.list_groups(args.category)
            if groups:
                filter_text = f" in category '{args.category}'" if args.category else ""
                print(f"\nGroups{filter_text} ({len(groups)} total):")
                print("-" * 80)
                for group in groups:
                    subgroup_count = group.get('subgroup_count', 0)
                    print(f"{group['id']:<8} | {group['category']:<15} | {subgroup_count:<3} subgroups | {group['name']}")
                    if group.get('description'):
                        print(f"         | Description: {group['description']}")
            else:
                filter_text = f" in category '{args.category}'" if args.category else ""
                print(f"No groups found{filter_text}")
        
        elif args.command == "list-subgroups":
            subgroups = manager.list_subgroups(args.group_id)
            if subgroups:
                if args.group_id:
                    print(f"\nSubgroups in group '{args.group_id}':")
                    for subgroup_id, subgroup_data in subgroups[args.group_id].items():
                        print(f"  {subgroup_id}: {subgroup_data['name']}")
                        if subgroup_data.get('description'):
                            print(f"    Description: {subgroup_data['description']}")
                else:
                    print(f"\nAll subgroups ({sum(len(subs) for subs in subgroups.values())} total):")
                    print("-" * 60)
                    for group_id, subs in subgroups.items():
                        print(f"{group_id}:")
                        for subgroup_id, subgroup_data in subs.items():
                            print(f"  {subgroup_id}: {subgroup_data['name']}")
            else:
                if args.group_id:
                    print(f"No subgroups found in group '{args.group_id}'")
                else:
                    print("No subgroups found")
        
        elif args.command == "add-requirement":
            test_needed = not args.no_test
            manager.add_requirement(args.id, args.title, args.category, 
                                  args.subsystems, args.description, test_needed, args.test_files,
                                  args.group_id, args.subgroup_id)
        
        elif args.command == "list-all":
            requirements = manager.list_all_requirements()
            if requirements:
                print(f"\nAll Requirements ({len(requirements)} total):")
                print("-" * 80)
                for req in requirements:
                    subsystems_str = ", ".join(req.get("subsystems", []))
                    print(f"{req['id']:<12} | {req['category']:<15} | {req['title']}")
                    if subsystems_str:
                        print(f"             | Subsystems: {subsystems_str}")
            else:
                print("No requirements found")
        
        elif args.command == "list-by-category":
            requirements = manager.list_requirements_by_category(args.category)
            if requirements:
                print(f"\nRequirements in category '{args.category}' ({len(requirements)} total):")
                print("-" * 80)
                for req in requirements:
                    subsystems_str = ", ".join(req.get("subsystems", []))
                    print(f"{req['id']:<12} | {req['title']}")
                    if subsystems_str:
                        print(f"             | Subsystems: {subsystems_str}")
            else:
                print(f"No requirements found in category '{args.category}'")
        
        elif args.command == "read":
            requirement = manager.get_requirement(args.id)
            if requirement:
                print(f"\nRequirement: {requirement['id']}")
                print("-" * 40)
                print(f"Title: {requirement['title']}")
                print(f"Category: {requirement['category']}")
                
                # Group/subgroup information
                if requirement.get('group_id'):
                    print(f"Group: {requirement['group_id']}")
                if requirement.get('subgroup_id'):
                    print(f"Subgroup: {requirement['subgroup_id']}")
                
                print(f"Status: {requirement.get('status', 'active')}")
                if requirement.get('subsystems'):
                    print(f"Subsystems: {', '.join(requirement['subsystems'])}")
                if requirement.get('description'):
                    print(f"Description: {requirement['description']}")
                
                # Test information
                test_needed = requirement.get('test_needed', True)
                if test_needed:
                    # Show validation status based on assigned tests
                    validation = manager.get_requirement_validation_status(requirement['id'])
                    if "error" not in validation:
                        print(f"Validation Status: {validation['validation_status'].upper()}")
                        summary = validation['test_summary']
                        if summary['total'] > 0:
                            print(f"Assigned Tests: {summary['total']} (Passed: {summary['passed']}, Failed: {summary['failed']}, Pending: {summary['pending']})")
                            for test in validation['assigned_tests']:
                                print(f"  - {test['id']}: {test['name']} ({test['status']})")
                        else:
                            print("Assigned Tests: None")
                else:
                    print("Testing: Not Required")
                
                print(f"Created: {requirement.get('created', 'N/A')}")
                print(f"Modified: {requirement.get('last_modified', 'N/A')}")
            else:
                print(f"Requirement '{args.id}' not found")
        
        elif args.command == "export-markdown":
            output_path = os.path.join(os.path.dirname(args.file), args.output)
            if manager.export_markdown(output_path):
                print("Markdown export completed successfully")
            else:
                print("Markdown export failed")
        
        elif args.command == "list-without-tests":
            requirements = manager.list_requirements_without_tests()
            if requirements:
                print(f"\nRequirements needing tests but without assigned tests ({len(requirements)} total):")
                print("-" * 80)
                for req in requirements:
                    print(f"{req['id']:<12} | {req['category']:<15} | {req['title']}")
            else:
                print("All requirements that need testing have tests assigned")
        
        elif args.command == "add-test":
            manager.add_test(args.test_id, args.test_name, args.test_file, 
                           args.description, args.type)
        
        elif args.command == "update-test-status":
            manager.update_test_status(args.test_id, args.status)
        
        elif args.command == "list-tests":
            tests = manager.list_tests(args.type, args.status)
            if tests:
                filter_info = []
                if args.type:
                    filter_info.append(f"type={args.type}")
                if args.status:
                    filter_info.append(f"status={args.status}")
                filter_str = f" ({', '.join(filter_info)})" if filter_info else ""
                
                print(f"\nIndependent Tests{filter_str} ({len(tests)} total):")
                print("-" * 80)
                for test in tests:
                    print(f"{test['id']:<15} | {test['type']:<12} | {test['status']:<12} | {test['name']}")
                    print(f"{'':>15} | File: {test['file']}")
                    if test.get('description'):
                        print(f"{'':>15} | Description: {test['description']}")
            else:
                filter_str = ""
                if args.type or args.status:
                    filters = []
                    if args.type:
                        filters.append(f"type '{args.type}'")
                    if args.status:
                        filters.append(f"status '{args.status}'")
                    filter_str = f" with {' and '.join(filters)}"
                print(f"No tests found{filter_str}")
        
        elif args.command == "assign-test":
            manager.assign_test_to_requirement(args.requirement_id, args.test_id)
        
        elif args.command == "unassign-test":
            manager.unassign_test_from_requirement(args.requirement_id, args.test_id)
        
        elif args.command == "check-validation":
            status = manager.get_requirement_validation_status(args.requirement_id)
            if "error" in status:
                print(status["error"])
            else:
                req_id = status["requirement_id"]
                validation = status["validation_status"]
                summary = status["test_summary"]
                tests = status["assigned_tests"]
                
                print(f"\nValidation Status for {req_id}:")
                print("-" * 50)
                print(f"Overall Status: {validation.upper()}")
                print(f"Total Tests: {summary['total']}")
                if summary['total'] > 0:
                    print(f"  Passed: {summary['passed']}")
                    print(f"  Failed: {summary['failed']}")
                    print(f"  Pending: {summary['pending']}")
                    print(f"  In Progress: {summary['in_progress']}")
                    
                    print(f"\nAssigned Tests:")
                    for test in tests:
                        print(f"  {test['id']:<15} | {test['type']:<12} | {test['status']:<12} | {test['name']}")
                        print(f"  {'':>15} | File: {test['file']}")
        
        elif args.command == "clear-all-tests":
            manager.clear_all_test_statuses()
    
    except Exception as e:
        print(f"Error: {e}", file=sys.stderr)
        sys.exit(1)


if __name__ == "__main__":
    main()