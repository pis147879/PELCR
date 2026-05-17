#!/bin/bash

# Test syntax of all .plcr files in pelcrexamples directory with 200s timeout per file

PEXDIR="pelcrexamples"
MPIR_HOME="/usr/local"
EXECS="build/combustion.mac"
LOOP=100000
BUILDDIR="build"
LOGDIR="LOGS"
TIMEOUT=200

# Create directories
mkdir -p "$LOGDIR" "GML" "REPORTS" "scripts" "OUTPUT"

# Check if executable exists
if [ ! -f "$EXECS" ]; then
    echo "Error: $EXECS not found. Run 'make gcombustion' first."
    exit 1
fi

# Arrays to track results
passed=()
failed=()
timedout=()

echo "Testing all .plcr files in $PEXDIR with ${TIMEOUT}s timeout per file"
echo "=============================================================================="

file_count=0
# Test each .plcr file
for testfile in "$PEXDIR"/*.plcr; do
    if [ -f "$testfile" ]; then
        file_count=$((file_count + 1))
        filename=$(basename "$testfile")
        logfile="$LOGDIR/test_${filename%.plcr}.log"
        
        printf "%-40s ... " "Testing $filename"
        
        # Run the parser test with timeout
        if timeout $TIMEOUT bash -c "printf '#setdir \"%s\" ; #open \"%s\"\n' \"$PEXDIR\" \"$filename\" | \"$MPIR_HOME/bin/mpirun\" -np 1 \"$EXECS\" -- -loop $LOOP > \"$logfile\" 2>&1"; then
            echo "✓ PASS"
            passed+=("$filename")
        else
            exit_code=$?
            if [ $exit_code -eq 124 ]; then
                echo "⏱ TIMEOUT (${TIMEOUT}s)"
                timedout+=("$filename")
            else
                echo "✗ FAIL"
                failed+=("$filename")
            fi
        fi
    fi
done

echo ""
echo "=============================================================================="
echo "Summary ($file_count files tested):"
echo "  Passed:  ${#passed[@]}"
echo "  Failed:  ${#failed[@]}"
echo "  Timeout: ${#timedout[@]}"

if [ ${#failed[@]} -gt 0 ]; then
    echo ""
    echo "Failed files:"
    for f in "${failed[@]}"; do
        echo "  - $f"
    done
fi

if [ ${#timedout[@]} -gt 0 ]; then
    echo ""
    echo "Timeout files (${TIMEOUT}s limit exceeded):"
    for f in "${timedout[@]}"; do
        echo "  - $f"
    done
fi

if [ ${#failed[@]} -gt 0 ] || [ ${#timedout[@]} -gt 0 ]; then
    echo ""
    echo "For error details, check LOGS/test_*.log files"
    exit 1
else
    echo ""
    echo "All tests passed!"
    exit 0
fi
