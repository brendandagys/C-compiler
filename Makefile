SRCS= \
	code_generation_x86-64.c \
	declarations.c \
	expressions.c \
	code_generation.c \
	main.c \
	miscellaneous.c \
	scanner.c \
	statements.c \
	symbols.c \
	tree.c

bin/b: $(SRCS)
	cc -o bin/b -g $(SRCS)

clean:
	rm -f bin/b *.o *.s out

test: bin/b
	bin/b tests/input05
	cc -o out out.s
	./out

	bin/b tests/input02
	cc -o out out.s
	./out
