"""Convert a GML snapshot written by PELCR's Print (raw or *.clean.gml) into yEd GraphML.

  python3 gml_to_yed.py input.gml [output.graphml]     (default output: input with .graphml)

Nodes and edges keep the colours of Print (edges: red = LEFT side of the target, blue = RIGHT,
green = messages still in the incoming buffer; nodes: yellow = no stored edge, olive = with edges);
edge labels are the weights.  If Graphviz is available, nodes get a top-down layout (`dot`; `sfdp` above 3000 nodes);
otherwise they are placed on a circle (in yEd: Layout > Hierarchical or Organic).  Only the node
positions are taken from Graphviz (splines=false keeps large graphs fast); yEd draws straight edges.
"""
import math, re, shutil, subprocess, sys
from xml.sax.saxutils import escape

NODE = re.compile(r'node\s*\[\s*id\s+(-?\d+)\s+label\s+"([^"]*)".*?fill\s+"(#[0-9A-Fa-f]{6})"', re.S)
EDGE = re.compile(r'edge\s*\[\s*(?:id\s+\d+\s+)?source\s+(-?\d+)\s+target\s+(-?\d+).*?fill\s+"(#[0-9A-Fa-f]{6})".*?'
                  r'label\s+"([^"]*)"\s*\]', re.S)
SIZE = 24.0


def parse(text):
    nodes, seen = [], set()
    for m in NODE.finditer(text):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            nodes.append((m.group(1), m.group(2), m.group(3)))
    edges = [m.groups() for m in EDGE.finditer(text)]
    return nodes, edges


def layout(nodes, edges):
    idx = {n[0]: i for i, n in enumerate(nodes)}
    if shutil.which('dot'):
        src = ['digraph G { rankdir=TB; nodesep=0.5; ranksep=0.9; splines=false; node [shape=circle width=0.35 fixedsize=true label=""];']
        src += ['n%d;' % i for i in range(len(nodes))]
        big = len(nodes) > 3000          # without edge labels the layout of large graphs is much faster
        src += ['n%d -> n%d%s;' % (idx[s], idx[t], '' if big else ' [label="%s"]' % w.replace('"', "'"))
                for s, t, _, w in edges]
        src.append('}')
        engine = 'dot' if len(nodes) <= 3000 else 'sfdp'      # dot is too slow on very large graphs
        out = subprocess.run([engine, '-Tplain'], input='\n'.join(src), capture_output=True, text=True, check=True).stdout
        pos, height = {}, 0.0
        for line in out.splitlines():
            f = line.split()
            if f[0] == 'graph':
                height = float(f[3])
            elif f[0] == 'node':
                pos[int(f[1][1:])] = (float(f[2]) * 72.0, (height - float(f[3])) * 72.0)
        return [pos[i] for i in range(len(nodes))]
    r = max(200.0, 12.0 * len(nodes))
    return [(r + r * math.cos(2 * math.pi * i / len(nodes)), r + r * math.sin(2 * math.pi * i / len(nodes)))
            for i in range(len(nodes))]


def graphml(nodes, edges, pos, title):
    idx = {n[0]: i for i, n in enumerate(nodes)}
    out = ['<?xml version="1.0" encoding="UTF-8" standalone="no"?>',
           '<graphml xmlns="http://graphml.graphdrawing.org/xmlns" xmlns:xsi="http://www.w3.org/2001/XMLSchema-instance" '
           'xmlns:y="http://www.yworks.com/xml/graphml" xmlns:yed="http://www.yworks.com/xml/yed/3" '
           'xsi:schemaLocation="http://graphml.graphdrawing.org/xmlns http://www.yworks.com/xml/schema/graphml/1.1/ygraphml.xsd">',
           '  <key attr.name="Description" attr.type="string" for="graph" id="d0"/>',
           '  <key attr.name="description" attr.type="string" for="node" id="d5"/>',
           '  <key for="node" id="d6" yfiles.type="nodegraphics"/>',
           '  <key attr.name="description" attr.type="string" for="edge" id="d9"/>',
           '  <key for="edge" id="d10" yfiles.type="edgegraphics"/>',
           '  <graph edgedefault="directed" id="G">',
           '    <data key="d0">%s</data>' % escape(title)]
    for i, (nid, label, fill) in enumerate(nodes):
        x, y = pos[i]
        out += ['    <node id="n%d">' % i,
                '      <data key="d5">%s</data>' % escape(label),
                '      <data key="d6">',
                '        <y:ShapeNode>',
                '          <y:Geometry height="%.1f" width="%.1f" x="%.1f" y="%.1f"/>' % (SIZE, SIZE, x - SIZE / 2, y - SIZE / 2),
                '          <y:Fill color="%s" transparent="false"/>' % fill.upper(),
                '          <y:BorderStyle color="#000000" type="line" width="1.0"/>',
                '          <y:NodeLabel fontFamily="Dialog" fontSize="10" modelName="internal" modelPosition="c" '
                'textColor="#000000" visible="true">%d</y:NodeLabel>' % i,
                '          <y:Shape type="ellipse"/>',
                '        </y:ShapeNode>',
                '      </data>',
                '    </node>']
    for k, (s, t, color, w) in enumerate(edges):
        out += ['    <edge id="e%d" source="n%d" target="n%d">' % (k, idx[s], idx[t]),
                '      <data key="d9">%s</data>' % escape(w),
                '      <data key="d10">',
                '        <y:PolyLineEdge>',
                '          <y:Path sx="0.0" sy="0.0" tx="0.0" ty="0.0"/>',
                '          <y:LineStyle color="%s" type="line" width="1.5"/>' % color.upper(),
                '          <y:Arrows source="none" target="standard"/>',
                '          <y:EdgeLabel fontFamily="Dialog" fontSize="9" modelName="centered" modelPosition="center" '
                'textColor="#000000" visible="true">%s</y:EdgeLabel>' % escape(w),
                '          <y:BendStyle smoothed="false"/>',
                '        </y:PolyLineEdge>',
                '      </data>',
                '    </edge>']
    out += ['  </graph>', '</graphml>', '']
    return '\n'.join(out)


if __name__ == '__main__':
    src = sys.argv[1]
    dst = sys.argv[2] if len(sys.argv) > 2 else re.sub(r'(\.clean)?\.gml$', '', src) + '.graphml'
    text = open(src).read()
    m = re.search(r'label\s+"([^"]*)"', text)
    title = ' '.join(m.group(1).split()) if m else src
    nodes, edges = parse(text)
    ids = {n[0] for n in nodes}
    missing = [e for e in edges if e[0] not in ids or e[1] not in ids]
    if missing:
        raise SystemExit('%d edges with an undeclared endpoint' % len(missing))
    open(dst, 'w').write(graphml(nodes, edges, layout(nodes, edges), title))
    print('%s: %d nodes, %d edges -> %s' % (src, len(nodes), len(edges), dst))
