bin/b: code_generation.c code_generation_x86-64.c expressions.c interpreter.c main.c scanner.c tree.c
	gcc -o bin/b -g code_generation.c code_generation_x86-64.c expressions.c interpreter.c main.c scanner.c tree.c

clean:
	rm -f bin/b *.o *.s out

test: bin/b
	bin/b tests/input01
	gcc -o out out.s
	./out
