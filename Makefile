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
	tree.c \
	types.c

bin/b: $(SRCS)
	cc -o bin/b -g $(SRCS)

clean:
	rm -f bin/b *.o *.s out

test: bin/b tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test10: bin/b tests/input10
	bin/b tests/input10
	cc -o out out.s
	./out
