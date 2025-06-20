#!/bin/bash

# Parse requirements.md to create a lookup file with requirement IDs and descriptions
# Output format: REQ-X.X.X|Description text

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
REQUIREMENTS_FILE="$PROJECT_ROOT/requirements.md"
OUTPUT_FILE="$PROJECT_ROOT/requirements_lookup.txt"

if [[ ! -f "$REQUIREMENTS_FILE" ]]; then
    echo "Error: requirements.md not found at $REQUIREMENTS_FILE"
    exit 1
fi

echo "Parsing requirements from $REQUIREMENTS_FILE..."

# Create temporary file
TEMP_FILE="/tmp/req_parse_$$"

# Parse the requirements file
# Look for patterns like "- **REQ-X.X.X**: Description"
grep -E "^\s*-\s*\*\*REQ-[0-9]+\.[0-9]+\.[0-9]+\*\*:" "$REQUIREMENTS_FILE" | \
while IFS= read -r line; do
    # Extract requirement ID
    req_id=$(echo "$line" | grep -o "REQ-[0-9]\+\.[0-9]\+\.[0-9]\+")
    
    # Extract description (everything after the colon)
    description=$(echo "$line" | sed 's/.*\*\*://; s/^\s*//; s/\s*$//')
    
    # Output in format: REQ-X.X.X|Description
    echo "${req_id}|${description}"
done > "$TEMP_FILE"

# Sort and save to output file
sort "$TEMP_FILE" > "$OUTPUT_FILE"
rm -f "$TEMP_FILE"

# Show summary
total_reqs=$(wc -l < "$OUTPUT_FILE")
echo "Parsed $total_reqs requirements"
echo "Output saved to: $OUTPUT_FILE"

# Show sample output
echo ""
echo "Sample entries:"
head -5 "$OUTPUT_FILE" | while IFS='|' read -r req desc; do
    printf "%-12s %s\n" "$req" "$desc"
done