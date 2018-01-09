# Utilities für AProf

# sym #

sym:  sym.o p3rdsym.o pro.h
	mcppc -o sym -b sym.o p3rdsym.o -lamiga

sym.c: rcs/sym.c
   co sym.c

p3rdsym.c: /p3rdsym.c
   copy /p3rdsym.c ""

pro.h: /pro.h
   copy /pro.h "" 
   copy /p3text.h ""

sym.o: sym.c pro.h
   mcppc -c -b sym.c

p3rdsym.o: p3rdsym.c pro.h
   mcppc -c -b p3rdsym.c

