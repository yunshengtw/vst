CC = gcc
#CFLAGS = -shared -std=c99 -g -fPIC -I./ -I../ -I../include -I../../src -DVST
CFLAGS = -shared -std=c99 -g -O3 -fPIC -I./ -I../ -I../include -I../../src -DVST -Wno-int-to-pointer-cast -Wno-pointer-to-int-cast

ftl.so: ftl.c shashtbl.c ../port.c
	$(CC) $^ $(CFLAGS) -o $@

clean:
	rm -rf *.so

.PHONY: clean

