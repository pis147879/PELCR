#!/bin/bash
# GML snapshot of a PELCR run at a chosen moment, without changing the sources and without trace mode.
#
#   agents/gml/pelcr_snapshot.sh <file.plcr> <nodes|compositions|end> [N] [output-prefix]
#
#   nodes N         first NodeCombustion with graph_nodes >= N
#   compositions N  first NodeCombustion with edge_compositions >= N
#   end             at read_back (end of the run: the final graph)
#
# The snapshot is taken only when the pending buffer incoming[0] has not wrapped around
# (first <= last): write_buffer in print.c still steps with % MAXPENDING while the buffer is now elastic
# (% capacity), so with a wrapped buffer Print reads past the buffer and crashes (seen on dd4, 20000 nodes).
#
# gdb stops PELCR at that moment and calls PELCR's own Print(); the run is then killed.
# Output: <output-prefix><rank %5d><loops %5d><compositions>.gml (default prefix GML/<name>_).
# Run from the PELCR root on Linux (needs gdb and build/combustion.linux).
set -e
plcr=$1; mode=$2; n=$3
name=$(basename "$plcr" .plcr)
prefix=${4:-$PWD/GML/${name}_${mode}${n}_}
case $mode in
  nodes) where="break NodeCombustion if graph_nodes >= $n && incoming[0].first <= incoming[0].last" ;;
  compositions) where="break NodeCombustion if edge_compositions >= $n && incoming[0].first <= incoming[0].last" ;;
  end) where="break read_back" ;;
  *) echo "mode: nodes | compositions | end" >&2; exit 2 ;;
esac
tmp=$(mktemp -d)
printf '#write "%s" ;\n#setdir "%s" ;\n#open "%s"\n' "$prefix" "$(cd "$(dirname "$plcr")" && pwd)" "$(basename "$plcr")" > "$tmp/cmd"
cat > "$tmp/gdb" <<GDB
set pagination off
set confirm off
$where
run < $tmp/cmd
call Print(G, incoming, edge_compositions)
printf "snapshot: %d nodes, %ld compositions\n", graph_nodes, edge_compositions
kill
quit
GDB
OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1 \
  gdb -q -batch -x "$tmp/gdb" --args build/combustion.linux -- -loop 10000000 > "$tmp/log" 2>&1 || true
rm -f "$tmp/cmd" "$tmp/gdb"
if ! grep -E "^snapshot:" "$tmp/log"; then
  echo "no snapshot: the condition was never met, or Print failed (gdb log: $tmp/log)" >&2
  grep -E "signaled|SIGSEGV|exited" "$tmp/log" >&2 || true
  exit 1
fi
ls -1 "$prefix"*.gml
rm -rf "$tmp"
