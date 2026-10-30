# Memoria di progetto: PELCR

_Ultimo aggiornamento: 2026-10-29_

## Repository

- Branch principale: `master` (`origin/HEAD -> origin/master`).
- Branch di lavoro al 2026-10-29: `codex/no-ffi-min-memory`, allineato con origin (HEAD `3546c3a`).
- Worktree segnati *prunable* in `/private/tmp`: `cianf-benchmarking`, `cleaned-up-version`, una baseline in detached HEAD.
- I report `PERFORMANCE_DD4_REPORT.md`, `PERFORMANCE_DD4_IULIA_REPORT.md`, `PERFORMANCE_DD5_IULIA_REPORT.md`
  esistono solo sui branch `codex/free-booktable-memory`, `codex/restore-hot-cold-graph`, `comment-fra-hot-stats`.

## Build ed esecuzione

- `Makefile` include `Makefile.head` (symlink a `Makefile.mac`); varianti: `Makefile.linux`, `Makefile.iulia`, `Makefile.docker`.
- Compilazione con `mpicc` (MPI); parser con `flex` (`les.fl`) e `bison` (`parser.y`).
- Eseguibile: `build/combustion.mac` (variabile `EXECS`).
- Esecuzione tipo (dal Makefile):
  `printf '#setdir "pelcrexamples" ; #open "dd3.plcr"\n' | mpirun -np N build/combustion.mac -- -loop 10000000`
- Opzioni in `main.c`: `-v`, `-t`, `-I <file>`, `-loop N`, `-fires N`, `-prnstep N`, `-o <outfile>`, `--no-dump-output` / `-qdump`.
- `make GMP=1` per valori utente a precisione arbitraria; `MAXPENDING=…` opzionale.

## Linguaggio .plcr

- Esempi in `pelcrexamples/`.
- Direttive riconosciute dal lexer: `#def`, `#open`/`#load`, `#setdir`, `#gml`, `#write`, `#trace`, `#verbose`, `#status`, `#symbol`, `#setloops`, `#setfires`, `#quit`, ...
- `dd2.plcr` è `(delta)(delta)two` con `delta = \x.(x)x` e `two` numerale di Church 2; la forma normale è il numerale di Church 256.

## Output dei grafi

- Formato GML, scritto da `print.c` in file `<outfile><rank>.<loops>.<sq>.gml` (grafo hot e cold, archi con etichette di peso come `++-w(0,0)q`).
- `pelcrexamples/dd4.gml` è un esempio statico prodotto da `lambda2gml`.
- Ignorati da git: `GML/`, `LOGS/`, `REPORTS/`, `OUTPUT/`, `build/`, `*.gml`, `*.log`, `*.dat`.

## Vincoli e decisioni

- 2026-10-29: per ora non modificare i sorgenti.
- 2026-10-29: creata la cartella `agents/`; prima non esistevano file per agenti.

## Build verificata (2026-10-29)

- La VM Linux collegata alla cartella non ha `mpicc`, `flex`, `bison` e non può installarli (apt bloccato, niente sudo).
- Build riuscita nel workspace cloud (Ubuntu 24.04, x86_64) su una copia dei sorgenti, senza modificarli:
  `apt-get install flex libopenmpi-dev openmpi-bin`, `Makefile.head -> Makefile.linux`,
  `make gcombustion shared MPIR_HOME=/usr LEX=flex` -> `build/combustion.linux` (11 warning innocui).
- Run: `printf '#setdir "pelcrexamples" ; #open "dd2.plcr"\n' | mpirun -np 1 build/combustion.linux -- -loop 10000000`
  (da root servono `OMPI_ALLOW_RUN_AS_ROOT=1 OMPI_ALLOW_RUN_AS_ROOT_CONFIRM=1`).
- dd2 su 1 rank: ~0,4 s, 88 nodi finali (tutti cold), 482 edge compositions, 241 fires, 14 family reductions.
  Senza `#gml`/`#write`/`-o` non scrive GML.

## Read-back

