"""Read-back of the normal form from the final graph of a PELCR (C) run.

Pipeline:
  1. PELCR writes the final graph as JSON at the end of a single-rank run
     (option -readback <file>, read_back.c); the gdb extension pelcr_gdb_dump.py writes the same
     format for the binaries built before that option existed;
  2. this script turns that graph into a dump in the format of the ND-PELCR Python model
     (code/ND-VM-PELCR/python/pelcr.py of the ND-PELCR article): edges (et, es, vt, vs, weight);
  3. the observation (paths between boundary ports, Definition pelcr-observation) is computed
     by chains, and the token decoder of pelcr_readback.py reads back the normal form,
     choices included.

Conventions of the C implementation, translated here:
  - weights are elements of Lambda*_nd = Lambda* (x) Lambda* (Definition ndexgoi2), written
    "u1|u0" (component 1, then component 0) or "u1" for the pair (erase(u1), u1), where erase
    deletes p and q (choice-free weights);
  - words are strings of letters p, q, w(i,l) (fan generator w_i of lift l), c(h,l) (choice
    generator c_h, lift 1, component 0 only), 1, and !k, where !k lifts everything that follows
    it: the index of a letter is the sum of the !k before it;
  - p and q are exchanged with respect to the ND-PELCR translation (lambda: variable side p in C, q in ND);
  - an edge is stored at its target node, on side LEFT (1) or RIGHT (0); e.side is the side at the source.
Boundary ports: the root (PELCR's `principal`) and, in the dump written by PELCR, the var nodes of
the free variables ("free").  Limits: no external symbols X(...), single rank.  A JSON written by gdb must come from a run with free() disabled
(see pelcr_gdb_dump.py); the dump written by PELCR itself resolves the sources of the edges
to node addresses and needs no such precaution.
"""
import collections, json, os, re, sys

HERE = os.path.dirname(os.path.abspath(__file__))


def find_nd_python():
    env = os.environ.get('NDPELCR_PYTHON')
    cands = [env] if env else []
    cands.append(os.path.expanduser(
        '~/marco.live/TEX/2026/ND-PELCRVM2/ND-PELCRVM.github/code/ND-VM-PELCR/python'))
    cands.append(os.path.join(os.path.expanduser('~'), 'mnt', 'ND-PELCRVM2', 'ND-PELCRVM.github',
                              'code', 'ND-VM-PELCR', 'python'))
    for c in cands:
        if c and os.path.isfile(os.path.join(c, 'pelcr_readback.py')):
            return c
    raise SystemExit('pelcr_readback.py not found: set NDPELCR_PYTHON to code/ND-VM-PELCR/python')


sys.dont_write_bytecode = True
sys.path.insert(0, find_nd_python())
import ndalg as A               # noqa: E402
import pelcr_readback as R      # noqa: E402

SWAP = {'p': 'q', 'q': 'p'}
TOKEN = re.compile(r'!(\d+)|([wc])\((\d+),(\d+)\)|([pq1])')
KIND = {'w': 'f', 'c': 'c'}             # fan / choice generators of the ND-PELCR model


class WeightError(Exception):
    pass


def parse_word(s, component):
    """C word -> positive word of Lambda^* (tuple of (letter, index)), p/q exchanged.
    component 1 admits p, q, w; component 0 admits w, c."""
    word, depth, i = [], 0, 0
    while i < len(s):
        m = TOKEN.match(s, i)
        if m is None:
            raise WeightError('unsupported weight %r (at %r)' % (s, s[i:]))
        bang, gen, gi, gl, pq = m.groups()
        if bang is not None:
            depth += int(bang)
        elif gen is not None:
            if gen == 'c' and component == 1:
                raise WeightError('choice generator in component 1: %r' % s)
            x, lift_ = ('w', (KIND[gen], int(gi))), int(gl)
            if gen == 'c' and lift_ != 1:
                raise WeightError('choice generator c(%s) of lift %d' % (gi, lift_))
            if A.LIFT.get(x, lift_) != lift_:
                raise WeightError('generator %s(%s) with two lifts' % (gen, gi))
            A.LIFT[x] = lift_
            word.append((x, depth))
        elif pq in SWAP:
            if component == 0:
                raise WeightError('%s in component 0: %r' % (pq, s))
            word.append((SWAP[pq], depth))
        i = m.end()
    return A.canon(word)


