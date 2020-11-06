CFLAGS := -g -Wall -Wextra

default : program

program : input/lexer.o input/parser.o data_structures/list.o data_structures/hashmap.o data_structures/network.o features/bspace.o features/diag.o main.o
	cc $(CFLAGS) -o $@ $^

input/lexer.o : input/lexer.c data_structures/list.h data_structures/hashmap.h data_structures/network.h

input/parser.o : input/parser.c data_structures/list.h data_structures/hashmap.h data_structures/network.h

data_structures/list.o : data_structures/list.h data_structures/list.c

data_structures/hashmap.o : data_structures/hashmap.h data_structures/hashmap.c

data_structures/network.o : data_structures/network.h data_structures/network.c

features/bspace.o : features/bspace.h features/bspace.c data_structures/list.h data_structures/hashmap.h data_structures/network.h

features/diag.o : features/diag.h features/diag.c data_structures/list.h data_structures/hashmap.h data_structures/network.h

main.o : main.c features/bspace.h data_structures/list.h data_structures/hashmap.h data_structures/network.h

input/lexer.c : input/lexer.lex input/parser.h data_structures/list.h data_structures/hashmap.h data_structures/network.h
	flex -o $@ input/lexer.lex

input/parser.h input/parser.c : input/parser.y data_structures/list.h data_structures/hashmap.h data_structures/network.h
	bison -d -o input/parser.c input/parser.y


.PHONY : clean

FILES := input/lexer.o input/parser.o data_structures/list.o data_structures/hashmap.o data_structures/network.o features/bspace.o features/diag.o main.o input/lexer.c input/parser.c input/parser.h program

clean :
	touch $(FILES)
	rm $(FILES)
