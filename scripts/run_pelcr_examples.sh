#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
. "$ROOT_DIR/scripts/test_stats_lib.sh"

EXAMPLES_DIR="${EXAMPLES_DIR:-$ROOT_DIR/pelcrexamples}"
LOG_DIR="${LOG_DIR:-$ROOT_DIR/LOGS/example-tests}"
NP="${NP:-32}"
LOOP="${LOOP:-1}"
TIMEOUT="${TIMEOUT:-0}"
MPIRUN="${MPIRUN:-/usr/local/bin/mpirun}"
EXEC="${EXEC:-$ROOT_DIR/build/combustion.mac}"
PATTERN="${PATTERN:-*.plcr}"
EXCLUDE_REGEX="${EXCLUDE_REGEX:-}"
FFI_SEQUENTIAL="${FFI_SEQUENTIAL:-1}"
SKIP_FFI="${SKIP_FFI:-0}"
RESUME="${RESUME:-1}"
RESET_RESUME="${RESET_RESUME:-0}"
STATE_FILE="${STATE_FILE:-$LOG_DIR/passed-np${NP}-loop${LOOP}.state}"
PASSED_STATS_FILE="${PASSED_STATS_FILE:-$LOG_DIR/passed-stats-np${NP}-loop${LOOP}.csv}"
PASSED_STATS_BY_NP_FILE="${PASSED_STATS_BY_NP_FILE:-$LOG_DIR/passed-stats-by-np-np${NP}-loop${LOOP}.csv}"

if [ "${1:-}" = "--help" ] || [ "${1:-}" = "-h" ]; then
	cat <<EOF
Run PELCR once for every matching example file.

Environment variables:
  NP=$NP
  LOOP=$LOOP
  TIMEOUT=$TIMEOUT
  PATTERN=$PATTERN
  EXAMPLES_DIR=$EXAMPLES_DIR
  LOG_DIR=$LOG_DIR
  MPIRUN=$MPIRUN
  EXEC=$EXEC
  EXCLUDE_REGEX=$EXCLUDE_REGEX
  FFI_SEQUENTIAL=$FFI_SEQUENTIAL
  SKIP_FFI=$SKIP_FFI
  RESUME=$RESUME
  RESET_RESUME=$RESET_RESUME
  STATE_FILE=$STATE_FILE
  PASSED_STATS_FILE=$PASSED_STATS_FILE
  PASSED_STATS_BY_NP_FILE=$PASSED_STATS_BY_NP_FILE
  CPU_COUNT=$CPU_COUNT

Examples:
  NP=32 LOOP=1 scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 PATTERN='dd*.plcr' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 EXCLUDE_REGEX='^(dd5|dd6)\\.plcr$' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 SKIP_FFI=1 EXCLUDE_REGEX='^(dd5|dd6)\\.plcr$' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 TIMEOUT=3600 scripts/run_pelcr_examples.sh
  RESET_RESUME=1 NP=32 LOOP=1 scripts/run_pelcr_examples.sh
EOF
	exit 0
fi

mkdir -p "$LOG_DIR"
mkdir -p "$(dirname "$STATE_FILE")"

if [ "$RESET_RESUME" != "0" ]; then
	rm -f "$STATE_FILE"
fi

if [ ! -f "$EXEC" ]; then
	printf 'Error: %s not found. Build it first or set EXEC.\n' "$EXEC" >&2
	exit 1
fi

summary="$LOG_DIR/summary-np${NP}-loop${LOOP}.csv"
sorted_summary="$LOG_DIR/summary-np${NP}-loop${LOOP}-by-elapsed.csv"
printf 'file,np_requested,np_effective,loop,ffi,status,real_seconds,elapsed_max,family_sum,log\n' > "$summary"
stats_write_passed_header "$PASSED_STATS_FILE"

fingerprint_file() {
	cksum "$1" | awk '{ print $1 ":" $2 }'
}

EXEC_FINGERPRINT="$(fingerprint_file "$EXEC")"

passed_key() {
	local example="$1"
	local base="$2"
	local np_effective="$3"
	local ffi="$4"
	local example_fingerprint

	example_fingerprint="$(fingerprint_file "$example")"
	printf '%s\t%s\tnp=%s\tnp_effective=%s\tloop=%s\ttimeout=%s\tffi=%s\texec=%s:%s\tmpirun=%s\n' \
		"$base" "$example_fingerprint" "$NP" "$np_effective" "$LOOP" "$TIMEOUT" "$ffi" "$EXEC" "$EXEC_FINGERPRINT" "$MPIRUN"
}

already_passed() {
	local key

	[ "$RESUME" != "0" ] || return 1
	[ -f "$STATE_FILE" ] || return 1
	key="$(passed_key "$1" "$2" "$3" "$4")"
	grep -Fqx -- "$key" "$STATE_FILE"
}

