# algos
automaton networks and stuff

***Requisiti***
* Flex
* Bison

***Compilazione***
```bash
cd algos
make
```

***Funzionalità***
```
Usage:
	./program action [file_in]

Actions:
	help    Show this help message
	test    Test network loading and serialization
	dot     Output dot representation of the network

	bspace  Compute the behavioral space of the network
	comp    Compute a behavioral subspace of the network given an observation
	diag    Output a diagnosis given a behavioral subspace
	dctor   Build the diagnosticator of a network given its behavioral space
	dcdiag	Output a diagnosis given a diagnosticator and an observation

```

***Esempi***
* Calcolare lo spazio comportamentale di una rete relativo ad un'osservazione lineare (specificata in coda alla rete) e produrre una diagnosi
```bash
./program comp reti_test/rete_test | ./program diag
```
* Costruire il diagnosticatore relativo ad una rete e salvarlo su file
```bash
./program bspace reti_test/rete_test | ./program dctor > diagnosticatore
```
* Utilizzare il diagnosticatore ottenuto per produrre una diagnosi data un'osservazione lineare (specificata in coda al diagnosticatore)
```bash
./program dcdiag diagnosticatore
```
* Produrre un file .pdf che rappresenta una rete/spazio comportamentale/diagnosticatore **(richiede un compilatore dot e.g. graphviz)**
```bash
./program dot reti_test/rete_test | dot -Tpdf -o rete.pdf
```
