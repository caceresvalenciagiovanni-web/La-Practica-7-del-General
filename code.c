#include "hoc.h"
#include "y.tab.h"
#include <stdio.h>
#include <stdlib.h>
#define NSTACK  256
extern Vector *creaVector(int n); /* Prototipo del motor vectorial */
static Datum stack[NSTACK];    /* la pila */
static Datum *stackp;          /* siguiente sitio libre en la pila */
#define NPROG  2000
Inst   prog[NPROG];   /* la Máquina */
Inst   *progp;        /* siguiente sitio libre para la generación de código */
Inst   *pc;              /* contador de programa durante la ejecución */
Inst   *progbase = prog; /* Inicie del subprograaa actual */
int    returning;      /* 1 si se ve la proposiolón de retorno */
typedef struct Frame {     /* nivel da pila par» Huida a proc/func */
Symbol  *sp;         /* entrada en la tabla da simbolos */
Inst    *retpc;   /* dónde reanudar después del retorno */
Datum  *argn;     /* n-esimo argumento en la pila */
int    nargs;     /* número da argumentos */
} Frame;
#define NFRAME  10000 
Frame  frame[NFRAME] ;
Frame   *fp;  /* apuntador a nivel */
void execute(Inst *p);
Vector *copy_vector(Vector *v) {
    Vector *nuevo = creaVector(v->n);
    for (int i = 0; i < v->n; i++) {
        nuevo->vec[i] = v->vec[i];
    }
    return nuevo;
}
void initcode() {
   progp = progbase;
   stackp = stack;
   fp = frame;
   returning = 0; 
}
void push(Datum d){
   if (stackp >= &stack[NSTACK])
      execerror("stack too deep", (char *)0); 
   *stackp++ = d;
}
Datum pop( ){
   //puts("pop");
   if (stackp == stack)
      execerror("stack underflow" , (char *)0);
   return *--stackp;
}
void pop1( ){
   //puts("pop1");
   if (stackp == stack)
      execerror("stack underflow" , (char *)0);
   --stackp;
}
void constpush(){
   Datum d;
   d.val  = ((Symbol *)*pc++)->u.val;
   //printf("dval = < %g >", d.val);
   push(d);
}
void varpush(){
   Datum d;
   d.sym =  (Symbol *)(*pc++);
   //puts("varpush");
   //puts(d.sym->name);
   push(d);
}
void whilecode(){
Datum d;
Inst *savepc = pc;
execute(savepc+2);      /* condición */ 
d = pop( ); 
while (d.val) {
execute(*((Inst **)(savepc)));  /* cuerpo */
if (returning) break;
execute(savepc+2);      /* condición */ 
d = pop();
} 
if (!returning)
pc = *((Inst **)(savepc+1)); /* siguiente proposición */ 
}
void ifcode() {
   Datum d;
   Inst *savepc = pc;      /* parte then */
   execute(savepc+3);     /* condición */ 
   d = pop (); 
   if (d.val)
      execute(*((Inst **)(savepc))); 
   else if (*((Inst **)(savepc+1))) /* ¿parte else? */
   execute(*((Inst **)(savepc+1))); 
   if (!returning)
   pc = *((Inst **)(savepc+2)); /* siguiente proposición */ 
} 
void define(Symbol *sp){
   sp->u.defn = progbase;    /* lnlolo de código */ 
   progbase = progp;      /* el siguiente código comienza aquí */
}
void call() {
Symbol  *sp  =   (Symbol *)pc[0];   /*   entrada en tabla da simbolos  */ 
                                    /*   para la función   */
if   (fp++   >=  &frame[NFRAME-1])
execerror(sp->name,   "call  nested too deeply");
   fp->sp = sp;
   fp->nargs =   (int)(long)pc[1];
   fp->retpc = pc  + 2;
   fp->argn  =  stackp  -   1;     /*   último argumento   */
   execute(sp->u.defn);
   returning = 0; 
} 
void ret( ) {
   int i;

   for (i = 0; i < fp->nargs; i++)
      pop();  /* sacar argunentos de la pila */
   pc = (Inst *)fp->retpc;
   --fp;
   returning = 1; 
}

void funcret(void) {
    Datum d;
    if (fp->sp->type == PROCEDURE)
        execerror(fp->sp->name, "(proc) returns value");
    
    d = pop(); /* Salvar el vector de retorno */
    pc = (Inst *)fp->retpc;
    
    /* BARRIDO DE MEMORIA: Destruir los argumentos del Frame */
    for (int i = 1; i <= fp->nargs; i++) {
        Vector *v = fp->argn[i - fp->nargs].val;
        free(v->vec);
        free(v);
    }
    
    stackp = fp->argn + 1 - fp->nargs; /* Limpiar la pila de la máquina */
    --fp; /* Destruir contexto */
    
    push(d); /* Colocar el vector de retorno en la pila limpia */
}

void procret(void) {
    if (fp->sp->type == FUNCTION)
        execerror(fp->sp->name, "(func) returns no value");
    
    pc = (Inst *)fp->retpc;
    
    /* BARRIDO DE MEMORIA: Destruir los argumentos del Frame */
    for (int i = 1; i <= fp->nargs; i++) {
        Vector *v = fp->argn[i - fp->nargs].val;
        free(v->vec);
        free(v);
    }
    
    stackp = fp->argn + 1 - fp->nargs;
    --fp;
}

double *getarg( ) {
int narg = (int)(long)*pc++;
if (narg > fp->nargs)
	execerror(fp-> sp->name, "not enough arguments");
return &fp->argn[narg - fp->nargs].val; 
}