mark_passed() {
	local key
	local tmp

	key="$(passed_key "$1" "$2" "$3" "$4")"
	tmp="${STATE_FILE}.$$"

	if [ -f "$STATE_FILE" ]; then
		grep -Fvx -- "$key" "$STATE_FILE" > "$tmp" || true
	else
		: > "$tmp"
	fi

	printf '%s\n' "$key" >> "$tmp"
	mv "$tmp" "$STATE_FILE"
}

run_one() {
	local example="$1"
	local base status real elapsed family log cmd_status np_effective ffi

	base="$(basename "$example")"
	log="$LOG_DIR/${base%.plcr}-np${NP}-loop${LOOP}.out"
	status="OK"
	np_effective="$NP"
	ffi=0

	if grep -Eq '(^|[^[:alnum:]_])#uselib([^[:alnum:]_]|$)' "$example"; then
		ffi=1
		if [ "$SKIP_FFI" -ne 0 ]; then
			printf 'skip FFI: %s\n' "$base"
			printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$base" "$NP" "$np_effective" "$LOOP" "$ffi" "SKIP_FFI" 0 0 0 "" >> "$summary"
			return
		fi
		if [ "$FFI_SEQUENTIAL" -ne 0 ]; then
			np_effective=1
			log="$LOG_DIR/${base%.plcr}-np1-ffi-loop${LOOP}.out"
		fi
	fi

	if already_passed "$example" "$base" "$np_effective" "$ffi"; then
		real="$(stats_real_seconds "$log")"
		elapsed="$(stats_elapsed_max "$log")"
		family="$(stats_family_sum "$log")"
		stats_append_passed "$PASSED_STATS_FILE" "$base" "$NP" "$np_effective" "$LOOP" "$ffi" "PASS_CACHED" "$log"
		printf 'skip passed: %s\n' "$base"
		printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$base" "$NP" "$np_effective" "$LOOP" "$ffi" "PASS_CACHED" "$real" "$elapsed" "$family" "$log" >> "$summary"
		return
	fi

	printf '==> %s NP=%s LOOP=%s' "$base" "$np_effective" "$LOOP"
	if [ "$ffi" -eq 1 ]; then
		printf ' FFI'
	fi
	printf '\n'

	if [ "$TIMEOUT" -gt 0 ]; then
		printf '#setdir "%s" ; #open "%s"\n' "$(basename "$EXAMPLES_DIR")" "$base" \
			| /usr/bin/time -p timeout "$TIMEOUT" "$MPIRUN" -np "$np_effective" "$EXEC" -- -loop "$LOOP" \
			> "$log" 2>&1
		cmd_status=$?
	else
		printf '#setdir "%s" ; #open "%s"\n' "$(basename "$EXAMPLES_DIR")" "$base" \
			| /usr/bin/time -p "$MPIRUN" -np "$np_effective" "$EXEC" -- -loop "$LOOP" \
			> "$log" 2>&1
		cmd_status=$?
	fi

	if [ "$cmd_status" -ne 0 ]; then
		status="FAIL:$cmd_status"
	else
		mark_passed "$example" "$base" "$np_effective" "$ffi"
	fi

	real="$(stats_real_seconds "$log")"
	elapsed="$(stats_elapsed_max "$log")"
	family="$(stats_family_sum "$log")"

	if [ "$cmd_status" -eq 0 ]; then
		stats_append_passed "$PASSED_STATS_FILE" "$base" "$NP" "$np_effective" "$LOOP" "$ffi" "$status" "$log"
	fi

	printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' "$base" "$NP" "$np_effective" "$LOOP" "$ffi" "$status" "$real" "$elapsed" "$family" "$log" >> "$summary"
}

find "$EXAMPLES_DIR" -maxdepth 1 -type f -name "$PATTERN" | sort | while read -r example; do
	if [ -n "$EXCLUDE_REGEX" ] && basename "$example" | grep -Eq "$EXCLUDE_REGEX"; then
		printf 'skip: %s\n' "$(basename "$example")"
		continue
	fi
	run_one "$example"
done

{
	head -1 "$summary"
	tail -n +2 "$summary" | sort -t, -k8,8n -k7,7n
} > "$sorted_summary"

stats_write_by_np "$PASSED_STATS_FILE" "$PASSED_STATS_BY_NP_FILE"

printf 'summary: %s\n' "$summary"
printf 'by elapsed: %s\n' "$sorted_summary"
printf 'passed stats: %s\n' "$PASSED_STATS_FILE"
printf 'passed stats by np: %s\n' "$PASSED_STATS_BY_NP_FILE"
printf 'resume state: %s\n' "$STATE_FILE"
