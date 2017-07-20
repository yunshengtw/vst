/**
 * main.c
 * Authors: Yun-Sheng Chang
 */
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <inttypes.h>
#include <time.h>
#include <unistd.h>
#include <getopt.h>
#include <dlfcn.h>
#include "vst.h"
#include "config.h"

/* global variables */
double time_spent;
int trace_cnt;
uint64_t write_bytes, read_bytes;
int succeed;
char *g_filename;
time_t begin, end;
extern char *optarg;
extern int optind;

int main(int argc, char *argv[])
{
	FILE *fp_trace;
	double time;
	void *handle;
	char *dl_err;
	uint32_t rsv, lba, sec_num, rw;
	int opt;
	uint64_t bound;
	void (*ftl_open)(void);
	void (*ftl_read)(uint32_t const, uint32_t const);
	void (*ftl_write)(uint32_t const, uint32_t const);
	void (*ftl_flush)(void);
	int done;

	bound = 1;
	while ((opt = getopt(argc, argv, "ab:")) != -1) {
		switch (opt) {
		case 'a':
			bound = 1099511627776;
			break;
		case 'b':
			bound = atoll(optarg);
			break;
		default:
			fprintf(stderr, "Invalid option.\n");
			return 1;
		}
	}

	if (argc <= optind) {
		fprintf(stderr, "usage: ./vst trace_file ftl_obj\n");
		return 1;
	}
	
	fp_trace = fopen(argv[optind], "r");
	if (fp_trace == NULL) {
		fprintf(stderr, "Fail opening trace file.\n");
		return 1;
	}
	g_filename = argv[optind];

	handle = dlopen(argv[optind + 1], RTLD_LAZY);
	if (handle == NULL) {
		fprintf(stderr, "Fail opening ftl shared object.\n");
		return 1;
	}

	ftl_open = (void (*)(void))dlsym(handle, "ftl_open");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_open.\n");
		return 1;
	}

	ftl_read = (void (*)(uint32_t const, uint32_t const))dlsym(handle, "ftl_read");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_read.\n");
		return 1;
	}

	ftl_write = (void (*)(uint32_t const, uint32_t const))dlsym(handle, "ftl_write");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_write.\n");
		return 1;
	}

	ftl_flush = (void (*)(void))dlsym(handle, "ftl_flush");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_flush.\n");
		return 1;
	}

	//printf("Trace file: %s\n", argv[1]);

    init_vst();
	done = 0;
	begin = clock();
	ftl_open();
	while (!done) {
		//printf("write_bytes = %" PRIu64 "\n", write_bytes);
		fseek(fp_trace, 0, SEEK_SET);
		while (fscanf(fp_trace, "%lf %d %d %d %d", \
			&time, &rsv, &lba, &sec_num, &rw) != EOF) {
			lba += (trace_cnt * 1024); // offset
			if (lba >= (NUM_LPAGES * SECTORS_PER_PAGE))
				lba %= VST_SECTORS;
			if (lba + sec_num >= VST_NUM_SECTORS)
				sec_num = VST_NUM_SECTORS - lba - 1;
			/* write */
			if (rw == 0) {
				ftl_write(lba, sec_num);
				write_bytes += (sec_num * VST_BYTES_PER_SECTOR);
				if (write_bytes > bound) {
					done = 1;
					break;
				}
			}
			/* read */
			else {
				ftl_read(lba, sec_num);
				read_bytes += (sec_num * VST_BYTES_PER_SECTOR);
			}
		}
		trace_cnt++;
	}
	ftl_flush();
	end = clock();
	succeed = 1;
	fclose(fp_trace);

	time_spent = (double)(end - begin);
	return 0;
}

