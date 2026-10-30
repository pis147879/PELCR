"""Compare PELCR (C) with the Wolfram prototype of the ND-PELCR article (VM-PELCR5.ma).

  python3 compare_wolfram.py [--exe build/combustion.linux] [--wolfram CMD] [--wl-out FILE] [term ...]

For every term (default: the fixed terms of test_c_readback.py, with and without choices):
  - the Wolfram prototype computes EX(tau(t)) (ExecutionFormula: weighted straight paths between
    boundary ports) and the read-back of the normal form from EX (ReadbackEX);
  - PELCR runs the term, writes its final graph (-readback) and the observation Obs of that graph is
    computed by pelcr_c_readback;
  - the two are compared: the same paths between the same boundary ports, with the same weights up to
    the names of the generators (every fan generator f_j and choice generator c_h is replaced by its kind,
    keeping the positions and the multiplicative letters p, q), and the same normal form up to alpha
    (and up to vacuous binders moved out of choices, the only information missing from Obs).

The Wolfram side runs with CMD (default: `wolframscript -file`; Mathics 3 also works,
`--wolfram "mathics --no-readline -f"`, but it is slow).  --wl-out FILE[,FILE...] reuses the output of
previous Wolfram runs instead of running it again (terms without a complete result are skipped).  VM-PELCR5.ma is looked up next to the Python code of ND-PELCR
(NDPELCR_PYTHON/..).
"""
import collections, os, subprocess, sys, tempfile
HERE = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True
sys.path.insert(0, HERE)
import pelcr_run as PR                    # noqa: E402
import pelcr_c_readback as C              # noqa: E402
import test_c_readback as T               # noqa: E402
from terms import named_to_db             # noqa: E402
R = C.R


def wl_string(s):
    return '"' + s.replace('\\', '\\\\').replace('"', '\\"') + '"'


def wolfram_script(terms, vm):
    src = 'Get[%s];\n$IterationLimit = 100000;\n' % wl_string(vm)
    src += 'terms = {%s};\n' % ', '.join(wl_string(s) for s in terms)
    src += r'''Do[
  t = ParseTerm[s];
  TranslateTerm[t];
  ex = ExecutionFormula[2000];
  WriteString["stdout", "TERM\t" <> s <> "\n"];
  Scan[WriteString["stdout", "PATH\t" <> #[[1]] <> "\t" <> #[[2]] <> "\t" <> #[[3]] <> "\n"] &, ex[[1]]];
  rb = ReadbackEX[t];
  WriteString["stdout", "RB\t" <> If[Head[rb] === RbError, "ERROR", TermString[rb]] <> "\n"],
  {s, terms}];
'''
    return src


def parse_wolfram(text):
    """TERM / PATH / RB lines -> {term: (Counter of (x, y, weight string), read-back string)}"""
    out, cur = {}, None
    for line in text.splitlines():
        f = line.split('\t')
        if f[0] == 'TERM':
            cur = f[1]
            out[cur] = [collections.Counter(), None]
        elif f[0] == 'PATH' and cur is not None:
            out[cur][0][(f[1], f[2], f[3])] += 1
        elif f[0] == 'RB' and cur is not None:
            out[cur][1] = f[1]
    return out


def kinds_of_string(w):
    """'<c1;c1.f1*|p.p;p.q.f1*>' -> generators replaced by their kind"""
    def word(s):
        if s == '1':
            return ()
        res = []
        for letter in s.split('.'):
            name, _, m = letter.partition('_')
            res.append((name if name in ('p', 'q') else name[0], int(m or 0)))
        return tuple(res)

    def one(s):
        c, _, d = s.partition(';')
        return word(c), word(d.rstrip('*')) if d else ()
    a, b = w.strip('<>').split('|')
    return one(a) + one(b)


def kinds_of_weight(w):
    def word(ws):
        return tuple(((x if isinstance(x, str) else x[1][0]), m) for (x, m) in ws)
    (c0, d0), (c1, d1) = w
    return (word(c0), word(d0), word(c1), word(d1))


def main(argv):
    exe, cmd, wl_out, terms = 'build/combustion.linux', 'wolframscript -file', None, []
    it = iter(argv)
    for a in it:
        if a == '--exe': exe = next(it)
        elif a == '--wolfram': cmd = next(it)
        elif a == '--wl-out': wl_out = next(it)
        else: terms.append(a)
    if not terms:
        terms = [s for _, s in T.FIXED_ND] + [e for _, e in T.PLCR_ND] + [s for _, s in T.FIXED[:8]]
    work = tempfile.mkdtemp(prefix='pelcr-wl-')
    if wl_out is None:
        vm = os.path.join(os.path.dirname(C.find_nd_python()), 'VM-PELCR5.ma')
        script = os.path.join(work, 'ex_terms.wl')
        with open(script, 'w') as f:
            f.write(wolfram_script(terms, vm))
        wl_out = os.path.join(work, 'ex_terms.out')
        with open(wl_out, 'w') as f:
            subprocess.run(cmd.split() + [script], stdout=f, stderr=subprocess.STDOUT, check=False)
    W = {}
    for name in wl_out.split(','):          # several outputs (e.g. parallel Wolfram runs) can be merged
        W.update({k: v for k, v in parse_wolfram(open(name, errors='replace').read()).items() if v[1] is not None})
    stats = collections.Counter()
    for k, s in enumerate(terms):
        if s not in W:
            print('%-60s no Wolfram result' % s[:60])
            stats['no Wolfram result'] += 1
            continue
        wpaths, wrb = W[s]
        plcr = os.path.join(work, 'case%03d.plcr' % k)
        PR.write_plcr(R.term_string(R.parse_paper(s)), plcr)
        js, _, _ = PR.run_pelcr(plcr, exe, work, timeout=300)
        d, obs, r, _ = C.readback_file(js)
        names = {d['principal']: 'r'}
        for v in d.get('free', []):
            names[v['id']] = 'v:' + v['name']
        csk = collections.Counter()
        for (x, y, w), n in obs.items():
            csk[(names[x], names[y], kinds_of_weight(w))] += n
        wsk = collections.Counter()
        for (x, y, w), n in wpaths.items():
            wsk[(x, y, kinds_of_string(w))] += n
        same_paths = csk == wsk
        same_rb = wrb not in (None, 'ERROR') and \
            R.vacout(named_to_db(R.parse_paper(wrb))) == R.vacout(named_to_db(r))
        stats['paths: PELCR Obs = Wolfram EX' if same_paths else 'paths DIFFER'] += 1
        stats['read-back: PELCR = Wolfram' if same_rb else 'read-back DIFFERS'] += 1
        print('%-6s %-6s %3d paths  %s' % ('ok' if same_paths else 'PATHS', 'ok' if same_rb else 'RB',
                                           sum(obs.values()), s[:80]), flush=True)
    print(dict(stats))
    return 0 if set(stats) <= {'paths: PELCR Obs = Wolfram EX', 'read-back: PELCR = Wolfram'} else 1


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
