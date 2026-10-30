"""Draw a PELCR GML snapshot (raw or *.clean.gml) as an SVG suitable for a web-page background.

  python3 gml_to_background.py input.gml output.svg [--engine neato] [--theme light|dark|pelcr]
                               [--labels] [--width 1920] [--height 1080] [--start 7] [--keep-ratio]

The layout comes from Graphviz (`neato` up to 3000 nodes, `sfdp` above; `dot`, `fdp` also work) and the edges keep
Graphviz's splines; the drawing itself (strokes, opacity, colours, fit to the canvas) is done here.
By default the layout is stretched to fill the canvas (--keep-ratio keeps its proportions).
Background is transparent: the page colour shows through.
"""
import argparse, collections, re, subprocess
from xml.sax.saxutils import escape

NODE = re.compile(r'node\s*\[\s*id\s+(-?\d+)\s+label\s+"([^"]*)".*?fill\s+"(#[0-9A-Fa-f]{6})"', re.S)
EDGE = re.compile(r'edge\s*\[\s*(?:id\s+\d+\s+)?source\s+(-?\d+)\s+target\s+(-?\d+).*?fill\s+"(#[0-9A-Fa-f]{6})".*?'
                  r'label\s+"([^"]*)"\s*\]', re.S)

THEMES = {
    # ink, edge opacity, node opacity, label opacity
    'light': dict(ink='#1d2433', edge=0.30, node=0.45, label=0.40),
    'dark': dict(ink='#dfe7f3', edge=0.28, node=0.45, label=0.38),
    # the colours of PELCR's Print, muted: LEFT side red, RIGHT side blue, pending messages green
    'pelcr': dict(ink='#3b4252', edge=0.45, node=0.55, label=0.45,
                  colors={'#AA0000': '#c2504a', '#0000AA': '#4a6fc2', '#00FF00': '#4aa36b'}),
}


def parse(text):
    nodes, seen = [], set()
    for m in NODE.finditer(text):
        if m.group(1) not in seen:
            seen.add(m.group(1))
            nodes.append(m.group(1))
    return nodes, [m.groups() for m in EDGE.finditer(text)]


def layout(nodes, edges, engine, start, splines=True):
    idx = {n: i for i, n in enumerate(nodes)}
    src = ['digraph G { overlap=false; splines=%s; sep="+12"; start=%d; esep="+6";' % ('true' if splines else 'false', start),
           'node [shape=circle width=0.18 fixedsize=true label=""]; edge [arrowhead=none];']
    src += ['n%d;' % i for i in range(len(nodes))]
    src += ['n%d -> n%d;' % (idx[s], idx[t]) for s, t, _, _ in edges]
    src.append('}')
    out = subprocess.run([engine, '-Tplain'], input='\n'.join(src), capture_output=True, text=True, check=True).stdout
    pos, splines = {}, []
    for line in out.splitlines():
        f = line.split()
        if f[0] == 'node':
            pos[int(f[1][1:])] = (float(f[2]), float(f[3]))
        elif f[0] == 'edge':
            n = int(f[3])
            pts = [(float(f[4 + 2 * k]), float(f[5 + 2 * k])) for k in range(n)]
            splines.append((int(f[1][1:]), int(f[2][1:]), pts))
    pos = [pos[i] for i in range(len(nodes))]
    # Graphviz ends the splines on the node boundary: move the end points to the node centres
    order = collections.defaultdict(list)
    for tail, head, pts in splines:
        order[(tail, head)].append([pos[tail]] + pts[1:-1] + [pos[head]])
    return pos, [order[(idx[s], idx[t])].pop(0) for s, t, _, _ in edges]


