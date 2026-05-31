#!/bin/bash

# Test syntax of all .plcr files in pelcrexamples directory with 200s timeout per file

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
. "$SCRIPT_DIR/test_stats_lib.sh"

PEXDIR="pelcrexamples"
MPIR_HOME="/usr/local"
EXECS="build/combustion.mac"
LOOP=100000
BUILDDIR="build"
LOGDIR="LOGS"
TIMEOUT=200
RESUME="${RESUME:-1}"
STATE_FILE="${STATE_FILE:-$LOGDIR/test_all_plcr_sequential.passed}"
RESET_RESUME="${RESET_RESUME:-0}"
PASSED_STATS_FILE="${PASSED_STATS_FILE:-$LOGDIR/test_all_plcr_sequential_passed_stats.csv}"
PASSED_STATS_BY_NP_FILE="${PASSED_STATS_BY_NP_FILE:-$LOGDIR/test_all_plcr_sequential_passed_stats_by_np.csv}"

# Create directories
mkdir -p "$LOGDIR" "GML" "REPORTS" "scripts" "OUTPUT"
mkdir -p "$(dirname "$STATE_FILE")"

if [ "$RESET_RESUME" != "0" ]; then
    rm -f "$STATE_FILE"
fi

# Check if executable exists
if [ ! -f "$EXECS" ]; then
    echo "Error: $EXECS not found. Run 'make gcombustion' first."
    exit 1
fi

stats_write_passed_header "$PASSED_STATS_FILE"

fingerprint_file() {
    cksum "$1" | awk '{ print $1 ":" $2 }'
}

RUNNER_FINGERPRINT="$(fingerprint_file "$EXECS")"

passed_key() {
    local testfile="$1"
    local filename="$2"
    local test_fingerprint

    test_fingerprint="$(fingerprint_file "$testfile")"
    printf '%s\t%s\tloop=%s\ttimeout=%s\texec=%s:%s\tmpirun=%s/bin/mpirun\n' \
        "$filename" "$test_fingerprint" "$LOOP" "$TIMEOUT" "$EXECS" "$RUNNER_FINGERPRINT" "$MPIR_HOME"
}

already_passed() {
    local key

    [ "$RESUME" != "0" ] || return 1
    [ -f "$STATE_FILE" ] || return 1
    key="$(passed_key "$1" "$2")"
    grep -Fqx -- "$key" "$STATE_FILE"
}

mark_passed() {
    local key
    local tmp

    key="$(passed_key "$1" "$2")"
    tmp="${STATE_FILE}.$$"

    if [ -f "$STATE_FILE" ]; then
        grep -Fvx -- "$key" "$STATE_FILE" > "$tmp" || true
    else
        : > "$tmp"
    fi

    printf '%s\n' "$key" >> "$tmp"
    mv "$tmp" "$STATE_FILE"
}

# Arrays to track results
passed=()
failed=()
timedout=()
skipped=()

echo "Testing all .plcr files in $PEXDIR with ${TIMEOUT}s timeout per file"
echo "=============================================================================="

file_count=0
# Test each .plcr file
for testfile in "$PEXDIR"/*.plcr; do
    if [ -f "$testfile" ]; then
        file_count=$((file_count + 1))
        filename=$(basename "$testfile")
        logfile="$LOGDIR/test_${filename%.plcr}.log"

        if already_passed "$testfile" "$filename"; then
            printf "%-40s ... SKIP (already passed)\n" "Testing $filename"
            stats_append_passed "$PASSED_STATS_FILE" "$filename" 1 1 "$LOOP" 0 "PASS_CACHED" "$logfile"
            passed+=("$filename")
            skipped+=("$filename")
            continue
        fi
        
        printf "%-40s ... " "Testing $filename"
        
        # Run the parser test with timeout
        if printf '#setdir "%s" ; #open "%s"\n' "$PEXDIR" "$filename" | \
           /usr/bin/time -p timeout "$TIMEOUT" "$MPIR_HOME/bin/mpirun" -np 1 "$EXECS" -- -loop "$LOOP" > "$logfile" 2>&1; then
            mark_passed "$testfile" "$filename"
            stats_append_passed "$PASSED_STATS_FILE" "$filename" 1 1 "$LOOP" 0 "OK" "$logfile"
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

stats_write_by_np "$PASSED_STATS_FILE" "$PASSED_STATS_BY_NP_FILE"

echo ""
echo "=============================================================================="
echo "Summary ($file_count files considered):"
echo "  Passed:  ${#passed[@]}"
echo "  Skipped: ${#skipped[@]}"
echo "  Failed:  ${#failed[@]}"
echo "  Timeout: ${#timedout[@]}"
echo "  Passed stats: $PASSED_STATS_FILE"
echo "  Passed stats by np: $PASSED_STATS_BY_NP_FILE"
echo "  Resume state: $STATE_FILE"

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
