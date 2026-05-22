%{
#include "hoc.h"
#include <string.h>
#include <stdio.h>
Vector *creaVector(int n);
#define code2(c1,c2)     code(c1); code(c2)
#define code3(c1,c2,c3)  code(c1); code(c2); code(c3)

extern int indef;
void defnonly(char *s);
void yyerror(char *s);
int yylex(void);
/* Prototipos agregados para resolver advertencias */
int backslash(int c);
int follow(int expect, int ifyes, int ifno);
void run(void);
void warning(char *s, char *t);
void define(Symbol *sp);
%}
%union {
Symbol     *sym;      /*   Apuntador a la tabla de símbolos */
Inst       *inst;     /*   instrucción de máquina */
int        narg;      /*   número de argumentos */
}

%token     <sym>   FUNCTION PROCEDURE RETURN FUNC PROC READ
%token     <narg>  ARG
%token     <sym>   NUMBER STRING PRINT VAR BLTIN UNDEF WHILE IF ELSE FOR
%type      <inst>  expr stmt asgn prlist stmtlist for forexpr
%type      <narg>  vector_elements
%type      <inst>  cond while if begin end
%type      <sym>   procname
%type      <narg>  arglist
%right     '='
%left       OR
%left       AND
%left       GT GE LT LE EQ NE
%left       '+' '-'
%left       '*' '/' '@' 'X'
%left       UNARYMINUS NOT
%right      '^'
%%
list:            /* nada */
   | list '\n'
   | list defn '\n'
   | list asgn '\n'  { puts("list asgn");
                       code2(pop1, STOP); return 1; }
   | list stmt '\n'  { code(STOP); return 1; }
   | list expr '\n'  { code2(print, STOP); return 1; }
   | list error '\n' { yyerrok; }
   ;
asgn: VAR '=' expr { //puts("VAR = expr ###");
              code3(varpush,(Inst)$1, assign); $$=$3; }
   |  ARG '=' expr
{ defnonly("$"); code2(argassign, (Inst)(long)$1); $$=$3; }
   ;
stmt:     expr  { code(pop1); }
   | RETURN { defnonly("return"); code(procret); } 
   | RETURN expr
          { defnonly( "return" ); $$ = $2; code(funcret) ; } 
     //   $1      $2   $3   $4     $5
   | PROCEDURE begin '(' arglist ')'
          { $$ = $2; code3(call, (Inst)$1, (Inst)(long)$4); }
   | PRINT prlist  { $$ = $2; }
   | while cond stmt end {
           ($1)[1] = (Inst)$3;     /* cuerpo del ciclo */
           ($1)[2] = (Inst)$4; } /* fin, si la condición no se cumple */ 
   | if cond stmt end {    /* if sin else */
           ($1)[1] = (Inst)$3;     /* parte then */
           ($1)[3] = (Inst)$4; } /* fin, si la condición no se cumple */ 
   | if cond stmt end ELSE stmt end {      /* if con else */
           ($1)[1] = (Inst)$3;     /* parte then */
           ($1)[2] = (Inst)$6;     /* parte else */
           ($1)[3] = (Inst)$7; } /* fin, si la condición no se cumple */ 
   | '{' stmtlist '}'     { $$ = $2; }
   | for '(' forexpr ';' forexpr ';' forexpr ')' stmt end {
              ($1)[1] = (Inst)$3;  /* Inicialización */
              ($1)[2] = (Inst)$5;  /* Condición */
              ($1)[3] = (Inst)$7;  /* Incremento */
              ($1)[4] = (Inst)$9;  /* Cuerpo del ciclo */
              ($1)[5] = (Inst)$10; /* Dirección de salida */
     } 
   ;
cond: '(' expr ')'     {  code(STOP);   $$ =  $2;   }
   ;
for:    FOR { $$ = code(forcode); code(STOP); code(STOP); code(STOP); code(STOP); code(STOP); } 
   ;
forexpr:  expr { code(STOP); $$ = $1; } | asgn { code(STOP); $$ = $1; } 
   ;
while:  WHILE { $$ = code3(whilecode,STOP,STOP); }
   ;
if:     IF   { $$ = code(ifcode); code3(STOP,STOP,STOP); }
   ;
begin:  /* nada */          { $$ = progp; }
   ;
end:    /* nada */          { code(STOP); $$ = progp; }
   ;
stmtlist: /* nada */        { $$ = progp; }
   | stmtlist '\n' 
   | stmtlist stmt
   ;
