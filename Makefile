fns: fns.c
	gcc -o fns fns.c -w

run: run.c
	gcc -o run run.c -std=c99 -w
