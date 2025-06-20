#!/bin/bash

# Static analysis of requirement coverage by examining test files
# This provides instant coverage analysis without running any tests

set -e

PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
BLUE='\033[0;34m'
CYAN='\033[0;36m'
NC='\033[0m' # No Color

echo -e "${BLUE}========================================${NC}"
echo -e "${BLUE}Static Requirement Coverage Analysis${NC}"
echo -e "${BLUE}========================================${NC}"
echo "Scanning test files for requirement coverage (no test execution)..."
echo ""

# Create temp directory
TEMP_DIR="/tmp/req_static_$$"
mkdir -p "$TEMP_DIR"

# Find all test files with requirement patterns
echo -e "${YELLOW}Scanning for requirement tests...${NC}"

# Find requirement test files
find "$PROJECT_ROOT/core" "$PROJECT_ROOT/tests" -type f -name "*.cpp" 2>/dev/null | \
    grep -v build | \
    while read -r file; do
        if grep -q "REQ_[0-9_]*" "$file" 2>/dev/null; then
            echo "$file"
        fi
    done > "$TEMP_DIR/files_with_reqs.txt"

# Process each file
echo -e "${YELLOW}Extracting requirement information...${NC}"

> "$TEMP_DIR/requirements.txt"
> "$TEMP_DIR/test_details.csv"

while IFS= read -r file; do
    # Extract all REQ patterns from the file
    grep -o "REQ_[0-9_]*" "$file" 2>/dev/null >> "$TEMP_DIR/requirements.txt" || true
    
    # Extract test functions with REQ patterns
    grep "TEST.*REQ_[0-9_]*" "$file" 2>/dev/null | while read -r line; do
        # Use simpler pattern matching
        # Extract content between TEST( and )
        if echo "$line" | grep -q "TEST"; then
            # Extract the part between parentheses
            content=$(echo "$line" | sed -n 's/.*TEST[_F]*(\([^)]*\)).*/\1/p')
            if [[ -n "$content" ]]; then
                # Split by comma
                test_suite=$(echo "$content" | cut -d',' -f1 | sed 's/^ *//;s/ *$//')
                test_name=$(echo "$content" | cut -d',' -f2 | sed 's/^ *//;s/ *$//')
                
                # Extract REQ ID from test name
                req_num=$(echo "$test_name" | grep -o "REQ_[0-9_]*" | head -1)
                if [[ -n "$req_num" ]]; then
                    # Convert REQ_1_2_3 to REQ-1.2.3
                    req_id=$(echo "$req_num" | sed 's/REQ_/REQ-/' | sed 's/_/\./g')
                    echo "$req_id,$test_suite,$test_name,$file" >> "$TEMP_DIR/test_details.csv"
                fi
            fi
        fi
    done
done < "$TEMP_DIR/files_with_reqs.txt"

# Count statistics
total_files=$(wc -l < "$TEMP_DIR/files_with_reqs.txt" || echo 0)
total_tests=$(wc -l < "$TEMP_DIR/test_details.csv" || echo 0)
unique_reqs=$(sort "$TEMP_DIR/requirements.txt" | uniq | wc -l || echo 0)

# Parse main requirements.md file to get all requirements
ALL_REQS_FILE="$PROJECT_ROOT/requirements.md"
EXCLUSIONS_FILE="$PROJECT_ROOT/requirements_exclusions.txt"
> "$TEMP_DIR/all_requirements.txt"
> "$TEMP_DIR/excluded_requirements.txt"

if [[ -f "$ALL_REQS_FILE" ]]; then
    # Extract all REQ-X.X.X patterns from requirements.md
    grep -o "REQ-[0-9]\+\.[0-9]\+\.[0-9]\+" "$ALL_REQS_FILE" | sort | uniq > "$TEMP_DIR/all_requirements.txt"
    total_requirements=$(wc -l < "$TEMP_DIR/all_requirements.txt" || echo 0)
else
    echo "Warning: requirements.md not found, coverage percentage unavailable"
    total_requirements=0
fi

