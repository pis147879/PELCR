"""Check PELCR (C) runs, choices included, against the ND-PELCR article and its reference code.

  python3 test_c_readback.py [--exe build/combustion.linux] [--random N] [--seed S] [--keep DIR] [--gdb]
                             [--no-ex] [--only det|nd]

For every term t (closed or open, with or without choices [t + u]):
  1. PELCR runs t and writes its final graph (-readback, read_back.c; --gdb: gdb extension);
  2. the read-back of that graph (pelcr_c_readback) is compared with nf(t) (terms.nf of the
     ND-PELCR code: beta and one-sided distribution), up to the equation
     [\\x.A + \\y.B] = \\x.[A + B] for vacuous binders (vacout), the only information that the
     observation does not carry (ND-PELCR read-back);
  3. unless --no-ex, the observation of the C dump is compared with EX(tau(t)) computed by the Python
     model of the article (pelcr_readback.ex_weights): same number of paths between the same boundary
     ports, with the same weights up to the names of the generators (each w_j / c_h replaced by its kind
     and lift).  At the end of a run Obs = EX (Proposition pelcr-observation-run).
Then the divergent term (delta)[I + delta] of Section 5 is run with -fires limits: the read-back of
the interrupted runs gives the approximants [[I + delta] + _|_], [[I + delta] + [[I + delta] + _|_]], ...
"""
import os, random, sys, tempfile, time, collections, itertools
HERE = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True
sys.path.insert(0, HERE)
import pelcr_run as PR                    # noqa: E402
import pelcr_c_readback as C              # noqa: E402
R = C.R
A = C.A
import pelcr as M                         # noqa: E402  (ND-PELCR model, found by pelcr_c_readback)
import sg                                 # noqa: E402
from terms import named_to_db, nf, random_term, parse_named   # noqa: E402

D = r'\d.(d)d'
FIXED = [
    ('I', r'\x.x'), ('K I', r'(\x.\y.x)\z.z'), ('K I I', r'((\x.\y.x)\z.z)\w.w'),
    ('dup I', r'(\x.(x)x)\y.y'), ('two I', r'(\f.\x.(f)(f)x)\y.y'), ('one I', r'(\f.\x.(f)x)\y.y'),
    ('f-dup', r'(\f.(f)f)\x.\y.(x)y'), ('two', r'\f.\x.(f)(f)x'),
    ('delta two', r'(\x.(x)x)\f.\x.(f)(f)x'),
    ('two two', r'(\f.\x.(f)(f)x)\g.\y.(g)(g)y'),
    ('exp22', r'(\n.\f.\x.((n)(n)f)x)\s.\o.(s)(s)o'),
    ('exp23', r'((\m.\n.(n)m)\s.\o.(s)(s)o)\s.\o.(s)(s)(s)o'),
    ('succ two', r'(\n.\f.\x.(f)((n)f)x)\s.\o.(s)(s)o'),
    ('B', r'((\f.\g.\x.(f)(g)x)\y.y)\z.z'), ('C', r'(\x.\y.(y)x)\z.z'),
    ('shared-lam', r'(\x.(\y.y)x)\w.w'),
    ('three three', r'(\f.\x.(f)(f)(f)x)\g.\y.(g)(g)(g)y'),
    ('open', r'(\x.x)y'), ('capture', r'(\x.\y.x)y'),
]
# with choices; syntax of the paper ((t)u takes the whole term that follows)
FIXED_ND = [
    ('choice I K', r'[\x.x + \y.\z.z]'),
    ('[I+K]I', r'([\i.i + \k.\l.k])\j.j'),
    ('dI+K', r'(\x.(x)x)[\u.u + \v.\w.v]'),
    ('dI+I', r'(\x.(x)x)[\i.i + \j.j]'),
    ('xx+(Iz)', r'(\x.[x + x])(\y.y)\z.z'),
    ('ch-arg', r'(\g.(g)\u.u)\x.[x + \v.v]'),
    ('nested', r'([[\i.i + \k.\l.k] + \m.m])\j.j'),
    ('f(f)z[I+K]', r'(\f.(f)(f)\z.z)[\i.i + \k.\l.k]'),
    ('church2-choice', r'(\n.(n)\p.[p + p])\s.\o.(s)(s)o'),
    ('dist-in-arg', r'(\g.(g)(g)\z.z)\x.([\u.u + \v.(v)v])x'),
    ('ch-in-arg', r'\x.(x)[\y.y + \z.z]'), ('lam-ch', r'\x.[\y.y + x]'),
    ('num-choice', r'(\n.\f.\x.((n)f)((n)f)x)[\s.\o.(s)o + \s.\o.(s)(s)o]'),
    ('church2-I-K', r'((\s.\o.(s)(s)o)[\i.i + \k.\l.k])\j.j'),
    ('choice-nf', r'\a.\b.\y.[(a)y + (b)y]'),
    ('open-ch', r'(\x.[(a)x + (b)x])y'),
    ('choice-dup-arg', r'(\x.((a)x)x)[(b)c + \y.y]'),
    # worked reductions of the paper (test_paper_examples.py of ND-PELCR)
    ('dup-choice', r'(\x.(x)x)[u + v]'),
    ('phases', r'([\i.i + y])z'),
    ('distribution', r'([u + v])w'),
    ('shared-redex', r'(\x.[x + x])(\y.y)z'),
    ('three-phases', r'(\x.([(x)%s + (y)x])y)\z.z' % D),
    ('shared-choice', r'(\x.(\y.(y)y)[x + x])t'),
    ('readback-trace', r'\y.(\z.((x)z)z)[y + (y)x]'),
    ('reduction5', r'\x.\y.(\z.(z)(z)y)\w.([x + y])w'),
]
# example files of pelcrexamples/nd (#def symbols are copied with fresh choice codes) and the term
# they denote, symbols expanded
TWO, THREE = r'\f.\x.(f)(f)x', r'\f.\x.(f)(f)(f)x'
PLCR_ND = [
    ('def-copies.plcr', r'([\x.x + \y.\z.z])[\x.x + \y.\z.z]'),
    ('three-phases.plcr', r'(\x.([(x)%s + (y)x])y)\z.z' % D),
    ('reduction5.plcr', r'\x.\y.(\z.(z)(z)y)\w.([x + y])w'),
    ('choice-numerals.plcr', r'(\n.\f.\x.((n)f)((n)f)x)[%s + %s]' % (TWO, THREE)),
]
ATOMS = [r'\x.x', r'\x.\y.x', r'\x.\y.y', r'\f.\x.(f)(f)x', r'\f.\x.(f)x', r'\x.(x)x', r'\f.(f)\z.z',
         r'\x.\y.(y)x', r'\f.\g.\x.(f)(g)x', r'\n.\f.\x.(f)((n)f)x']
