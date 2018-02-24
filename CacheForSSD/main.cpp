#include <stdio.h>
#include <iostream>
#include <iostream>
#include <fstream>
#include <set>
#include <map>
#include <vector>
#include <cstdlib>
#include <cstdio>
#include <cmath>
#include <ctime>
#include <cassert>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/time.h>

#include "disksim_interface.h"

using namespace std;

typedef	struct
{
    int n;				//请求总数
    double sum;         //请求延迟时间的累加
} Stat;
static Stat st;

typedef	double SysTime;		/* system time in seconds.usec */

/* current time */
static SysTime now = 0;		
static SysTime subtime = 0;		/* current time */
/* next event */
static SysTime next_event = -1;	
/* last request was completed */
static int completed = 0;	

void
panic(const char *s)
{
    perror(s);
    exit(1);
}

void
syssim_schedule_callback(disksim_interface_callback_t fn,
                         SysTime t,
                         void *ctx)
{
    next_event = t;
//  printf("syssim_schedule_callback\n");
}


void
syssim_deschedule_callback(double t, void *ctx)
{
    next_event = -1;
    printf("syssim_schedule_callback\n");
}


void
syssim_report_completion(SysTime t, struct disksim_request *r, void *ctx)
{
    completed = 1;
    subtime = t;

    return;
}

void
print_statistics(Stat *s, const char *title)
{
    double avg;

    avg = s->sum/s->n;
    printf("%s: total IO request number=%d Avg.latency per request=%f\n ", title, s->n, avg);
}

int main(int argc, char*argv[])
{
	struct stat buf;
	struct disksim_interface* disksim;

	if (argc != 4)
	{
		fprintf(stderr, "usage: %s <param file> <output file> <tracefile>n", argv[0]);
		exit(1);
	}
	if (stat(argv[1], &buf) < 0)
		panic(argv[1]);

	disksim = disksim_interface_initialize(argv[1], argv[2],
		syssim_report_completion,
		syssim_schedule_callback,
		syssim_deschedule_callback, 0, 0, 0);

	FILE *tracefile = fopen(argv[3], "r");

	double time=now;
	int devno;
	int appID;
	unsigned long logical_block_number;

	int size;

	int isread;

	double senttime = 0.0;

	char line[201];

	fgets(line, 200, tracefile);

	while (!feof(tracefile)) {

		if (sscanf(line, "%lf %d %ld %d %d", &time, &devno, &logical_block_number, &size, &isread) != 5) 
		{
			fprintf(stderr, "Wrong number of arguments for I / O trace event typen");
			fprintf(stderr, "line: %s", line);
		}

		struct disksim_request* r = (struct disksim_request*)malloc(sizeof(struct disksim_request));
		r->start = time;
		r->flags = isread;
		r->devno = devno;
		r->blkno = logical_block_number;
		r->bytecount = size * 512;
		disksim_interface_request_arrive(disksim, time, r);
		fgets(line, 200, tracefile);
	}

	fclose(tracefile);

	while (next_event >= 0)
	{
		now = next_event;
		next_event = -1;
		disksim_interface_internal_event(disksim, now, 0);
	}

	disksim_interface_shutdown(disksim, now);
	print_statistics(&st, "response time");
    printf("total execution time = %f\n",now);
    //printf("total request number = %lld\n",version);
	//printf("total cross_request number = %lld\n",cross);
    //printf("total traffic size (block) = %lld\n",totalBlocks);
    //printf("Avg.latency(per request)= %f\n",(now/(double)version) );
    //printf("Avg.latency(per block)= %f\n",(now/(double)totalBlocks) );
    //printf("Bandwidth(MB/s)= %f\n",((double)totalBlocks/((double)now)*BLOCKSIZE/1024) );
    //printf("Hit ratio= %lf\n",(double)totalHits / (double)totalBlocks );


	
	exit(0);

}