# NDPELCR: PELCR con choice non deterministica

Branch `non-deterministico` (dal 2026-10-30). Estensione di PELCR (C) secondo l'articolo
"Optimal Reduction for Non-Deterministic Lambda Calculus" (Pedicini, Piazza; TOCL-2024-0041,
`ND-PELCRVM2/ND-PELCRVM.github/main.tex`, branch `claude-split`), con il prototipo Wolfram
`code/ND-VM-PELCR/VM-PELCR5.ma` e la sua traduzione Python (`code/ND-VM-PELCR/python/`) come
ispirazione e come verifica.

## Sintassi

Una choice si scrive `[t + u]`, come nell'articolo:

```
#def delta = \x.(x)x ;
#def two = \f.\x.(f)(f)x ;
#def three = \f.\x.(f)(f)(f)x ;
(delta)[two + three] ;
```

Esempi in `pelcrexamples/nd/` (riduzioni dell'articolo, simboli `#def` copiati, termine divergente).

## Algebra dinamica: Λ*_nd (`symbolic.c`)

L'articolo calcola i pesi nel quadrato ridotto Λ*_nd = Λ*⊗Λ* (Def. `ndexgoi2`): coppie (u0, u1) con
prodotto, involuzione e lift componente per componente, e una coppia con una componente nulla vale 0.
Gli elementi designati sono P = (1,p), Q = (1,q), W_j = (w_j,w_j), C_h = (w_h,1) con e(w_h) = 1.

- Un peso (positivo) è scritto `u1|u0` (componente 1, poi componente 0), oppure `u1` per la coppia
  (erase(u1), u1), dove erase cancella `p`, `q` e le lettere esterne `X(..)`. La seconda forma è esatta per
  ogni prodotto di P, Q, W_j e lift (la componente 0 è la stessa parola senza p e q): i calcoli senza choice
  restano nella rappresentazione classica e usano il prodotto classico, quindi le run deterministiche fanno
  esattamente gli stessi passi di prima.
- Le choice usano il generatore `c(h,1)`, solo nella componente 0; i fan `w(i,l)` stanno in entrambe.
  `c` e `w` sono generatori distinti (J_f ∩ J_⊕ = ∅): c_h* c_k = 0 per h ≠ k, c_h* w_j = 0.
- `product(a, b, a1, b1)` (stessa convenzione di prima: a* b = b1 a1*) calcola la componente 0 e poi la 1
  con il prodotto classico `product1`; la componente 0 per prima perché non ha effetti collaterali (le
  lettere esterne stanno solo nella componente 1). `nd_join` rimette il risultato nella forma classica
  quando la componente 0 coincide con erase(u1).
- In `product1`: le regole di `w` valgono anche per `c` (ANNIHILATION, COMMUTATION !(u)x = x!^{e(x)}(u));
  aggiunte le commutazioni !(u)* p = p !(u)* e p* !(u) = !(u) p* (lift 1) del Lemma `nd-product-equations`
  e della Def. `exgoi`, che prima mancavano (le statistiche delle run deterministiche non cambiano).
- Le relazioni del Lemma `nd-product-equations` (c_h* p = p c_h*, !(u) c_h = c_h !(u), …) seguono dal calcolo
  componente per componente, come nella prova dell'articolo.
- `isone` riconosce anche `c`; `CheckWeightLength` ferma la run se un peso supera `MAXLENWEIGHT`
  (default 100, `make MAXLENWEIGHT=...`); la lunghezza massima osservata è stampata a fine run
  (`max weight length`; ≤ 25 caratteri negli esempi).

## Parser e traduzione (`parser.y`, `buildgraph.c`)

- Regola `'[' lterm OP1 lterm ']'` (con OP1 = `+`) → `BuildChoice`.
- `BuildChoice(t1, t2)`: i cammini dalla radice al ramo i attraversano la porta di choice del ramo i, di peso
  C_{h_i} (Def. `weights`): ogni arco alla radice di t_i è moltiplicato a sinistra per `c(h_i,1)` nella
  componente 0, le due radici sono fuse e le variabili libere comuni sono contratte come in un'applicazione
  (`MergeNodes`). Il lift di profondità arriva dopo, come per gli altri costruttori (`Lift` sugli argomenti).
- Codici: la i-esima choice ha `c(2i,1)` sul ramo sinistro e `c(2i+1,1)` sul destro (iniettivi sui rami,
  Def. `address-generator-map`); il contatore non si azzera tra un termine e l'altro.
- Le copie dei simboli `#def` (`Duplicate`) ricevono famiglie di codici nuove (`RenameChoices`), conservando
  la parità sinistra/destra: il read-back ordina i rami secondo i codici.
- `LiftEdges`, `P_Q` passano per `LiftWeight` e `PrefixWeight1`, che conoscono le due componenti: il lift
  del generatore di fan in testa a un arco di variabile libera cresce in entrambe le componenti; p e q
  moltiplicano solo la componente 1.

## Read-back (`read_back.c`, `agents/readback/`)

- `read_back` scrive il grafo finale in JSON con `-readback <file>` (o `PELCR_READBACK_JSON`): nodi, archi con
  lati e pesi di Λ*_nd, porta principale e variabili libere del termine (registrate da
  `InitializeIncoming`). Le sorgenti degli archi sono risolte negli indirizzi dei nodi (tramite `BookTable`
  per le chiavi del costruttore), quindi non servono più gdb né `free()` disattivata.
- `agents/readback/pelcr_c_readback.py` legge i pesi `u1|u0`, calcola l'osservazione Obs per catene e chiama
  il decoder dell'articolo (`pelcr_readback.readback`): a fine run Obs = EX(τ(t)) e il decoder ricostruisce la
  forma normale, choice comprese, a meno delle λ vacue rispetto alle choice
  ([λx.A + λy.B] = λx.[A + B] se x ∉ FV(A), y ∉ FV(B)), l'unica informazione che Obs non contiene.
- Termini aperti: le variabili libere sono porte di bordo.
- Run interrotte (`-fires N`): read-back parziale, approssimante con ⊥.

## Verifica (2026-10-30)

- Regressione deterministica: dd2, dd3, d4, ki, k2i, 2I, succ, pure, fibonacci, longid, jfig2, s0, s1 danno
  le stesse statistiche di prima (nodi finali, composizioni, fires, ottimizzazioni banali, riduzioni di famiglia).
- `agents/readback/test_c_readback.py` (vedi il README lì): 48 termini fissi (29 con choice: esempi di
  ND-PELCR, riduzioni svolte nell'articolo e 4 file `.plcr` con `#def`) e 460 casuali in tre serie
  (seme 7: 200; seme 2026, solo termini con choice: 200; seme 99: 60): read-back = nf(t) (6 a meno di λ vacue)
  e Obs = EX(τ(t)) del modello Python per tutti; (δ)[I⊕δ] con run interrotte (300, 1000, 3000 composizioni:
  18, 74, 242 cammini) dà gli approssimanti [[I⊕δ] ⊕ [[I⊕δ] ⊕ … ⊥]] dell'articolo (§5).
- Prototipo Wolfram (`VM-PELCR5.ma`, eseguito con Mathics 3 nel cloud): `compare_wolfram.py` confronta EX(τ(t))
  calcolato dal prototipo con Obs del grafo finale di PELCR e i due read-back; esiti nel log qui sotto.
- Run parallele (np = 1, 2, 4) degli esempi `pelcrexamples/nd/`: terminano, stesse riduzioni di famiglia.

### Log del confronto con il prototipo Wolfram (2026-10-30)

Prototipo `VM-PELCR5.ma` eseguito con Mathics 3 (10.0.1, con la patch all'asserzione interna descritta in
`README-VM-PELCR5.md`) nel workspace cloud. Mathics impiega da secondi a decine di minuti per termine, quindi
il confronto completo va rifatto con Mathematica: `python3 agents/readback/compare_wolfram.py` (usa
`wolframscript -file`; `--wolfram "<comando>"` per un altro eseguibile).

- `esegui-termine.wl` (macchina Wolfram, Obs = EX, read-back dal dump), stessi cammini e stessa forma normale
  della run C: `[\x.x + \y.\z.z]` (4 cammini), `([\i.i + \k.\l.k])\j.j` (4),
  `(\x.(x)x)[\u.u + \v.\w.v]` (8), three-phases `(\x.([(x)δ + (y)x])y)\z.z` (10, `[(y)y + ((y)\z.z)y]`).
- `compare_wolfram.py` (EX del prototipo contro Obs del grafo finale di PELCR, cammini uguali anche negli
  indici delle lettere, generatori a meno del nome; read-back): 14 termini uguali, 6 con choice
  (`[\x.x + \y.\z.z]`, `([\i.i + \k.\l.k])\j.j`, `(\x.(x)x)[\u.u + \v.\w.v]`, `(\x.(x)x)[\i.i + \j.j]`,
  `(\x.[x + x])(\y.y)\z.z`, `(\g.(g)\u.u)\x.[x + \v.v]`) e 8 senza (`I`, `K I`, `K I I`, `dup I`, `two I`,
  `one I`, `f-dup`, `two`); nessuna differenza. Gli altri termini della lista non erano ancora finiti in Mathics.

## Limiti e passi successivi

- Read-back su un solo rank; simboli esterni `X(...)` non letti (la loro interazione con le choice non è
  verificata: le lettere esterne stanno nella componente 1).
- Il decoder del read-back è in Python (codice dell'articolo); un decoder in C resta da scrivere.
- Termini con forma normale molto grande (es. (δ)(δ)[two + three]) non finiscono in tempi brevi.
