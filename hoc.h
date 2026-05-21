typedef void (*Inst)(void);

typedef struct Symbol { /* entrada en la tabla de símbolos */
   char   *name;
   short  type;
   union {
      double  val;           /* VAR */
      double  (*ptr)(double);      /* BLTIN */
      Inst    *defn;             /* FUNCIÓN, PROCEDIMIENTO */
      char    *str;         /* CADENA */
   } u;
   struct Symbol   *next;  /* para ligar a otro */
} Symbol;

Symbol  *install(char *s, int t, double d);
Symbol  *lookup(char *s);

typedef union Datum {   /* tipo de la pila del intérprete: */
   double val;
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
void execerror(char *s, char *t);
int moreinput(void);
void execute(Inst *p);
