CC = gcc
#CFLAGS = -std=c99 -g -Wall -rdynamic -I./ -I../src -I./include -DVST
CFLAGS = -std=c99 -g -O3 -Wall -rdynamic -I./ -I../src -I./include -DVST
LDFLAGS = -ldl -T ld_script

all: vst-jasmine vst-jasmine-dbg

vst-jasmine: ../src/vst.c ../src/main.c
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

vst-jasmine-dbg: ../src/vst.c ../src/main.c
	$(CC) $(CFLAGS) -DDEBUG -DREPORT_WARNING $^ $(LDFLAGS) -o $@

clean:
	rm vst-jasmine vst-jasmine-dbg
