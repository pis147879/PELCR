"""Check the read-back of PELCR (C) runs against the normal form computed by beta-reduction.

  python3 test_c_readback.py [--exe build/combustion.linux] [--random N] [--seed S] [--keep DIR]

Closed terms without choice: fixed examples, then random applications of combinators and random
closed terms.  Each term goes through PELCR under gdb (pelcr_run.run_pelcr), its final graph is read
back (pelcr_c_readback) and compared with nf(t) (terms.nf of the ND-PELCR article code), up to alpha.
"""
import os, random, sys, tempfile, time, collections
HERE = os.path.dirname(os.path.abspath(__file__))
sys.dont_write_bytecode = True
sys.path.insert(0, HERE)
import pelcr_run as PR                    # noqa: E402
import pelcr_c_readback as C              # noqa: E402
R = C.R
from terms import named_to_db, nf, random_term   # noqa: E402

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
]
ATOMS = [r'\x.x', r'\x.\y.x', r'\x.\y.y', r'\f.\x.(f)(f)x', r'\f.\x.(f)x', r'\x.(x)x', r'\f.(f)\z.z',
         r'\x.\y.(y)x', r'\f.\g.\x.(f)(g)x', r'\n.\f.\x.(f)((n)f)x']


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


def gen_comb(rnd, d):
    if d == 0 or rnd.random() < 0.3:
        return R.parse_paper(rnd.choice(ATOMS))
    return ('app', gen_comb(rnd, d - 1), gen_comb(rnd, d - 1))


def gen_closed(rnd, d):
    return ('lam', 'a', random_term(rnd, d, bound=('a',), free=(), pchoice=0.0))


def candidates(nrand, seed):
    import itertools
    rnd = random.Random(seed)
    ctr = itertools.count()
    out = [(n, R.parse_paper(s)) for n, s in FIXED]
    seen = set()
    while len(out) < len(FIXED) + nrand:
        t = gen_comb(rnd, rnd.randint(1, 4)) if rnd.random() < 0.6 else gen_closed(rnd, rnd.randint(2, 6))
        t = rename(t, ctr)
        n = nf(named_to_db(t), maxsteps=300, maxsize=300)
        key = named_to_db(t)
        if n is None or key in seen:
            continue
        seen.add(key)
        out.append(('rand%d' % len(out), t))
    return out


def main(argv):
    exe, nrand, seed, keep = 'build/combustion.linux', 40, 7, None
    it = iter(argv)
    for a in it:
        if a == '--exe': exe = next(it)
        elif a == '--random': nrand = int(next(it))
        elif a == '--seed': seed = int(next(it))
        elif a == '--keep': keep = next(it)
    workdir = keep or tempfile.mkdtemp(prefix='pelcr-rbtest-')
    os.makedirs(workdir, exist_ok=True)
    stats = collections.Counter()
    bad = []
    t0 = time.time()
    for k, (name, t) in enumerate(candidates(nrand, seed)):
        s = R.term_string(t)
        plcr = os.path.join(workdir, 'case%03d.plcr' % k)
        PR.write_plcr(s, plcr)
        expect = nf(named_to_db(t), maxsteps=2000, maxsize=5000)
        try:
            js, _, _ = PR.run_pelcr(plcr, exe, workdir, timeout=120)
            d, obs, r, an = C.readback_file(js)
            ok = named_to_db(r) == expect
            stats['ok' if ok else 'WRONG'] += 1
            if not ok:
                bad.append((name, s, 'got ' + R.term_string(r)))
        except Exception as ex:          # noqa: BLE001
            stats['ERROR'] += 1
            bad.append((name, s, '%s: %s' % (type(ex).__name__, str(ex)[:200])))
        print('%-12s %-6s %s' % (name, 'ok' if not bad or bad[-1][0] != name else 'FAIL', s[:90]), flush=True)
    print('\n%d terms in %.0f s: %s' % (sum(stats.values()), time.time() - t0, dict(stats)))
    for b in bad:
        print('  FAIL', b)
    return 1 if bad else 0


if __name__ == '__main__':
    sys.exit(main(sys.argv[1:]))
