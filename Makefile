Gram=y.tab.c y.tab.h

all: $(Gram)
	@gcc -o hoc y.tab.c symbol.c code.c init.c math.c -lm
	@echo Compiled

$(Gram): hoc6.y
	@yacc -d hoc6.y
clean:
	@rm -f *.out  *.tab.* com
	@echo Clean