expr:  NUMBER {   $$ = code2(constpush, (Inst)$1); }
   |   VAR    {   $$ = code3(varpush, (Inst)$1, eval); }
   |   ARG    { defnonly("$"); $$ = code2(arg, (Inst)(long)$1); }
   |   asgn
   | FUNCTION begin '(' arglist ')'
           { $$ = $2; code3(call, (Inst)$1, (Inst)(long)$4); }  
   | READ '(' VAR ')' { $$ = code2(varread, (Inst)$3); }  
   | BLTIN '(' expr ')' { $$=$3; code2(bltin, (Inst)$1->u.ptr); }  
   | '(' expr ')'  { $$ = $2; }
   | '|' expr '|'        { $$ = $2; code(magop); }
   | '[' vector_elements ']' { $$ = code2(buildvec, (Inst)(long)$2); }
   | expr '@' expr       { code(dotop); }
   | expr 'X' expr       { code(crossop); }
   | expr   '+'   expr {   code(add); }
   | expr   '-'   expr {   code(sub); }
   | expr   '*'   expr {   code(mul); }
   | expr   '/'   expr {   code(div_); }
   | expr   '^'   expr {   code(power); }
   | '-' expr  %prec UNARYMINUS   { $$=$2; code(negate); }
   | expr   GT	  expr	{ code(gt); }
   | expr   GE	  expr	{ code(ge); }
   | expr   LT	  expr	{ code(lt); }
   | expr   LE	  expr	{ code(le); }
   | expr   EQ	  expr	{ code(eq); } 
   | expr   NE	  expr	{ code(ne); }
   | expr  AND    expr  { code(and);}
   | expr   OR    expr  { code(or); }
   | NOT expr     { $$= $2; code(not); }
   ;
vector_elements: expr { $$ = 1; }
   | vector_elements ',' expr { $$ = $1 + 1; }
   ;
prlist: expr               {   code(prexpr); }
   |   STRING              {    $$ = code2(prstr, (Inst)$1); }
   |   prlist ','  expr    {   code(prexpr); }
   |   prlist ','  STRING  {   code2(prstr, (Inst)$3); }
   ;
      // 
     //     $1     $2                $3
defn:    FUNC procname { $2->type=FUNCTION; indef=1; }
//$4   $5  $6                   $7
  '(' ')' stmt { code(procret); define($2); indef=0; } 
     
    //  $1     $2                 $3
   | PROC procname { $2->type=PROCEDURE; indef=1; }
//$4  $5  $6
 '(' ')' stmt { code(procret); define($2); indef=0; }
   ;
procname: VAR
   | FUNCTION 
   | PROCEDURE
   ;
arglist:  /* nada */      { $$ = 0; }
   | expr                 { $$ = 1; }
   | arglist ',' expr     { $$ = $1 + 1; }
   ;
%%

/* fin de la gramática */ 
int indef;
#include <stdio.h> 
#include <ctype.h> 

char    *progname; 
int    lineno = 1 ; 

#include <signal.h> 
#include <setjmp.h> 
jmp_buf begin; 
int    indef;
char   *infile;       /* nombre de archivo de entrada */ 
FILE    *fin;         /* apuntador a archivo de entrada */
char   **gargv;       /* lista global de argumentos */ 
int    gargc; 
int c;  /* global, para uso de warning() */
char *emalloc(unsigned n);

