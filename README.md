# algos
Diagnose an automaton network given a linear observation

***Dependencies***
* Flex
* Bison

\
***Build***
```bash
cd algos
make
```
\
***Usage***
```
Usage:
	algos action [file_in]

Actions:
	help    Show this help message
	load    Load network then exit
        test    Test network loading and serialization
	dot     Output dot representation of the network

	bspace  Compute the behavioral space of the network
	comp    Compute a behavioral subspace of the network given an observation
	diag    Output a diagnosis given a behavioral subspace
	dctor   Build the diagnosticator of a network given its behavioral space
	dcdiag	Output a diagnosis given a diagnosticator and an observation

```
\
***Offline approach***
* Compute the behavioral subspace of a network, using the linear observation specified in the file.
```bash
./algos comp test/test_network > cm
```
* Use the generated subspace to output a diagnosis. 
```bash
./algos diag cm
```
\
***Online approach***
* Perform some preprocessing on the network. The resulting automaton (diagnosticator) is observation independent.
```bash
./algos bspace test/test_network | ./algos dctor > dc
```
* Now diagnose the network, using the diagnosticator and the observation specified in the same file. You can edit the observation without repeating the preprocessing step.
```bash
./algos dcdiag dc
```
\
***Dot***
* You can use the dot command to convert networks, behavioral spaces/subspaces and diagnosticators. The output can then be converted to different formats with a dot compiler (e.g. graphviz).
```bash
./algos dot test/test_network | dot -Tpdf -o test_network.pdf
```
