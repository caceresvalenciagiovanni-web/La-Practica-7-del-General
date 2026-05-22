#include "hoc.h"
#include "y.tab.h"
#include <stdio.h>
#include <stdlib.h>

/* Prototipos del motor vectorial */
extern Vector *creaVector(int n);
extern Vector *copiaVector(Vector *v);
extern Vector *sumaVector(Vector *v1, Vector *v2);
extern Vector *restaVector(Vector *v1, Vector *v2);
extern Vector *multiVector(Vector *v1, Vector *v2);
extern Vector *diviVector(Vector *v1, Vector *v2);
extern Vector *productoPunto(Vector *v1, Vector *v2);
extern Vector *productoCruz(Vector *v1, Vector *v2);
extern double magnitudVector(Vector *v);
extern void liberaVector(Vector *v);
extern void imprimeVector(Vector *v);
extern int esVerdadero(Vector *v);
extern Vector *mayorQueVector(Vector *v1, Vector *v2);
extern Vector *menorQueVector(Vector *v1, Vector *v2);
extern Vector *mayorIgualVector(Vector *v1, Vector *v2);
extern Vector *menorIgualVector(Vector *v1, Vector *v2);
extern Vector *igualVector(Vector *v1, Vector *v2);
extern Vector *diferenteVector(Vector *v1, Vector *v2);
extern Vector *andVector(Vector *v1, Vector *v2);
extern Vector *orVector(Vector *v1, Vector *v2);
extern Vector *notVector(Vector *v);

#define NSTACK 256
static Datum stack[NSTACK];  /* La pila de la máquina virtual */
static Datum *stackp;        /* Puntero al siguiente lugar libre en la pila */

#define NPROG 2000
Inst prog[NPROG];            /* Memoria RAM que almacena las instrucciones */
Inst *progp;                 /* Puntero al siguiente lugar libre para generación de código */
Inst *pc;                    /* Contador de programa durante la ejecución */

/* --- INICIALIZACIÓN Y CONTROL DE LA PILA --- */

void initcode() {      
    stackp = stack;
    progp = prog;
}

void push(Datum d) {   
    if (stackp >= &stack[NSTACK])
        execerror("Desbordamiento de la pila (stack overflow)", (char *) 0);
    *stackp++ = d;
}

Datum pop() {          
    if (stackp <= stack)
        execerror("Pila vacía (stack underflow)", (char *) 0);
    return *--stackp;
}

/* --- GESTIÓN DE MEMORIA --- */
void liberaVector(Vector *v) {
    if (v != NULL) {
        if (v->vec != NULL) free(v->vec);
        free(v);
    }
}

/* Sacar y liberar de la pila el elemento del tope (Silencioso, para limpiar) */
void pop1() {
    Datum d;
    d = pop();
    liberaVector(d.val);
}

/* --- INSTRUCCIONES DE ACCESO A DATOS --- */

void constpush() {     
    Datum d;
    Symbol *s = (Symbol *)*pc++;
    d.val = copiaVector(s->u.val); 
    push(d);
}

void varpush() {       
    Datum d;
    d.sym = (Symbol *)(*pc++);
    push(d);
}

void eval() {          
    Datum d;
    d = pop();
    if (d.sym->type == UNDEF)
        execerror("Variable no definida", d.sym->name); 
    d.val = copiaVector(d.sym->u.val); 
    push(d);
}

/* --- INSTRUCCIONES MATEMÁTICAS --- */

void add() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = sumaVector(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }
void sub() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = restaVector(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }
void mul() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = multiVector(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }
void divop() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = diviVector(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }
void dotop() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = productoPunto(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }
void crossop() { Datum d1, d2, d3; d2 = pop(); d1 = pop(); d3.val = productoCruz(d1.val, d2.val); liberaVector(d1.val); liberaVector(d2.val); push(d3); }

void magop() {         
    Datum d; double mag; Vector *res;
    d = pop(); 
    mag = magnitudVector(d.val); 
    liberaVector(d.val); 
    res = creaVector(1); res->vec[0] = mag; d.val = res;
    push(d); 
}

void buildvec() {
    int n = (int)(long)*pc++; 
    Datum d; int i, j, idx; int total_n = 0;
    Vector **temp = (Vector **)malloc(n * sizeof(Vector *));
    if (temp == NULL) execerror("Sin memoria en", "buildvec");
    
    for (i = n - 1; i >= 0; i--) temp[i] = pop().val;
    for (i = 0; i < n; i++) total_n += temp[i]->n;
    
    Vector *res = creaVector(total_n); idx = 0;
    for (i = 0; i < n; i++) {
        for (j = 0; j < temp[i]->n; j++) res->vec[idx++] = temp[i]->vec[j];
        liberaVector(temp[i]); 
    }
    free(temp); d.val = res; push(d);    
}