int yylex(void){
while  ((c=getc(fin)) ==  ' ' ||  c ==   '\t')
          ;
if (c == EOF)
	return 0; 
if (c == '.' || isdigit(c)) {   /* número */
    double d;
    Vector *v;
    ungetc(c, fin);
    fscanf(fin, "%lf", &d);
    v = creaVector(1);
    v->vec[0] = d;
    yylval.sym = install("", NUMBER, v);
    return NUMBER;
}
if (isalpha(c) || c == '_') {//ID
	Symbol *s;
	char sbuf[100], *p = sbuf; 
	do {
		if (p >= sbuf + sizeof(sbuf) - 1) { 
			*p = '\0'; 
			execerror("name too long", sbuf);
		}
	*p++ = c;
        //putchar(c);
	} while ((c=getc(fin)) != EOF && (isalnum(c) || c == '_'));
	ungetc(c, fin); 
	*p = '\0'; 
    if (strcmp(sbuf, "X") == 0) return 'X';    
	if ((s=lookup(sbuf)) == 0)
       		s=install(sbuf, UNDEF, NULL); 
        //printf("sbuf = < %s > tipo=(%d)", sbuf, s->type);
	yylval.sym = s;
	return s->type == UNDEF ? VAR : s->type;
}
if (c == '$') { /* ¿argumento? */ 
	int n = 0; 
	while (isdigit(c=getc(fin)))
		n = 10 * n + c - '0'; 
	ungetc(c, fin); 
	if (n == 0)
		execerror("strange $...", (char *)0); 
	yylval.narg = n; 
	return ARG;
}
if (c == '"') { /* cadena entre comillas */ 
	char sbuf[100], *p;
	for (p = sbuf; (c=getc(fin)) != '"'; p++) { 
		if (c == '\n' || c == EOF)
			execerror("missing quote", ""); 
		if (p >= sbuf + sizeof(sbuf) - 1) { 
			*p = '\0';
			execerror("string too long", sbuf); 
		}
		*p = backslash(c); 
	}
	*p = 0;
	yylval.sym = (Symbol *)emalloc(strlen(sbuf)+1); 
	strcpy((char *)yylval.sym, sbuf); 
	return STRING;
}
	switch (c) {
	case '>':                return follow('=', GE, GT);
	case '<':                return follow('=', LE, LT);
	case '=':               putchar(c);putchar(c); putchar(c);
                                 return follow('=', EQ, '=');
	case '!':                return follow('=', NE, NOT);
	case '|':                return follow('|', OR, '|');
	case '&':                return follow('&', AND, '&');
	case '\n':              lineno++; return '\n';
	default:                  return c; 
        }
}
int backslash(int   c ){/*tomar siguiente carácter con las \ interpretadas   */
   static char transtab[] = "b\bf\fn\nr\rt\t";
   if (c != '\\')
	return c;
   c = getc(fin);
   if (islower(c) && strchr(transtab, c)) 
	return strchr(transtab, c)[1];
   return c; 
}
int follow(int expect, int ifyes, int ifno){ /* búsqueda hacia adelante para > -, etc. */
   int c = getc(fin);
   if (c == expect)
	return ifyes;
   ungetc(c, fin);
   return ifno; 
} 
void defnonly( char *s )     /* advertena la si hay definición i legal */
{
if (!indef)
	execerror(s, "used outside definition"); 
} 
void yyerror(char *s)      /* comunicar errores de tiempo de compilación */
{
warning(s, (char *)0); 
} 
void execerror(char *s, char *t) /* recuperación de errores de tiempo de ejecución */
{
warning(s, t);
fseek( fin, 0L, 2);       /* sacar el resto del archivo */
longjmp(begin, 0); 
}

void fpecatch()      /* detectar errores por punto flotante */ 
{
execerror("floating point exception", (char *) 0); 
}		

void init(void);
void initcode(void);
void define(Symbol *sp);
int main(int argc, char **argv){  /* hoc6 */ 
   int i;
   void fpecatch();
progname = argv[0];
if (argc == 1) {        /* simular una lista de argumentos */ 
	static char *stdinonly[] = { "-" };
	gargv = stdinonly;
	gargc = 1; } 
else {
	gargv = argv+1;
	gargc = argc-1; 
}
init(); 
while (moreinput())
	run(); 
}                                                                              
int moreinput( ) {
if (gargc-- <= 0)
	return 0; 
if (fin && fin != stdin)
fclose(fin); 
infile = *gargv++; 
printf("arch ent=(%s)\n",infile);
lineno = 1; 
if (strcmp(infile, "-") == 0) {
	fin = stdin;
	infile = 0; 
} else if ((fin=fopen(infile, "r")) == NULL) {
	fprintf(stderr, "%s: can't open %s\n" , progname, infile);
	return moreinput();
}
return 1;
}
void run()   /* ejecutar hasta el fin de archivo (EOF) */
{
setjmp(begin);
signal(SIGFPE,   fpecatch);
for   (initcode();   yyparse();   initcode()){
        //puts("ant execute");
	execute(progbase); 
 }
}
void warning(char *s, char *t)        /*   imprimir mensaje de advertencia   */
{
fprintf(stderr, "%s: %s", progname, s); 
if (t)
	fprintf(stderr, " %s", t); 
if (infile)
	fprintf(stderr, " in %s", infile); 
fprintf(stderr, " near line %d\n", lineno); 
while (c != '\n' && c != EOF)
c = getc(fin);  /* sacar el resto del renglón de entrada */
if (c == '\n')
	lineno++;
}











