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
#include "forney.h"
#include "application.h"

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
#define WindowsSize 1000
#define MinLowBound 100
#define MaxReqSize 128    //防止disksim出错
#define IncreaseUnit 128   //0.002*64000
//#define ThreshCS 5         //判断是constomer还是supplier
#define ThreshInc 0.5    //判断上升还是下降
#define ThreshDes -0.5

//static struct disksim_interface *disksim;
static SysTime now = 0;		/* current time */
static SysTime subtime = 0;		/* current time */
static SysTime next_event = -1;	/* next event */
static int completed = 0;	/* last request was completed */

static unsigned long long totalBlocks = 0;
static unsigned long BUFSIZE;
static int StripeUnit;
static int ThreshCS;
static unsigned long long totalHits = 0;

map<int, Application> apps;    //id, application

class Partition;

class Partition
{
public:
    int id;
    int type;   //1, cooling; 2, waring; 3, cooling; 4, warm
    long customerDegree;
    long desiredSize;
    unsigned long N;   //每个窗口的累计请求数
    double avgPerf;
    double lastPerf;
    BufList bufPart;
    struct disksim_interface *disksim;
    //Partition( int id );
    Partition( int x);
    void averageTime(double delay);
};

Partition::Partition( int x )
{
    id = x;
    type = 1;
    avgPerf = lastPerf = 0;
    N = 0;
    customerDegree = IncreaseUnit;
    desiredSize = BUFSIZE / DEVNUM;  //0也可以，每个周期都会覆盖
    initBufList( &bufPart);
}
void Partition::averageTime(double delay)
{
    avgPerf = (avgPerf * N + delay) / (N + 1);
    N++;
    return;
}

vector<Partition> parts;

//ofstream debug("debug.list");

typedef	struct
{
    int n;				//请求总数
    double sum;         //请求延迟时间的累加
} Stat;
static Stat st;

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
    printf("%s: 总块数=%d 平均块延迟=%f\n ", title, s->n, avg);
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


