bin/b: expressions.c interpreter.c main.c scan.c tree.c
	gcc -o bin/b expressions.c interpreter.c main.c scan.c tree.c

clean:
	rm -f bin/b *.o
