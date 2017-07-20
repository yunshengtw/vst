CC = gcc
CFLAGS = -std=c99 -g -Wall -rdynamic -I./ -I../src -I./include
LDFLAGS = -ldl -T ld_script

vst-jasmine: ../src/vst.c ../src/main.c
	$(CC) $^ $(CFLAGS) $(LDFLAGS) -o $@

clean:
	rm vst-jasmine
