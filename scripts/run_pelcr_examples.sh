#!/usr/bin/env bash
set -u

ROOT_DIR="$(cd "$(dirname "$0")/.." && pwd)"
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

Examples:
  NP=32 LOOP=1 scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 PATTERN='dd*.plcr' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 EXCLUDE_REGEX='^(dd5|dd6)\\.plcr$' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 SKIP_FFI=1 EXCLUDE_REGEX='^(dd5|dd6)\\.plcr$' scripts/run_pelcr_examples.sh
  NP=32 LOOP=1 TIMEOUT=3600 scripts/run_pelcr_examples.sh
EOF
	exit 0
fi

mkdir -p "$LOG_DIR"

summary="$LOG_DIR/summary-np${NP}-loop${LOOP}.csv"
sorted_summary="$LOG_DIR/summary-np${NP}-loop${LOOP}-by-elapsed.csv"
printf 'file,np_requested,np_effective,loop,ffi,status,real_seconds,elapsed_max,family_sum,log\n' > "$summary"

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
	fi

	real="$(awk '/^real / { value=$2 } END { print value+0 }' "$log")"
	elapsed="$(awk '/elapsed time/ { if ($NF > max) max=$NF } END { print max+0 }' "$log")"
	family="$(awk '/family reductions/ { sum += $NF } END { print sum+0 }' "$log")"

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

printf 'summary: %s\n' "$summary"
printf 'by elapsed: %s\n' "$sorted_summary"
