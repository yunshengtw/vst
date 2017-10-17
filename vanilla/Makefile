CC = gcc
#CFLAGS = -std=c99 -g -Wall -rdynamic -I./ -I../src -I./include -DVST
CFLAGS = -std=c99 -g -O0 -Wall -rdynamic -I./ -I../src -I./include -DVST
LDFLAGS = -ldl -T ld_script

all: vst-vanilla vst-vanilla-dbg

.PHONY: all

vst-vanilla: ../src/vst.c ../src/main.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

vst-vanilla-dbg: ../src/vst.c ../src/main.c
	$(CC) $(CFLAGS) -DDEBUG -DREPORT_WARNING $^ $(LDFLAGS) -o $@

clean:
	rm vst-vanilla vst-vanilla-dbg

.PHONY: clean
