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
#include "vflash.h"
#include "vbuffer.h"
#include "stat.h"
#include "log.h"
#include "config.h"

#define MAX_SIZE_TRACE 168638965

/* trace struct */
struct trace_ent {
    uint32_t lba, sec_num, rw;
};

static void print_ssd_config(void);
static int load_trace(FILE *fp_trace, struct trace_ent *traces);
static void init(void);
static void cleanup(void);

/* time spent */
time_t begin, end;
double time_spent;

/* misc */
int trace_cnt, req_id;
int pass = 0;
char *g_filename;

/* unix getopt */
extern char *optarg;
extern int optind;

int main(int argc, char *argv[])
{
    //TODO: make main conciser
	FILE *fp_trace;
	void *handle;
	char *dl_err;
	int opt;
	uint64_t bound;
	uint32_t lba, sec_num, rw;
    int size_trace;
	void (*vst_open_ftl)(void);
	void (*vst_read_sector)(uint32_t const, uint32_t const);
	void (*vst_write_sector)(uint32_t const, uint32_t const);
	void (*vst_flush_cache)(void);
	int done;
    struct trace_ent *traces;

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

	vst_open_ftl = (void (*)(void))dlsym(handle, "vst_open_ftl");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol vst_open_ftl.\n");
		return 1;
	}

	vst_read_sector = (void (*)(uint32_t const, uint32_t const))dlsym(
            handle, 
            "vst_read_sector");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol vst_read_sector.\n");
		return 1;
	}

	vst_write_sector = (void (*)(uint32_t const, uint32_t const))dlsym(
            handle, 
            "vst_write_sector");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol vst_write_sector.\n");
		return 1;
	}

	vst_flush_cache = (void (*)(void))dlsym(handle, "vst_flush_cache");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol vst_flush_cache.\n");
		return 1;
	}

	printf("Trace file: %s\n", g_filename);

    print_ssd_config();
    atexit(cleanup);

    init();
	done = 0;
    traces = (struct trace_ent *)malloc(MAX_SIZE_TRACE * 
            sizeof(struct trace_ent));
    size_trace = load_trace(fp_trace, traces);
    // TODO: better begin/end points
	begin = clock();
	vst_open_ftl();
	while (!done) {
        for (int i = 0; i < size_trace; i++) {
            lba = traces[i].lba;
            sec_num = traces[i].sec_num;
            rw = traces[i].rw;
			lba += (trace_cnt * 1024); // offset
			if (lba > VST_MAX_LBA) {
				lba %= (VST_MAX_LBA + 1);
                traces[i].lba = lba;
            }
			if (lba + sec_num > VST_MAX_LBA + 1)
				sec_num = VST_MAX_LBA + 1 - lba;
			/* write */
			if (rw == 0) {
                #ifdef DEBUG
                //printf("[DEBUG] W: %u/%u\n", lba, sec_num);
                #endif
				vst_write_sector(lba, sec_num);
				inc_byte_write(sec_num * VST_BYTES_PER_SECTOR);
				if (get_byte_write() > bound) {
					done = 1;
					break;
				}
			}
			/* read */
			else {
                #ifdef DEBUG
                //printf("[DEBUG] R: %u/%u\n", lba, sec_num);
                #endif
				vst_read_sector(lba, sec_num);
				inc_byte_read(sec_num * VST_BYTES_PER_SECTOR);
			}
            #ifdef DEBUG
            req_id = i;
            printf("req id = %d\n", req_id);
            #endif
		}
		trace_cnt++;
        #ifdef DEBUG
        printf("trace_cnt = %d\n", trace_cnt);
        #endif
	}
	vst_flush_cache();
	end = clock();
	pass = 1;

	time_spent = (double)(end - begin);

	return 0;
}

static void init(void)
{
    open_flash();
    open_buffer();
    open_stat();
    open_log("/tmp/vst.log");
}

static void cleanup(void)
{
    close_flash();
    close_buffer();
    close_stat();
    close_log();
}

static void print_ssd_config(void)
{
    printf("----------SSD Configuration----------\n");
    printf("# banks: %d\n", VST_NUM_BANKS);
    printf("# blocks: %d\n", VST_NUM_BLOCKS);
    printf("# pages: %d\n", VST_NUM_PAGES);
    printf("# sectors: %d\n", VST_NUM_SECTORS);
    printf("# blocks per bank: %d\n", VST_BLOCKS_PER_BANK);
    printf("# pages per block: %d\n", VST_PAGES_PER_BLOCK);
    printf("# sectors per page: %d\n", VST_SECTORS_PER_PAGE);
    printf("Max LBA: %d\n", VST_MAX_LBA);
    printf("Sector size: %d\n", VST_BYTES_PER_SECTOR);
    printf("DRAM base: 0x%x\n", VST_DRAM_BASE);
    printf("DRAM size: %d\n", VST_DRAM_SIZE);
    printf("----------SSD Configuration----------\n");
}

static int load_trace(FILE *fp_trace, struct trace_ent *traces)
{
	char buf[64];
	uint32_t rsv, lba, sec_num, rw;
    int n = 0;

    fseek(fp_trace, 0, SEEK_SET);
    while (fscanf(fp_trace, "%s %d %d %d %d", 
        buf, &rsv, &lba, &sec_num, &rw) != EOF &&
        n < MAX_SIZE_TRACE) {
        traces[n].lba = lba;
        traces[n].sec_num = sec_num;
        traces[n].rw = rw;
        n++;
    }
	fclose(fp_trace);
    return n;
}