# Load exclusions if available
excluded_count=0
if [[ -f "$EXCLUSIONS_FILE" ]]; then
    # Extract REQ-X.X.X patterns from exclusions (ignore comments)
    grep "^REQ-[0-9]\+\.[0-9]\+\.[0-9]\+" "$EXCLUSIONS_FILE" | cut -d' ' -f1 | sort | uniq > "$TEMP_DIR/excluded_requirements.txt" || true
    excluded_count=$(wc -l < "$TEMP_DIR/excluded_requirements.txt" || echo 0)
    
    # Remove excluded requirements from the total
    if [[ $excluded_count -gt 0 ]]; then
        comm -23 "$TEMP_DIR/all_requirements.txt" "$TEMP_DIR/excluded_requirements.txt" > "$TEMP_DIR/all_requirements_filtered.txt"
        mv "$TEMP_DIR/all_requirements_filtered.txt" "$TEMP_DIR/all_requirements.txt"
        total_requirements=$(wc -l < "$TEMP_DIR/all_requirements.txt" || echo 0)
    fi
fi

# Find covered requirements (normalize format)
cut -d',' -f1 "$TEMP_DIR/test_details.csv" | sed 's/\.$//g' | sort | uniq > "$TEMP_DIR/covered_requirements.txt"

# Count how many of the testable requirements are covered
covered_testable_count=0
if [[ -f "$TEMP_DIR/covered_requirements.txt" ]] && [[ -f "$TEMP_DIR/all_requirements.txt" ]]; then
    # Count requirements that are both covered AND in our testable list
    covered_testable_count=$(comm -12 "$TEMP_DIR/all_requirements.txt" "$TEMP_DIR/covered_requirements.txt" | wc -l)
fi

# Find missing requirements (after exclusions)
if [[ $total_requirements -gt 0 ]]; then
    # Get all requirements that are neither covered nor excluded
    comm -23 "$TEMP_DIR/all_requirements.txt" "$TEMP_DIR/covered_requirements.txt" > "$TEMP_DIR/missing_requirements.txt"
    missing_count=$(wc -l < "$TEMP_DIR/missing_requirements.txt" || echo 0)
    # Use the count of covered testable requirements for percentage
    coverage_percentage=$(( (covered_testable_count * 100) / total_requirements ))
else
    missing_count=0
    coverage_percentage=0
fi

echo ""
echo -e "${GREEN}========================================${NC}"
echo -e "${GREEN}Coverage Summary${NC}"
echo -e "${GREEN}========================================${NC}"
echo "Test files with requirements: $total_files"
echo "Total requirement tests found: $total_tests"
echo "Unique requirements found: $unique_reqs"
if [[ $total_requirements -gt 0 ]]; then
    original_total=$((total_requirements + excluded_count))
    echo -e "Total requirements in spec: ${YELLOW}$original_total${NC}"
    if [[ $excluded_count -gt 0 ]]; then
        echo -e "Excluded requirements: ${CYAN}$excluded_count${NC} (see requirements_exclusions.txt)"
        echo -e "Testable requirements: ${YELLOW}$total_requirements${NC}"
    fi
    echo -e "Coverage: ${GREEN}$coverage_percentage%${NC} ($covered_testable_count/$total_requirements)"
    echo -e "Missing requirements: ${RED}$missing_count${NC}"
fi

# Show files by subsystem
echo ""
echo -e "${CYAN}Requirement Test Files by Subsystem:${NC}"

grep "/core/" "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
    subsystem=$(echo "$file" | sed 's|.*/core/||' | cut -d'/' -f1)
    basename=$(basename "$file")
    test_count=$(grep -c "$file" "$TEMP_DIR/test_details.csv" || echo 0)
    printf "  %-20s: %3d tests in %s\n" "$subsystem" "$test_count" "$basename"
done

# Show integration tests
if grep -q "/tests/" "$TEMP_DIR/files_with_reqs.txt"; then
    echo ""
    echo -e "${CYAN}Integration Test Files:${NC}"
    grep "/tests/" "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
        basename=$(basename "$file")
        test_count=$(grep -c "$file" "$TEMP_DIR/test_details.csv" || echo 0)
        printf "  %3d tests in %s\n" "$test_count" "$basename"
    done
fi

# Show requirement categories
echo ""
echo -e "${CYAN}Requirements by Category:${NC}"

