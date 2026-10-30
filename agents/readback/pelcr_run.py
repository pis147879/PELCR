"""Run a lambda term (or a .plcr file) through PELCR and read back the normal form.

  python3 pelcr_run.py [--exe build/combustion.linux] [--keep DIR] [--fires N] [--gdb] '<term>' | file.plcr ...

Terms use the PELCR / paper syntax: \\x.t, (t)u, [t + u], where (t)u takes as argument the whole term
that follows.  PELCR writes its final graph with -readback (read_back.c); --gdb uses the gdb extension
instead (for binaries without that option).  --fires N stops the run after N edge compositions: the
read-back of an interrupted run is an approximant, with _|_ where nothing has been observed yet.
Single-rank runs.
"""
import json, os, subprocess, sys, tempfile, time

HERE = os.path.dirname(os.path.abspath(__file__))
LOGCAP = 5 * 1024 * 1024
sys.dont_write_bytecode = True
sys.path.insert(0, HERE)
import pelcr_c_readback as C     # noqa: E402


def nofree_lib():
    """LD_PRELOAD shim that disables free(): node addresses are then never reused, and the book table
    (key -> node address) cannot hold stale entries that point to a newer node."""
    so = os.path.join(HERE, 'nofree.so')
    if not os.path.exists(so):
        subprocess.run(['cc', '-shared', '-fPIC', '-O2', '-o', so, os.path.join(HERE, 'nofree.c')], check=True)
    return so


def run_pelcr(plcr_path, exe, workdir, timeout=600, fires=0, use_gdb=False):
    """returns (json path, log path, seconds)"""
    base = os.path.join(workdir, os.path.splitext(os.path.basename(plcr_path))[0])
    cmd = base + '.cmd'
    with open(cmd, 'w') as f:
        f.write('#setdir "%s" ; #open "%s"\n' % (os.path.dirname(os.path.abspath(plcr_path)),
                                                 os.path.basename(plcr_path)))
    env = dict(os.environ, OMPI_ALLOW_RUN_AS_ROOT='1', OMPI_ALLOW_RUN_AS_ROOT_CONFIRM='1')
    if os.path.exists(base + '.json'):
        os.remove(base + '.json')
    args = [os.path.abspath(exe), '--', '-loop', '10000000']
    if fires:
        args += ['-fires', str(fires)]
    if use_gdb:
        env.update(PELCR_INPUT=cmd, PELCR_DUMP_JSON=base + '.json', PELCR_NOFREE=nofree_lib())
        run = ['gdb', '-q', '-batch', '-x', os.path.join(HERE, 'pelcr_gdb_dump.py'), '--args'] + args
        stdin = ''
    else:
        run = args + ['-readback', base + '.json']
        stdin = ' < %s' % cmd
    t0 = time.time()
    # the log is capped: on a syntax error the PELCR parser can loop printing the prompt
    shell = '%s%s 2>&1 | head -c %d > %s' % (' '.join("'%s'" % a for a in run), stdin, LOGCAP, base + '.log')
    root = os.path.dirname(os.path.dirname(os.path.abspath(exe)))
    os.makedirs(os.path.join(root, 'LOGS'), exist_ok=True)
    try:
        subprocess.run(['bash', '-c', shell], env=env, timeout=timeout, cwd=root)
    except subprocess.TimeoutExpired:
        subprocess.run(['pkill', '-f', os.path.abspath(exe)])
        raise RuntimeError('PELCR did not end within %d s, see %s' % (timeout, base + '.log'))
    with open(base + '.log', errors='replace') as f:
        head = f.read(LOGCAP)
    if 'ERROR:' in head:
        raise RuntimeError('PELCR parse error, see %s' % (base + '.log'))
    if 'weight too long' in head:
        raise RuntimeError('weight longer than MAXLENWEIGHT, see %s' % (base + '.log'))
    if not os.path.exists(base + '.json'):
        raise RuntimeError('no graph dump, see %s' % (base + '.log'))
    return base + '.json', base + '.log', time.time() - t0


def write_plcr(term, path):
    with open(path, 'w') as f:
        f.write('%s ;\n#quit ;\n' % term)


def main(argv):
    exe = 'build/combustion.linux'
    keep = None
    fires = 0
    use_gdb = False
    items = []
    it = iter(argv)
    for a in it:
        if a == '--exe':
            exe = next(it)
        elif a == '--keep':
            keep = next(it)
        elif a == '--fires':
            fires = int(next(it))
        elif a == '--gdb':
            use_gdb = True
        else:
            items.append(a)
    workdir = keep or tempfile.mkdtemp(prefix='pelcr-rb-')
    os.makedirs(workdir, exist_ok=True)
    for k, item in enumerate(items):
        if item.endswith('.plcr') and os.path.exists(item):
            plcr = item
        else:
            plcr = os.path.join(workdir, 'term%d.plcr' % k)
            write_plcr(item, plcr)
        print(item)
        try:
            js, log, secs = run_pelcr(plcr, exe, workdir, fires=fires, use_gdb=use_gdb)
            t1 = time.time()
            d, obs, term, an = C.readback_file(js, partial=bool(fires))
        except Exception as ex:          # noqa: BLE001
            print('   FAILED: %s: %s' % (type(ex).__name__, ex))
            continue
        print('   PELCR: %.1f s, %d compositions; final graph %d nodes, %d edges; %d observed paths'
              % (secs, d['stats']['edge_compositions'], len(d['nodes']), len(d['edges']), sum(obs.values())))
        what = 'approximant' if fires else 'normal form'
        print('   %s (read back in %.1f s): %s' % (what, time.time() - t1, C.R.term_string(term)))
        if an:
            print('   anomalies: %s' % dict(an))


if __name__ == '__main__':
    main(sys.argv[1:])