void negate() {        
    Datum d; Vector *temp; int i;
    d = pop(); temp = creaVector(d.val->n);
    for(i = 0; i < d.val->n; i++) temp->vec[i] = -(d.val->vec[i]);
    liberaVector(d.val); d.val = temp; push(d);
}

void power() { execerror("Operacion de potencia aun no soportada", (char *)0); }

/* --- INSTRUCCIONES LÓGICAS Y RELACIONALES (HOC5) --- */

/* Macro para ensamblar dinámicamente las instrucciones booleanas */
#define IMPLEMENTA_OP_LOGICA(nombre_instruccion, funcion_vectorial) \
void nombre_instruccion() { \
    Datum d1, d2, d3; \
    d2 = pop(); \
    d1 = pop(); \
    d3.val = funcion_vectorial(d1.val, d2.val); \
    liberaVector(d1.val); \
    liberaVector(d2.val); \
    push(d3); \
}

IMPLEMENTA_OP_LOGICA(gt, mayorQueVector)
IMPLEMENTA_OP_LOGICA(lt, menorQueVector)
IMPLEMENTA_OP_LOGICA(ge, mayorIgualVector)
IMPLEMENTA_OP_LOGICA(le, menorIgualVector)
IMPLEMENTA_OP_LOGICA(eq, igualVector)
IMPLEMENTA_OP_LOGICA(ne, diferenteVector)
IMPLEMENTA_OP_LOGICA(and, andVector)
IMPLEMENTA_OP_LOGICA(or, orVector)

void not() {
    Datum d, res;
    d = pop();
    res.val = notVector(d.val);
    liberaVector(d.val);
    push(res);
}

/* --- CONTROL DE FLUJO: IF Y WHILE CON GESTIÓN DE MEMORIA ESTRICTA --- */

void whilecode() {
    Datum d;
    Inst *savepc = pc;     /* savepc apunta al inicio del bloque del while */
    
    execute(savepc + 2);   /* Ejecuta la condición, la cual deja un vector en la pila */
    d = pop(); 
    
    while (esVerdadero(d.val)) {
        liberaVector(d.val);           /* ¡GARBAGE COLLECTION! Liberamos la condición antes de entrar al cuerpo */
        execute(*((Inst **)(savepc))); /* Ejecuta el cuerpo del ciclo */
        
        execute(savepc + 2);           /* Vuelve a evaluar la condición para la siguiente vuelta */
        d = pop(); 
    }
    liberaVector(d.val);               /* Liberar el último vector de condición (cuando se volvió falsa) */
    
    pc = *((Inst **)(savepc + 1));     /* Salta al código que sigue después del while */
}

void forcode() {
    Datum d;
    Inst *savepc = pc; /* savepc apunta a nuestro bloque de 5 punteros */
    
    /* 1. Ejecutar Inicialización (ej. i = 0) */
    execute(*((Inst **)(savepc))); 
    pop1(); /* ¡VITAL! Limpiamos el resultado de la asignación para no saturar la pila */
    
    /* 2. Evaluar Condición (ej. i < 10) */
    execute(*((Inst **)(savepc + 1))); 
    d = pop();
    
    while (esVerdadero(d.val)) {
        liberaVector(d.val); /* Garbage collection de la condición */
        
        /* 3. Ejecutar Cuerpo del ciclo */
        execute(*((Inst **)(savepc + 3))); 
        
        /* 4. Ejecutar Incremento (ej. i = i + 1) */
        execute(*((Inst **)(savepc + 2))); 
        pop1(); /* ¡VITAL! Limpiamos el resultado del incremento de la pila */
        
        /* 5. Volver a evaluar Condición */
        execute(*((Inst **)(savepc + 1))); 
        d = pop();
    }
    liberaVector(d.val); /* Liberar el último vector falso */
    
    /* 6. Terminar y saltar al exterior del ciclo */
    pc = *((Inst **)(savepc + 4)); 
}

void ifcode() {
    Datum d;
    Inst *savepc = pc; 
    
    execute(savepc + 3); /* Ejecuta la condición */
    d = pop(); 
    int es_verdad = esVerdadero(d.val); /* Determinamos si todo el vector es verdadero */
    liberaVector(d.val);                /* ¡GARBAGE COLLECTION INMEDIATO! */
    
    if (es_verdad) {
        execute(*((Inst **)(savepc)));  /* Ejecuta la parte THEN */
    } else if (*((Inst **)(savepc + 1))) {
        execute(*((Inst **)(savepc + 1))); /* Ejecuta la parte ELSE si existe */
    }
    pc = *((Inst **)(savepc + 2));      /* Salta al siguiente bloque de código exterior */
}