- In PELCR (C) il read-back non è implementato: `read_back.c` stampa solo l'indirizzo della porta principale.
- Implementazione esistente (articolo ND-PELCR): `~/marco.live/TEX/2026/ND-PELCRVM2/ND-PELCRVM.github/code/ND-VM-PELCR/python/pelcr_readback.py`
  (VM Python con choice; decoder a token su Obs = EX; usa `agents/review-3.3` via `_paths.py`).
  Uso: `python3 -B pelcr_readback.py --timeout=150 --steps=10000000 '<termine>'`.
- Sintassi dei termini come in PELCR: `(f)(f)x = (f)((f)x)`, quindi `(delta)(delta)two = δ(δ 2)`.
- Verifica 2026-10-29 con la VM Python: `(δ)two` -> λf.λx.f⁴x (482 passi);
  `(δ)(δ)two` -> λf.λx.f²⁵⁶x (92490 passi, 20 cammini osservati, ~81 s).
  Questo verifica il termine; per l'output della run C vedi sotto.

## Adattatore read-back per PELCR (C) (2026-10-29)

- In `agents/readback/` (vedi README lì): gdb legge il grafo finale a `read_back` (`pelcr_gdb_dump.py`),
  `pelcr_c_readback.py` lo traduce nel dump del modello Python di ND-PELCR e chiama il decoder;
  `pelcr_run.py` fa tutto per un termine o un `.plcr`; `test_c_readback.py` confronta con nf(t).
- La stampa finale `Print(G, incoming, 1000000)` è commentata in `combustion.c`; il GML di `Print`
  non riporta il lato della sorgente degli archi (`e->side`), quindi non basta per il read-back.
- Convenzioni C -> ND: `p` e `q` scambiati; `!k` solleva tutto ciò che segue; componente 0 = parola senza p, q;
  la sorgente di un arco è (creator, chiave): creator = size per le chiavi del costruttore, risolte da `BookTable[size]`.
- `SinkRemove` libera i nodi ma lascia le voci nella book table (`FREE_HOT_BOOKTABLE_ENTRIES` = 0):
  con indirizzi riusati i nomi si confondono, perciò la run va fatta con `free()` disattivata (`nofree.c`, LD_PRELOAD).
- Verifica: dd2 -> λf.λx.f²⁵⁶x (88 nodi, 97 archi, 20 cammini; read-back ~20-27 s);
  17 termini fissi + 150 casuali chiusi tutti uguali a nf(t).
- Limiti: termini chiusi, niente `X(...)`, un solo rank, gdb (Linux). JSON di dd2 in `OUTPUT/readback/dd2.json`.

## Grafi intermedi in GML (2026-10-29)

- PELCR scrive snapshot GML solo in modalità trace: `#write "<prefisso>" ; #trace ;` prima di `#open`.
  `Print` viene chiamata a ogni one-optimization; file `<prefisso><rank %5d><loops %5d><composizioni %3d>.gml`
  (nomi con spazi). Per dd2: 92 snapshot, composizioni 2…481 su 482; la run in trace dà le stesse statistiche.
- Colori di `Print`: archi rossi `#aa0000` = lato LEFT del nodo, blu `#0000aa` = RIGHT, verdi = messaggi
  ancora nel buffer `incoming`; nodi gialli `#FFFF0F` = senza archi, `#AAAA00` = con archi.
- Il GML grezzo di `Print` non si apre nei programmi GML: id dei nodi = indirizzi (> 2^31) e etichetta del grafo su più righe.
  `agents/gml/clean_gml.py` rinumera i nodi 0..n-1 (indirizzo come label), mette l'etichetta su una riga e dà un id agli archi:
  il risultato `*.clean.gml` si apre con networkx e igraph.
