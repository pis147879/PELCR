# Read-back delle run di PELCR (C)

Ricostruisce la forma normale dal grafo finale di una run di PELCR, usando il decoder del read-back
scritto per l'articolo ND-PELCR (`ND-PELCRVM2/ND-PELCRVM.github/code/ND-VM-PELCR/python/pelcr_readback.py`).
I sorgenti di PELCR non vengono modificati.

## Come funziona

1. `pelcr_gdb_dump.py` è un'estensione di gdb. Mette un breakpoint su `read_back` (chiamata a fine run)
   e scrive in JSON il grafo finale: i nodi delle liste `G.hot` e `G.cold`, con gli archi di ciascun nodo.
   La stampa del grafo finale di PELCR (`Print`) è commentata in `combustion.c` e il suo GML non riporta
   il lato della sorgente degli archi: per questo il grafo si legge direttamente dalla memoria.
2. `pelcr_c_readback.py` traduce il grafo nel formato del modello Python dell'articolo (archi
   `(et, es, vt, vs, peso)` con pesi in Λ*_nd), calcola l'osservazione (cammini tra le porte di bordo
   seguendo le catene) e chiama `pelcr_readback.readback`.
3. `pelcr_run.py` mette insieme i due passi per un termine o un file `.plcr`.

Conversioni dalle convenzioni di PELCR (C) a quelle di ND-PELCR:

- pesi: stringhe di `p`, `q`, `w(i,l)` (generatore w_i di lift l), `1` e `!k`, dove `!k` solleva tutto
  ciò che segue: l'indice di una lettera è la somma dei `!k` che la precedono;
- `p` e `q` sono scambiati (λ: lato della variabile `p` in C, `q` in ND-PELCR);
- componente 0 di Λ*_nd: la stessa parola senza `p` e `q` (non ci sono nodi choice);
- LEFT = 1 diventa `+`, RIGHT = 0 diventa `-`;
- identità dei nodi: un arco tiene la sorgente come coppia (creator, chiave). Per i nodi creati dalla
  macchina la chiave è l'indirizzo; per quelli del costruttore del grafo (creator = size) è
  `BookTable[size]` a dare il nodo, creato la prima volta che la chiave è bersaglio.

`free()` è disattivata durante la run (`nofree.c`, caricato con `LD_PRELOAD`). PELCR libera i nodi caldi
ma lascia le loro voci nella book table: se un indirizzo viene riusato, il nome del nodo si confonde.
Senza questo accorgimento il read-back falliva su termini come `(\f.\x.(f)x)\y.y` e `(two)two`.

## Requisiti

- Linux con gdb e un eseguibile PELCR a un solo rank (es. `build/combustion.linux` da `Makefile.linux`);
- il codice Python di ND-PELCR: `NDPELCR_PYTHON=.../code/ND-VM-PELCR/python`, altrimenti viene cercato
  in `~/marco.live/TEX/2026/ND-PELCRVM2/...` e in `~/mnt/ND-PELCRVM2/...`.

Su macOS gdb non è disponibile in modo semplice; la parte Python (`pelcr_c_readback.py` su un JSON già
prodotto) gira ovunque.

## Uso

Dalla radice di PELCR, dopo la build:

```
python3 agents/readback/pelcr_run.py --keep OUTPUT/readback pelcrexamples/dd2.plcr
python3 agents/readback/pelcr_run.py '(\x.(x)x)\f.\x.(f)(f)x'
python3 agents/readback/pelcr_c_readback.py OUTPUT/readback/dd2.json
python3 agents/readback/test_c_readback.py --random 120 --seed 2026
```

Sintassi dei termini come in PELCR e nell'articolo: `\x.t`, `(t)u`, dove `(t)u` prende come argomento
tutto il termine che segue (`(f)(f)x = (f)((f)x)`). Niente parentesi superflue: `((f)x)` da solo non è un
termine per il parser di PELCR, che in quel caso entra in un ciclo di errori (il log è troncato a 5 MB).

## Verifiche (2026-10-29, branch `codex/no-ffi-min-memory`)

- `dd2.plcr`, `(δ)(δ)two`: run di PELCR 0,8 s, 482 composizioni, grafo finale con 88 nodi e 97 archi,
  20 cammini osservati; read-back in circa 27 s: λf.λx.f²⁵⁶x (256 occorrenze).
- `test_c_readback.py`: 17 termini fissi (fra cui `two two`, `three three`, `exp22`, `exp23`) più
  150 termini chiusi casuali (semi 7 e 2026): tutti uguali a nf(t) a meno di α.
- Su `λx.x`, `two` e `(δ)two` l'osservazione del grafo finale coincide con EX(τ(t)) del modello Python,
  a meno della rinomina dei generatori w.

## Limiti

- solo termini chiusi (l'unica porta di bordo è la radice, `principal`);
- niente simboli esterni `X(...)` (costanti, funzioni, `#xcall`);
- un solo rank MPI;
- il grafo si legge a fine run: per una run interrotta servirebbe il read-back parziale (`partial=True`).
