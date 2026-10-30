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

## Prossimi obiettivi

- Read-back C: termini aperti (nodi variabile di bordo), simboli esterni `X(...)`, più rank, run interrotte (read-back parziale).

## Completato

- 2026-10-29: creata la cartella `agents/` con `README.md`, `MEMORY.md`, `ROADMAP.md`.
- 2026-10-29: adattatore read-back per le run C in `agents/readback/`, verificato su 167 termini (17 fissi + 150 casuali).
