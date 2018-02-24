 #ifndef HYBRIDSS_H
#define HYBRIDSS_H

#include "disksim_interface.h"
#include "buflist.h"


using namespace std;

typedef	double SysTime;		/* system time in seconds.usec */

typedef struct request_t
{
    unsigned long start;
    unsigned long end;
    unsigned size;
    struct request_t *next;
    struct request_t *prev;
}Request;

struct requestList
{
	Request *head;
	Request *tail;
};

struct requestList readRequestList={NULL,NULL}; //, writeRequestList={NULL,NULL};

typedef struct wait_write_t
{
    unsigned long start;
    unsigned long end;
    Buf *head;
    Buf *tail;
} wWaitRequest;

map<unsigned long, wWaitRequest> wWaitRequestList;

BufList bufInvalid={NULL,NULL,0,NULL}, bufGlobal={NULL,NULL,0,NULL};;

#define HITPENALTY 0.000
#define BLOCKSIZE 4096

/* routines for translating between the system-level simulation's simulated */
/* time format (whatever it is) and disksim's simulated time format (a      */
/* double, representing the number of milliseconds from the simulation's    */
/* initialization).                                                         */

/* In this example, system time is in seconds since initialization */
#define SYSSIMTIME_TO_MS(syssimtime)    (syssimtime*1e3)
#define MS_TO_SYSSIMTIME(curtime)       (curtime/1e3)

/* routine for determining a read request */
#define	isread(r)	((r->type == 'R') || (r->type == 'r'))

/* exported by syssim_driver.c */
void syssim_schedule_callback(disksim_interface_callback_t, SysTime t, void *);
void syssim_report_completion(SysTime t, struct disksim_request *r, void *);
void syssim_deschedule_callback(double, void *);

#endif

