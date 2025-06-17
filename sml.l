%{
//  $Id: sml.l,v 1.5 1993/06/09 16:25:03 rmartin Exp $
// --------------------------------------------------------
// Name
// 	sm.l -- Lexical functions for parsing StateMap files..
// 
// Description
// 	This LEX file specifies the lexical analysis necessary for the
// 	tokenization of Statemap specification files.
// 
// See Also
// 	sm.y
// 
// Author(s)
// 	R. Martin
// 
// Date/History
//   30 May 93 0942
//     Change to "Context Derived" form.
// 
// 	27 Jun 91 0936
//

#include "smy.h"
#include "string.h"

extern YYSTYPE yylval;
int lineNumber;

%}
/* start the lexical rules */
%%

FSMNAME			|
fsmname 		|
FSMName 		|
FsmName 		{
				return(C_FSMNAME);
				}

CONTEXT			|
context			|
Context			{
				return(C_CONTEXT);
				}

INITIAL         |
Initial         |
initial         {
                return(C_INITIAL);
                }

HEADER			|
Header			|
header			{
				return(C_HEADER);
				}

Version.*\n		|
version.*\n		|
VERSION.*\n		{
				strcpy(yylval.s, yytext);
				lineNumber++;
				return(C_VERSION);
				}

[{}\*():]		{
				return(yytext[0]);
				}

[A-Za-z][A-Za-z0-9._]*	{
				strcpy(yylval.s, yytext);
				return(WORD);
				}

\/\/.*\n		|
\n				{
				lineNumber++;
				}

\/\*.*\*\/		|
\t				|
" "				;

%%


