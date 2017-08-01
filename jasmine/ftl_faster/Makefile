CC = gcc
#CFLAGS = -shared -std=c99 -g -fPIC -I./ -I../ -I../include -I../../src -DVST
CFLAGS = -shared -std=c99 -g -O0 -fPIC -I./ -I../ -I../include -I../../src -DVST

ftl.so: ftl.c shashtbl.c ../port.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf *.so

.PHONY: clean

