CC = gcc
SRCS = ../src/vst.c ../src/vflash.c ../src/vram.c ../src/stat.c ../src/log.c ../src/checker.c
#CFLAGS = -std=c99 -g -Wall -rdynamic -I./ -I../src -I./include -DVST
CFLAGS = -std=c99 -g -O3 -Wall -rdynamic -I./ -I../src -I./include -DVST
LDFLAGS = -ldl -T ld_script

all: vst-jasmine vst-jasmine-dbg
.PHONY: all

vst-jasmine: $(SRCS)
	$(CC) $(CFLAGS) $^ $(LDFLAGS) -o $@

vst-jasmine-dbg: $(SRCS)
	$(CC) $(CFLAGS) -DDEBUG -DREPORT_WARNING $^ $(LDFLAGS) -o $@

clean:
	rm -f vst-jasmine vst-jasmine-dbg
.PHONY: clean

test: vst-jasmine ftl_greedy/ftl.so
	./vst-jasmine ../traces/hm_0-aligned16.trace ftl_greedy/ftl.so

ltest: vst-jasmine ftl_greedy/ftl.so
	./vst-jasmine ../traces/hm_0-aligned16.trace ftl_greedy/ftl.so -a

ftest: vst-jasmine ftl_faster_bug/ftl.so
	./vst-jasmine ../traces/hm_0-aligned16.trace ftl_faster_bug/ftl.so -a
.PHONY: test ltest ftest

# build FTL shared objects
FTL = greedy dac faster

ftl: $(addsuffix /ftl.so, $(addprefix ftl_, $(FTL)))
.PHONY: ftl

ftl_%/ftl.so: ftl_%
	make -C $(dir $@)
