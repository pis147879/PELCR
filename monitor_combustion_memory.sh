#!/bin/sh

set -eu

interval="${1:-5}"
pattern="${2:-combustion.linux -- -I ciccio}"

echo "# timestamp pid_count total_rss_mb max_rss_mb max_hwm_mb"

while :; do
    pids="$(pgrep -f "$pattern" || true)"
    ts="$(date '+%Y-%m-%d %H:%M:%S')"

    if [ -z "$pids" ]; then
        echo "$ts 0 0 0 0"
        sleep "$interval"
        continue
    fi

    total_rss_kb=0
    max_rss_kb=0
    max_hwm_kb=0
    pid_count=0

    for pid in $pids; do
        status_file="/proc/$pid/status"
        if [ ! -r "$status_file" ]; then
            continue
        fi

        rss_kb="$(awk '/^VmRSS:/ { print $2; exit }' "$status_file")"
        hwm_kb="$(awk '/^VmHWM:/ { print $2; exit }' "$status_file")"

        rss_kb="${rss_kb:-0}"
        hwm_kb="${hwm_kb:-0}"

        total_rss_kb=$((total_rss_kb + rss_kb))
        if [ "$rss_kb" -gt "$max_rss_kb" ]; then
            max_rss_kb="$rss_kb"
        fi
        if [ "$hwm_kb" -gt "$max_hwm_kb" ]; then
            max_hwm_kb="$hwm_kb"
        fi
        pid_count=$((pid_count + 1))
    done

    total_rss_mb=$((total_rss_kb / 1024))
    max_rss_mb=$((max_rss_kb / 1024))
    max_hwm_mb=$((max_hwm_kb / 1024))

    echo "$ts $pid_count $total_rss_mb $max_rss_mb $max_hwm_mb"
    sleep "$interval"
done
