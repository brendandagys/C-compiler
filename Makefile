bin/b: \
	code_generation_x86-64.c \
	expressions.c \
	code_generation.c \
	main.c \
	miscellaneous.c \
	scanner.c \
	statements.c \
	tree.c

	cc -o bin/b -g \
		code_generation_x86-64.c \
		expressions.c \
		code_generation.c \
		main.c \
		miscellaneous.c \
		scanner.c \
		statements.c \
		tree.c

clean:
	rm -f bin/b *.o *.s out

test: bin/b
	bin/b tests/input01
	cc -o out out.s
	./out
