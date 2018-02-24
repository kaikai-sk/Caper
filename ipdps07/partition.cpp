#include <iostream>
#include <cstdlib>
#include <cassert>
#include "partition.h"

#define PRINT_EXIT(s) { cout<<s<<endl; exit(1); }

Segment::Segment( int size )
{
    length = size;
    mu = 0;
    next = prev = NULL;
    initBufList( &blocks );
}

Partition::Partition( int x, int ss )  //ghost-segment大小
{
    id = x;
    N = 0;
    avgDelay = 0;
    bufPart.head = NULL;
    bufPart.tail = NULL;
    bufPart.segNum = 0;
    ghost.init(ss);
}

void Partition::averageTime(double delay)
{
    avgDelay = (avgDelay * N + delay) / (N + 1);
    N++;
    return;
}

Segment* Partition::deleteSegFromTail()
{
    Buf* pBuf;
    Segment *pSeg = bufPart.tail;
    assert( pSeg != NULL );
    //if( pSeg == NULL ) return NULL;
    //if( pSeg == NULL ) PRINT_EXIT("error : 试图删除空的segment");
    if( bufPart.head == bufPart.tail )
    {
        bufPart.head = bufPart.tail = NULL;
    }
    else
    {
        bufPart.tail = pSeg->prev;
        pSeg->prev->next = NULL;
    }
    assert( pSeg->next == NULL );    //链表的末尾指针不为空，程序哪里出错了
    //if( pSeg->next != NULL ) PRINT_EXIT("error : 链表的末尾指针不为空，程序哪里出错了");
    pSeg->prev = NULL;

    bufPart.segNum--;
    assert( bufPart.segNum >= 0 );

    return pSeg;
}

void Partition::insertSegToTail( Segment *pSeg )
{
    Segment *pt = bufPart.tail;
    if( pSeg->blocks.blkNum == 0 ) PRINT_EXIT("error: 按照逻辑，空的segment不应该出现！");   //空的segment删除掉
    if( pt != NULL )
    {
        pt->next = pSeg;
        pSeg->prev = pt;
    }
    if( bufPart.head == NULL ) bufPart.head = pSeg;
    bufPart.tail = pSeg;

    bufPart.segNum++;
    assert( bufPart.segNum >= 0 );

    return;
}

Buf *Partition::isBufInPartition( unsigned long reqBlk, Segment* &pSeg )
{
    Buf *pBuf;

    pSeg = bufPart.head;
    //cout<<reqBlk<<" is in partition "<<id<<" :isBlkInSegList()"<<endl;
    while( pSeg != NULL )
    {
        pBuf = isBlkInList(reqBlk, &pSeg->blocks);
        if( pBuf != NULL )
        {
            return pBuf;
        }
        pSeg = pSeg->next;
    }
    return NULL;
}

