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
#include "hybridSS.h"
#include "application.h"
#include "seek.h"
#include <Python.h>

using namespace std;

#define	SECTORSIZE		512
#define	BLOCK2SECTOR	(BLOCKSIZE/SECTORSIZE)
#define MAX_REQ_BLK 	128
#define MAX_REQ_SIZE	(MAX_REQ_BLK * BLOCKSIZE)

//#define BUFSIZE 64000   //*4KB
#define PRINT_EXIT(s) { cout<<s<<endl; exit(1); }

//#define DEBUG_TIME 2

#ifdef DEBUG_TIME
ofstream funFile("debug-time.list");
#endif

//#define SSD_MAX_CAP  20000
#define DEVNUM 4
//#define StripeUnit 32    //单位同disksim，sector？
#define AdjustStep 0.1
//#define ProvisonRatio 0.2   //预取的最小区域
#define WindowsSize 10000
//#define MinLowBound 100
#define MaxReqSize 128    //防止disksim出错
#define func_num 9			//预测函数个数

//static struct disksim_interface *disksim;
static SysTime now = 0;		/* current time */
static SysTime subtime = 0;		/* current time */
static SysTime next_event = -1;	/* next event */
static int completed = 0;	/* last request was completed */

static unsigned long long totalBlocks = 0;
static unsigned long BUFSIZE;
static int StripeUnit;
static int MinLowBound;
static double ProvisonRatio;   //预取的最小区域
static unsigned long long totalHits = 0;
int counttt=0;
PyObject* pFunc=NULL;
PyObject classifier[2];
static int bio_num[DEVNUM];


class Partition;

map<int, Application> apps;    //id, application

class Partition
{
public:
    int id;
    int type;   //1, fast disk; 2, slow disk; 3, sdd
    long lowBound;
    unsigned long ranMin;
    unsigned long ranMax;
    unsigned long N;   //每个窗口的累计请求数
    unsigned long lastBlkno;
	SysTime lastdelay;
    double ranPG;
    double seqPG;
    double avgPerf;
    double lastPerf;
    double Tdisk;
    double Tseek;
    double Ttran;
    Buf* current;
    BufList bufPart;
    struct disksim_interface *disksim;
	int seq_n;
	int ran_n;
    //Partition( int id );
    Partition( int x, int tp, double t );
    void averageTime(double delay);
};

Partition::Partition( int x, int tp, double t )
{
    id = x;
    type = tp;
    lowBound = 0;
	N = 0;
    ranMin = ranMax = 0;
    ranPG = seqPG = 0;
    avgPerf = lastPerf = 0;
    Tdisk = Tseek = 0;
    Ttran = t;
    current = NULL;
    lastBlkno = 0;
    initBufList( &bufPart);
}
void Partition::averageTime(double delay)
{
    avgPerf = (avgPerf * N + delay) / (N + 1);
    N++;
    return;
}

//ofstream debug("debug.list");
/*
class Py{
public:
	PyObject* pMod;
    PyObject* pFunc;
	//PyObject* pArgs;
	//PyObject* pRetVal;
	Py();
};
Py::Py(){
	pMod = NULL;
	pFunc = NULL;
	//pArgs = NULL;
	//pRetVal = NULL;
}
Py pyargc;
*/


typedef	struct
{
    int n;				//请求总数
    double sum;         //请求延迟时间的累加
} Stat;
static Stat st;

vector<Partition> parts;

typedef struct sub_req_T
{
    unsigned long reqStart;
    unsigned long reqSize;
} SubReq;

void
panic(const char *s)
{
    perror(s);
    exit(1);
}

void
add_statistics(Stat *s, double x)
{
    s->n += 1;
    s->sum += x;
}


