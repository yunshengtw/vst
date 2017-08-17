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

#define MAX_SIZE_TRACE 168638965

/* trace struct */
struct trace_ent {
    uint32_t lba, sec_num, rw;
};

static void print_ssd_config(void);
static void print_statistic(void);
static int load_trace(FILE *fp_trace, struct trace_ent *traces);

/* time spent */
time_t begin, end;
double time_spent;

/* statistic results */
extern uint64_t byte_write, byte_read;
extern uint64_t cnt_flash_read, cnt_flash_write, cnt_flash_cb, cnt_flash_erase;

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
	void (*ftl_open)(void);
	void (*ftl_read)(uint32_t const, uint32_t const);
	void (*ftl_write)(uint32_t const, uint32_t const);
	void (*ftl_flush)(void);
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

	ftl_open = (void (*)(void))dlsym(handle, "ftl_open");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_open.\n");
		return 1;
	}

	ftl_read = (void (*)(uint32_t const, uint32_t const))dlsym(
            handle, 
            "ftl_read");
	dl_err = dlerror();
	if (dl_err != NULL) {
		fprintf(stderr, "Fail resolving symbol ftl_read.\n");
		return 1;
	}

	ftl_write = (void (*)(uint32_t const, uint32_t const))dlsym(
            handle, 
            "ftl_write");
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

	printf("Trace file: %s\n", g_filename);

    print_ssd_config();
    atexit(print_statistic);

    init_vst();
	done = 0;
    traces = (struct trace_ent *)malloc(MAX_SIZE_TRACE * 
            sizeof(struct trace_ent));
    size_trace = load_trace(fp_trace, traces);
    // TODO: better begin/end points
	begin = clock();
	ftl_open();
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
				ftl_write(lba, sec_num);
				byte_write += (sec_num * VST_BYTES_PER_SECTOR);
				if (byte_write > bound) {
					done = 1;
					break;
				}
			}
			/* read */
			else {
                #ifdef DEBUG
                //printf("[DEBUG] R: %u/%u\n", lba, sec_num);
                #endif
				ftl_read(lba, sec_num);
				byte_read += (sec_num * VST_BYTES_PER_SECTOR);
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
	ftl_flush();
	end = clock();
	pass = 1;

	time_spent = (double)(end - begin);

    //print_statistic();

	return 0;
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

static void print_statistic(void)
{
    printf("----------Statistic Results----------\n");
    if (pass)
        printf("Pass!\n");
    else
        printf("Fail!\n");
    printf("Total read (MB): %" PRIu64 "\n", byte_read / (1024 * 1024));
    printf("Total write (MB): %" PRIu64 "\n", byte_write / (1024 * 1024));
    printf("Total flash read (pages): %" PRIu64 "\n", cnt_flash_read);
    printf("Total flash write (pages): %" PRIu64 "\n", cnt_flash_write);
    printf("Total flash copyback (pages): %" PRIu64 "\n", cnt_flash_cb);
    printf("Total flash erase (blocks): %" PRIu64 "\n", cnt_flash_erase);
    printf("----------Statistic Results----------\n");
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

