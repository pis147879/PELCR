"""Run every example file of PELCR and check the read-back of its final graph where it makes sense.

  python3 check_examples.py [--exe build/combustion.linux] [--base EXE] [--timeout S] [--nf-maxsize N]
                            [--report FILE] [files ...]          (default: every pelcrexamples/**/*.plcr)

For every file:
  1. PELCR runs it (single rank, -readback: final graph in JSON); the run statistics are collected.
     A file whose run does not end within the time limit is reported as such; for a pure term it is run
     again with a limit on the compositions (-fires) and the read-back gives an approximant.
  2. The read-back is checked only for pure terms (lambda, application, [t + u], let, #def symbols
     expanded as PELCR does): constants, external functions, if/then/else, rec need the external algebra
     X(..), which the read-back does not decode.  A file with several pure terms is split, one run per term.
  With --base EXE (e.g. a build of master) every run is compared with the same run of EXE: the same final
  statistics (nodes, compositions, fires, family reductions), or, for a run that does not end, the same
  statistics after the same number of compositions (-fires): a regression check for the files whose
  read-back cannot be checked.
  3. Two checks, each when its size allows it:
     - Obs = EX: the paths of the final graph between boundary ports, with their weights in Lambda*_nd, are
       those of EX(tau(t)) computed by the Python model of the ND-PELCR article (up to the names of the
       generators).  Both are sets of paths of the shared normal form, so the check stays small when the
       lambda term of the normal form is huge (e.g. (delta)(delta)3 = 3^27, as a lambda term 7.6e12 symbols);
     - read-back = nf(t): the decoder unfolds the sharing and writes the lambda term of the normal form; it
       is compared with the normal form of the reference reducer of the article (terms.nf: leftmost-outermost
       beta and one-sided distribution), up to vacuous binders moved out of choices.  Done only when the
       lambda term of the normal form is small; when the reference reduction is too long but the normal form
       is known (EXPECTED below, with its reason), the read-back is compared with it.
"""
import collections, glob, json, multiprocessing, os, re, sys, tempfile, time
HERE = os.path.dirname(os.path.abspath(__file__))
ROOT = os.path.dirname(os.path.dirname(HERE))
sys.dont_write_bytecode = True
sys.path.insert(0, HERE)
import pelcr_run as PR                    # noqa: E402
import pelcr_c_readback as C              # noqa: E402
import test_c_readback as T               # noqa: E402
from terms import named_to_db, nf, size   # noqa: E402
R = C.R

NF_MAXSIZE = 4000        # lambda term of the normal form: largest size for which it is read back
NF_MAXSTEPS = 20000      # reference reducer: steps
CHECK_TIMEOUT = 300      # seconds for each of the two checks
# normal forms known when the reference reducer is too slow
EXPECTED = {
    '2dd3.plcr': (r'\x.x', '(two)((delta)(delta)three)I = n(n I) with n = 3^27, and (n)I ->* I for every numeral n'),
    '2dd4.plcr': (r'\x.x', '(two)((delta)(delta)four)I = n(n I) with n = 4^256, and (n)I ->* I for every numeral n'),
}

# ------------------------------------------------------------------ PELCR files
KEYWORDS = {'if', 'then', 'else', 'rec', 'in', 'and', 'And', 'or', 'Or', 'Add', 'add', 'mult', 'Mult',
            'xfunction', 'not', 'Not', 'succ', 'Succ', 'pred', 'Pred', 'iszero', 'true', 'false', 'TRUE',
            'FALSE', 'True', 'False'}
TOKEN = re.compile(r"\s*(\\|\.|\(|\)|\[|\]|\+|=|,|>|[A-Za-z_'\-][A-Za-z_'\-0-9]*|[0-9]+|\S)")


class NotPure(Exception):
    pass


def statements(text):
    text = re.sub(r'/\*.*?\*/', ' ', text, flags=re.S)
    text = re.sub(r'//[^\n]*', ' ', text)
    return [s.strip() for s in text.split(';') if s.strip()]


