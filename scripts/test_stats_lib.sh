#!/usr/bin/env bash

stats_logical_cpu_count() {
	if command -v getconf >/dev/null 2>&1; then
		getconf _NPROCESSORS_ONLN 2>/dev/null && return
	fi

	if command -v sysctl >/dev/null 2>&1; then
		sysctl -n hw.logicalcpu 2>/dev/null && return
	fi

	printf '1\n'
}

stats_detect_machine() {
	if command -v hostname >/dev/null 2>&1; then
		hostname -s 2>/dev/null && return
		hostname 2>/dev/null && return
	fi

	uname -n 2>/dev/null && return
	printf 'unknown\n'
}

MACHINE="${MACHINE:-$(stats_detect_machine)}"
CPU_COUNT="${CPU_COUNT:-$(stats_logical_cpu_count)}"
STATS_TAIL_LINES="${STATS_TAIL_LINES:-4000}"

stats_real_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	tail -n "$STATS_TAIL_LINES" "$1" | awk '/^real / { value=$2 } END { print value+0 }'
}

stats_user_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	tail -n "$STATS_TAIL_LINES" "$1" | awk '/^user / { value=$2 } END { print value+0 }'
}

stats_sys_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	tail -n "$STATS_TAIL_LINES" "$1" | awk '/^sys / { value=$2 } END { print value+0 }'
}

stats_elapsed_max() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	tail -n "$STATS_TAIL_LINES" "$1" | awk '/elapsed time/ { if ($NF > max) max=$NF } END { print max+0 }'
}

stats_family_sum() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	tail -n "$STATS_TAIL_LINES" "$1" | awk '/family reductions/ { sum += $NF } END { print sum+0 }'
}

stats_machine_load_percent() {
	awk -v user="$1" -v sys="$2" -v real="$3" -v cpus="$CPU_COUNT" \
		'BEGIN {
			if (real > 0 && cpus > 0) {
				printf "%.2f", ((user + sys) / (real * cpus)) * 100
			} else {
				printf "0.00"
			}
		}'
}

stats_write_passed_header() {
	printf 'file,machine,np_requested,np_effective,loop,ffi,status,real_seconds,elapsed_max,family_sum,cpu_user_seconds,cpu_sys_seconds,cpu_total_seconds,machine_load_percent,logical_cpus,log\n' > "$1"
}

stats_append_passed() {
	local stats_file="$1"
	local base="$2"
	local np_requested="$3"
	local np_effective="$4"
	local loop="$5"
	local ffi="$6"
	local status="$7"
	local log="$8"
	local real elapsed family user sys cpu_total load_percent

	real=0
	elapsed=0
	family=0
	user=0
	sys=0

	if [ -f "$log" ]; then
		real="$(stats_real_seconds "$log")"
		elapsed="$(stats_elapsed_max "$log")"
		family="$(stats_family_sum "$log")"
		user="$(stats_user_seconds "$log")"
		sys="$(stats_sys_seconds "$log")"
	fi

	cpu_total="$(awk -v user="$user" -v sys="$sys" 'BEGIN { printf "%.6f", user + sys }')"
	load_percent="$(stats_machine_load_percent "$user" "$sys" "$real")"

	printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
		"$base" "$MACHINE" "$np_requested" "$np_effective" "$loop" "$ffi" "$status" \
		"$real" "$elapsed" "$family" "$user" "$sys" "$cpu_total" \
		"$load_percent" "$CPU_COUNT" "$log" >> "$stats_file"
}

stats_write_by_np() {
	local stats_file="$1"
	local by_np_file="$2"

	{
		printf 'machine,np_effective,passed_count,total_real_seconds,avg_real_seconds,total_elapsed_max,avg_elapsed_max,total_family_reductions,total_cpu_seconds,avg_machine_load_percent,max_machine_load_percent,logical_cpus\n'
		awk -F, '
			NR == 1 {
				has_machine = ($2 == "machine")
				next
			}
			{
				if (has_machine) {
					machine = $2
					np = $4
					real_value = $8
					elapsed_value = $9
					family_value = $10
					cpu_value = $13
					load_value = $14
					cpus_value = $15
				} else {
					machine = "unknown"
					np = $3
					real_value = $7
					elapsed_value = $8
					family_value = $9
					cpu_value = $12
					load_value = $13
					cpus_value = $14
				}

				key = machine SUBSEP np
				count[key] += 1
				real[key] += real_value
				elapsed[key] += elapsed_value
				family[key] += family_value
				cpu[key] += cpu_value
				load[key] += load_value
				cpus[key] = cpus_value
				machines[key] = machine
				nps[key] = np
				if (count[key] == 1 || load_value > max_load[key]) {
					max_load[key] = load_value
				}
			}
			END {
				for (key in count) {
					printf "%s,%s,%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.6f,%.2f,%.2f,%s\n",
						machines[key], nps[key], count[key], real[key], real[key] / count[key],
						elapsed[key], elapsed[key] / count[key], family[key],
						cpu[key], load[key] / count[key], max_load[key], cpus[key]
				}
			}
		' "$stats_file" | sort -t, -k1,1 -k2,2n
	} > "$by_np_file"
}
