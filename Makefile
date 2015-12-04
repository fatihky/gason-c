all:
	gcc example.c -o example --std=c99 -lm -g
	gcc pretty-print.c -o pretty-print --std=c99 -lm -g

clean:
	rm -rf example
	rm -rf pretty-print