cat "$TEMP_DIR/requirements.txt" | sort | uniq | \
    sed 's/REQ_//' | cut -d'_' -f1 | sort | uniq -c | \
    while read count category; do
        printf "  Category %s: %d requirements\n" "$category" "$count"
    done

# Show all unique requirements
if [[ $unique_reqs -lt 50 ]]; then
    echo ""
    echo -e "${CYAN}All Requirements Found:${NC}"
    cat "$TEMP_DIR/requirements.txt" | sort | uniq | \
        sed 's/_/\./g' | sed 's/^/REQ-/' | \
        awk '{ 
            printf "%-15s", $0
            if (NR % 5 == 0) printf "\n"
        } 
        END { 
            if (NR % 5 != 0) printf "\n" 
        }'
fi

# Save detailed report
REPORT_FILE="$PROJECT_ROOT/requirement_coverage_static_report.csv"
echo "requirement_id,test_suite,test_name,file_path" > "$REPORT_FILE"
cat "$TEMP_DIR/test_details.csv" >> "$REPORT_FILE"

echo ""
echo -e "${GREEN}Detailed report saved to:${NC}"
echo "  $REPORT_FILE"

# Generate text summary report
SUMMARY_FILE="$PROJECT_ROOT/requirement_coverage_summary.txt"
echo "=====================================" > "$SUMMARY_FILE"
echo "Requirement Coverage Summary Report" >> "$SUMMARY_FILE"
echo "Generated: $(date)" >> "$SUMMARY_FILE"
echo "=====================================" >> "$SUMMARY_FILE"
echo "" >> "$SUMMARY_FILE"

echo "OVERVIEW" >> "$SUMMARY_FILE"
echo "--------" >> "$SUMMARY_FILE"
echo "Total test files with requirements: $total_files" >> "$SUMMARY_FILE"
echo "Total requirement tests found: $total_tests" >> "$SUMMARY_FILE"
echo "Unique requirements covered: $covered_testable_count" >> "$SUMMARY_FILE"
if [[ $total_requirements -gt 0 ]]; then
    echo "Total requirements in specification: $total_requirements" >> "$SUMMARY_FILE"
    echo "Coverage percentage: $coverage_percentage% ($covered_testable_count/$total_requirements)" >> "$SUMMARY_FILE"
    echo "Missing requirements: $missing_count" >> "$SUMMARY_FILE"
fi
echo "" >> "$SUMMARY_FILE"

echo "SUBSYSTEM BREAKDOWN" >> "$SUMMARY_FILE"
echo "------------------" >> "$SUMMARY_FILE"
grep "/core/" "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
    subsystem=$(echo "$file" | sed 's|.*/core/||' | cut -d'/' -f1)
    basename=$(basename "$file")
    test_count=$(grep -c "$file" "$TEMP_DIR/test_details.csv" || echo 0)
    printf "%-20s: %3d tests in %s\n" "$subsystem" "$test_count" "$basename" >> "$SUMMARY_FILE"
done

if grep -q "/tests/" "$TEMP_DIR/files_with_reqs.txt"; then
    echo "" >> "$SUMMARY_FILE"
    echo "INTEGRATION TESTS" >> "$SUMMARY_FILE"
    echo "----------------" >> "$SUMMARY_FILE"
    grep "/tests/" "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
        basename=$(basename "$file")
        test_count=$(grep -c "$file" "$TEMP_DIR/test_details.csv" || echo 0)
        printf "%3d tests in %s\n" "$test_count" "$basename" >> "$SUMMARY_FILE"
    done
fi

echo "" >> "$SUMMARY_FILE"
echo "REQUIREMENT CATEGORIES" >> "$SUMMARY_FILE"
echo "---------------------" >> "$SUMMARY_FILE"
cat "$TEMP_DIR/requirements.txt" | sort | uniq | \
    sed 's/REQ_//' | cut -d'_' -f1 | sort | uniq -c | \
    while read count category; do
        case $category in
            1) cat_name="Ground Plane Grid System" ;;
            2) cat_name="Voxel Placement System" ;;
            3) cat_name="Voxel Size Relationships" ;;
            4) cat_name="Visual Feedback System" ;;
            5) cat_name="Interaction Model" ;;
            6) cat_name="Performance Requirements" ;;
            7) cat_name="Technical Architecture" ;;
            8) cat_name="File Format and Storage" ;;
            9) cat_name="CLI Requirements" ;;
            *) cat_name="Category $category" ;;
        esac
        printf "Category %s - %-30s: %2d requirements\n" "$category" "$cat_name" "$count" >> "$SUMMARY_FILE"
    done

