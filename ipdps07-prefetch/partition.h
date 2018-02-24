#ifndef PARTITION_H
#define PARTITION_H

#include "buflist.h"
#include "ghost.h"

using namespace std;


class Segment
{
public:
    int length;
    double mu;
    BufList blocks;
    Segment *next;
    Segment *prev;
    Segment( int size );
};


/*
typedef struct segment_T
{
    int length;
    double mu;
    BufList blocks;
    struct segment_T *next;
    struct segment_T *prev;
}Segment;
*/
struct segList
{
    Segment *head;
    Segment *tail;
    int segNum;                //当前的使用大小?
};

typedef struct segList SegList;

class Partition
{
public:
    int id;
    double avgDelay;
    SegList bufPart;
    GhostCache ghost;
    struct disksim_interface *disksim;

    Partition( int id, int ss );
    Segment* deleteSegFromTail();
    void insertSegToTail( Segment *pSeg );
    Buf* isBufInPartition( unsigned long reqBlk, Segment* &pSeg );
    void insertBufToHead( Buf *pBuf );
    void moveBufToHead( Buf *pBuf );
    Buf* deleteBufFromTail( );
    void printSegments();
    void printBlocks();
    void averageTime(double delay);
private:
    unsigned long N;   //累计请求数
};

#endif








