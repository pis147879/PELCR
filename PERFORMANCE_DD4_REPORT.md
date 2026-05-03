# PELCR `dd4.plcr` Parallel Run Report

This note summarizes the runs executed on `pelcrexamples/dd4.plcr` while
validating multi-process execution.

## Correctness Check

The consistency check used for parallel runs is the total number of
`family reductions`. That total stayed constant across all tested process
counts:

- Sequential (`1` process): `72`
- Parallel (`2` processes): `72`
- Parallel (`4` processes): `72`
- Parallel (`8` processes): `72`
- Parallel (`16` processes): `72`
- Parallel (`32` processes): `72`

## Results Table

| Processes | Elapsed Time | Total Family Reductions | Speedup vs 1 proc | Efficiency |
| --- | ---: | ---: | ---: | ---: |
| 1 | 200 | 72 | 1.00x | 1.00 |
| 2 | 71 | 72 | 2.82x | 1.41 |
| 4 | 28 | 72 | 7.14x | 1.79 |
| 8 | 9 | 72 | 22.22x | 2.78 |
| 16 | 2 | 72 | 100.00x | 6.25 |
| 32 | 1 | 72 | 200.00x | 6.25 |

Speedup is computed as:

```text
speedup(p) = T1 / Tp
```

Efficiency is computed as:

```text
efficiency(p) = speedup(p) / p
```

## Elapsed Time Chart

```mermaid
xychart-beta
    title "PELCR dd4.plcr"
    x-axis [1, 2, 4, 8, 16, 32]
    y-axis "Elapsed Time" 0 --> 200
    line "Elapsed Time" [200, 71, 28, 9, 2, 1]
```

## Family Reductions Chart

```mermaid
xychart-beta
    title "Family Reductions"
    x-axis [1, 2, 4, 8, 16, 32]
    y-axis "Total Family Reductions" 0 --> 80
    line "Family Reductions" [72, 72, 72, 72, 72, 72]
```

## Speedup Chart

```mermaid
xychart-beta
    title "Speedup vs 1 Process"
    x-axis [1, 2, 4, 8, 16, 32]
    y-axis "Speedup" 0 --> 200
    line "Speedup" [1.00, 2.82, 7.14, 22.22, 100.00, 200.00]
```

## Memory Analysis: Queue, Fires, and Memory Pressure

The following plot compares the sequential `dd4` run with the `4`-process
run using the stats files and the output of `monitor_combustion_memory.sh`.
The stats logs are aligned with the memory log through the `wall_epoch`
column.

![dd4 pending, fires, and memory pressure](GML/dd4_pending_fires_memory.png)

The pressure signals have different meanings and should not be read on a
single shared scale:

- `pending` is an instantaneous backlog signal. In this run it does not grow
  monotonically with memory. The sequential run reaches an incoming-pending
  peak of about `12.7k`; the `4`-process run reaches about `6.2k`. Outgoing
  pending remains much smaller: about `199` in the sequential run and `463`
  in the `4`-process run.
- `fires` is cumulative, so the plot uses `fires/s`. The sequential run
  averages about `11k fires/s`, while the `4`-process run averages about
  `76k fires/s`. The parallel run compresses the same total amount of work
  into a much shorter wall-clock interval.
- Memory pressure grows almost monotonically during evaluation. The fresh
  sequential run peaked at about `644 MB` total RSS. The fresh `4`-process
  run peaked at about `1.55 GB` total RSS, with a maximum per process of
  about `389 MB`.

This suggests that, for `dd4`, memory pressure follows the growth of the
evaluated graph and cumulative work more closely than it follows the
instantaneous pending queues. There is no clear evidence in this run that a
pending backlog is the primary cause of memory growth.

## Interpretation Note

The `family reductions` totals are consistent and provide the main
correctness signal for these runs.

The reported `elapsed time` is useful as a comparative internal metric, but
the very high apparent speedups suggest it is not a strict wall-clock
measurement suitable for rigorous scalability claims without additional
validation.
