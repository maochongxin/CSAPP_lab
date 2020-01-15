#include "cachelab.h"
#include <stdlib.h>
#include <unistd.h>
#include <limits.h>
#include <getopt.h>
#include <string.h>
#include <stdio.h>

// ./csim-ref [-hv] -s <s> -E <E> -b <b> -t <tracefile>
// struct cache_line cache[S][E];
// S = 2^s
int help, verbose, s, E, b, S, hits, miss, eviction; // command args && results

char filename[1024];
char buffer[1024];

typedef struct {
	int valid_bit,
	tag,
	stamp;	// lru time stamp
			// only if not using a queue
} cache_line; // cache line struct

cache_line** cache = NULL;

// address
// |---tag---|---set index---|---block offset---|
void Request(unsigned int address) {
	int max_stamp = INT_MIN, max_stamp_id = -1;

	int tag_value, set_address;
	set_address = (address >> b) & ((-1U) >> (32 - s));
	tag_value = address >> (s + b);

	for (int i = 0; i < E; ++i) {
		if (cache[set_address][i].tag == tag_value) {
			cache[set_address][i].stamp = 0; // update time stamp
			hits++;
			return;
		}
	}

	for (int i = 0; i < E; ++i) {
		if (cache[set_address][i].valid_bit == 0) {
			cache[set_address][i].valid_bit = 1;
			cache[set_address][i].tag = tag_value;
			cache[set_address][i].stamp = 0;
			miss++;
			return;
		}
	}

	eviction++;
	miss++;
	for (int i = 0; i < E; ++i) {
		if (cache[set_address][i].stamp > max_stamp) {
			max_stamp = cache[set_address][i].stamp;
			max_stamp_id = i;
		}
	}
	cache[set_address][max_stamp_id].tag = tag_value;
	cache[set_address][max_stamp_id].stamp = 0;

	return;
}

// update the time stamp of each cache line
void update_stamp() {
	for (int i = 0; i < S; ++i) {
		for (int j = 0; j < E; ++j) {
			if (cache[i][j].valid_bit == 1) {
				cache[i][j].stamp++;
			}
		}
	}
}

int main(int argc, char* argv[]) {
	unsigned int address;

	int opt;
	while(-1 != (opt = (getopt(argc, argv, "hvs:E:b:t:")))) {
		switch (opt) {
			case 'h':	help = 1;
						break;
			case 'v':	verbose = 1;
						break;
			case 's':	s = atoi(optarg);
						break;
			case 'E':	E = atoi(optarg);
						break;
			case 'b':	b = atoi(optarg);
						break;
			case 't':	strcpy(filename, optarg);
						break;
			default:	fprintf(stderr, "wrong argument\n");
						break;
		}
	}

	if (help == 1) {
		system("cat help_info");
		exit(0);
	}

	FILE* fp = fopen(filename, "r");
	if (fp == NULL) { // open file error
		fprintf(stderr, "Open file error!\n");
		exit(-1);
	}
	S = (1 << s);

	// Allocate space for the cache
	cache = (cache_line**)malloc(sizeof(cache_line*) * S);
	for (int i = 0; i < S; ++i) {
		cache[i] = (cache_line*)malloc(sizeof(cache_line) * E);
	}

	// initialize the cache
	for (int i = 0; i < S; ++i) {
		for (int j = 0; j < E; ++j) {
			cache[i][j].valid_bit = 0;
			cache[i][j].tag = cache[i][j].stamp = -1;
		}
	}


	char type;
	int temp;
	while (fgets(buffer, 1024, fp)) {
		sscanf(buffer, " %c %xu,%d", &type, &address, &temp);
		switch(type) {
			case 'L':	Request(address);
						break;
			case 'M':	Request(address);
			case 'S':	Request(address);
						break;
		}
		update_stamp();
	}

	for (int i = 0; i < S; ++i) {
		free(cache[i]);
	}
	free(cache);
	fclose(fp);

    printSummary(hits, miss, eviction);
    return 0;
}


