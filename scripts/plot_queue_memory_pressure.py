#!/usr/bin/env python3
import argparse
import datetime as dt
import glob
import html
import math
import os
import re
from pathlib import Path


LEGACY_EDGE_COUNTER_COLUMNS = [
    "time",
    "rank",
    "loops",
    "bip2",
    "bip3",
    "fires",
    "ones",
    "nofires",
    "temp",
    "nhot",
    "fra_hot",
    "edges_counter",
    "local_pending",
    "incoming_pending",
    "outgoing_pending",
    "global_physical_msgs",
    "nTickSend",
    "nFullSend",
]

LEGACY_PENDING_COLUMNS = LEGACY_EDGE_COUNTER_COLUMNS[:11] + ["pending_actions"] + LEGACY_EDGE_COUNTER_COLUMNS[12:]

DUP_GRAPH_SIZE_COLUMNS = (
    LEGACY_PENDING_COLUMNS[:12] + ["graph_nodes", "graph_edges"] + LEGACY_PENDING_COLUMNS[12:]
)

STATS_COLUMNS = [
    "time",
    "rank",
    "loops",
    "processed_actions",
    "edge_compositions",
    "fires",
    "one_optimizations",
    "failed_compositions",
    "graph_nodes",
    "hot_nodes",
    "cold_nodes",
    "nhot",
    "pending_actions",
    "graph_edges",
    "local_pending",
    "incoming_pending",
    "incoming_buffer_capacity",
    "incoming_buffer_pct",
    "incoming_buffer_hwm",
    "incoming_buffer_hwm_pct",
    "incoming_buffer_max_slots",
    "incoming_buffer_max_hwm",
    "outgoing_pending",
    "global_physical_msgs",
    "nTickSend",
    "nFullSend",
    "incoming_buffer_capacity_hwm",
    "incoming_buffer_grow_count",
    "incoming_buffer_shrink_count",
    "incoming_buffer_max_capacity",
]

LEGACY_EDGE_COUNTER_COLUMNS_WITH_EPOCH = ["wall_epoch"] + LEGACY_EDGE_COUNTER_COLUMNS
LEGACY_PENDING_COLUMNS_WITH_EPOCH = ["wall_epoch"] + LEGACY_PENDING_COLUMNS
DUP_GRAPH_SIZE_COLUMNS_WITH_EPOCH = ["wall_epoch"] + DUP_GRAPH_SIZE_COLUMNS
STATS_COLUMNS_WITH_EPOCH = ["wall_epoch"] + STATS_COLUMNS


def to_number(value):
    try:
        if "." in value or "e" in value.lower():
            return float(value)
        return int(value)
    except ValueError:
        return float("nan")


def rank_from_name(path):
    name = os.path.basename(path)
    match = re.search(r"rank[=.:-]?(\d+)", name)
    if match:
        return int(match.group(1))
    match = re.search(r"stats\.(\d+)\.log$", name)
    if match:
        return int(match.group(1))
    return 0


def expand_paths(patterns):
    paths = []
    for pattern in patterns:
        expanded = sorted(glob.glob(pattern))
        if expanded:
            paths.extend(expanded)
        elif os.path.exists(pattern):
            paths.append(pattern)
    return sorted(set(paths), key=lambda p: (rank_from_name(p), p))