def draw(nodes, edges, pos, splines, W, H, theme, labels, fill=True):
    th = THEMES[theme]
    xs = [p[0] for p in pos] + [x for s in splines for x, _ in s]
    ys = [p[1] for p in pos] + [y for s in splines for _, y in s]
    x0, x1, y0, y1 = min(xs), max(xs), min(ys), max(ys)
    margin = 0.06
    sx = W * (1 - 2 * margin) / (x1 - x0)
    sy = H * (1 - 2 * margin) / (y1 - y0)
    if not fill:                                   # keep the proportions of the layout
        sx = sy = min(sx, sy)
    ox = (W - sx * (x1 - x0)) / 2
    oy = (H - sy * (y1 - y0)) / 2

    def P(x, y):                                   # Graphviz y grows upwards
        return ox + sx * (x - x0), H - (oy + sy * (y - y0))

    unit = min(W, H) / 1080.0
    # thinner strokes and smaller dots on dense graphs: 1 up to 300 nodes, about 0.6 at 2000, 0.4 at 10000
    n = max(len(nodes), 1)
    unit *= max(0.35, min(1.0, (300.0 / n) ** 0.25))
    dot = 4.2 * unit * min(1.0, (3000.0 / n) ** 0.5)      # dots shrink further above 3000 nodes
    out = ['<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 %d %d" width="%d" height="%d">' % (W, H, W, H),
           '<title>PELCR: (delta)(delta)two after 245 of 482 compositions</title>',
           '<g fill="none" stroke-linecap="round" stroke-width="%.2f">' % (1.6 * unit)]
    for (s, t, color, w), pts in zip(edges, splines):
        q = [P(x, y) for x, y in pts]
        d = 'M%.1f,%.1f' % q[0]
        for k in range(1, len(q) - 2, 3):
            d += ' C%.1f,%.1f %.1f,%.1f %.1f,%.1f' % (q[k] + q[k + 1] + q[k + 2])
        col = th.get('colors', {}).get(color.upper(), th['ink'])
        out.append('<path d="%s" stroke="%s" stroke-opacity="%.2f"/>' % (d, col, th['edge']))
    out.append('</g>')
    out.append('<g fill="%s" fill-opacity="%.2f">' % (th['ink'], th['node']))
    for i in range(len(nodes)):
        out.append('<circle cx="%.1f" cy="%.1f" r="%.2f"/>' % (P(*pos[i]) + (dot,)))
    out.append('</g>')
    if labels:
        out.append('<g font-family="ui-monospace, Menlo, Consolas, monospace" font-size="%.1f" fill="%s" '
                   'fill-opacity="%.2f" text-anchor="middle">' % (13 * unit, th['ink'], th['label']))
        for (s, t, color, w), pts in zip(edges, splines):
            mx, my = P(*pts[len(pts) // 2])
            out.append('<text x="%.1f" y="%.1f">%s</text>' % (mx, my - 5 * unit, escape(w or '1')))
        out.append('</g>')
    out.append('</svg>')
    return '\n'.join(out)


if __name__ == '__main__':
    ap = argparse.ArgumentParser()
    ap.add_argument('src'); ap.add_argument('dst')
    ap.add_argument('--engine', default='auto', help='Graphviz engine; auto = neato up to 3000 nodes, sfdp above')
    ap.add_argument('--theme', default='light', choices=sorted(THEMES))
    ap.add_argument('--labels', action='store_true')
    ap.add_argument('--width', type=int, default=1920); ap.add_argument('--height', type=int, default=1080)
    ap.add_argument('--start', type=int, default=7)
    ap.add_argument('--splines', default='auto', choices=['auto', 'true', 'false'],
                    help='curved edges (Graphviz spline routing is slow on large graphs: auto = up to 150 nodes)')
    ap.add_argument('--drop-isolated', action='store_true', help='leave out nodes without edges')
    ap.add_argument('--keep-ratio', action='store_true', help='do not stretch the layout to the canvas')
    a = ap.parse_args()
    nodes, edges = parse(open(a.src).read())
    if a.drop_isolated:
        used = {s for s, t, _, _ in edges} | {t for s, t, _, _ in edges}
        nodes = [n for n in nodes if n in used]
    curved = a.splines == 'true' or (a.splines == 'auto' and len(nodes) <= 150)
    engine = a.engine if a.engine != 'auto' else ('neato' if len(nodes) <= 3000 else 'sfdp')
    pos, splines = layout(nodes, edges, engine, a.start, curved)
    open(a.dst, 'w').write(draw(nodes, edges, pos, splines, a.width, a.height, a.theme, a.labels, not a.keep_ratio))
    print('%s: %d nodes, %d edges -> %s' % (a.src, len(nodes), len(edges), a.dst))
