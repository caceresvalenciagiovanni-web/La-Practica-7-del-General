YFLAGS = -d
OBJS = hoc6.o code.o init.o math.o symbol.o vector_math.o

hoc: $(OBJS)
	gcc $(OBJS) -lm -o hoc
	./hoc

hoc6.o: hoc.h
code.o: hoc.h
init.o: hoc.h
math.o: hoc.h
symbol.o: hoc.h
vector_math.o: hoc.h

clean:
	@rm -f *.o *.out *.tab.* com hoc yacc.acts yacc.tmp
	@echo Clean