def parse_term(src, defs):
    """PELCR term -> named term; every identifier that is a #def symbol is replaced by its term,
    as PELCR does at parse time (IsThere), also under a binder of the same name"""
    toks = TOKEN.findall(src)
    pos = [0]

    def peek():
        return toks[pos[0]] if pos[0] < len(toks) else None

    def eat(t=None):
        x = peek()
        if x is None or (t is not None and x != t):
            raise NotPure('expected %s at %r' % (t, x))
        pos[0] += 1
        return x

    def ident():
        x = eat()
        if not re.match(r"[A-Za-z_'\-]", x) or x in KEYWORDS or x == 'let':
            raise NotPure('identifier expected, got %r' % x)
        return x

    def term():
        x = peek()
        if x == '\\':
            eat(); v = ident(); eat('.')
            return ('lam', v, term())
        if x == '(':
            eat(); f = term(); eat(')')
            return ('app', f, term())
        if x == '[':
            eat(); a = term(); eat('+'); b = term(); eat(']')
            return ('cho', a, b)
        if x == 'let':
            eat(); v = ident(); eat('='); a = term(); eat('in'); b = term()
            return ('app', ('lam', v, b), a)
        v = ident()
        return defs[v] if v in defs else ('var', v)
    t = term()
    if peek() is not None:
        raise NotPure('unexpected %r' % peek())
    return t


def read_plcr(path):
    """-> (main terms (named, or NotPure), uses external libraries)"""
    defs, mains, ext = {}, [], False
    for st in statements(open(path, errors='replace').read()):
        if st.startswith('#def') or st.startswith('#define'):
            m = re.match(r'#def(?:ine)?\s+(\S+?)\s*=\s*(.*)$', st, re.S)
            try:
                defs[m.group(1)] = parse_term(m.group(2), defs)
            except NotPure as e:
                defs[m.group(1)] = ('notpure', str(e))
            continue
        if st.startswith('#') or st in ('quit', 'trace', 'verbose', 'symbol'):
            ext = ext or bool(re.match(r'#(uselib|use|include|xcall)', st))
            continue
        try:
            t = parse_term(st, defs)
            if 'notpure' in repr(t):
                raise NotPure('uses a symbol that is not a pure term')
            mains.append(t)
        except NotPure as e:
            mains.append(e)
    return mains, ext


def fv(t, bound=frozenset()):
    k = t[0]
    if k == 'var':
        return set() if t[1] in bound else {t[1]}
    if k == 'lam':
        return fv(t[2], bound | {t[1]})
    return fv(t[1], bound) | fv(t[2], bound)


# ------------------------------------------------------------------ checks (in child processes, with a timeout)
def _obs_ex(js, t, q):
    try:
        d, dump = C.load_dump(js)
        free = [(v['id'], v['name']) for v in d.get('free', [])]
        obs = C.observe(dump, {d['principal']} | {v for v, _ in free})
        osk = T.obs_skeletons(d, obs)
        try:
            exs = T.ex_skeletons(t)
        except RuntimeError as e:
            q.put(('EX non calcolabile (troppi cammini nel grafo iniziale)', sum(obs.values()), None)); return
        q.put(('Obs = EX' if osk == exs else 'Obs != EX', sum(obs.values()), sum(exs.values())))
    except Exception as e:          # noqa: BLE001
        q.put(('error: %s' % e, None, None))


def _readback(js, expect_db, partial, q):
    try:
        d, obs, r, an = C.readback_file(js, partial=partial)
        s = R.term_string(r)
        if expect_db is None:
            q.put(('read', s, dict(an)))
        else:
            got = named_to_db(r)
            ok = got == expect_db or R.vacout(got) == R.vacout(expect_db)
            q.put(('read-back = nf' if ok else 'read-back != nf', s, dict(an)))
    except Exception as e:          # noqa: BLE001
        q.put(('error: %s' % e, None, None))


def in_child(fn, args, timeout):
    q = multiprocessing.Queue()
    p = multiprocessing.Process(target=fn, args=args + (q,))
    p.start()
    p.join(timeout)
    if p.is_alive():
        p.terminate(); p.join()
        return None
    return q.get() if not q.empty() else None


# ------------------------------------------------------------------ main
STAT = re.compile(r'\(0\) (final nodes|edge compositions|fires|family reductions|max weight length)\s*:\s*(\d+)')


def run_stats(log):
    """statistics of the last evaluated term"""
    st = {}
    for k, v in STAT.findall(open(log, errors='replace').read()):
        st[k] = int(v)
    return st