/* --- INSTRUCCIONES DE CONTROL Y FUNCIONES --- */

void assign() {        
    Datum d1, d2;
    d1 = pop(); /* Símbolo destino */
    d2 = pop(); /* Vector a asignar */
    
    if (d1.sym->type != VAR && d1.sym->type != UNDEF) 
        execerror("Asignacion a no-variable", d1.sym->name);
        
    if (d1.sym->type == VAR) liberaVector(d1.sym->u.val);
    
    d1.sym->u.val = copiaVector(d2.val);
    d1.sym->type = VAR;
    push(d2); 
} 

void print() {         
    Datum d;
    d = pop();
    imprimeVector(d.val);
    liberaVector(d.val);
}

void prexpr() {        /* Imprimir expresiones sin consumir salto de línea (usado en loops) */
    Datum d;
    d = pop();
    imprimeVector(d.val);
    liberaVector(d.val);
}

void bltin() {         
    Datum d; Vector *res;
    d = pop();
    res = (*(Vector *(*)(Vector *))(*pc++))(d.val);
    liberaVector(d.val);
    d.val = res;
    push(d);
}

/* --- MOTOR DE EJECUCIÓN --- */

Inst *code(Inst f) {   
    Inst *oprogp = progp;
    if (progp >= &prog[NPROG])
        execerror("Programa demasiado grande", (char *) 0);
    *progp++ = f;
    return oprogp;
}

void execute(Inst *p) { 
    for (pc = p; *pc != STOP; ) 
        (*pc++)();
}

/* =========================================================
   EXTENSION HOC6: SUBRUTINAS Y GESTION DINAMICA DE MEMORIA
   ========================================================= */
#include <stdlib.h>

/* Estructura de Contexto (Frame) para el manejo de funciones */
typedef struct Frame {
    Symbol *sp;
    Inst *retpc;
    Datum *argn;
    int nargs;
} Frame;

#define NFRAME 100
Frame frame[NFRAME];
Frame *fp; /* Apuntador al frame actual de la pila de llamadas */

void define(Symbol *sp) {
    sp->u.defn = progbase;
    progbase = progp;
}

void call(void) {
    Symbol *sp = (Symbol *)pc[0];
    if (fp++ >= &frame[NFRAME-1])
        execerror(sp->name, "call nested too deeply");
    fp->sp = sp;
    fp->nargs = (int)(long)pc[1];
    fp->retpc = (Inst *)(pc + 2);
    fp->argn = stackp - 1;
    pc = (Inst *)sp->u.defn;
}

void arg(void) {
    Datum d;
    int n = (int)(long)*pc++;
    if (n > fp->nargs)
        execerror(fp->sp->name, "not enough arguments");
    
    /* Extrae el vector original y crea una copia profunda para la pila */
    Vector *vector_original = fp->argn[n - fp->nargs].val;
    d.val = copiaVector(vector_original);
    push(d);
}

void argassign(void) {
    Datum d;
    int n = (int)(long)*pc++;
    if (n > fp->nargs)
        execerror(fp->sp->name, "not enough arguments");
    
    d = pop();
    /* Liberar el vector viejo antes de sobrescribirlo */
    Vector *vector_viejo = fp->argn[n - fp->nargs].val;
    free(vector_viejo->vec);
    free(vector_viejo);
    
    fp->argn[n - fp->nargs].val = copiaVector(d.val);
    push(d);
}

void funcret(void) {
    Datum d;
    if (fp->sp->type == PROCEDURE)
        execerror(fp->sp->name, "(proc) returns value");
    
    d = pop();
    pc = (Inst *)fp->retpc;
    
    /* BARRIDO: Destruir los argumentos del Frame */
    for (int i = 1; i <= fp->nargs; i++) {
        Vector *v = fp->argn[i - fp->nargs].val;
        free(v->vec);
        free(v);
    }
    
    stackp = fp->argn + 1 - fp->nargs;
    --fp;
    push(d);
}

void procret(void) {
    if (fp->sp->type == FUNCTION)
        execerror(fp->sp->name, "(func) returns no value");
    
    pc = (Inst *)fp->retpc;
    
    /* BARRIDO: Destruir los argumentos del Frame */
    for (int i = 1; i <= fp->nargs; i++) {
        Vector *v = fp->argn[i - fp->nargs].val;
        free(v->vec);
        free(v);
    }
    
    stackp = fp->argn + 1 - fp->nargs;
    --fp;
}