void arg(void) {
    Datum d;
    int n = (int)(long)*pc++;
    if (n > fp->nargs)
        execerror(fp->sp->name, "not enough arguments");
    
    /* Extrae el vector original y crea una copia profunda para la pila */
    Vector *vector_original = fp->argn[n - fp->nargs].val;
    d.val = copy_vector(vector_original);
    
    push(d);
}

void argassign(void) {
    Datum d;
    int n = (int)(long)*pc++;
    if (n > fp->nargs)
        execerror(fp->sp->name, "not enough arguments");
    
    d = pop(); /* Obtiene el nuevo vector a asignar */
    
    /* 1. Liberar el vector viejo */
    Vector *vector_viejo = fp->argn[n - fp->nargs].val;
    free(vector_viejo->vec);
    free(vector_viejo);
    
    /* 2. Asignar una copia del nuevo vector */
    fp->argn[n - fp->nargs].val = copy_vector(d.val);
    
    push(d); /* Reinserta el valor, ya que la asignación es una expresión */
}

void bltin() {
Datum d;
d = pop();
d.val =  (*(double (*)(double))*pc++)(d.val);
push(d);
}

void eval(){
Datum d;
d = pop();
if (d.sym->type != VAR && d.sym->type != UNDEF)
execerror("attempt to evaluate non-variable", d.sym->name); 
if (d.sym->type == UNDEF)
execerror("undefined variable", d.sym->name); 
d.val = d.sym->u.val; 
push(d);
}

void add(){
Datum d1, d2; 
d2 = pop(); 
d1 = pop(); 
d1.val += d2.val; 
push(d1);
}

void sub(){
Datum d1,  d2; 
d2  = pop(); 
d1  = pop(); 
d1.val  -= d2.val; 
push(d1);
}

void mul(){
Datum d1, d2;
d2 = pop(); 
d1 = pop(); 
d1.val *= d2.val; 
push(d1);
}
void div_( ){
Datum d1, d2;
d2 = pop();
if (d2.val == 0.0)
execerror("division by zero", (char *)0);
d1 = pop(); 
d1.val /= d2.val; 
push(d1);
}
void negate(){
Datum d; 
d = pop(); 
d.val  =  -d.val; 
push(d);
}
void gt() {
Datum d1,  d2;
d2 = pop();
d1 = pop();
d1.val  =   (double)(d1.val  > d2.val);
push(d1);
}
void lt(){
Datum d1,  d2;
d2 = pop();
d1 = pop();
d1.val  =   (double)(d1.val  < d2.val);
push(d1);
}
void ge( ) {
Datum d1,  d2;
d2  = pop();
d1  = pop();
d1.val   =   (double)(d1.val  >= d2.val);
push(d1) ;
}
void le() {
Datum d1,  d2;
d2   =  pop();
d1   =  pop();
d1.val  =   (double)(d1.val   <=  d2.val);
push(d1);
}
void eq( ) {
Datum d1,  d2;
d2  = pop();
d1  = pop();
d1.val  =   (double) (d1.val  ==  d2.val);
push(d1);
}
void ne(){
Datum d1, d2;
d2 = pop();
d1 = pop();
d1.val = (double)(d1.val != d2.val);
push(d1);
}
void and(){
Datum d1,   d2;
d2   = pop();
d1   = pop();
d1.val = (double)(d1.val   !=   0.0 && d2.val   !=  0.0);
push(d1);
}
void or(){
Datum d1, d2;
d2 = pop();
d1 = pop();
d1.val = (double)(d1.val != 0.0 || d2.val != 0.0);
push(d1);
}
void not( ){
Datum d;
d = pop();
d.val = (double)(d.val == 0.0);
push(d);
}
void power(){
Datum d1, d2;
extern double Pow(double, double);
d2 = pop();
d1 = pop();
d1.val = Pow(d1.val, d2.val);
push(d1);
}
void assign(){
Datum d1, d2; 
//puts("**assign");

d1 = pop(); 
//puts(d1.sym->name);
d2 = pop();
//printf("d2.val = < %g >", d2.val);
if(d1.sym->type != VAR && d1.sym->type != UNDEF) 
execerror("assignment to non-variable",
d1.sym->name); 
d1.sym->u.val = d2.val; 
d1.sym->type = VAR; 
push(d2);
//puts("fin assign");
}
void print() {
Datum d;
d = pop();
printf("\t%.8g\n", d.val);
}
void prexpr() {      
   Datum d;
//puts("prexpr");
d = pop();
printf("%.8g ", d.val);
}
void prstr() {
/* Imprimir valor de la cadena */ printf("%s", (char *) *pc++);
}
void varread(){
Datum d;
extern FILE *fin; 
Symbol *var = (Symbol *) *pc++; 
Again:
switch (fscanf(fin, "%lf", &var->u.val)) { 
case EOF:
if (moreinput())
goto Again;
d.val = var->u.val = 0.0;
break; 
case 0:
execerror("non-number read into", var->name);
break; 
default:
d.val = 1.0;
break; 
}
var->type = VAR; 
push(d);
}
Inst *code(Inst f){
Inst *oprogp = progp;
if (progp >= &prog[NPROG])
execerror("program too big", (char *)0);
*progp++ = f;
return oprogp;
}
void execute(Inst *p){
for (pc = p; *pc != STOP && !returning; ) 
	((*pc++))();
 //puts("fin ejecucion");
}


//*pc = STOP o returning