echo "" >> "$SUMMARY_FILE"
echo "DETAILED REQUIREMENT LIST" >> "$SUMMARY_FILE"
echo "------------------------" >> "$SUMMARY_FILE"
cut -d',' -f1,2,3 "$TEMP_DIR/test_details.csv" | sort | while IFS=',' read -r req_id test_suite test_name; do
    printf "%-12s %-35s %s\n" "$req_id" "$test_suite" "$test_name" >> "$SUMMARY_FILE"
done

echo "" >> "$SUMMARY_FILE"
echo "FILES ANALYZED" >> "$SUMMARY_FILE"
echo "--------------" >> "$SUMMARY_FILE"
cat "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
    echo "- $file" >> "$SUMMARY_FILE"
done

# Add missing requirements section to summary with descriptions
if [[ $missing_count -gt 0 ]]; then
    echo "" >> "$SUMMARY_FILE"
    echo "MISSING REQUIREMENTS" >> "$SUMMARY_FILE"
    echo "-------------------" >> "$SUMMARY_FILE"
    
    # Check if lookup file exists, if not create it
    LOOKUP_FILE="$PROJECT_ROOT/requirements_lookup.txt"
    if [[ ! -f "$LOOKUP_FILE" ]]; then
        echo "Creating requirements lookup file..."
        "$PROJECT_ROOT/tools/parse_requirements.sh" >/dev/null 2>&1 || true
    fi
    
    # Show missing requirements with descriptions
    while IFS= read -r req; do
        if [[ -f "$LOOKUP_FILE" ]]; then
            # Look up requirement description
            desc=$(grep "^$req|" "$LOOKUP_FILE" | cut -d'|' -f2- || echo "No description available")
            echo "- $req - $desc" >> "$SUMMARY_FILE"
        else
            echo "- $req" >> "$SUMMARY_FILE"
        fi
    done < "$TEMP_DIR/missing_requirements.txt"
fi

echo -e "${GREEN}Text summary saved to:${NC}"
echo "  $SUMMARY_FILE"

# Display key summary information to console
echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo -e "${BLUE}                    REQUIREMENT COVERAGE SUMMARY                 ${NC}"
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"
echo ""

# Overview stats
echo -e "${CYAN}📊 Coverage Statistics:${NC}"
echo "   • Total requirement tests: $total_tests"
echo "   • Unique requirements covered: $unique_reqs"
if [[ $total_requirements -gt 0 ]]; then
    original_total=$((total_requirements + excluded_count))
    echo "   • Total requirements in spec: $original_total"
    if [[ $excluded_count -gt 0 ]]; then
        echo -e "   • Excluded (non-testable): ${CYAN}$excluded_count${NC}"
        echo "   • Testable requirements: $total_requirements"
    fi
    if [[ $coverage_percentage -ge 90 ]]; then
        color="${GREEN}"
    elif [[ $coverage_percentage -ge 70 ]]; then
        color="${YELLOW}"
    else
        color="${RED}"
    fi
    echo -e "   • Coverage: ${color}$coverage_percentage%${NC} ($covered_testable_count/$total_requirements testable)"
    if [[ $missing_count -gt 0 ]]; then
        echo -e "   • Missing requirements: ${RED}$missing_count${NC}"
    fi
fi
echo "   • Test files analyzed: $total_files"
echo ""

