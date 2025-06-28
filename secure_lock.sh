#!/bin/bash

# Usage: ./secure_lock.sh lock <directory>
#        ./secure_lock.sh unlock <directory>

ACTION="$1"
TARGET="$2"

if [ -z "$ACTION" ] || [ -z "$TARGET" ]; then
  echo "Usage: $0 [lock|unlock] <directory>"
  exit 1
fi

DIR="$(realpath "$TARGET")"

if [ ! -d "$DIR" ]; then
  echo "Error: '$DIR' is not a valid directory."
  exit 1
fi

lock_dir() {
  echo "Locking $DIR recursively (requires sudo)..."
  sudo find "$DIR" -type f -exec chmod a-w {} \;
  sudo find "$DIR" -type d -exec chmod a+rx,g-w,o-w {} \;
  echo "Locked: files are read-only, directories are enterable."
}

unlock_dir() {
  echo "Unlocking $DIR recursively (requires sudo)..."
  sudo find "$DIR" -type f -exec chmod u+rw {} \;
  sudo find "$DIR" -type d -exec chmod u+rwx {} \;
  echo "Unlocked: writable by owner again."
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
