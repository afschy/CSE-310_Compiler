%option noyywrap
%x STRING
%x COMMENT
%x MULCOMMENT

%{
#include <iostream>
#include <string>
#include <cstring>
#include <cstdio>
#include <cstdlib>
#include <vector>
#include "node.hpp"
#include "merged.hpp"
#include "y.tab.h"
typedef unsigned long long ull;

extern YYSTYPE yylval;
void yyerror(char *);

extern FILE* logout;
extern FILE* errout;

int line_count = 1;
int error_count = 0;
int first_line;

SymbolTable st(10);
std::string str, strtype, strerr;

void common_output(const char* type){
	// fprintf(tokenout,"<%s, %s>\n",type,yytext);
	fprintf(logout,"Line# %d: Token <%s> Lexeme %s found\n",line_count,type,yytext);
}

void string_output(){
	// fprintf(tokenout,"<%s, %s>\n",strtype.c_str(),str.c_str());
	fprintf(logout,"Line# %d: Token <%s> Lexeme %s found\n",first_line,strtype.c_str(),strerr.c_str());
}

void error(const char* message){
	fprintf(errout, "Line# %d: %s %s\n", line_count,message, yytext);
	fprintf(logout, "Line# %d: %s %s\n", line_count,message, yytext);
}

void string_error(const char* message){
	fprintf(errout, "Error at line# %d: %s %s\n", line_count,message, strerr.c_str());
}

char convert_escaped(const char escape){
	if(escape == '\\') return 92;
	if(escape == '\'') return 39;
	if(escape == '\"') return 34;
	if(escape == 'n') return 10;
	if(escape == 't') return 9;
	if(escape == 'a') return 7;
	if(escape == 'f') return 12;
	if(escape == 'r') return 13;
	if(escape == 'b') return 8;
	if(escape == 'v') return 11;
	if(escape == '0')return 0;
	return escape;
}
%}