def read_stats(path):
    columns = None
    rows = []
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            line = line.strip()
            if not line:
                continue
            if line.startswith("#"):
                candidate = line[1:].strip().split()
                if candidate and candidate[0] in ("time", "wall_epoch"):
                    columns = candidate
                continue

            parts = line.split()
            if columns is None:
                if len(parts) == len(STATS_COLUMNS_WITH_EPOCH):
                    columns = STATS_COLUMNS_WITH_EPOCH
                elif len(parts) == len(DUP_GRAPH_SIZE_COLUMNS_WITH_EPOCH):
                    columns = DUP_GRAPH_SIZE_COLUMNS_WITH_EPOCH
                elif len(parts) == len(LEGACY_PENDING_COLUMNS_WITH_EPOCH):
                    columns = LEGACY_PENDING_COLUMNS_WITH_EPOCH
                elif len(parts) == len(LEGACY_EDGE_COUNTER_COLUMNS_WITH_EPOCH):
                    columns = LEGACY_EDGE_COUNTER_COLUMNS_WITH_EPOCH
                elif len(parts) == len(STATS_COLUMNS):
                    columns = STATS_COLUMNS
                elif len(parts) == len(DUP_GRAPH_SIZE_COLUMNS):
                    columns = DUP_GRAPH_SIZE_COLUMNS
                elif len(parts) == len(LEGACY_PENDING_COLUMNS):
                    columns = LEGACY_PENDING_COLUMNS
                else:
                    columns = LEGACY_EDGE_COUNTER_COLUMNS
            if len(parts) < len(columns):
                continue

            row = {name: to_number(value) for name, value in zip(columns, parts)}
            if "pending_actions" not in row and "edges_counter" in row:
                row["pending_actions"] = row["edges_counter"]
            if "edges_counter" not in row and "pending_actions" in row:
                row["edges_counter"] = row["pending_actions"]
            if "processed_actions" not in row and "bip2" in row:
                row["processed_actions"] = row["bip2"]
            if "bip2" not in row and "processed_actions" in row:
                row["bip2"] = row["processed_actions"]
            if "edge_compositions" not in row and "bip3" in row:
                row["edge_compositions"] = row["bip3"]
            if "bip3" not in row and "edge_compositions" in row:
                row["bip3"] = row["edge_compositions"]
            if "one_optimizations" not in row and "ones" in row:
                row["one_optimizations"] = row["ones"]
            if "ones" not in row and "one_optimizations" in row:
                row["ones"] = row["one_optimizations"]
            if "failed_compositions" not in row and "nofires" in row:
                row["failed_compositions"] = row["nofires"]
            if "nofires" not in row and "failed_compositions" in row:
                row["nofires"] = row["failed_compositions"]
            if "graph_nodes" not in row and "temp" in row:
                row["graph_nodes"] = row["temp"]
            if "temp" not in row and "graph_nodes" in row:
                row["temp"] = row["graph_nodes"]
            row["path"] = path
            rows.append(row)

    if rows:
        rank = int(rows[0].get("rank", rank_from_name(path)))
    else:
        rank = rank_from_name(path)
    return {"path": path, "rank": rank, "rows": rows}


def parse_epoch(parts):
    if len(parts) >= 7:
        try:
            return int(parts[-1])
        except ValueError:
            pass

    try:
        return int(dt.datetime.strptime(parts[0] + " " + parts[1], "%Y-%m-%d %H:%M:%S").timestamp())
    except (IndexError, ValueError):
        return None


def read_memory(path):
    if not path or not os.path.exists(path):
        return []

    rows = []
    with open(path, "r", encoding="utf-8", errors="replace") as handle:
        for line in handle:
            line = line.strip()
            if not line or line.startswith("#"):
                continue
            parts = line.split()
            if len(parts) < 6:
                continue

            epoch = parse_epoch(parts)
            try:
                pid_count = int(parts[2])
                total_rss_mb = float(parts[3])
                max_rss_mb = float(parts[4])
                max_hwm_mb = float(parts[5])
            except ValueError:
                continue

            rows.append(
                {
                    "epoch": epoch,
                    "pid_count": pid_count,
                    "total_rss_mb": total_rss_mb,
                    "max_rss_mb": max_rss_mb,
                    "max_hwm_mb": max_hwm_mb,
                }
            )

    active = [i for i, row in enumerate(rows) if row["pid_count"] > 0 or row["total_rss_mb"] > 0]
    if active:
        first = max(0, active[0] - 1)
        last = min(len(rows), active[-1] + 2)
        return rows[first:last]
    return rows


def stats_x(row, base):
    if "wall_epoch" in row and not math.isnan(float(row["wall_epoch"])):
        return float(row["wall_epoch"]) - base
    return float(row["time"]) - base


def memory_x(row, base):
    if row.get("epoch") is not None:
        return float(row["epoch"]) - base
    return 0.0


def axis_base(stats_rows, memory_rows):
    stat_epochs = [
        float(row["wall_epoch"])
        for row in stats_rows
        if "wall_epoch" in row and not math.isnan(float(row["wall_epoch"]))
    ]
    mem_epochs = [float(row["epoch"]) for row in memory_rows if row.get("epoch") is not None]
    if stat_epochs and mem_epochs:
        return min(stat_epochs + mem_epochs), True
    if stat_epochs:
        return min(stat_epochs), True
    stat_times = [float(row["time"]) for row in stats_rows if "time" in row]
    if stat_times:
        return min(stat_times), False
    if mem_epochs:
        return min(mem_epochs), True
    return 0.0, False


def series_from_rows(rows, xfunc, column, divisor=1.0):
    points = []
    for row in rows:
        if column not in row:
            continue
        x = xfunc(row)
        y = float(row[column]) / divisor
        if math.isfinite(x) and math.isfinite(y):
            points.append((x, y))
    return points


