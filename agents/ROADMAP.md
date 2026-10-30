# Roadmap

## In corso

### Grafo di esecuzione di (δ)(δ)2 come sfondo del sito web

Stato: avviato il 2026-10-29.

- [x] Individuare l'esempio: `pelcrexamples/dd2.plcr`
- [x] Compilare PELCR (cloud, `Makefile.linux`) ed eseguire `dd2.plcr`: termina senza errori (2026-10-29)
- [x] Verificare la forma normale con il read-back della VM Python di ND-PELCR: numerale di Church 256 (2026-10-29)
- [x] Read-back dell'output della run C: adattatore in `agents/readback/` (gdb -> JSON -> decoder di ND-PELCR); dd2 dà 256 (2026-10-29)
- [x] Decidere cosa rappresentare: grafo di dd2 a metà esecuzione (245 di 482 composizioni) (2026-10-29)
- [x] Emettere il GML senza modificare i sorgenti: `#write "<prefisso>" ; #trace ;` (snapshot a ogni one-optimization)
- [x] GML in `GML/`: `dd2_mid_245of482` (.gml, .clean.gml, .graphml per yEd) e `dd3_early_380` (112 nodi, 130 archi)
- [x] SVG/PNG per lo sfondo: `agents/gml/gml_to_background.py` (layout neato, varianti chiaro/scuro/pelcr, pesi come etichette)
- [x] Immagini in `OUTPUT/website/` (SVG + PNG 2560x1440 trasparenti, `anteprima.png`) (2026-10-29)
- [ ] Scelta della variante e inserimento nel sito (Marco)

### NDPELCR: choice non deterministica secondo l'articolo ND-PELCR

Stato: avviato il 2026-10-30, branch `non-deterministico`; dettagli in `agents/ND-PELCR.md`.

- [x] Commit dei file non versionati (`9fed48c`) e branch `non-deterministico`
- [x] Algebra dinamica Λ*_nd = Λ*⊗Λ* in `symbolic.c` (coppie `u1|u0`, generatori di choice `c(h,1)`)
- [x] Parser: `[t + u]` (`parser.y`), costruttore `BuildChoice`, codici nuovi per le copie dei `#def`
- [x] Read-back: dump JSON nativo (`-readback`), pesi di Λ*_nd e variabili libere nell'adattatore Python
- [x] Verifiche: regressione deterministica, 48 termini fissi + 400 casuali (read-back = nf, Obs = EX),
      run interrotte di (δ)[I⊕δ], run parallele, confronto con il prototipo Wolfram (Mathics)
- [ ] Decoder del read-back in C (ora in Python, codice dell'articolo)
- [ ] Read-back con più rank; simboli esterni `X(...)` con le choice

## Prossimi obiettivi

- Read-back C: simboli esterni `X(...)`, più rank (termini aperti e run interrotte: fatti il 2026-10-30).

## Completato

- 2026-10-30: NDPELCR (algebra Λ*_nd, parser `[t + u]`, read-back con choice) sul branch `non-deterministico`.
- 2026-10-29: creata la cartella `agents/` con `README.md`, `MEMORY.md`, `ROADMAP.md`.
- 2026-10-29: adattatore read-back per le run C in `agents/readback/`, verificato su 167 termini (17 fissi + 150 casuali).