def stats_key(log):
    """(nodes, compositions, fires, family reductions) of every evaluated term"""
    text = open(log, errors='replace').read()
    blocks = re.findall(r'\(0\) final nodes\s*:\s*(\d+).*?edge compositions\s*:\s*(\d+).*?fires\s*:\s*(\d+)'
                        r'.*?family reductions\s*:\s*(\d+)', text, re.S)
    return [tuple(int(x) for x in b) for b in blocks]


def failure(msg, work, path):
    """why a run gave no graph: time limit, parse error, or the end of the log"""
    if 'did not end' in msg:
        return 'non termina entro il limite di tempo'
    if 'parse error' in msg:
        return 'errore di sintassi nel file'
    log = os.path.join(work, os.path.splitext(os.path.basename(path))[0] + '.log')
    tail = open(log, errors='replace').read()[-3000:] if os.path.exists(log) else ''
    for pat, why in (('Tabella delle costanti piena', 'tabella delle costanti piena (MAXNUMCOST)'),
                     ('Segmentation', 'segmentation fault'), ('cannot open', 'file non trovato')):
        if pat in tail:
            return 'si ferma: ' + why
    return 'si ferma senza grafo finale'


def compare_base(path, exe, base, work, timeout, done, fires=10000):
    """same statistics as the base executable (complete runs) or after the same compositions"""
    wb = os.path.join(work, 'base'); os.makedirs(wb, exist_ok=True)
    wn = os.path.join(work, 'new'); os.makedirs(wn, exist_ok=True)
    keys = []
    for e, w in ((base, wb), (exe, wn)):
        try:
            _, log, _ = PR.run_pelcr(path, e, w, timeout=timeout, fires=0 if done else fires, need_dump=False)
        except RuntimeError:
            log = os.path.join(w, os.path.splitext(os.path.basename(path))[0] + '.log')
        keys.append(stats_key(log) if os.path.exists(log) else [])
    what = 'finali' if done else 'dopo %d composizioni' % fires
    if not keys[0] and not keys[1]:
        return 'confronto con %s: nessuna statistica da nessuno dei due' % os.path.basename(base), None
    same = keys[0] == keys[1]
    return ('stesse statistiche %s di %s %s' % (what, os.path.basename(base), keys[1]) if same else
            'statistiche %s DIVERSE: %s %s, nuovo %s' % (what, os.path.basename(base), keys[0], keys[1])), same


