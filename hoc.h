#ifndef _HOC_H_
#define _HOC_H_

typedef void (*Inst)(void);

/* 1. Definición nativa del tipo estructural Vector */
typedef struct Vector {
   int n;
   double *vec;
} Vector;

/* 2. Tabla de símbolos adaptada para objetos vectoriales */
typedef struct Symbol { /* entrada en la tabla de símbolos */
   char   *name;
   short  type;
   union {
      Vector  *val;              /* VAR */
      Vector  *(*ptr)(Vector *); /* BLTIN */
      Inst    *defn;             /* FUNCIÓN, PROCEDIMIENTO */
      char    *str;              /* CADENA */
   } u;
   struct Symbol   *next;  /* para ligar a otro */
} Symbol;

Symbol  *install(char *s, int t, Vector *d);
Symbol  *lookup(char *s);

/* 3. Pila de evaluación adaptada a 64 bits con soporte Vectorial */
typedef union Datum {   /* tipo de la pila del intérprete: */
   Vector *val;
   Symbol *sym; 
} Datum;

extern void pop1(void);
extern Datum pop(void);
extern void eval(void), add(void), sub(void), mul(void), div_(void), negate(void), power(void);

#define STOP   (Inst) 0

extern    Inst *progp, *progbase, prog[];
extern    Inst *code(Inst f);

extern    void assign(void), bltin(void), varpush(void), constpush(void), print(void), varread(void);
extern    void prexpr(void), prstr(void);
extern    void gt(void), lt(void), eq(void), ge(void), le(void), ne(void), and(void), or(void), not(void);
extern    void ifcode(void), whilecode(void), call(void), arg(void), argassign(void);
extern    void funcret(void), procret(void);

/* Operaciones de la máquina virtual exclusivas de la calculadora vectorial */
extern    void magop(void), buildvec(void), dotop(void), crossop(void), forcode(void);

void execerror(char *s, char *t);
int moreinput(void);
void execute(Inst *p);

#endif
