#!/bin/bash

# Test syntax of all .plcr files in pelcrexamples directory

PEXDIR="pelcrexamples"
MPIR_HOME="/usr/local"
EXECS="build/combustion.mac"
LOOP=100000
BUILDDIR="build"
LOGDIR="LOGS"

# Create directories
mkdir -p "$LOGDIR" "GML" "REPORTS" "scripts" "OUTPUT"

# Check if executable exists
if [ ! -f "$EXECS" ]; then
    echo "Error: $EXECS not found. Run 'make gcombustion' first."
    exit 1
fi

# Array to track results
declare -a passed
declare -a failed

echo "Testing all .plcr files in $PEXDIR..."
echo "=============================================="

# Test each .plcr file
for testfile in "$PEXDIR"/*.plcr; do
    if [ -f "$testfile" ]; then
        filename=$(basename "$testfile")
        logfile="$LOGDIR/test_${filename%.plcr}.log"
        
        echo -n "Testing $filename ... "
        
        # Run the parser test
        if printf '#setdir "%s" ; #open "%s"\n' "$PEXDIR" "$filename" | \
           "$MPIR_HOME/bin/mpirun" -np 1 "$EXECS" -- -loop "$LOOP" > "$logfile" 2>&1; then
            echo "✓ OK"
            passed+=("$filename")
        else
            echo "✗ FAILED"
            failed+=("$filename")
            echo "  Error log: tail -20 $logfile"
        fi
    fi
done

echo ""
echo "=============================================="
echo "Summary:"
echo "  Passed: ${#passed[@]}"
echo "  Failed: ${#failed[@]}"

if [ ${#failed[@]} -gt 0 ]; then
    echo ""
    echo "Failed files:"
    for f in "${failed[@]}"; do
        echo "  - $f"
    done
    echo ""
    echo "For error details, check LOGS/test_*.log files"
    exit 1
else
    echo ""
    echo "All tests passed!"
    exit 0
fi