def downsample(points, limit=1600):
    if len(points) <= limit:
        return points
    step = max(1, len(points) // limit)
    sampled = points[::step]
    if sampled[-1] != points[-1]:
        sampled.append(points[-1])
    return sampled


def nice_max(value):
    if value <= 0 or not math.isfinite(value):
        return 1.0
    magnitude = 10 ** math.floor(math.log10(value))
    for factor in (1, 2, 5, 10):
        candidate = factor * magnitude
        if candidate >= value:
            return candidate
    return value


def path_points(points, sx, sy):
    points = downsample(points)
    if not points:
        return ""
    commands = [f"M {sx(points[0][0]):.2f} {sy(points[0][1]):.2f}"]
    commands.extend(f"L {sx(x):.2f} {sy(y):.2f}" for x, y in points[1:])
    return " ".join(commands)


def text(x, y, content, size=12, color="#172026", anchor="start", weight="400"):
    return (
        f'<text x="{x:.1f}" y="{y:.1f}" font-size="{size}" fill="{color}" '
        f'text-anchor="{anchor}" font-family="Arial, sans-serif" font-weight="{weight}">'
        f"{html.escape(str(content))}</text>"
    )


def polyline(points, sx, sy, color, width=1.8, dash=None, opacity=1.0):
    d = path_points(points, sx, sy)
    if not d:
        return ""
    dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
    return (
        f'<path d="{d}" fill="none" stroke="{color}" stroke-width="{width}" '
        f'stroke-linejoin="round" stroke-linecap="round" opacity="{opacity}"{dash_attr}/>'
    )


def draw_panel(x, y, w, h, title, stats_rows, memory_rows):
    base, epoch_based = axis_base(stats_rows, memory_rows)

    stat_x = lambda row: stats_x(row, base)
    mem_x = lambda row: memory_x(row, base)
    graph_nodes = series_from_rows(stats_rows, stat_x, "graph_nodes")
    incoming = series_from_rows(stats_rows, stat_x, "incoming_pending")
    outgoing = series_from_rows(stats_rows, stat_x, "outgoing_pending")
    total_mem = series_from_rows(memory_rows, mem_x, "total_rss_mb", 1024.0)
    max_mem = series_from_rows(memory_rows, mem_x, "max_rss_mb", 1024.0)

    stat_points = graph_nodes + incoming + outgoing
    mem_points = total_mem + max_mem
    max_x = max([p[0] for p in stat_points + mem_points] or [1.0])
    min_x = min([p[0] for p in stat_points + mem_points] or [0.0])
    if max_x <= min_x:
        max_x = min_x + 1.0

    max_stat = nice_max(max([p[1] for p in stat_points] or [1.0]) * 1.05)
    raw_max_mem = max([p[1] for p in mem_points] or [0.0])
    max_mem_y = nice_max(raw_max_mem * 1.05) if raw_max_mem > 0 else 0.0

    def sx(value):
        return x + (value - min_x) / (max_x - min_x) * w

    def sy_stat(value):
        return y + h - value / max_stat * h

    def sy_mem(value):
        return y + h - value / max_mem_y * h if max_mem_y > 0 else y + h

    out = []
    out.append(f'<rect x="{x}" y="{y}" width="{w}" height="{h}" fill="#ffffff" stroke="#cfd7df"/>')
    out.append(text(x, y - 12, title, 13, "#172026", weight="700"))

    for i in range(5):
        gy = y + h * i / 4
        out.append(f'<line x1="{x}" y1="{gy:.2f}" x2="{x + w}" y2="{gy:.2f}" stroke="#edf1f4" stroke-width="1"/>')
        stat_label = max_stat * (4 - i) / 4
        out.append(text(x - 7, gy + 4, f"{stat_label:.0f}", 10, "#5f6b75", "end"))
        if max_mem_y > 0:
            mem_label = max_mem_y * (4 - i) / 4
            out.append(text(x + w + 7, gy + 4, f"{mem_label:.1f}", 10, "#5f6b75", "start"))

    for i in range(5):
        gx = x + w * i / 4
        out.append(f'<line x1="{gx:.2f}" y1="{y}" x2="{gx:.2f}" y2="{y + h}" stroke="#f3f6f8" stroke-width="1"/>')
        label = min_x + (max_x - min_x) * i / 4
        out.append(text(gx, y + h + 17, f"{label:.0f}s", 10, "#5f6b75", "middle"))

    out.append(polyline(graph_nodes, sx, sy_stat, "#1769aa", 1.9))
    out.append(polyline(incoming, sx, sy_stat, "#00866a", 1.5, "5 4", 0.9))
    out.append(polyline(outgoing, sx, sy_stat, "#ca6702", 1.7))
    if max_mem_y > 0:
        out.append(polyline(total_mem, sx, sy_mem, "#6d28d9", 2.2))
        out.append(polyline(max_mem, sx, sy_mem, "#9333ea", 1.4, "4 4", 0.85))
    else:
        out.append(text(x + w / 2, y + h / 2, "no positive memory samples", 12, "#8a96a3", "middle"))

    xlabel = "seconds from first epoch sample" if epoch_based else "relative internal time"
    out.append(text(x + w / 2, y + h + 35, xlabel, 10, "#5f6b75", "middle"))
    out.append(text(x - 40, y + h / 2, "queue / pending", 10, "#5f6b75", "middle"))
    if max_mem_y > 0:
        out.append(text(x + w + 42, y + h / 2, "rss GiB", 10, "#5f6b75", "middle"))
    return "\n".join(out)


def legend(x, y):
    items = [
        ("#1769aa", "graph_nodes", None),
        ("#00866a", "incoming pending", "5 4"),
        ("#ca6702", "outgoing pending", None),
        ("#6d28d9", "total RSS GiB", None),
        ("#9333ea", "max rank RSS GiB", "4 4"),
    ]
    out = []
    cursor = x
    for color, label, dash in items:
        dash_attr = f' stroke-dasharray="{dash}"' if dash else ""
        out.append(f'<line x1="{cursor}" y1="{y}" x2="{cursor + 25}" y2="{y}" stroke="{color}" stroke-width="3"{dash_attr}/>')
        out.append(text(cursor + 31, y + 4, label, 11, "#172026"))
        cursor += 145
    return "\n".join(out)


def render(single_stats, single_memory, parallel_stats, parallel_memory, output, title):
    single = read_stats(single_stats) if single_stats else None
    parallel = [read_stats(path) for path in parallel_stats]
    parallel = [item for item in parallel if item["rows"]]
    single_mem = read_memory(single_memory)
    parallel_mem = read_memory(parallel_memory)

    width = 1620
    height = 900
    out = [
        f'<svg xmlns="http://www.w3.org/2000/svg" width="{width}" height="{height}" viewBox="0 0 {width} {height}">',
        '<rect width="100%" height="100%" fill="#f7f9fb"/>',
        text(60, 45, title, 22, "#172026", weight="700"),
        legend(60, 72),
    ]

    if single and single["rows"] and parallel:
        out.append(draw_panel(70, 125, 600, 650, "np=1", single["rows"], single_mem))
        panel_w = 330
        panel_h = 280
        start_x = 760
        start_y = 125
        for index, item in enumerate(parallel[:4]):
            px = start_x + (index % 2) * (panel_w + 95)
            py = start_y + (index // 2) * (panel_h + 105)
            out.append(draw_panel(px, py, panel_w, panel_h, f"rank {item['rank']}", item["rows"], parallel_mem))
    elif parallel:
        panel_w = 610
        panel_h = 330
        for index, item in enumerate(parallel[:4]):
            px = 80 + (index % 2) * (panel_w + 90)
            py = 130 + (index // 2) * (panel_h + 95)
            out.append(draw_panel(px, py, panel_w, panel_h, f"rank {item['rank']}", item["rows"], parallel_mem))
    elif single and single["rows"]:
        out.append(draw_panel(120, 125, 1200, 650, "np=1", single["rows"], single_mem))
    else:
        out.append(text(width / 2, height / 2, "no stats logs found", 18, "#5f6b75", "middle"))

    out.append("</svg>")
    Path(output).parent.mkdir(parents=True, exist_ok=True)
    Path(output).write_text("\n".join(out), encoding="utf-8")


def main():
    parser = argparse.ArgumentParser(description="Plot PELCR queue/pending pressure with memory monitor data.")
    parser.add_argument("--single-stats", default=None, help="stats log for a single-rank run")
    parser.add_argument("--single-memory", default=None, help="monitor_combustion_memory log for the single-rank run")
    parser.add_argument("--parallel-stats", nargs="*", default=[], help="parallel stats logs or glob patterns")
    parser.add_argument("--parallel-memory", default=None, help="monitor_combustion_memory log for the parallel run")
    parser.add_argument("--output", default="GML/queue_memory_pressure.svg")
    parser.add_argument("--title", default="PELCR queue/pending pressure vs memory pressure")
    args = parser.parse_args()

    parallel_stats = expand_paths(args.parallel_stats)
    render(args.single_stats, args.single_memory, parallel_stats, args.parallel_memory, args.output, args.title)


if __name__ == "__main__":
    main()