void Partition::insertBufToHead( Buf *pBuf )
{
    Buf *pn = pBuf;
    Segment *pSeg = bufPart.head;

    while( pSeg != NULL )
    {
        //assert( pSeg->blocks.blkNum == pSeg->length );    //segment一直都是满的，初始化也被填满valid=0的blocks; 是否某处删除后没有及时填回
        insertToHead( pn, &pSeg->blocks );
        if( pSeg->next != NULL ) pn = delListTail( &pSeg->blocks );
        pSeg = pSeg->next;
    }

    return;
}
void Partition::moveBufToHead( Buf *pBuf )
{
    Buf *pb;
    Segment *pSeg = bufPart.head;

    while( pSeg != NULL )
    {
        pb = isBlkInList( pBuf->blkno, &pSeg->blocks);
        if( pb != NULL )
        {
            pb = deleteFromList( pBuf->blkno, &pSeg->blocks );
            break;
        }
        pb = delListTail( &pSeg->blocks );
        pSeg = pSeg->next;
        insertToHead( pb, &pSeg->blocks );
    }
    assert( pSeg != NULL );    //移动list中不存在的缓存块
    pSeg = bufPart.head;
    insertToHead( pb, &pSeg->blocks );

    return;
}
/*
Buf* Partition::deleteBufFromList( unsigned long reqBlk ) //应该用不到这个操作吧？
{
    Buf *pBuf = NULL, *pb;
    Segment *pSeg = bufPart.head;
    //查找所在的segment，删除pBuf
    while( pSeg != NULL )
    {
        pBuf = isBlkInList(reqBlk, &pSeg->blocks);
        if( pBuf != NULL )
        {
            pBuf = deleteFromList( reqBlk, &pSeg->blocks);
            assert( pSeg->blocks.blkNum != 0 );   //segment的blocks只有2中，n和n-1
            if( pBuf->old == 0 )
            {
                pSeg->size++;
                pBuf->old = 1;
                assert( pSeg->size <= SEGMENT_MAX_SIZE );
            }
            break;
        }
        pSeg = pSeg->next;
    }
    if( pSeg == NULL ) return NULL;
    return pBuf;
}
*/
Buf* Partition::deleteBufFromTail( )
{
    Buf *pBuf = NULL;
    Segment *pSeg = bufPart.tail;

    if( pSeg == NULL ) PRINT_EXIT("error: 删除list为空的tail");

    pBuf = delListTail( &pSeg->blocks );
    assert( pBuf != NULL );

    //if( pSeg->blocks.blkNum == 0 ) PRINT_EXIT("error:逻辑错误！因为只是在自己的list中删除插入，所以不会出现空的segment");
    assert( pSeg->blocks.blkNum > 0 );

    return pBuf;
}
/*
Buf* Partition::releaseBufFromTail( )
{
    Buf *pBuf = NULL;
    Segment *pt, *pSeg = bufPart.tail;
    if( pSeg == NULL ) PRINT_EXIT("error: 删除list为空的tail");


    pBuf = delListTail( &pSeg->blocks );
    if( pBuf == NULL )
    {
        if( 0 == pSeg->blocks.blkNum )
        {
            bufPart.tail = pSeg->prev;
            delete pSeg;
            if( bufPart.tail == NULL)
            {
                return NULL;
            }
            bufPart.tail->next = NULL;
            pSeg = bufPart.tail;
            pBuf = delListTail( &pSeg->blocks );
        }
        else
        {
            cout<<"error: list非空但是返回空Buf"<<endl;
            exit(1);
        }
    }
    assert( pBuf != NULL );

    return pBuf;
}
void Partition::releaseSegmentList( )
{
    bufPart.head = bufPart.tail = NULL;
}
*/
/*
void Partition::printSegments()
{
    int i = 0;
    Buf *pBuf;
    Segment *pSeg;

    i = 0;
    pSeg = bufPart.head;
    cout<<"segnums = "<<bufPart.segNum<<endl;
    while( pSeg != NULL )
    {
        i++;
        cout<<i<<"("<<pSeg->blocks.blkNum<<"), ";
        pSeg = pSeg->next;
    }
    cout<<endl;
    cout<<"number of segment(iterate) = "<<i<<endl;
    return;
}

void Partition::printBlocks()
{
    int j;
    Buf *pBuf;
    Segment *pSeg;

    pSeg = bufPart.head;
    while( pSeg != NULL )
    {
        j = 0;
        for( pBuf = pSeg->blocks.head; pBuf != NULL; pBuf = pBuf->next )
        {
            cout<<pBuf->blkno<<"("<<pBuf->valid<<"), ";
            j++;
        }
        cout<<", total = "<<j<<endl;
        pSeg = pSeg->next;
    }
    cout<<endl;
    return;
}
*/
/*
int main()
{
    int i = 1, j, k;
    double value;
    unsigned long blkno;
    Buf *pBuf;
    Segment *pSeg;
    Partition part(0);
    k=0;
    while( i )
    {
        cout<<"input the operations: 1:insertSegment; 2:deleteSegment; 3:insertBuf(buf); 4:deleteBuf; 5:moveBuf(blkno); 6:isHit(blkno); 7:walkout()"<<endl;
        cin>>i;
        switch(i)
        {
            case 1:
                pSeg = new Segment( SegLength );
//                pSeg = (Segment *)malloc(sizeof(Segment));
//                pSeg->length = SegLength;
//                pSeg->mu = 0;
//                pSeg->next = pSeg->prev = NULL;
//                initBufList( &pSeg->blocks );
                for( i = 0; i < SegLength; i++, k++ )
                {
                    pBuf = (Buf *)malloc(sizeof(Buf));
                    pBuf->blkno = k;
                    pBuf->valid = 0;
                    pBuf->dirty = 0;
                    pBuf->nextW = NULL;
                    insertToHead( pBuf, &pSeg->blocks );
                }
                part.insertSegToTail(pSeg);
                break;
            case 2:
                part.deleteSegFromTail();
                break;
            case 3:
                cin>>blkno;
                pBuf = part.deleteBufFromTail();
                pBuf->blkno = blkno;
                pBuf->valid = 1;
                part.insertBufToHead( pBuf );
                break;
            case 4:
                pBuf = part.deleteBufFromTail();
                if( pBuf == NULL ) cout<<"delete fail"<<endl;
                break;
            case 5:
                cin>>blkno;
                pBuf = part.isBufInPartition( blkno );
                if( pBuf != NULL ) part.moveBufToHead( pBuf );
                else cout<<"move an no-exist buf"<<endl;
                break;
            case 6:
                cin>>blkno;
                pBuf = part.isBufInPartition( blkno );
                if( pBuf == NULL ) cout<<"not in partition"<<endl;
                else cout<<"hit in partition"<<endl;
                break;
            case 7:
                part.printSegments();
                part.printBlocks();
                break;
            default:
                cout<<"no this opration"<<endl;
                break;
        }
    }

    return 0;
}
*/
