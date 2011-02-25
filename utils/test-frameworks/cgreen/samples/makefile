CC=gcc
CFLAGS=-g

sample: cgreen sample.o ../cgreen.h
	gcc sample.o ../cgreen.a -o sample

order: cgreen order.o ../cgreen.h
	gcc order.o ../cgreen.a  -o final

test: order sample
	./final
    ./sample

cgreen:
	cd ..; make 