# Coverage by category with nice formatting
echo -e "${CYAN}📋 Requirements by Category:${NC}"
cat "$TEMP_DIR/requirements.txt" | sort | uniq | \
    sed 's/REQ_//' | cut -d'_' -f1 | sort | uniq -c | \
    while read count category; do
        case $category in
            1) cat_name="Ground Plane Grid" ;;
            2) cat_name="Voxel Placement" ;;
            3) cat_name="Size Relationships" ;;
            4) cat_name="Visual Feedback" ;;
            5) cat_name="Interaction Model" ;;
            6) cat_name="Performance" ;;
            7) cat_name="Architecture" ;;
            8) cat_name="File Storage" ;;
            9) cat_name="CLI Features" ;;
            *) cat_name="Other" ;;
        esac
        printf "   • Category %s - %-20s: %2d requirements\n" "$category" "$cat_name" "$count"
    done

echo ""
echo -e "${CYAN}🧪 Test Distribution:${NC}"
grep "/core/" "$TEMP_DIR/files_with_reqs.txt" | while read -r file; do
    subsystem=$(echo "$file" | sed 's|.*/core/||' | cut -d'/' -f1)
    test_count=$(grep -c "$file" "$TEMP_DIR/test_details.csv" || echo 0)
    printf "   • %-20s: %3d tests\n" "$subsystem" "$test_count"
done

# Show integration tests separately
if grep -q "/tests/" "$TEMP_DIR/files_with_reqs.txt"; then
    int_count=$(grep -c "integration" "$TEMP_DIR/test_details.csv" || echo 0)
    if [[ $int_count -gt 0 ]]; then
        printf "   • %-20s: %3d tests\n" "integration" "$int_count"
    fi
fi

echo ""
echo -e "${BLUE}════════════════════════════════════════════════════════════════${NC}"

# Show missing requirements if any
if [[ $missing_count -gt 0 ]]; then
    echo ""
    echo -e "${RED}⚠️  Missing Requirements: $missing_count requirements without test coverage${NC}"
    
    # Check if lookup file exists
    LOOKUP_FILE="$PROJECT_ROOT/requirements_lookup.txt"
    if [[ ! -f "$LOOKUP_FILE" ]]; then
        "$PROJECT_ROOT/tools/parse_requirements.sh" >/dev/null 2>&1 || true
    fi
    
    # Show first few missing requirements with descriptions
    if [[ $missing_count -le 10 ]]; then
        echo ""
        while IFS= read -r req; do
            if [[ -f "$LOOKUP_FILE" ]]; then
                desc=$(grep "^$req|" "$LOOKUP_FILE" | cut -d'|' -f2- || echo "")
                if [[ -n "$desc" ]]; then
                    # Truncate long descriptions for console
                    if [[ ${#desc} -gt 60 ]]; then
                        desc="${desc:0:57}..."
                    fi
                    printf "   • %-12s %s\n" "$req" "$desc"
                else
                    echo "   • $req"
                fi
            else
                echo "   • $req"
            fi
        done < "$TEMP_DIR/missing_requirements.txt"
    else
        # Show just first 5 with descriptions
        echo ""
        echo "   Sample of missing requirements:"
        head -5 "$TEMP_DIR/missing_requirements.txt" | while IFS= read -r req; do
            if [[ -f "$LOOKUP_FILE" ]]; then
                desc=$(grep "^$req|" "$LOOKUP_FILE" | cut -d'|' -f2- || echo "")
                if [[ -n "$desc" ]]; then
                    # Truncate long descriptions for console
                    if [[ ${#desc} -gt 60 ]]; then
                        desc="${desc:0:57}..."
                    fi
                    printf "   • %-12s %s\n" "$req" "$desc"
                else
                    echo "   • $req"
                fi
            else
                echo "   • $req"
            fi
        done
        echo ""
        echo "   ... and $(( missing_count - 5 )) more"
    fi
    echo ""
    echo "   📄 See $SUMMARY_FILE for complete list with descriptions"
    echo ""
fi

# Cleanup
rm -rf "$TEMP_DIR"

echo ""
echo -e "${GREEN}✅ Analysis complete!${NC}"
echo ""
echo "📁 Generated files:"
echo "   • $REPORT_FILE (CSV)"
echo "   • $SUMMARY_FILE (Text)"
echo ""
echo "💡 Next steps:"
echo "   • Run './tools/requirement_coverage.sh' to execute tests"
echo "   • Review the text summary for detailed requirement mapping"
echo "   • Use 'ctest -R REQ_[0-9_]*' to run specific tests"