void
print_statistics(Stat *s, const char *title)
{
    double avg;

    avg = s->sum/s->n;
	printf("%s: total IO request number=%d Avg.latency per request=%f\n ", title, s->n, avg);

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


void doInit( char *initFile )
{
    int i, hot;
    long tlong;
    unsigned long blkAddr, taddr;
	char *function="predict2";
    Buf *pBuf;
	PyObject* pMod;
    string line;

    cout<<"cache size = "<<BUFSIZE<<endl;
    cout<<"Stripe Unit = "<<StripeUnit<<endl;
    cout<<"Min Low Bound = "<<MinLowBound<<endl;
    cout<<"Provison Ratio = "<<ProvisonRatio<<endl;

    initBufList( &bufInvalid);
	Py_Initialize();
	PyRun_SimpleString("import sys");
    PyRun_SimpleString("sys.path.append('./')");
	if (!(pMod = PyImport_ImportModule("myPredict")))
	{
			//return -1;
			cout<<"model can't be loaded"<<endl;
	}
	if (!(pFunc = PyObject_GetAttrString(pMod, function)))
        {
                //return -2;
                cout<<"function can't be loaded"<<endl;
        }
	classifier[0] = getModel(pMod,"exectrace_ibm_ts0_ch9.csv_cdei.csv20_train_model.m", "getClf");
	//"exectrace_ibm_ts0_ch9.csv_cedil.csv20_train_model.m", "getClf");
	classifier[1] = getModel(pMod,"exectrace_ts0disk_ch9_policy3_pre_s.csv_cdei.csv20_train_model.m", "getClf");
	//"exectrace_ts0disk_ch9_policy3_pre_s.csv_cedil.csv20_train_model.m", "getClf");
	classifier[2] = getModel(pMod,//"exectrace_ts0disk_ssd_policy3_pre_s.csv_cdei.csv20_train_model.m", "getClf");
	"exectrace_ts0disk_ssd_policy3_pre_s.csv_cedil.csv20_train_model.m", "getClf");
	
	
    for( i = 0; i < BUFSIZE; i++ )
    {
        pBuf = (Buf *)malloc(sizeof(Buf));
        pBuf->blkno = i;
        pBuf->counter = 0;
        pBuf->valid = 0;
        pBuf->dirty = 0;
        pBuf->nextW = NULL;
        insertToHead(pBuf,&bufInvalid);
    }
    tlong = ( ProvisonRatio * BUFSIZE ) / 4;
    for( i=0; i<DEVNUM; i++ )
    {
        parts[i].lowBound = tlong;
		parts[i].ran_n=0;
		parts[i].seq_n=0;
		parts[i].seqPG=0;
		parts[i].ranPG=0;
    }
    parts[0].lowBound += ProvisonRatio * BUFSIZE - tlong * 4;
}
void error( unsigned long blkno){
   printf("error blkno %ld\n",blkno);
}

void submitRequest( struct disksim_interface *disksim, struct disksim_request *r, SysTime startTime)
{
    completed = 0;
    next_event = -1;
    subtime = startTime;  //开始执行时间 --**
    //disksim_interface_request_arrive(disksim, now, r);
    disksim_interface_request_arrive(disksim, subtime, r);   //所有子请求同一个时间开始，然后取最大的结束时间作为最后的时间

    //cout<<"请求情况: start-time="<<r->start<<", start blkno="<<(r->blkno/BLOCK2SECTOR)<<", size="<<(r->bytecount/BLOCKSIZE)<<endl;
    //cout<<" 进入while(next_event >= 0)前"<<endl;
    while(next_event >= 0)
    {
        subtime = next_event;
        next_event = -1;
        disksim_interface_internal_event(disksim, subtime, 0);
        //cout<<"next_event循环一次"<<endl;
    }
    //cout<<" 进入while(next_event >= 0)后"<<endl;
    if (!completed)
    {
        fprintf(stderr, "internal error. Last event not completed \n");
        exit(1);
    }
}

void hybridRead( vector<SubReq> (&ioreqs)[DEVNUM] )
{
    int i, j;
    SubReq *psubreq;
    //double timeStart, penalty;
    struct disksim_request r;
    SysTime maxtime, timeStart, penalty;   //maxtime,子请求中最晚完成的时间

    timeStart = maxtime = now;    //一起发送，等待全部完成才算完成，并返回
    for( i=0; i<DEVNUM; i++ )
    {
		if (ioreqs[i].size() == 0) continue;
        subtime = timeStart;
        for(j=0; j<ioreqs[i].size(); j++)
        {
            psubreq = &ioreqs[i][j];
            while( psubreq->reqSize > MaxReqSize )
            {
                
                r.start = subtime;
                r.blkno = psubreq->reqStart * BLOCK2SECTOR;
                r.flags = DISKSIM_READ;
                r.bytecount = MaxReqSize * BLOCKSIZE;
                r.devno = 0;
                submitRequest( parts[i].disksim, &r, subtime );
                psubreq->reqStart += MaxReqSize;
                psubreq->reqSize -= MaxReqSize;
				
            }
            r.start = subtime;
            r.blkno = psubreq->reqStart * BLOCK2SECTOR;
            r.flags = DISKSIM_READ;
            r.bytecount = psubreq->reqSize * BLOCKSIZE;
            r.devno = 0;
            submitRequest( parts[i].disksim, &r, subtime );
        }
		parts[i].lastdelay = subtime - timeStart;
		//parts[i].averageTime( subtime-timeStart);
        if( subtime > maxtime ) maxtime = subtime;
    }
    now = maxtime;
    //penalty = now - timeStart;
    //add_statistics( &st, penalty );

    return;
}
void hybridWrite( vector<SubReq> (&ioreqs)[DEVNUM] )
{
    int i, j;
    SubReq *psubreq;
    //double timeStart, penalty;
    struct disksim_request r;
    SysTime maxtime, timeStart, penalty;   //maxtime,子请求中最晚完成的时间

    timeStart = maxtime = now;    //一起发送，等待全部完成才算完成，并返回
    for( i=0; i<DEVNUM; i++ )
    {
        subtime = timeStart;
		if(ioreqs[i].size()==0) continue;
        for(j=0; j<ioreqs[i].size(); j++)
        {
            psubreq = &ioreqs[i][j];
            while( psubreq->reqSize > MaxReqSize )
            {
                r.start = subtime;
                r.blkno = psubreq->reqStart * BLOCK2SECTOR;
                r.flags = DISKSIM_WRITE;
                r.bytecount = MaxReqSize * BLOCKSIZE;
                r.devno = 0;
				
				if(r.blkno==92159040){
						error(psubreq->reqStart);
						exit(1);
				 }
                submitRequest( parts[i].disksim, &r, subtime );
                psubreq->reqStart += MaxReqSize;
                psubreq->reqSize -= MaxReqSize;
            }
            r.start = subtime;
            r.blkno = psubreq->reqStart * BLOCK2SECTOR;
            r.flags = DISKSIM_WRITE;
            r.bytecount = psubreq->reqSize * BLOCKSIZE;
            r.devno = 0;
			if(r.blkno==92159040){
						error(psubreq->reqStart);
						exit(1);
				 }
            submitRequest( parts[i].disksim, &r, subtime );
        }
		parts[i].lastdelay = subtime - timeStart;
		//parts[i].averageTime( subtime-timeStart);
        if( subtime > maxtime ) maxtime = subtime;
    }
    now = maxtime;
   penalty = now - timeStart;
   //add_statistics( &st, penalty );
    return;
}

/*
void hybridRead( unsigned long addr, unsigned size )
{
    unsigned long ioaddr;
    //double timeStart, penalty;
    struct disksim_request r;
    vector<SubReq> sreqs; //sub-reqeusts
    SysTime maxtime, timeStart, penalty;   //maxtime,子请求中最晚完成的时间

    timeStart = maxtime = now;    //一起发送，等待全部完成才算完成，并返回
    sreqs = lbnMapping( addr, size );
    for( i=0; i<sreqs.size(); i++ )
    {
        r.start = timeStart;
        r.blkno = sreqs[i].start * BLOCK2SECTOR;
        r.flags = DISKSIM_READ;
        r.bytecount = sreqs[i].size * BLOCKSIZE;
        r.devno = 0;

        //printf("submit request blkno = %u, size = %d, type = R\n", r.blkno, r.bytecount/BLOCKSIZE);
        submitRequest( sreqs[i].disksim, &r, timeStart );
        if( subtime > maxtime ) maxtime = subtime;   //结束执行时间 --**
    }
    now = maxtime;
    //penalty = now - timeStart;
    //add_statistics( &st, penalty );

    return;
}
*/
/*void hybridWrite( unsigned long addr, unsigned size )
{
    cout<<"error: is not support write now"<<endl;
    exit(2);
}

void hybridWrite( unsigned long addr, unsigned size )
{
	int uno, cno, dno;
	unsigned long blkno,reqStart, fd; 

    unsigned long ioaddr;
    double timeStart, penalty;
    struct disksim_request r;
	
    subtime = now;    //一起发送，等待全部完成才算完成，并返回
    blkno= addr;
     uno = (blkno - 1) / StripeUnit;
    cno = uno / DEVNUM;
    dno = uno % DEVNUM;
    fd = blkno - uno * StripeUnit;
    reqStart = cno * StripeUnit + fd;
     
        r.start = subtime;
        r.blkno = reqStart * BLOCK2SECTOR;
        r.flags = DISKSIM_WRITE;
        r.bytecount = BLOCKSIZE;
        r.devno = 0;

        //printf("submit request blkno = %u, size = %d, type = R\n", r.blkno, r.bytecount/BLOCKSIZE);
    submitRequest( parts[dno].disksim, &r, subtime );
    now = subtime;
    //penalty = now - timeStart;
    //add_statistics( &st, penalty );

    return;
}
*/

void merge_request( struct requestList *list, unsigned long start, int size )
{
    Request *tmp = list->head, *r;
    unsigned long  end = start + size -1;

    while( tmp != NULL )
    {
        if( start <= tmp->start )
        {
            do             //改成do while
            {
                if( (end + 1) < (tmp->start) )
                {
                    r = (Request *)malloc( sizeof(Request) );
                    r->start = start;
                    r->size = size;
                    r->end = end;
                    r->prev = tmp->prev;
                    r->next = tmp;
                    if(tmp != list->head) tmp->prev->next = r;
                    else list->head = r;
                    tmp->prev = r;
                    break;
                }
                else if( (tmp->next == NULL ) || (end <= tmp->end +1) )
                {
                    tmp->start = ( tmp->start < start ) ? tmp->start : start;
                    tmp->end = ( tmp->end > end ) ? tmp->end : end;
                    tmp->size = tmp->end - tmp->start +1;
                    break;
                }
                tmp->next->prev = tmp->prev;
                if(tmp != list->head) tmp->prev->next = tmp->next;
                else list->head = tmp->next;
                tmp = tmp->next;
            }
            while( tmp != NULL );
            break;
        }
        else if( (start <= tmp->end + 1) )
        {
            start = tmp->start;
            do
            {
                if( end <= tmp->end + 1 )
                {
                    tmp->start = ( tmp->start < start ) ? tmp->start : start;
                    tmp->end = ( tmp->end > end ) ? tmp->end : end;
                    tmp->size = tmp->end - tmp->start +1;
                    break;
                }
                else if( (tmp->next == NULL ) || (end < (tmp->next->start - 1)) )
                {
                    tmp->start = ( tmp->start < start ) ? tmp->start : start;
                    tmp->end = ( tmp->end > end ) ? tmp->end : end;;
                    tmp->size = tmp->end - tmp->start +1;
                    break;
                }
                tmp->next->prev = tmp->prev;
                if(tmp != list->head) tmp->prev->next = tmp->next;
                else list->head = tmp->next;
                tmp = tmp->next;
            }
            while( tmp != NULL );
            break;
        }
        tmp = tmp->next;
    }
    if( list->head == NULL )
    {
        list->head = ( Request *)malloc( sizeof(Request) );
        list->head->start = start;
        list->head->size = size;
        list->head->end = end;
        list->head->next = NULL;
        list->head->prev = NULL;
        list->tail = list->head;
    }
    else if( tmp == NULL )
    {
        tmp = ( Request *)malloc( sizeof(Request) );
        tmp->start = start;
        tmp->size = size;
        tmp->end = end;
        tmp->next = NULL;
        tmp->prev = list->tail;
        list->tail->next = tmp;
        list->tail = tmp;
    }
    return;
}

void insert_wWaitList( Buf *pBuf )
{
    int done = 0;
    unsigned long Blkno = pBuf->blkno;
    wWaitRequest temp;
    map<unsigned long, wWaitRequest>::iterator it, nextIt;

    for( it = wWaitRequestList.begin(); it != wWaitRequestList.end(); it++ )
    {
        if( (Blkno + 1) < (it->first) )    //因为first刚好是0，所以将所有的请求都作为新的插入
        {
            temp.start = temp.end = Blkno;
            temp.head = temp.tail = pBuf;
            wWaitRequestList.insert( make_pair(Blkno, temp) );
            break;
        }
        else
        {
            if( (Blkno + 1) == (it->first) )
            {
                pBuf->nextW = it->second.head;
                it->second.head = pBuf;        //insert
                it->second.start = Blkno;
                wWaitRequestList.insert( make_pair(Blkno, it->second) );      //新item
                wWaitRequestList.erase( it->first );        //删除旧iterm
                break;
            }
            else if( Blkno < (it->second.end + 1) )
            {
                //PRINT_EXIT("error@insert_wWaitList, 重复写，block的dirty的逻辑出错");
                break;
            }
            else if( Blkno == (it->second.end + 1) )
            {
                done = 0;
                if( it != wWaitRequestList.end() )
                {
                    nextIt = it;
                    nextIt++;
                    //cout<<"next's first block = "<<extIt->first<<endl;
                    if( (Blkno + 1) == (nextIt->first) )    //可以合并
                    {
                        it->second.end = nextIt->second.end;
                        it->second.tail->nextW = pBuf;
                        pBuf->nextW = nextIt->second.head;
                        it->second.tail = nextIt->second.tail;
                        wWaitRequestList.erase( nextIt->first );
                        done = 1;
                    }
                }
                //没有合并, 如果可以合并就跳出循环了
                if( done == 0 )
                {
                    it->second.tail->nextW = pBuf;
                    it->second.tail = pBuf;
                    it->second.end = Blkno;
                }
                break;
            }
        }
    }
    if( it == wWaitRequestList.end() )    //为空,或者最后面
    {
        temp.start = temp.end = Blkno;
        temp.head = temp.tail = pBuf;
        wWaitRequestList.insert( make_pair(Blkno, temp) );
    }

    return;
}
/*
void do_write( unsigned long blkno)
{
    cout<<"error: not support wirte now"<<endl;
    exit(1);
}
*/
	int blockMapping( unsigned long blkno )
	{
		int uno, dno;	//uno: strip序号（从0开始），cno：行号（从0开始），dno：设备序号（从0开始）
	
		uno = (blkno - 1) / StripeUnit;
		dno = uno % DEVNUM;
	
		return dno;
	}
void requestMapping( unsigned long blkno, long blknum, SubReq (&subReqs)[DEVNUM]);

void do_write( unsigned long blkno)
{
    int i,j,size;
    unsigned long addr,tul;
	int devno;
    double timeTemp, penalty;
    Buf *pBuf;
    struct disksim_request r;
	SubReq  *psubreq;
	SubReq subReqs[DEVNUM];
    vector<SubReq> ioreqs[DEVNUM];
    map<unsigned long, wWaitRequest>::iterator it = wWaitRequestList.begin();
	devno = blockMapping( blkno );

    for( ; it != wWaitRequestList.end(); it++)
    {
        if( blkno < it->first )
        {
            //error(blkno);
            cout<<"error@do_write(), cann't find blkno in the write requests list"<<endl;
			//error(blkno);
			
            exit(1);
        }
        else if( blkno <= it->second.end )
        {
            timeTemp = now;

            pBuf = it->second.head;
            while( pBuf != NULL )
            {
                pBuf->dirty = 0;
                pBuf = pBuf->nextW;
            }
            size = ( it->second.end - it->first + 1);
            addr = it->first;
			requestMapping( addr, size, subReqs);
            for( i=0; i<DEVNUM; i++)
            {
                if(subReqs[i].reqStart != 0)
                {
                    j = ioreqs[i].size();
                    if( j == 0 )
                    {
                        ioreqs[i].push_back(subReqs[i]);
                    }
                    else
                    {
                        psubreq = &ioreqs[i][j-1];
						
                        tul = psubreq->reqStart + psubreq->reqSize - 1;
                        if( subReqs[i].reqStart == tul ) psubreq->reqSize += subReqs[i].reqSize;
                        else  ioreqs[i].push_back(subReqs[i]);
                    }
					
                }
            }
			/*
            while( size > 1024 )
            {
                hybridWrite( addr, 1024);
                size -= 1024;
                addr += 1024;
            }*/
            hybridWrite(ioreqs);

            penalty = now - timeTemp;
            add_statistics( &st, penalty );
			//parts[devno].averageTime( penalty );

            wWaitRequestList.erase( it->first );
            break;
        }
    }
    if( it == wWaitRequestList.end() ) {
		//error(blkno);
		printf( "liyong, error : do_write(), cann't find blkno in the write requests list");
        exit(1);
	}

    return;
}


void requestMapping( unsigned long blkno, long blknum, SubReq (&subReqs)[DEVNUM])
{
    int uno, cno, dno, i;   //uno: strip序号（从0开始），cno：行号（从0开始），dno：设备序号（从0开始）
    unsigned long fd;    //第一个strip内部的开始地址 former-distance
    long totalSize, tsize;            //totalSize表示请求大小
    //SubReq subReqs[DEVNUM];

    //初始化，start=0表示没有该子请求
    assert( blknum > 0);
    for(i=0; i<DEVNUM; i++)
    {
        subReqs[i].reqStart = subReqs[i].reqSize = 0;    //为避免disksim错误，请求地址从1开始
    }

    uno = (blkno - 1) / StripeUnit;
    cno = uno / DEVNUM;
    dno = uno % DEVNUM;
    fd = blkno - uno * StripeUnit;
    subReqs[dno].reqStart = cno * StripeUnit + fd;
    totalSize = StripeUnit - fd + 1;        //totalSize在使用前作一次临时变量，第一个请求的最大请求大小
    if( totalSize >= blknum )                //长度不足一个
    {
        subReqs[dno].reqSize = blknum;
        totalSize = -1;   //直接结束
    }
    else
    {
        subReqs[dno].reqSize = totalSize;
        totalSize = blknum - subReqs[dno].reqSize;
    }
    while( totalSize > 0 )
    {
        dno++;
        assert(dno <= DEVNUM);
        if( dno == DEVNUM )
        {
            dno = 0;
            cno++;
        }
        if( subReqs[dno].reqStart == 0 )   //开始地址
        {
            subReqs[dno].reqStart = cno * StripeUnit + 1;
        }
        if( totalSize >= StripeUnit )   //剩余的请求量大于个strip；子请求大小-逐步累加
        {
            subReqs[dno].reqSize += StripeUnit;
            totalSize -= StripeUnit;
        }
        else
        {
            subReqs[dno].reqSize += totalSize;
            totalSize = -1;
        }
    }

    return;
}

//子请求在各个存储设备中的Tdisk, Tseek, Ttran, 要先于computePG()进行
void prediction( SubReq (&reqs)[DEVNUM] , int reqType)
{
    int i;
    unsigned long blkDist;
	vector <double> vec[DEVNUM];
	
    for( i=0; i< DEVNUM; i++ )
    {
		if( reqs[i].reqStart == 0 ) continue;
		vec[i].push_back(reqs[i].reqStart);	//起始地址
		vec[i].push_back(reqs[i].reqSize);	//请求块大小
		blkDist = (reqs[i].reqStart - parts[i].lastBlkno);
		parts[i].lastBlkno = reqs[i].reqStart + reqs[i].reqSize - 1;
		vec[i].push_back(blkDist);			//寻道距离
		vec[i].push_back(reqType);			//读写类型
		// vec[i].push_back(parts[i].lastdelay);	//上次io的访问延迟
		switch( parts[i].type )
		{
		case 1:
			parts[i].Tdisk= slowHddSeek(pFunc,classifier[0],vec[i]);
			break;
		case 2:
			parts[i].Tdisk= slowHddSeek(pFunc,classifier[1],vec[i]);
			break;
		case 3:
			parts[i].Tdisk= SsdSeek(pFunc,classifier[2],vec[i]);
			break;
		default:
			cout<<"error: no this type of devices"<<endl;
			exit(1);
        }
		//printf("%f\n",parts[i].Tdisk);
		parts[i].Tdisk/=reqs[i].reqSize;
    }
}

//在请求被处理前，完成各个子设备的PG计算。一方面更加合理，因为缓存分配使得PG1 = PG2 = ...。并且partition记录最近请求的PG（分类别保存）
void computePG( SubReq (&reqs)[DEVNUM], unsigned long prefetch, long reqBlkNum, int appType )
{
    int i;
    unsigned long totalSize;
    double td,tempPG;

    //TODO PG应该是double
    //顺序访问在预取缓存中命中的不再重复计算
    for( i=0; i < DEVNUM; i++ )
    {
        if( reqs[i].reqStart == 0 ) continue;
        if( appType == 0 )
        {
            //totalSize充当临时变量
            totalSize = reqs[i].reqStart + reqs[i].reqSize - 1;
            if( reqs[i].reqStart < parts[i].ranMin ) parts[i].ranMin = reqs[i].reqStart;
            if( totalSize > parts[i].ranMax ) parts[i].ranMax = totalSize;
            //if cache size > Ran.max - min, 说明可以保存全部，就不会发生miss了。
            if( parts[i].bufPart.blkNum >= (parts[i].ranMax - parts[i].ranMin) ){
				td = 1; 
				return;
            }
            else{
				td = (double)parts[i].bufPart.blkNum / (double)(parts[i].ranMax - parts[i].ranMin);
            } 
			tempPG = ( 1 - td ) * parts[i].Tdisk;
			parts[i].ranPG=(parts[i].ranPG*parts[i].ran_n+tempPG)/(parts[i].ran_n+1);
			parts[i].ran_n+=1;
        }
        else
        {
            //prefetch = prefetchEnd, 非0表示执行过预取
				tempPG= ((parts[i].Tdisk ) * reqBlkNum ) / (reqs[i].reqSize * DEVNUM);
            
			parts[i].seqPG=(parts[i].seqPG*parts[i].seq_n+tempPG)/(parts[i].seq_n+1);
			parts[i].seq_n+=1;

		}
    }

    return;
}

int victimSelection( int pid, int apptype )
{
    int i, index = pid;
    double minPG = parts[pid].ranPG/parts[pid].bufPart.blkNum;

    assert( pid >= 0 );
    assert( pid < DEVNUM );
    if( apptype == 0 )
    {
        for( i=0; i < DEVNUM; i++ )
        {
            if( parts[i].bufPart.blkNum <= parts[i].lowBound ) continue;
            if( parts[i].ranPG/parts[i].bufPart.blkNum< minPG )
            {
                index = i;
                minPG = parts[i].ranPG/parts[i].bufPart.blkNum;
            }
        }
    }
    else
    {
        for( i=0; i < DEVNUM; i++ )
        {
            if( parts[i].bufPart.blkNum <= parts[i].lowBound ) continue;
            if( parts[i].seqPG/parts[i].bufPart.blkNum < minPG )
            {
                index = i;
                minPG = parts[i].seqPG/parts[i].bufPart.blkNum;
            }
        }
    }

    //if( index == -1 ) index = pid;
    return index;
}

/*void adjustment()
{
    int i;
    long totalSize, tl;
    double delta[DEVNUM], all, tdou;
    vector<int> custom;
    vector<int> supply;

    all = 0; //后面计算的分母
    for( i=0; i < DEVNUM; i++ )
    {
        delta[i] = fabs(parts[i].avgPerf - parts[i].lastPerf);
        if( parts[i].avgPerf < parts[i].lastPerf )
        {
            custom.push_back( i );
            all += delta[i];
        }
        else supply.push_back( i );
        //pats[i].{avg,last}Perf后面用不到了
        parts[i].lastPerf = parts[i].avgPerf;
        //parts[i].avgPerf = 0;
        //parts[i].N = 0;
    }
    totalSize = 0;  //计算该周期的总的调整量
    for( i=0; i < supply.size(); i++ )
    {
        if( parts[supply[i]].lowBound <= MinLowBound ) continue;
        tl = parts[supply[i]].lowBound * AdjustStep;
        totalSize += tl;
        parts[supply[i]].lowBound -= tl;
    }
    tl = 0;
    for( i=0; i < custom.size(); i++ )
    {
        tdou = delta[custom[i]] / all;
        parts[custom[i]].lowBound += (long)(tdou * totalSize);
        tl += (long)(tdou * totalSize);
    }
    //因为浮点数，整数转换产生的余头，为了照顾慢设备
    if( tl < totalSize ) parts[0].lowBound += (totalSize - tl);

    return;
}
*/
	void adjustment()
	{
		int i,k;
		long totalSize, tl;
		double delta[DEVNUM], all, tdou;
		vector<int> custom;
		vector<int> supply;
		double avgPerf=0;
		
		
		for( i=0; i < DEVNUM; i++ )
		{
			avgPerf+=parts[i].avgPerf;
		}
		avgPerf/=DEVNUM;
		 printf("\nAvg.Pef:%f\n",avgPerf);
		for(k=0;k<DEVNUM;k++){
							  printf("dev-%d blkNum=%d,lowBound=%d,bio_num=%d",k,parts[k].bufPart.blkNum,parts[k].lowBound,bio_num[k]);
							printf("dev-%d:avgPerf:%f Tdisk:%f, ranPG:%f, seqPG:%f\n",k,parts[k].avgPerf,parts[k].Tdisk,parts[k].ranPG,parts[k].seqPG);
		} 
		all = 0; //后面计算的分母
		for( i=0; i < DEVNUM; i++ )
		{
			delta[i] = fabs(parts[i].avgPerf -avgPerf);
			if(parts[i].avgPerf>avgPerf)
			{
				custom.push_back( i );
				all += delta[i];
			}
			else{
				supply.push_back( i );
			}
			//pats[i].{avg,last}Perf后面用不到了
			parts[i].lastPerf = parts[i].avgPerf;
			//parts[i].avgPerf = 0;
			//parts[i].N = 0;
		}
		totalSize = 0;	//计算该周期的总的调整量
		if(custom.size()<=0)return;
		if(all<=0)return;
		for( i=0; i < supply.size(); i++ )
		{
			if( parts[supply[i]].lowBound <= MinLowBound ) continue;
			tl = parts[supply[i]].lowBound * AdjustStep;
			totalSize += tl;
			parts[supply[i]].lowBound -= tl;
		}
		tl = 0;
		for( i=0; i < custom.size(); i++ )
		{
			tdou = delta[custom[i]] / all;
			parts[custom[i]].lowBound += (long)(tdou * totalSize);
			tl += (long)(tdou * totalSize);
		}
		//因为浮点数，整数转换产生的余头，为了照顾慢设备
		if( tl < totalSize ) parts[0].lowBound += (totalSize - tl);
		/*printf("\n");
		for(k=0;k<DEVNUM;k++){
			printf("dev-%d blkNum=%d,lowBound=%d,bio_num=%d",k,parts[k].bufPart.blkNum,parts[k].lowBound,bio_num[k]);
			printf("dev-%d:avgPerf:%f ranPG:%f, seqPG:%f\n",k,parts[k].avgPerf,parts[k].ranPG,parts[k].seqPG);
			if(parts[k].lowBound<0)exit(1);
		}*/
	
		return;
	}

int main(int argc, char *argv[] )
{
    struct stat buf;
    ifstream fin;
    SysTime timeStart, penalty;
    int reqType, reqBlkNum, reqBlkNumOld;
    unsigned long reqStartBlk, reqBlk=0, ioaddr, prefetchEnd;
    unsigned long long version = 0;

    int i, j, k,RetVal, windows=0, devno, id;
    double tdouble;
    string line, line2, line3, line4;
    Buf *pBuf, *tpBuf, *tempBuf;
    unsigned long hits = 0, tul;
    struct disksim_interface *disksim;
    map<unsigned long, unsigned long>::iterator it;
    map<int, Application>::iterator itapp;
    Request *tmp_request, *trp;
    SubReq  *psubreq;
    SubReq subReqs[DEVNUM];
	SubReq subReqs_2[DEVNUM];
    vector<SubReq> ioreqs[DEVNUM];
    int temp,cross=0;
	double avgPerf=0;
	

    if (argc != 7)
    {
        fprintf(stderr, "usage: %s <设备参数文件> <负载文件> \n", argv[0]);
        exit(1);
    }

    if (stat(argv[1], &buf) < 0) panic(argv[1]);

    StripeUnit = atoi(argv[3]);
    BUFSIZE = strtoul(argv[4], NULL, 10);
    ProvisonRatio = atof( argv[5] );
    MinLowBound = atoi( argv[6] );

    fin.open(argv[1]);   //负载文件
    for( i=0; i<DEVNUM; i++)
    {
        getline( fin, line);
        getline( fin, line2);
        getline( fin, line3);
        getline( fin, line4);
        j =  atoi( line4.c_str() );  //设备类型
        tdouble = atof(line3.c_str());
        Partition part(i, j, tdouble);
        part.disksim = disksim_interface_initialize(line.c_str(),
                       line2.c_str(),
                       syssim_report_completion,
                       syssim_schedule_callback,
                       syssim_deschedule_callback,
                       0,
                       0,
                       0);
        parts.push_back(part);
		bio_num[i]=0;
    }

    doInit( argv[3] );
	cout<<"succeed to read the CART model.\n";
    fin.close();
    fin.open(argv[2]);   //负载文件

    while( getline( fin, line) )
    {
		if(counttt%2000==0)
			//printf("line %d\n",counttt);
		counttt+=1;
        //                             起始地址       大小      类型
        RetVal = sscanf( line.c_str(),"%d %ld %d %d", &id, &reqStartBlk, &reqBlkNum, &reqType );
        if( RetVal != 4 ) PRINT_EXIT("error : 文件读取格式出错");
		reqStartBlk/=BLOCK2SECTOR;
		reqBlkNum=(reqBlkNum-1)/BLOCK2SECTOR+1;

        reqBlkNumOld = reqBlkNum;
        version++;
        totalBlocks += reqBlkNum;
          if(version%100000==0){
          for( i=0; i < DEVNUM; i++ )
	          {
		            avgPerf+=parts[i].avgPerf;
	           }
               avgPerf/=DEVNUM;
               cout<<"\nAvg.Pef:"<<avgPerf<<endl;
	              for(k=0;k<DEVNUM;k++){
						  cout<<"dev-"<<k<<" blkNum="<<parts[k].bufPart.blkNum<<" lowBound="<<parts[k].lowBound<<" bio_num="<<bio_num[k]<<endl;
						  printf("dev-%d:avgPerf:%f Tdisk:%f, ranPG:%f, seqPG:%f\n",k,parts[k].avgPerf,parts[k].Tdisk,parts[k].ranPG,parts[k].seqPG);
	              }
          }
        //这里计时，考虑全部
        timeStart = now;
        windows++;
        if( windows >= WindowsSize ){
            windows=0;
		//	adjustment();
        }
        itapp = apps.find( id );
        if( itapp == apps.end() )   //no this app
        {
            Application tapp( id );
            apps.insert( make_pair(id, tapp) );
            itapp = apps.find( id );
            assert( itapp != apps.end() );
        }
        //识别出type，和预取长度
        prefetchEnd = itapp->second.arriveRequest( reqStartBlk, reqBlkNum );
        tul = reqStartBlk + reqBlkNum - 1;
        if( prefetchEnd > 0 )
        {
            assert( prefetchEnd > tul );
            reqBlkNum = prefetchEnd - reqStartBlk + 1;
        }
        //分出多个子请求
        //assert(reqBlkNum > 0);
        requestMapping( reqStartBlk, reqBlkNum, subReqs);
        //预测各个性能参数
        prediction(subReqs,reqType);
        computePG(subReqs, prefetchEnd, reqBlkNum, itapp->second.type );
        //assert( prefetchEnd >= (reqStartBlk + reqBlkNum));
        //if( prefetchEnd == 0 ) prefetchEnd = reqStartBlk + reqBlkNum - 1;
        for( j = 0 ; j<reqBlkNum; j++ )
        {
            reqBlk = reqStartBlk + j;
            devno = blockMapping( reqBlk );
			bio_num[devno]++;
            pBuf = isBlkInList( reqBlk, &parts[devno].bufPart );
            /*
            for(i=0; i<DEVNUM; i++)
            {
                pBuf = isBlkInList( reqBlk, &parts[i].bufPart );
                if( pBuf != NULL ) break;
            }
            if( pBuf != NULL )
            {
                cout<<"find-"<<devno<<endl;
                cout<<"in-"<<i<<endl;
            }
            */
            if( pBuf != NULL )
            {
                totalHits++;
                //insertToHead(pBuf,&bufGlobal);
                switch(itapp->second.type)
                {
                case 0:
                    pBuf->counter = 1;
                    break;
                case 1:
                    pBuf->counter = 0;
                    break;
                default:
                    pBuf->counter = itapp->second.looplen;
                    break;
                }
                if( reqType == 0 )//write
                {
                    if( pBuf->dirty == 0 )
                    {
                        pBuf->nextW = NULL;
                        insert_wWaitList( pBuf );
                    }
                }
            }
            else
            {
                if( bufInvalid.blkNum > 0  )
                {
                    pBuf = delListHead(&bufInvalid);
                    //cout<<"从bufInvalid中下来的pBuf号="<<pBuf->blkno<<endl;
                    pBuf->blkno = reqBlk;
                    pBuf->valid = 1;
                    insertToTail( pBuf, &parts[devno].bufPart );
                    //insertToHead( pBuf, &parts[devno].bufPart );
                    if( parts[devno].current == NULL ) parts[devno].current = parts[devno].bufPart.head;
                }
                else
                {
                    i = victimSelection( devno,itapp->second.type);
                    if(parts[i].current == NULL)
                    {
                        printf("current is NULL!\n");
                        exit(1);
                    }
                    //会不会存在死循环?
                    while( parts[i].current->counter != 0 )
                    {
                        parts[i].current->counter--;
                        assert( parts[i].current->counter >= 0 );
                        parts[i].current = parts[i].current->next;
                        if( parts[i].current == NULL ) parts[i].current = parts[i].bufPart.head;
                    }
                    tpBuf = parts[i].current;
                    assert( tpBuf != NULL );
                    parts[i].current = parts[i].current->next;
                    if( parts[i].current == NULL ) parts[i].current = parts[i].bufPart.head;
					if( tpBuf == parts[i].current ){
					  for(k=0;k<DEVNUM;k++){
                        printf("dev-%d blkNum=%d,lowBound=%d,bio_num=%d",k,parts[k].bufPart.blkNum,parts[k].lowBound,bio_num[k]);
					    printf("dev-%d: avgPerf:%f lastPerf:%f\n",k,parts[k].avgPerf,parts[k].lastPerf);
					  }

					}
                    assert( tpBuf != parts[i].current );
					if( tpBuf->dirty)
					{
						 do_write(tpBuf->blkno);
					}

                    if( i == devno )
                    {
                        pBuf = tpBuf;
                        updateBlkno( reqBlk, tpBuf, &parts[devno].bufPart );
						pBuf->blkno = reqBlk;
                    }
                    else
                    {
                        //会不会删除多度，使得current指向空;不会，有最少值
                        pBuf = deleteFromList(tpBuf->blkno, &parts[i].bufPart );
                        pBuf->blkno = reqBlk;
                        //insertToHead(pBuf, &parts[devno].bufPart);
                        insertToTail(pBuf, &parts[devno].bufPart);
						if( parts[devno].current == NULL ) parts[devno].current = parts[devno].bufPart.head;
/*
                        if( parts[devno].bufPart.blkNum < 2)
                        {
                            insertToTail(pBuf, &parts[devno].bufPart);
                            if( parts[devno].current == NULL ) parts[devno].current = parts[devno].bufPart.head;
                        }
                        else insertToList(parts[devno].current->blkno, pBuf, &parts[devno].bufPart);
*/
                    }
					
                }
                assert( pBuf != NULL );
               
                //pBuf->blkno = reqBlk;
                pBuf->valid = 1;
                pBuf->dirty = 0;
                switch(itapp->second.type)
                {
                case 0:
                    pBuf->counter = 1;
                    break;
                case 1:
                    pBuf->counter = 0;
                    break;
                default:
                    pBuf->counter = itapp->second.looplen;
                    break;
                }
                if( reqType == 1 )//read
                {
                    merge_request( &readRequestList, reqBlk, 1);
                }
                else
                {
                    pBuf->nextW = NULL;
                    insert_wWaitList( pBuf );
                }
            }
            if( reqType == 0 ) pBuf->dirty = 1;
        }
        //这里计时，不会考虑写
        //timeStart = now;
        for( i=0; i<DEVNUM; i++)
        {
            ioreqs[i].clear();
        }
        tmp_request = readRequestList.head;
        while( tmp_request != NULL )    //多个不连续的请求分别处理，TODO， 验证下面这段代码是否正确
        {
            free( tmp_request->prev);
            requestMapping( tmp_request->start, tmp_request->size, subReqs_2 );
            for( i=0; i<DEVNUM; i++)
            {
                if(subReqs_2[i].reqStart != 0)
                {
                    j = ioreqs[i].size();
                    if( j == 0 )
                    {
                        ioreqs[i].push_back(subReqs_2[i]);
                    }
                    else
                    {
                        psubreq = &ioreqs[i][j-1];
                        tul = psubreq->reqStart + psubreq->reqSize - 1;
                        if( subReqs_2[i].reqStart == tul ) psubreq->reqSize += subReqs_2[i].reqSize;
                        else  ioreqs[i].push_back(subReqs_2[i]);
                    }

                }
            }
            tmp_request = tmp_request->next;
        }
        readRequestList.head = NULL;
        hybridRead( ioreqs );
        penalty = now - timeStart;
        add_statistics( &st, penalty );
       temp=0;
		for( i=0; i<DEVNUM; i++)
        {
                if(subReqs[i].reqStart != 0)
                {
                        parts[i].averageTime(penalty/subReqs[i].reqSize);
						temp+=1;
                }
		}
		if(temp>1){
             cross++;
		}
/*
                cout<<"free blocks="<<bufInvalid.blkNum<<endl;
        for( i=0; i<DEVNUM; i++ )
        {
            cout<<"dev-"<<i<<"'s blocks="<<parts[i].bufPart.blkNum<<endl;
            tempBuf = parts[i].bufPart.head;
            while( tempBuf != NULL )
            {
                cout<<tempBuf->blkno<<", ";
                tempBuf = tempBuf->next;
            }
            cout<<endl;
        }
*/
    }//next request
	Py_Finalize();
    fin.close();
    //debug.close();
#ifdef DEBUG_TIME
    funFile.close();
#endif
    printf("End of the simulation\n");
    for( k=0; k<DEVNUM; k++)
    {
        disksim_interface_shutdown(parts[k].disksim, now);
        printf("dev-%d blkNum=%d,lowBound=%d,bio_num=%d",k,parts[k].bufPart.blkNum,parts[k].lowBound,bio_num[k]);
	    printf("dev-%d:idtype%d avgPerf:%f ranPG:%f, seqPG:%f\n",k,itapp->second.type,parts[k].avgPerf,parts[k].ranPG,parts[k].seqPG);
		
    }
    printf("Shutdown the simulator\n");
    /*
        printf("End of the simulation\n");
        disksim_interface_shutdown(devSSD, now);
        disksim_interface_shutdown(devDisk, now);
        printf("Shutdown the simulator\n");
    */
    print_statistics( &st, "response time" );
    printf("total execution time = %f\n",now);
    printf("total request number = %lld\n",version);
	printf("total cross_request number = %lld\n",cross);
    printf("total traffic size (block) = %lld\n",totalBlocks);
    printf("Avg.latency(per request)= %f\n",(now/(double)version) );
    printf("Avg.latency(per block)= %f\n",(now/(double)totalBlocks) );
    printf("Bandwidth(MB/s)= %f\n",((double)totalBlocks/((double)now)*BLOCKSIZE/1024) );
    printf("Hit ratio= %lf\n",(double)totalHits / (double)totalBlocks );

//printf("Hits:%d ,  Miss:%d\n",blkHit,blkMiss);
//printf("Hit Ratio: %f\n",blkHit*1.0/(blkHit+blkMiss));

}


