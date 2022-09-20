a.out: generate.c
	gcc generate.c -o a.out -g

run: a.out
	./a.out

debug: a.out
	gdb a.out
