#!/bin/bash

# Usage: ./secure_lock.sh lock <directory>
#        ./secure_lock.sh unlock <directory>

ACTION="$1"
TARGET="$2"

if [ -z "$ACTION" ] || [ -z "$TARGET" ]; then
  echo "Usage: $0 [lock|unlock] <directory>"
  exit 1
fi

# Just use the target as provided (assume running from root)
# Remove trailing slashes
DIR="${TARGET%/}"
UNIT_IGNORE="unit_ignore"

if [ ! -d "$DIR" ]; then
  echo "Error: '$DIR' is not a valid directory."
  exit 1
fi

# Function to add path to unit_ignore
add_to_unit_ignore() {
  local path="$1"
  
  # Create unit_ignore if it doesn't exist
  if [ ! -f "$UNIT_IGNORE" ]; then
    echo "# Unit Test Exclusion List" > "$UNIT_IGNORE"
    echo "# Auto-managed by secure_lock.sh" >> "$UNIT_IGNORE"
    echo "" >> "$UNIT_IGNORE"
  fi
  
  # Ensure file ends with newline
  if [ -f "$UNIT_IGNORE" ] && [ -s "$UNIT_IGNORE" ]; then
    # Add newline if file doesn't end with one
    tail -c1 "$UNIT_IGNORE" | read -r _ || echo >> "$UNIT_IGNORE"
  fi
  
  # Check if path already exists (not commented)
  if ! grep -q "^[[:space:]]*${path}[[:space:]]*$" "$UNIT_IGNORE" 2>/dev/null; then
    echo "Adding $path to unit_ignore..."
    echo "$path" >> "$UNIT_IGNORE"
  else
    echo "$path already in unit_ignore"
  fi
}

# Function to remove path from unit_ignore
remove_from_unit_ignore() {
  local path="$1"
  
  if [ ! -f "$UNIT_IGNORE" ]; then
    return
  fi
  
  # Remove the path (exact match, not commented)
  if grep -q "^[[:space:]]*${path}[[:space:]]*$" "$UNIT_IGNORE" 2>/dev/null; then
    echo "Removing $path from unit_ignore..."
    # Use a temp file to avoid issues with sed -i on different platforms
    grep -v "^[[:space:]]*${path}[[:space:]]*$" "$UNIT_IGNORE" > "$UNIT_IGNORE.tmp"
    mv "$UNIT_IGNORE.tmp" "$UNIT_IGNORE"
  fi
}

lock_dir() {
  echo "Locking $DIR recursively (requires sudo)..."
  sudo find "$DIR" -type f -exec chmod a-w {} \;
  sudo find "$DIR" -type d -exec chmod a+rx,g-w,o-w {} \;
  echo "Locked: files are read-only, directories are enterable."
  
  # Add to unit_ignore if this looks like a test directory or core subsystem
  local path_to_add="$DIR"
  # Ensure ./ prefix for consistency
  if [[ "$path_to_add" != ./* ]]; then
    path_to_add="./$path_to_add"
  fi
  
  # Add to unit_ignore
  add_to_unit_ignore "$path_to_add"
}

unlock_dir() {
  echo "Unlocking $DIR recursively (requires sudo)..."
  sudo find "$DIR" -type f -exec chmod u+rw {} \;
  sudo find "$DIR" -type d -exec chmod u+rwx {} \;
  echo "Unlocked: writable by owner again."
  
  # Remove from unit_ignore
  local path_to_remove="$DIR"
  # Ensure ./ prefix for consistency
  if [[ "$path_to_remove" != ./* ]]; then
    path_to_remove="./$path_to_remove"
  fi
  
  remove_from_unit_ignore "$path_to_remove"
}

case "$ACTION" in
  lock)
    lock_dir
    ;;
  unlock)
    unlock_dir
    ;;
  *)
    echo "Invalid action: $ACTION (use 'lock' or 'unlock')"
    exit 1
    ;;
esac