- Marco usa yEd: `agents/gml/gml_to_yed.py` converte il GML di PELCR in GraphML yEd (colori di `Print`,
  pesi come etichette degli archi, layout dall'alto in basso calcolato con Graphviz `dot`).
- dd2 a metà esecuzione: `GML/dd2_mid_245of482.gml` (grezzo) `GML/dd2_mid_245of482.clean.gml` e `GML/dd2_mid_245of482.graphml` (61 nodi, 69 archi, di cui 26 messaggi in attesa).
- dd3 dopo pochi passi: `#setfires 800` con trace dà 110 snapshot; `GML/dd3_early_380.*` (380 composizioni, 112 nodi, 130 archi).
  La run limitata si ferma a 813 composizioni (298 nodi).

## Sfondo per il sito (2026-10-29)

- `agents/gml/gml_to_background.py`: GML di PELCR -> SVG (layout Graphviz neato, archi come spline, nodi al centro degli archi,
  layout stirato a 16:9, sfondo trasparente; `--theme light|dark|pelcr`, `--labels`, `--keep-ratio`).
  `agents/gml/svg2png.py` rende l'SVG in PNG con Chromium (playwright: solo nel cloud).
- Prodotti in `OUTPUT/website/`: `dd2-sfondo-{chiaro,scuro,pelcr}.svg` e `-2560x1440.png`, `anteprima.png`.
- dd3 a ~500 nodi: con `#setfires 1600` (trace) la run fa 1854 composizioni, 240 snapshot; i nodi saltano da 460
  (composizione 1381) a 518 (1478). `GML/dd3_1478_518nodi.{gml,clean.gml,graphml}` (518 nodi, 591 archi).
- Su ~500 nodi il routing delle spline di Graphviz non finisce in 100 s: `gml_to_background.py --splines auto`
  le usa fino a 150 nodi, poi archi dritti; `gml_to_yed.py` prende solo le posizioni (`splines=false`).
  Le etichette dei pesi su 500 nodi si sovrappongono: sfondi dd3 senza `--labels`, con `--drop-isolated`.
- Sfondi dd3: `OUTPUT/website/dd3-518nodi-sfondo-{chiaro,scuro,pelcr}.svg` (~80 KB) e `-2560x1440.png`, `anteprima-dd3-518nodi.png`.
- dd3 a ~1000 nodi: `#setfires 4000` (trace) -> 4216 composizioni, 342 snapshot (54 MB), 1931 nodi a fine run;
  scelto 2423 composizioni: 1001 nodi (996 con archi), 1163 archi. `GML/dd3_2423_1001nodi.{gml,clean.gml,graphml}`,
  sfondi `OUTPUT/website/dd3-1001nodi-sfondo-{chiaro,scuro,pelcr}.svg` (~160 KB) e `-2560x1440.png`.
- dd3 a ~2000 nodi: `#setfires 6000` (trace) -> 361 snapshot (64 MB), l'ultimo a 5753 composizioni con 2801 nodi;
  scelto 4418 composizioni: 2041 nodi (2013 con archi), 2176 archi. `GML/dd3_4418_2041nodi.{gml,clean.gml,graphml}`,
  sfondi `OUTPUT/website/dd3-2041nodi-sfondo-*` (~300 KB SVG). `gml_to_background.py` ora assottiglia tratti e punti
  sui grafi densi (fattore (300/n)^0.25, minimo 0.6).

## Snapshot a un momento preciso (2026-10-29)

- Metodo migliore della modalità trace: gdb ferma PELCR al primo `NodeCombustion` con `graph_nodes >= N`
  (o `edge_compositions >= N`, o a `read_back`) e chiama la `Print` di PELCR; nessuno snapshot intermedio.
  Script: `agents/gml/pelcr_snapshot.sh <file.plcr> nodes|compositions|end [N]` (Linux, gdb).
- dd3 completo: 13439 composizioni, ~1 s, grafo finale 7138 nodi e 7301 archi: dd3 non arriva a 10000 nodi.
  `GML/dd3_finale_13439_7138nodi.{gml,clean.gml,graphml}`.
- dd4 ((δ)(δ)4) a 10000 nodi: 16979 composizioni, 11002 archi. `GML/dd4_16979_10000nodi.{gml,clean.gml,graphml}` (graphml 11,6 MB).
  Sfondi `OUTPUT/website/dd4-10000nodi-sfondo-*`: SVG 1,5 MB (300 KB gzip), PNG ~2,6 MB, WebP ~0,7 MB.
- Oltre 3000 nodi: neato non finisce in 5 minuti; `gml_to_background.py` e `gml_to_yed.py` usano `sfdp`.
  Punti e tratti si riducono con la densità (tratto ~0,4 e punti ~0,23 a 10000 nodi).
