%{

#include "../data_structures/list.h"
#include "../data_structures/hashmap.h"
#include "../data_structures/network.h"

#include "parser.h"

int line = 1;
char *lexval;

char *newstring(char *str);

%}

%option noyywrap

spacing		([ \t])+
comment		#(.)*\n
digit		[0-9]
letter		[a-zA-Z]
alphanum	{letter}|{digit}
id		{alphanum}*(_{alphanum}+)*
other		[\":,;()]

%%

{spacing}	;
{comment}	{line++;}
\n		{line++;}
network		{return NETWORK;}
automatons	{return AUTS;}
events		{return EVS;}
"->"		{return ARROW;}
end             {return END;}
automaton	{return AUT;}
states		{return STS;}
initial		{return INIT;}
obs		{return OBS;}
rel		{return REL;}
in		{return IN;}
out		{return OUT;}
{id}		{lexval = newstring(yytext); return ID;}
{other}		{return *yytext;}
.		{return ERROR;}

%%

char *newstring(char *str) {
     char *p = malloc(strlen(str) + 1);
     strcpy(p, str);
     return p;
}
