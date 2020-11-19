# algos
automaton networks and stuff

[***VERSIONE 1***](https://github.com/Floccari/algos/tree/aaebab611efb431a7b0aa292b843196179cd28eb) 
* Costruzione delle strutture dati etc
* Costruzione linguaggio di input/output
* Costruzione dei primi comandi help, test e comp
* Costruzione della funzione Step per il calcolo dello spazio comportamentale, **(pseudocodice)**
* Aggiornamento strutture dati soprattutto aggiunta in state dell'enum color,costruzione della funzione di Pruning 
  usando DFS per trovare gli stati prunabili e una serie di rimozioni non semplici per prunare effettivamente
  (Approccio che favorisce la ricerca a scapito della quantità di strutture dati
  e della complessità nella rimozione) (pseudocodice)
* Un primo refactoring per comprendere meglio tutto il processo di rimozione
* Secondo refacorting per ordinare il codice per favorire una maggiore leggibilità per implementazioni future più una
  ridenominazione del comando comp in bstate
* Aggiunto il comando dot per serializzare una rete in formato dot, così da poter generare al volo grafici in formato pdf

[***VERSIONE 2***](https://github.com/Floccari/algos/tree/cd6460aef928257adb20a75356701b9fd7364b2b)
* Implementazione della seconda funzionalità richiesta (comando comp, stavolta per davvero), per il calcolo dello spazio
  raggiungibile data una serie di etichette **(pseudocodice)**
* Aggiornamento del linguaggio permettendo l'aggiunta di una osservazione lineare (lista di etichette)
* Piccolo aggiornamento delle strutture dati, fondamentale nella struttura context
* Refactoring per la funzione Step e Compute per aggiungere la funzionalità

[***VERSIONE 3***](https://github.com/Floccari/algos/tree/c137d026e8c2f82b682bc14c3b2a8e2d75d1743f)
* Refactoring per permettere di agganciare/sganciare velocemente transizioni e stati
* Aggiunta della terza funzionalità (comando diag), la relativa funzione é strutturata seguendo lo pseudocodice della prof
  e divisa in 3 fasi per maggiore leggibilità **(pseudocodice)**
* Piccola revisione della fase due di Get_Diagnosis

***VERSIONE 4***
* Aggiunto il secondo algoritmo per le espressioni regolari, rifattorizzato assieme al primo
* Aggiunto l'algoritmo per calcolare le chiusure silenziose (da decorare) **(pseudocodice)**
* Aggiunto l'algoritmo per calcolare lo spazio delle chiusure (da decorare) **(pseudocodice)**
* Aggiunto il comando dctor per calcolare il diagnosticatore **(pseudocodice)**
* Modificato il linguaggio di input per permettere la corretta lettura del diagnosticatore
