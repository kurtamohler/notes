all: test0

clean:
	rm test0.o test0

test0: test0.o
	cc -o test0 test0.o -L/usr/local/lib -lzlog -lpthread

test0.o: test0.c
	cc -c -o test0.o test0.c -I/usr/local/include