WHITESPACE [ \t\f\v]+
LETTER [a-zA-Z]
DIGIT [0-9]
NEWLINE (\r)?\n
INTEGER {DIGIT}+
DECIMAL {DIGIT}*\.{DIGIT}+
FLOAT {DECIMAL}([eE][+-]?{DIGIT}+)?|{INTEGER}[eE][+-]?{DIGIT}+
IDENTIFIER [_A-Za-z][_A-Za-z0-9]*
PRINTABLE [^\\'\n\r]
CHAR [^'\n\r]
ESCAPED [\\'"ntafrbv0]
STR_PRINTABLE [^\\"\n\r]

%%

{NEWLINE} {line_count++;}
{WHITESPACE} {}

"if"	{common_output("IF"); yylval.line = line_count; return IF;}
"else"	{common_output("ELSE"); yylval.line = line_count; return ELSE;}
"for"	{common_output("FOR"); yylval.line = line_count; return FOR;}
"while"	{common_output("WHILE"); yylval.line = line_count; return WHILE;}
"do"	{common_output("DO");}
"break"	{common_output("BREAK");}
"int"	{common_output("INT"); yylval.line = line_count; return INT;}
"char"	{common_output("CHAR");}
"float"	{common_output("FLOAT"); yylval.line = line_count; return FLOAT;}
"double"	{common_output("DOUBLE");}
"void"	{common_output("VOID"); yylval.line = line_count; return VOID;}
"return"	{common_output("RETURN"); yylval.line = line_count; return RETURN;}
"switch"	{common_output("SWITCH");}
"case"	{common_output("CASE");}
"default"	{common_output("DEFAULT");}
"continue"	{common_output("CONTINUE");}
"println"	{common_output("PRINTLN"); yylval.line = line_count; return PRINTLN;}

"+"		|
"-"		{
			common_output("ADDOP");
			yylval.symbolInfo = new SymbolInfo(yytext, "ADDOP", line_count);
			return ADDOP;
		}

"*"		|
"/" 	|
"%"		{common_output("MULOP"); yylval.symbolInfo = new SymbolInfo(yytext, "MULOP", line_count); return MULOP;}

"++"	{common_output("INCOP"); yylval.symbolInfo = new SymbolInfo(yytext, "INCOP", line_count); return INCOP;}

"--"	{common_output("DECOP"); yylval.symbolInfo = new SymbolInfo(yytext, "DECOP", line_count); return DECOP;}

"<"		|
"<="	|
">"		|
">="	|
"=="	|
"!="	{common_output("RELOP"); yylval.symbolInfo = new SymbolInfo(yytext, "RELOP", line_count); return RELOP;}

"="		{common_output("ASSIGNOP"); yylval.symbolInfo = new SymbolInfo(yytext, "ASSIGNOP", line_count); return ASSIGNOP;}

"&&"	|
"||"	{common_output("LOGICOP"); yylval.symbolInfo = new SymbolInfo(yytext, "LOGICOP", line_count); return LOGICOP;}

"&"		|
"|"		|
"^"		|
"<<"	|
">>"	{common_output("BITOP");}

"!"		{common_output("NOT"); yylval.symbolInfo = new SymbolInfo(yytext, "NOT", line_count); return NOT;}

"("		{common_output("LPAREN"); yylval.line = line_count; return LPAREN;}
")"		{common_output("RPAREN"); yylval.line = line_count; return RPAREN;}

"{"		{
			common_output("LCURL");
			yylval.line = line_count;
			return LCURL;
		}
"}"		{
			common_output("RCURL");
			yylval.line = line_count;
			return RCURL;
		}

"["		{common_output("LSQUARE"); yylval.line = line_count; return LSQUARE;}
"]"		{common_output("RSQUARE"); yylval.line = line_count; return RSQUARE;}

","		{common_output("COMMA"); yylval.line = line_count; return COMMA;}
";"		{common_output("SEMICOLON"); yylval.line = line_count; return SEMICOLON;}

{IDENTIFIER}	{
					common_output("ID");
					yylval.symbolInfo = new SymbolInfo(yytext, "ID", line_count);
					return ID;
				}
		
{INTEGER} 	{common_output("CONST_INT"); yylval.symbolInfo = new SymbolInfo(yytext, "CONST_INT", line_count); return CONST_INT;}
{FLOAT}	{common_output("CONST_FLOAT"); yylval.symbolInfo = new SymbolInfo(yytext, "CONST_FLOAT", line_count); return CONST_FLOAT;}

{DECIMAL}(\.|{DECIMAL})+({INTEGER}|{FLOAT}|\.)* {error("TOO_MANY_DECIMAL_POINTS"); error_count++;}
{DIGIT}+\.[\.]+({INTEGER}|{FLOAT}|\.)*	{error("TOO_MANY_DECIMAL_POINTS"); error_count++;}
\.[\.]+{DIGIT}({INTEGER}|{FLOAT}|\.)*	{error("TOO_MANY_DECIMAL_POINTS"); error_count++;}

({INTEGER}|{FLOAT})({INTEGER}|{FLOAT}|\.)+	{error("ILLFORMED_NUMBER"); error_count++;}
({INTEGER}|{FLOAT}){IDENTIFIER}	{error("INVALID_ID_SUFFIX_NUM_PREFIX"); error_count++;}

'{PRINTABLE}'	{
					yytext[0] = yytext[1];
					yytext[1] = 0;
					common_output("CONST_CHAR");
				}

'\\{ESCAPED}'	{
					char c = convert_escaped(yytext[2]);
					yytext[0] = c;
					yytext[1] = 0;
					common_output("CONST_CHAR");
				}

''	{error("EMPTY_CONST_CHAR"); error_count++;}

'{CHAR}*	{error("UNFINISHED_CONST_CHAR"); error_count++;}
'{CHAR}*\\'	{error("UNFINISHED_CONST_CHAR"); error_count++;}

'{PRINTABLE}{CHAR}+'	{error("MULTICHAR_CONST_CHAR"); error_count++;}
'\\{CHAR}+'	{error("MULTICHAR_CONST_CHAR"); error_count++;}

\"	{
		str = "";
		strerr = "\"";
		strtype = "SINGLE LINE STRING";
		first_line = line_count;
		BEGIN(STRING);
	}

<STRING>{STR_PRINTABLE}*	{str += yytext; strerr += yytext;}
<STRING>\\{NEWLINE}	{
						line_count++;
						strerr += yytext;
						strtype = "MULTI LINE STRING";
					}

<STRING>\\({CHAR}|{ESCAPED})	{str += convert_escaped(yytext[1]); strerr += yytext;}

<STRING>\"	{
				strerr += "\"";
				string_output();
				BEGIN(INITIAL);
			}

<STRING><<EOF>>	{
					string_error("UNFINISHED_STRING");
					error_count++;
					BEGIN(INITIAL);
				}
<STRING>{NEWLINE}	{
						string_error("UNFINISHED_STRING");
						error_count++;
						line_count++;
						BEGIN(INITIAL);
					}

\/\/	{
			str = "//";
			strtype = "SINGLE LINE COMMENT";
			first_line = line_count;
			BEGIN(COMMENT);
		}

<COMMENT>\\{NEWLINE}	{
							str += yytext;
							line_count++;
						}
<COMMENT>{NEWLINE}	{
						fprintf(logout, "Line# %d: Token <SINGLE LINE COMMENT> Lexeme %s found\n", first_line, str.c_str());
						line_count++;
						BEGIN(INITIAL);
					}						

<COMMENT>[ !-\[\]-~\t\f\v]*	{str += yytext;}
<COMMENT>\\/[^{NEWLINE}]	{str += "\\";}

\/\*	{
			str = "/*";
			first_line = line_count;
			BEGIN(MULCOMMENT);
		}

<MULCOMMENT>[ !-\)+-~\t\f\v]* {str += yytext;}
<MULCOMMENT>{NEWLINE}	{
							str += "\n";
							line_count++;
						}

<MULCOMMENT>\*/[^\/]	{str += "*";}
<MULCOMMENT>\*\/	{
						str += "*/";
						fprintf(logout, "Line# %d: Token <MULTI LINE COMMENT> Lexeme %s found\n", first_line, str.c_str());
						BEGIN(INITIAL);
					}

<MULCOMMENT><<EOF>>	{
						strerr = str;
						error_count++;
						string_error("UNFINISHED_COMMENT");
						BEGIN(INITIAL);
					}

.	{error("UNRECOGNIZED_CHAR"); error_count++;}
%%