void doInit()
{
    int i, hot;
    long tlong;
    unsigned long blkAddr, taddr;
    Buf *pBuf;
    string line;

    initBufList( &bufInvalid);

    cout<<"cache size = "<<BUFSIZE<<endl;
    cout<<"Stripe Unit = "<<StripeUnit<<endl;

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
    int i, j, n;
    SubReq *psubreq;
    //double timeStart, penalty;
    double delay;
    struct disksim_request r;
    SysTime maxtime, timeStart, penalty;   //maxtime,子请求中最晚完成的时间

    timeStart = maxtime = now;    //一起发送，等待全部完成才算完成，并返回
    for( i=0; i<DEVNUM; i++ )
    {
        n = 0;
        subtime = timeStart;
        for(j=0; j<ioreqs[i].size(); j++)
        {
            psubreq = &ioreqs[i][j];
            n += psubreq->reqSize;
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
        if( subtime > maxtime ) maxtime = subtime;
        if( n != 0 )
        {
            assert( subtime > timeStart );
            delay = subtime - timeStart;
            parts[i].averageTime( delay );
        }
    }
    now = maxtime;
    //penalty = now - timeStart;
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
void hybridWrite( unsigned long addr, unsigned size )
{
    cout<<"error: is not support write now"<<endl;
    exit(2);
}
/*
void hybridWrite( unsigned long addr, unsigned size )
{
    unsigned long ioaddr;
    double timeStart, penalty;
    struct disksim_request r;
    vector<SubReq> sreqs; //sub-reqeusts
    SysTime maxtime, timeStart, penalty;   //maxtime,子请求中最晚完成的时间

    timeStart = maxtime = now;    //一起发送，等待全部完成才算完成，并返回
    sreqs = lbnMapping( addr, size );
    for( i=0; i<sreqs.size(); i++ )
    {
        r.start = timeStart;
        r.blkno = sreqs[i].start * BLOCK2SECTOR;
        r.flags = DISKSIM_WRITE;
        r.bytecount = sreqs[i].size * BLOCKSIZE;
        r.devno = 0;

        //printf("submit request blkno = %u, size = %d, type = R\n", r.blkno, r.bytecount/BLOCKSIZE);
        submitRequest( sreqs[i].disksim, &r, timeStart );
        if( subtime > maxtime ) maxtime = subtime;
    }
    now = maxtime;
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

void do_write( unsigned long blkno)
{
    cout<<"error: not support wirte now"<<endl;
    exit(1);
}

/*
void do_write( unsigned long blkno)
{
    int size;
    unsigned long addr;
    double timeTemp, penalty;
    Buf *pBuf;
    struct disksim_request r;
    map<unsigned long, wWaitRequest>::iterator it = wWaitRequestList.begin();

    for( ; it != wWaitRequestList.end(); it++)
    {
        if( blkno < it->first )
        {
            cout<<"error@do_write(), cann't find blkno in the write requests list"<<endl;
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
            while( size > 1024 )
            {
                hybridWrite( addr, 1024);
                size -= 1024;
                addr += 1024;
            }
            hybridWrite( addr, size);

            penalty = now - timeTemp;
            add_statistics( &st, penalty );

            wWaitRequestList.erase( it->first );
            break;
        }
    }
    if( it == wWaitRequestList.end() ) PRINT_EXIT( "liyong, error : do_write(), cann't find blkno in the write requests list");

    return;
}
*/

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

int blockMapping( unsigned long blkno )
{
    int uno, dno;   //uno: strip序号（从0开始），cno：行号（从0开始），dno：设备序号（从0开始）

    uno = (blkno - 1) / StripeUnit;
    dno = uno % DEVNUM;

    return dno;
}

int victimSelection( int pid )
{
    int i, index = -1;
    double maxPG = 0;

    assert( pid >= 0 );
    assert( pid < DEVNUM );

    //for( i=0; i<DEVNUM; i++)
    for( i=(DEVNUM-1); i >= 0; i--)
    {
        if( parts[i].bufPart.blkNum > parts[i].desiredSize )
        {
            if( parts[i].bufPart.blkNum > MinLowBound )
            {
                index = i;
                break;
            }
        }
    }

    if( index == -1 ) index = pid;
    return index;
}

void adjustment()
{
    int i, j;
    double td, tavg, rwt[DEVNUM], totalIrwt, irwt[DEVNUM];
    long totalCus, totalSup, tlong;
    vector<int> suppliers;

    td = 0;
    for( i=0; i<DEVNUM; i++)
    {
        if( parts[i].lastPerf < 0.0001 ) return;     //第一个周期，无需调整
        td += parts[i].avgPerf;
    }
    tavg = td / 4;       //计算平均等待时间
    totalCus = 0;
    for( i=0; i<DEVNUM; i++)
    {
        rwt[i] = parts[i].avgPerf / tavg;    //转换为相对relate wait time
        if( rwt[i] >= ThreshCS )
        {
            td = (parts[i].avgPerf - parts[i].lastPerf) / parts[i].lastPerf;
            if( td > ThreshInc )  //warming
            {
                if( parts[i].type = 2 ) parts[i].customerDegree *= 2;
                parts[i].type = 2;
                parts[i].desiredSize =  parts[i].bufPart.blkNum + parts[i].customerDegree;
                totalCus += parts[i].customerDegree;
            }
            else if( td >= ThreshDes )  //warm
            {
                parts[i].type = 4;
                parts[i].customerDegree = IncreaseUnit;
                parts[i].desiredSize = parts[i].bufPart.blkNum + parts[i].customerDegree;
                totalCus += parts[i].customerDegree;
            }
            else //cooling
            {
                parts[i].type = 3;
                parts[i].desiredSize = parts[i].bufPart.blkNum;
            }
        }
        else   //cool
        {
            parts[i].type = 1;
            suppliers.push_back(i);
        }
    }
    totalIrwt = 0;
    for( i=0; i<suppliers.size(); i++ )
    {
        j = suppliers[i];
        irwt[j] = ThreshCS - rwt[j];
        assert( irwt[j] >= 0 );
        totalIrwt += irwt[j];
    }
    for( i=0; i<suppliers.size(); i++ )
    {
        j = suppliers[i];
        tlong = (irwt[j] / totalIrwt ) * totalCus;
        parts[j].desiredSize = parts[i].bufPart.blkNum - tlong;
    }

    for( i=0; i<DEVNUM; i++)
    {
        parts[i].lastPerf = parts[i].avgPerf;
        parts[i].avgPerf = 0;
        parts[i].N =0;
    }

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

    int i, j, RetVal, windows=0, devno, id;
    double tdouble;
    string line, line2, line3, line4;
    Buf *pBuf, *tpBuf, *tempBuf;
    unsigned long hits = 0, tul;
    Request *tmp_request, *trp;
    SubReq  *psubreq;
    SubReq subReqs[DEVNUM];
    vector<SubReq> ioreqs[DEVNUM];
    map<int, Application>::iterator itapp;

    if (argc != 6)
    {
        fprintf(stderr, "usage: %s <设备参数文件> <负载文件> <ThreshCS>\n", argv[0]);
        exit(1);
    }

    if (stat(argv[1], &buf) < 0) panic(argv[1]);
    StripeUnit = atoi(argv[3]);
    BUFSIZE = strtoul(argv[4], NULL, 10);
	ThreshCS = atoi( argv[5] );

    fin.open(argv[1]);   //负载文件
    for( i=0; i<DEVNUM; i++)
    {
        getline( fin, line);
        getline( fin, line2);
        getline( fin, line3);
        getline( fin, line4);
        j =  atoi( line4.c_str() );  //设备类型
        tdouble = atof(line3.c_str());
        Partition part(i);
        part.disksim = disksim_interface_initialize(line.c_str(),
                       line2.c_str(),
                       syssim_report_completion,
                       syssim_schedule_callback,
                       syssim_deschedule_callback,
                       0,
                       0,
                       0);
        parts.push_back(part);
    }

    doInit();
    fin.close();
    fin.open(argv[2]);   //负载文件

    while( getline( fin, line) )
    {
        //                             起始地址       大小      类型
        RetVal = sscanf( line.c_str(),"%d %ld %d %d", &id, &reqStartBlk, &reqBlkNum, &reqType );
        if( RetVal != 4 ) PRINT_EXIT("error : 文件读取格式出错");

        reqBlkNumOld = reqBlkNum;
        version++;
        totalBlocks += reqBlkNum;

        //cout<<"arrive a reqeust "<<version<<" : "<<reqStartBlk<<", size = "<<reqBlkNum<<endl;
        //这里计时，考虑全部
        timeStart = now;
        windows++;
        if( windows >= WindowsSize ) adjustment();

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

        for( j = 0 ; j<reqBlkNum; j++ )
        {
            reqBlk = reqStartBlk + j;
            devno = blockMapping( reqBlk );
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
                insertToHead(pBuf, &parts[devno].bufPart );
                if( reqType == 1 )
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
                }
                else
                {
                    i = devno;
                    if( parts[devno].bufPart.blkNum < parts[devno].desiredSize )
                    {
                        i = victimSelection( devno );
                    }
                    pBuf = delListTail( &parts[i].bufPart );   //如果i==devno，那么删除自己；否则，删除victim
                }
                assert( pBuf != NULL );
                if( pBuf->dirty )
                {
                    do_write( pBuf->blkno );
                }
                pBuf->blkno = reqBlk;
                pBuf->valid = 1;
                pBuf->dirty = 0;
                insertToHead( pBuf, &parts[devno].bufPart );
                if( reqType == 0 )
                {
                    merge_request( &readRequestList, reqBlk, 1);
                }
                else
                {
                    pBuf->nextW = NULL;
                    insert_wWaitList( pBuf );
                }
            }
            if( reqType == 1 ) pBuf->dirty = 1;
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
            requestMapping( tmp_request->start, tmp_request->size, subReqs );
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
            tmp_request = tmp_request->next;
        }
        readRequestList.head = NULL;
        hybridRead( ioreqs );
        penalty = now - timeStart;
        add_statistics( &st, penalty );
//        parts[devno].averageTime( penalty );
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

    fin.close();
    //debug.close();
#ifdef DEBUG_TIME
    funFile.close();
#endif
    printf("End of the simulation\n");
    for( i=0; i<DEVNUM; i++)
    {
        disksim_interface_shutdown(parts[i].disksim, now);
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
    printf("请求总数 = %lld\n",version);
    printf("总块数 = %lld\n",totalBlocks);
    printf("平均延时(请求) = %f\n",(now/(double)version) );
    printf("平均延时(块) = %f\n",(now/(double)totalBlocks) );
    printf("平均带宽 = %f\n",((double)totalBlocks/((double)now)*2) );
    printf("平均命中率 = %lf\n",(double)totalHits / (double)totalBlocks );

//printf("Hits:%d ,  Miss:%d\n",blkHit,blkMiss);
//printf("Hit Ratio: %f\n",blkHit*1.0/(blkHit+blkMiss));

}


