build:
	gcc main.c -o a.out
build-debug:
	gcc -g main.c -o a.out
run:
	./a.out
start:
	make build && make run
start-debug:
	make build-debug && make run