def main(argv):
    global NF_MAXSIZE
    exe, base, timeout, report, files = 'build/combustion.linux', None, 180, None, []
    it = iter(argv)
    for a in it:
        if a == '--exe': exe = next(it)
        elif a == '--base': base = next(it)
        elif a == '--timeout': timeout = int(next(it))
        elif a == '--nf-maxsize': NF_MAXSIZE = int(next(it))
        elif a == '--report': report = next(it)
        else: files.append(a)
    if not files:
        files = sorted(glob.glob(os.path.join(ROOT, 'pelcrexamples', '**', '*.plcr'), recursive=True))
    work = tempfile.mkdtemp(prefix='pelcr-ex-')
    lines = []

    def out(s=''):
        print(s, flush=True)
        lines.append(s)
    out('PELCR: esecuzione degli esempi e controllo del read-back (%s)' % time.strftime('%Y-%m-%d %H:%M'))
    out('eseguibile %s, limite di tempo %d s per run' % (exe, timeout))
    summary = collections.Counter()
    queue = list(files)
    while queue:
        path = queue.pop(0)
        name = os.path.relpath(path, os.path.join(ROOT, 'pelcrexamples')) if path.startswith(ROOT) else os.path.basename(path)
        mains, ext = read_plcr(path)
        out('\n== %s' % name)
        if not mains:
            out('   nessun termine da valutare (solo #def)'); summary['nessun termine'] += 1
            continue
        if len(mains) > 1 and not any(isinstance(m, Exception) for m in mains):
            # several pure terms: one file per term, with the same #def and directives
            sts = statements(open(path, errors='replace').read())
            heads = [x for x in sts if x.startswith('#') and not x.startswith('#quit')]
            terms_src = [x for x in sts if not x.startswith('#') and x not in ('quit', 'trace', 'verbose', 'symbol')]
            parts = []
            for k, src in enumerate(terms_src):
                p = os.path.join(work, '%s.term%d.plcr' % (os.path.basename(path)[:-5], k + 1))
                with open(p, 'w') as f:
                    f.write(''.join(h + ' ;\n' for h in heads) + src + ' ;\n#quit ;\n')
                parts.append(p)
            out('   %d termini puri: un run per termine (%s)' % (len(parts), ', '.join(os.path.basename(p) for p in parts)))
            queue[0:0] = parts
            continue
        t0 = time.time()
        try:
            js, log, secs = PR.run_pelcr(path, exe, work, timeout=timeout)
            done = True
        except RuntimeError as e:
            done, js, secs = False, None, time.time() - t0
            msg = str(e)
        if done:
            st = run_stats(log)
            out('   run: %.1f s, %s nodi finali, %s composizioni, %s riduzioni di famiglia'
                % (secs, st.get('final nodes', '?'), st.get('edge compositions', '?'), st.get('family reductions', '?')))
            if len(stats_key(log)) > 1:
                out('   statistiche dei termini: %s' % stats_key(log))
        else:
            why = failure(msg, work, path)
            out('   run: %s (%d s)' % (why, timeout) if 'tempo' in why else '   run: %s' % why)
        has_choice = any(not isinstance(m, Exception) and 'cho' in repr(m) for m in mains)
        if base and has_choice:
            out('   confronto con %s non applicabile: termine con choice' % os.path.basename(base))
        elif base:
            text, same = compare_base(path, exe, base, work, timeout, done)
            out('   %s' % text)
            if same is not None:
                summary['come %s' % os.path.basename(base) if same else 'DIVERSO da %s' % os.path.basename(base)] += 1
        pure = len(mains) == 1 and not isinstance(mains[0], Exception)
        if len(mains) > 1:
            out('   %d termini nel file, con costanti o funzioni esterne: solo esecuzione' % len(mains))
            summary['solo esecuzione (algebra esterna)'] += 1
            continue
        if not pure:
            out('   termine con costanti o funzioni esterne (%s): solo esecuzione' % mains[0])
            summary['solo esecuzione (algebra esterna)'] += 1
            continue
        t = mains[0]
        if not done:
            fires = 3000
            js, log, secs = PR.run_pelcr(path, exe, work, timeout=timeout, fires=fires)
            res = in_child(_readback, (js, None, True), CHECK_TIMEOUT)
            s = res[1] if res else None
            out('   run interrotta dopo %d composizioni: approssimante %s'
                % (fires, (s[:160] + ('...' if len(s) > 160 else '')) if s else '(read-back non riuscito)'))
            summary['run non terminata: approssimante'] += 1
            continue
        # reference normal form
        n = nf(named_to_db(t), maxsteps=NF_MAXSTEPS, maxsize=NF_MAXSIZE)
        src = 'riduttore di riferimento'
        if n is None and name.split('/')[-1] in EXPECTED:
            e, why = EXPECTED[name.split('/')[-1]]
            n, src = named_to_db(R.parse_paper(e)), 'forma normale nota: ' + why
        # Obs = EX
        res = in_child(_obs_ex, (js, t), CHECK_TIMEOUT)
        if res is None:
            out('   Obs = EX: non controllato (oltre %d s)' % CHECK_TIMEOUT)
        else:
            out('   %s: %s cammini osservati%s' % (res[0], res[1], '' if res[2] is None else ', %s in EX' % res[2]))
            summary[res[0]] += 1
        # read-back
        if n is None:
            out('   read-back: non eseguito, la forma normale come lambda-termine supera %d simboli '
                '(o %d passi del riduttore)' % (NF_MAXSIZE, NF_MAXSTEPS))
            summary['read-back non eseguito (forma normale enorme)'] += 1
            continue
        res = in_child(_readback, (js, n, False), CHECK_TIMEOUT)
        if res is None:
            out('   read-back: oltre %d s' % CHECK_TIMEOUT); summary['read-back oltre il tempo'] += 1
            continue
        s = res[1] or ''
        out('   %s (%s, %d simboli): %s' % (res[0], src, size(n), s[:160] + ('...' if len(s) > 160 else '')))
        if res[2]:
            out('   anomalie: %s' % res[2])
        summary[res[0]] += 1
    out('\nRiepilogo: %s' % dict(summary))
    if report:
        with open(report, 'w') as f:
            f.write('\n'.join(lines) + '\n')


if __name__ == '__main__':
    main(sys.argv[1:])
