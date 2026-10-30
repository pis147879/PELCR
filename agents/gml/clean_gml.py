"""Make a GML snapshot written by PELCR's Print readable by GML tools (yEd, Gephi, Cytoscape, networkx).

  python3 clean_gml.py input.gml [output.gml]      (default output: input with .clean.gml)

Changes, the graph itself is untouched:
  - node ids are node addresses (> 2^31): they become 0, 1, 2, ...; the address stays as the node label;
  - the label of the graph spans several lines: it becomes a one-line string;
  - every edge gets an "id".
"""
import re, sys


def clean(text):
    header = re.match(r'\s*graph\s*\[\s*id\s+\S+\s+directed\s+(\d)\s+label\s+"([^"]*)"', text)
    if not header:
        raise SystemExit('not a PELCR GML snapshot (graph header not found)')
    body = text[header.end():]
    label = ' '.join(header.group(2).split())
    ids = {}

    def nid(x):
        if x not in ids:
            ids[x] = len(ids)
        return ids[x]

    # first pass: node declarations, in order
    for m in re.finditer(r'node\s*\[\s*id\s+(-?\d+)', body):
        nid(m.group(1))
    body = re.sub(r'(node\s*\[\s*id\s+)(-?\d+)', lambda m: m.group(1) + str(nid(m.group(2))), body)
    ecount = [0]

    def edge(m):
        s, t = nid(m.group(1)), nid(m.group(2))
        ecount[0] += 1
        return 'edge [ id %d source %d target %d' % (ecount[0] - 1, s, t)
    body = re.sub(r'edge\s*\[\s*source\s+(-?\d+)\s+target\s+(-?\d+)', edge, body)
    out = 'graph [\n  directed %s\n  label "%s"\n%s' % (header.group(1), label, body)
    return out, len(ids), ecount[0]


if __name__ == '__main__':
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else re.sub(r'\.gml$', '', src) + '.clean.gml'
    out, n, e = clean(open(src).read())
    open(dst, 'w').write(out)
    print('%s: %d nodes, %d edges -> %s' % (src, n, e, dst))