ATOMS_ND = ATOMS + [r'\x.[x + x]', r'[\x.x + \x.\y.x]', r'\x.[(x)x + x]', r'[\x.x + \f.(f)\z.z]']


def rename(t, ctr):
    def go(t, env):
        k = t[0]
        if k == 'var':
            return ('var', env.get(t[1], t[1]))
        if k == 'lam':
            y = 'v%d' % next(ctr)
            e = dict(env); e[t[1]] = y
            return ('lam', y, go(t[2], e))
        return (k, go(t[1], env), go(t[2], env))
    return go(t, {})


def gen_comb(rnd, d, atoms, pchoice):
    if d == 0 or rnd.random() < 0.3:
        return R.parse_paper(rnd.choice(atoms))
    if rnd.random() < pchoice:
        return ('cho', gen_comb(rnd, d - 1, atoms, pchoice), gen_comb(rnd, d - 1, atoms, pchoice))
    return ('app', gen_comb(rnd, d - 1, atoms, pchoice), gen_comb(rnd, d - 1, atoms, pchoice))


def candidates(nrand, seed, only=None):
    rnd = random.Random(seed)
    ctr = itertools.count()
    out = []
    if only != 'nd':
        out += [(n, R.parse_paper(s)) for n, s in FIXED]
    if only != 'det':
        out += [(n, R.parse_paper(s)) for n, s in FIXED_ND]
    seen = set()
    target = len(out) + nrand
    while len(out) < target:
        nd = (only == 'nd') or (only != 'det' and rnd.random() < 0.6)
        r = rnd.random()
        if r < 0.5:
            t = gen_comb(rnd, rnd.randint(1, 4), ATOMS_ND if nd else ATOMS, 0.15 if nd else 0.0)
        elif r < 0.8:
            t = ('lam', 'a', random_term(rnd, rnd.randint(2, 6), bound=('a',), free=(), pchoice=0.2 if nd else 0.0))
        else:
            t = random_term(rnd, rnd.randint(2, 6), free=('a', 'b'), pchoice=0.2 if nd else 0.0)
        t = rename(t, ctr)
        n = nf(named_to_db(t), maxsteps=300, maxsize=300)
        key = named_to_db(t)
        if n is None or key in seen:
            continue
        seen.add(key)
        out.append(('rand%d' % len(out), t))
    return out


# ------------------------------------------------------------------ Obs (C) against EX(tau(t)) (Python model)
def skeleton(w):
    """weight of Lambda*_nd with every generator replaced by its kind and lift"""
    def word(ws):
        return tuple(((x if isinstance(x, str) else (x[1][0], A.lift(x))), m) for (x, m) in ws)
    (c0, d0), (c1, d1) = w
    return (word(c0), word(d0), word(c1), word(d1))


