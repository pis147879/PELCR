# Read-back delle run di PELCR (C), anche non deterministiche

Ricostruisce la forma normale dal grafo finale di una run di PELCR, choice comprese, usando il decoder
del read-back scritto per l'articolo ND-PELCR
(`ND-PELCRVM2/ND-PELCRVM.github/code/ND-VM-PELCR/python/pelcr_readback.py`).

## Come funziona

1. PELCR scrive il grafo finale in JSON con l'opzione `-readback <file>` (o la variabile d'ambiente
   `PELCR_READBACK_JSON`): lo fa `read_back.c`, chiamata a fine run. Il file contiene i nodi delle liste
   `G.hot` e `G.cold`, gli archi di ciascun nodo con lato del bersaglio, lato della sorgente e peso, la
   porta principale (`principal`) e i nodi delle variabili libere del termine (`free`).
   La sorgente di un arco è risolta nell'indirizzo del nodo che denota (direttamente per i nodi creati
   dalla macchina, tramite `BookTable` per le chiavi del costruttore del grafo, creator = size): non
   serve più disattivare `free()`.
2. `pelcr_c_readback.py` traduce il grafo nel formato del modello Python dell'articolo (archi
   `(et, es, vt, vs, peso)` con pesi in Λ*_nd), calcola l'osservazione (cammini tra le porte di bordo
   seguendo le catene) e chiama `pelcr_readback.readback`.
3. `pelcr_run.py` mette insieme i due passi per un termine o un file `.plcr`.

`pelcr_gdb_dump.py` (estensione di gdb, con `nofree.c` caricato con `LD_PRELOAD`) scrive lo stesso
formato per gli eseguibili senza l'opzione `-readback`: `pelcr_run.py --gdb`.

Conversioni dalle convenzioni di PELCR (C) a quelle di ND-PELCR:

- pesi: elementi di Λ*_nd = Λ*⊗Λ* (Def. `ndexgoi2`), scritti `u1|u0` (componente 1, poi componente 0)
  oppure `u1` per la coppia (erase(u1), u1), dove erase cancella `p` e `q` (pesi senza choice);
- parole: stringhe di `p`, `q`, `w(i,l)` (generatore di fan w_i di lift l), `c(h,1)` (generatore di choice
  c_h, lift 1, solo nella componente 0), `1` e `!k`, dove `!k` solleva tutto ciò che segue: l'indice di
  una lettera è la somma dei `!k` che la precedono;
- `w(i,l)` diventa il generatore di fan `f<i>`, `c(h,1)` il generatore di choice `c<h>`: i rami di una
  choice hanno i codici 2i (sinistro) e 2i+1 (destro), e il decoder ordina i rami secondo i codici;
- `p` e `q` sono scambiati (λ: lato della variabile `p` in C, `q` in ND-PELCR);
- LEFT = 1 diventa `+`, RIGHT = 0 diventa `-`.

## Requisiti

- un eseguibile PELCR a un solo rank (es. `build/combustion.linux` da `Makefile.linux`); gdb solo con `--gdb`;
- il codice Python di ND-PELCR: `NDPELCR_PYTHON=.../code/ND-VM-PELCR/python`, altrimenti viene cercato
  in `~/marco.live/TEX/2026/ND-PELCRVM2/...` e in `~/mnt/ND-PELCRVM2/...`.

## Uso

Dalla radice di PELCR, dopo la build:

```
python3 agents/readback/pelcr_run.py '(\x.(x)x)[\u.u + \v.\w.v]'
python3 agents/readback/pelcr_run.py pelcrexamples/nd/three-phases.plcr pelcrexamples/dd2.plcr
python3 agents/readback/pelcr_run.py --fires 3000 pelcrexamples/nd/delta-I-delta.plcr
python3 agents/readback/pelcr_c_readback.py OUTPUT/readback/dd2.json
python3 agents/readback/test_c_readback.py --random 200 --seed 7
python3 agents/readback/compare_wolfram.py                      # serve wolframscript (Mathematica)
python3 agents/readback/check_examples.py --report OUTPUT/check_examples.txt   # tutti gli esempi
python3 agents/readback/check_examples.py --base build/combustion.master ...   # confronto con un'altra build
```

Oppure a mano: `printf '#setdir "pelcrexamples" ; #open "dd2.plcr"\n' | build/combustion.linux -- -loop 10000000 -readback dd2.json`.

Sintassi dei termini come in PELCR e nell'articolo: `\x.t`, `(t)u`, `[t + u]`, dove `(t)u` prende come
argomento tutto il termine che segue (`(f)(f)x = (f)((f)x)`). Niente parentesi superflue: `((f)x)` da solo
non è un termine per il parser di PELCR, che in quel caso entra in un ciclo di errori (il log è troncato a 5 MB).

`--fires N` ferma la run dopo N composizioni: per un termine che diverge il dump si osserva comunque e il
read-back, in modalità parziale, dà un approssimante con `_|_` dove non si è ancora osservato nulla.

## Verifiche (2026-10-30, branch `non-deterministico`)

`test_c_readback.py` confronta, per ogni termine t, il read-back con nf(t) (β e distribuzione a sinistra,
`terms.nf` di ND-PELCR) a meno di λ vacue portate fuori dalle choice, e l'osservazione del grafo finale con
EX(τ(t)) del modello Python dell'articolo (stessi cammini tra le stesse porte di bordo, stessi pesi a meno
dei nomi dei generatori):

- 48 termini fissi: 19 senza choice (fra cui `two two`, `three three`, `exp22`, `exp23`, termini aperti),
  25 con choice (gli esempi di `test_readback.py` di ND-PELCR e le riduzioni svolte nell'articolo:
  `dup-choice`, `phases`, `distribution`, `shared-redex`, `three-phases`, `shared-choice`, `readback-trace`,
  `reduction5`), 4 file `pelcrexamples/nd/*.plcr` con simboli `#def`: tutti uguali a nf(t), Obs = EX;
- termini casuali: seme 7 (200, 248 termini in tutto, 113 con choice), seme 2026 con `--only nd`
  (200, 229 in tutto, 164 con choice), seme 99 (60, 108 in tutto): tutti uguali a nf(t) (6 a meno di λ vacue),
  Obs = EX per tutti;
- (δ)[I⊕δ] (articolo, §5) con `-fires` 300, 1000, 3000: run interrotte, 18, 74, 242 cammini osservati e
  approssimanti crescenti [[I⊕δ] ⊕ [[I⊕δ] ⊕ … ⊥]], come nella VM Python e in quella Wolfram;
- `dd2.plcr` con il dump nativo: λf.λx.f²⁵⁶x (88 nodi, 97 archi, 20 cammini), senza gdb.

`compare_wolfram.py` confronta PELCR con il prototipo Wolfram `VM-PELCR5.ma`: EX(τ(t)) calcolato dalla VM
Wolfram e Obs del grafo finale di PELCR (stessi cammini, pesi uguali a meno dei nomi dei generatori) e il
read-back. Con Mathics 3 (lento) sono stati controllati 14 termini, 6 con choice, senza differenze
(log in `agents/ND-PELCR.md`); `--wolfram "mathics --no-readline -f"` per usare Mathics invece di wolframscript.

## Limiti

- un solo rank MPI per il read-back (con più rank `read_back.c` scrive un file per rank, `<file>.<rank>`,
  che il decoder non sa ricomporre);
- niente simboli esterni `X(...)` (costanti, funzioni, `#xcall`) nel read-back.