def parse_weight(s):
    """C weight string -> component 1 as a classical positive word (kept for compatibility)"""
    return parse_word(s.split('|')[0], 1)


def nd_weight_of(s):
    """C weight string -> positive element of Lambda^*_nd"""
    if '|' in s:
        u1, u0 = s.split('|', 1)
        w1, w0 = parse_word(u1, 1), parse_word(u0, 0)
    else:
        w1 = parse_word(s, 1)
        w0 = tuple((x, m) for (x, m) in w1 if x not in ('p', 'q'))     # erase(u1)
    return ((w0, ()), (w1, ()))


def nd_weight(word):
    """classical positive word -> element of Lambda^*_nd (component 0: no p, q)."""
    w0 = tuple((x, m) for (x, m) in word if x not in ('p', 'q'))
    return ((w0, ()), (word, ()))


def side(v):
    return '+' if v == 1 else '-'          # LEFT = 1, RIGHT = 0


def load_dump(path):
    d = json.load(open(path))
    A.LIFT.clear()                     # lifts of the generators w_i of this run
    dump = []
    for e in d['edges']:
        w = nd_weight_of(e['weight'])
        dump.append((side(e['target_side']), side(e['source_side']), e['target'], e['source'], w))
    return d, dump


def observe(dump, boundary, maxdepth=100000):
    """Obs by chains (Definition pelcr-observation).  The C machine may keep several edges on the
    same (source, side) (one-optimization), so every chain is followed and the paths are enumerated."""
    by_source = collections.defaultdict(list)
    for e in dump:
        by_source[(e[3], e[1])].append(e)

    memo = {}

    def ends(u, eps, seen):
        """list of (end node, weight) of the maximal chains leaving (u, eps)"""
        key = (u, eps)
        if key in memo:
            return memo[key]
        es = by_source.get(key, [])
        if not es:
            return None                      # (u, eps) is not a source: u is an end
        if key in seen:
            raise RuntimeError('cycle in the dump at %r' % (key,))
        out = []
        for e in es:
            nxt = (e[2], '-' if e[0] == '+' else '+')
            rest = ends(e[2], nxt[1], seen | {key})
            if rest is None:
                out.append((e[2], e[4]))
            else:
                for (x, w) in rest:
                    w2 = A.mul(w, e[4])
                    if w2 is not A.ZERO:
                        out.append((x, w2))
        memo[key] = out
        return out

    obs = collections.Counter()
    for u in {e[3] for e in dump}:
        for eps in ('+', '-'):
            k1 = ends(u, eps, frozenset())
            k2 = ends(u, '-' if eps == '+' else '+', frozenset())
            if not k1 or not k2:
                continue
            for (x, w1) in k1:
                if x not in boundary:
                    continue
                for (y, w2) in k2:
                    if y not in boundary:
                        continue
                    w = A.mul(w1, A.star(w2))
                    if w is not A.ZERO:
                        obs[(x, y, w)] += 1
    return obs


class BoundaryGraph:
    """the part of tau(t) used by the decoder: the boundary nodes and their labels"""
    def __init__(self, root, free=()):
        self.root = root
        self.nodes = {root: {'type': 'root'}}
        for v, name in free:
            self.nodes[v] = {'type': 'var', 'name': name}


def readback_file(path, partial=False):
    """partial=True for an interrupted run: the decoder returns an approximant (_|_ where nothing
    has been observed yet)"""
    d, dump = load_dump(path)
    root = d['principal']
    free = [(v['id'], v['name']) for v in d.get('free', [])]
    obs = observe(dump, {root} | {v for v, _ in free})
    term, anomalies = R.readback(obs, BoundaryGraph(root, free), partial=partial)
    return d, obs, term, anomalies


if __name__ == '__main__':
    for p in sys.argv[1:]:
        d, obs, term, an = readback_file(p)
        print(p)
        print('   final graph: %d nodes, %d edges; %d observed paths'
              % (len(d['nodes']), len(d['edges']), sum(obs.values())))
        print('   normal form: %s' % R.term_string(term))
        if an:
            print('   anomalies: %s' % dict(an))