def ex_skeletons(t):
    G = sg.translate(t)
    codes = M.Codes(G)
    ex = R.ex_weights(G, codes, maxlen=4000, maxwork=400000)

    def name(v):
        n = G.nodes[v]
        return 'root' if n['type'] == 'root' else n['name']
    out = collections.Counter()
    for (x, y, w), k in ex.items():
        out[(name(x), name(y), skeleton(w))] += k
    return out


def obs_skeletons(d, obs):
    names = {d['principal']: 'root'}
    for v in d.get('free', []):
        names[v['id']] = v['name']
    out = collections.Counter()
    for (x, y, w), k in obs.items():
        out[(names[x], names[y], skeleton(w))] += k
    return out


def main(argv):
    exe, nrand, seed, keep, use_gdb, with_ex, only = 'build/combustion.linux', 40, 7, None, False, True, None
    it = iter(argv)
    for a in it:
        if a == '--exe': exe = next(it)
        elif a == '--random': nrand = int(next(it))
        elif a == '--seed': seed = int(next(it))
        elif a == '--keep': keep = next(it)
        elif a == '--gdb': use_gdb = True
        elif a == '--no-ex': with_ex = False
        elif a == '--only': only = next(it)
    workdir = keep or tempfile.mkdtemp(prefix='pelcr-rbtest-')
    os.makedirs(workdir, exist_ok=True)
    stats = collections.Counter()
    bad = []
    t0 = time.time()
    cases = [(name, t, None) for name, t in candidates(nrand, seed, only)]
    if only != 'det':
        exdir = os.path.join(os.path.dirname(os.path.dirname(HERE)), 'pelcrexamples', 'nd')
        cases += [(f, R.parse_paper(e), os.path.join(exdir, f)) for f, e in PLCR_ND]
    for k, (name, t, plcr) in enumerate(cases):
        s = R.term_string(t)
        if plcr is None:
            plcr = os.path.join(workdir, 'case%03d.plcr' % k)
            PR.write_plcr(s, plcr)
        expect = nf(named_to_db(t), maxsteps=2000, maxsize=5000)
        has_choice = 'cho' in repr(t)
        status = 'ok'
        try:
            js, _, _ = PR.run_pelcr(plcr, exe, workdir, timeout=120, use_gdb=use_gdb)
            d, obs, r, an = C.readback_file(js)
            # skeletons now: the lifts of the C generators live in the global table A.LIFT,
            # which the Python translation below overwrites with its own generators
            osk = obs_skeletons(d, obs)
            got = named_to_db(r)
            if got == expect:
                stats['read-back = nf'] += 1
            elif R.vacout(got) == R.vacout(expect):
                stats['read-back = nf up to vacuous binders'] += 1
            else:
                status = 'WRONG'
                bad.append((name, s, 'got ' + R.term_string(r)))
            if with_ex and status == 'ok':
                try:
                    exs = ex_skeletons(t)
                except RuntimeError:
                    stats['EX not computed (too large)'] += 1
                else:
                    if exs == osk:
                        stats['Obs = EX (up to generator names)'] += 1
                    else:
                        status = 'OBS!=EX'
                        bad.append((name, s, 'Obs %d paths, EX %d paths' % (sum(obs.values()), sum(exs.values()))))
        except Exception as ex:          # noqa: BLE001
            status = 'ERROR'
            bad.append((name, s, '%s: %s' % (type(ex).__name__, str(ex)[:200])))
        stats['terms'] += 1
        stats['with choice'] += has_choice
        stats[status] += 1
        print('%-15s %-7s %s' % (name, status, s[:90]), flush=True)

    # divergent term of Section 5: interrupted runs give approximants
    s = r'(%s)[\i.i + %s]' % (D, D)
    plcr = os.path.join(workdir, 'divergent.plcr')
    PR.write_plcr(s, plcr)
    prev = 0
    for fires in (300, 1000, 3000):
        try:
            js, _, _ = PR.run_pelcr(plcr, exe, workdir, timeout=120, fires=fires, use_gdb=use_gdb)
            d, obs, r, an = C.readback_file(js, partial=True)
            rs = R.term_string(r)
            n = sum(obs.values())
            ok = rs.startswith(r'[[\v0.v0 + \v1.(v1)v1] + ') and n >= prev
            prev = n
            print('%-15s %-7s -fires %d: %d paths, approximant %s' % ('delta[I+delta]', 'ok' if ok else 'FAIL',
                                                                     fires, n, rs[:100] + ('...' if len(rs) > 100 else '')))
            if not ok:
                bad.append(('divergent', s, rs))
        except Exception as ex:          # noqa: BLE001
            bad.append(('divergent', s, '%s: %s' % (type(ex).__name__, str(ex)[:200])))

    print('\n%d terms in %.0f s: %s' % (stats['terms'], time.time() - t0, dict(stats)))
    for b in bad:
        print('  FAIL', b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
