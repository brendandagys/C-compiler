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

ARM_SRCS= \
	code_generation_arm.c \
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

# COMPILE
bin/b: $(SRCS)
	cc -o bin/b -g $(SRCS)

bin/b-arm:: $(ARM_SRCS)
	cc -o bin/b-arm -g $(ARM_SRCS)

# ALL TESTS
test: bin/b tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

test-arm: bin/b-arm tests/runtests
	(cd tests; chmod +x runtests; ./runtests)

# LAST TEST
test14: bin/b tests/input14 lib/printint.c
	bin/b tests/input14
	cc -o out out.s lib/printint.c
	./out

test14-arm: bin/b-arm tests/input14 lib/printint.c
	bin/b tests/input14
	cc -o out out.s lib/printint.c
	./out

# CLEAN
clean:
	rm -f bin/b *.o *.s out
