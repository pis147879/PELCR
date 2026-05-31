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

CPU_COUNT="${CPU_COUNT:-$(stats_logical_cpu_count)}"

stats_real_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	awk '/^real / { value=$2 } END { print value+0 }' "$1"
}

stats_user_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	awk '/^user / { value=$2 } END { print value+0 }' "$1"
}

stats_sys_seconds() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	awk '/^sys / { value=$2 } END { print value+0 }' "$1"
}

stats_elapsed_max() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	awk '/elapsed time/ { if ($NF > max) max=$NF } END { print max+0 }' "$1"
}

stats_family_sum() {
	[ -f "$1" ] || {
		printf '0\n'
		return
	}
	awk '/family reductions/ { sum += $NF } END { print sum+0 }' "$1"
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
	printf 'file,np_requested,np_effective,loop,ffi,status,real_seconds,elapsed_max,family_sum,cpu_user_seconds,cpu_sys_seconds,cpu_total_seconds,machine_load_percent,logical_cpus,log\n' > "$1"
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

	printf '%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s,%s\n' \
		"$base" "$np_requested" "$np_effective" "$loop" "$ffi" "$status" \
		"$real" "$elapsed" "$family" "$user" "$sys" "$cpu_total" \
		"$load_percent" "$CPU_COUNT" "$log" >> "$stats_file"
}

stats_write_by_np() {
	local stats_file="$1"
	local by_np_file="$2"

	{
		printf 'np_effective,passed_count,total_real_seconds,avg_real_seconds,total_elapsed_max,avg_elapsed_max,total_family_reductions,total_cpu_seconds,avg_machine_load_percent,max_machine_load_percent,logical_cpus\n'
		awk -F, '
			NR == 1 { next }
			{
				np = $3
				count[np] += 1
				real[np] += $7
				elapsed[np] += $8
				family[np] += $9
				cpu[np] += $12
				load[np] += $13
				cpus[np] = $14
				if (count[np] == 1 || $13 > max_load[np]) {
					max_load[np] = $13
				}
			}
			END {
				for (np in count) {
					printf "%s,%d,%.6f,%.6f,%.6f,%.6f,%.0f,%.6f,%.2f,%.2f,%s\n",
						np, count[np], real[np], real[np] / count[np],
						elapsed[np], elapsed[np] / count[np], family[np],
						cpu[np], load[np] / count[np], max_load[np], cpus[np]
				}
			}
		' "$stats_file" | sort -t, -k1,1n
	} > "$by_np_file"
}
