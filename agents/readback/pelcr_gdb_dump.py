"""gdb extension: at the end of a PELCR run (breakpoint on read_back) write the final graph as JSON.

Usage (no change to the sources; pelcr_run.py does all this):
  PELCR_INPUT=commands.txt PELCR_DUMP_JSON=out.json PELCR_NOFREE=nofree.so \
    gdb -q -batch -x pelcr_gdb_dump.py --args build/combustion.linux -- -loop 10000000

Node identities.  An edge keeps its source as a pair (creator, key): creator = rank for nodes created
by the machine (key = address), creator = size for the keys of the graph builder, which BookTable[size]
maps to a node only when the key is first targeted.  A node is named "creator:key" (hex); an address
found in a book table is named by its key, any other address by "rank:address".
Freed nodes leave stale book-table entries, so a reused address would get a wrong name: run with
PELCR_NOFREE (LD_PRELOAD of nofree.so) so that addresses are never reused.
Only single-rank runs (np = 1).
"""
import gdb, json, os

OUT = os.environ.get('PELCR_DUMP_JSON', 'pelcr_graph.json')


def walk_nodes(head):
    p = head
    while int(p) != 0:
        yield p
        p = p['nextpuit']


def edges_of(seminode):
    e = seminode['vector']
    while int(e) != 0:
        yield e
        e = e['vector']


def booked_addresses(ntables):
    """address -> (creator, key) for the nodes booked in BookTable[c] (nodes created on demand for
    the keys of another process, or of the graph builder, which uses creator = size)"""
    rev = {}
    for c in range(ntables):
        t = gdb.parse_and_eval('BookTable[%d]' % c)
        if int(t) == 0:
            continue
        for h in range(int(t['size'])):
            entry = t['table'][h]
            while int(entry) != 0:
                rev[int(entry['address'])] = (c, int(entry['key']))
                entry = entry['next']
    return rev


def dump():
    G = gdb.parse_and_eval('G')
    rank = int(gdb.parse_and_eval('rank'))
    size = int(gdb.parse_and_eval('size'))
    if size != 1:
        raise gdb.GdbError('pelcr_gdb_dump: only single-rank runs are supported (size=%d)' % size)
    rev = booked_addresses(size + 1)

    def ident_addr(a):                   # an actual node address
        c, k = rev.get(a, (rank, a))
        return '%d:%x' % (c, k)

    def ident_key(c, k):                 # the (creator, key) reference kept in an edge
        return ident_addr(k) if c == rank else '%d:%x' % (c, k)

    nodes, edges = [], []
    for part in ('hot', 'cold'):
        for P in walk_nodes(G[part]):
            nodes.append({'id': ident_addr(int(P)), 'address': int(P), 'part': part, 'sto': int(P['sto']),
                          'eot': [int(P['left']['eot']), int(P['right']['eot'])]})
            for side_name, side_val in (('left', 1), ('right', 0)):
                for e in edges_of(P[side_name]):
                    edges.append({
                        'target': ident_addr(int(P)), 'target_side': side_val,
                        'source': ident_key(int(e['creator']), int(e['source'])),
                        'source_side': int(e['side']),
                        'sign': int(e['sign']), 'sto': int(e['sto']),
                        'rankpuit': int(e['rankpuit']), 'creator': int(e['creator']),
                        'weight': e['weight'].string(),
                    })
    principal = ident_addr(int(gdb.parse_and_eval('principal')))
    stats = {k: int(gdb.parse_and_eval(k)) for k in
             ('edge_compositions', 'fires', 'one_optimizations', 'graph_edges')}
    with open(OUT, 'w') as f:
        json.dump({'rank': rank, 'principal': principal, 'nodes': nodes, 'edges': edges,
                   'stats': stats, 'LEFT': 1, 'RIGHT': 0}, f, indent=1)
    gdb.write('pelcr_gdb_dump: %d nodes, %d edges -> %s\n' % (len(nodes), len(edges), OUT))


class DumpAtReadBack(gdb.Breakpoint):
    def stop(self):
        dump()
        return False          # do not stop: the run goes on to its normal end


NOFREE = os.environ.get('PELCR_NOFREE')
if NOFREE:
    gdb.execute('set environment LD_PRELOAD=%s' % NOFREE)
gdb.execute('set pagination off')
gdb.execute('set confirm off')
DumpAtReadBack('read_back')
INP = os.environ.get('PELCR_INPUT')
gdb.execute('run < %s' % INP if INP else 'run')
