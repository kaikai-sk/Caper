#include <stdio.h>
#include <string.h>
#include <math.h>
#include <time.h>
#include <stdlib.h>
#include <sys/time.h>
#include <unistd.h>
#include "buflist.h"

#define PE(s) {printf(s); exit(1);}

using namespace std;

void initBufList(BufList *list)
{
    list->table = (hash_table *)malloc(sizeof(hash_table));
    if( !list->table ) PE("error: 5， 内存分配错误");
    list->head = NULL;
    list->tail = NULL;
    list->blkNum = 0;
    list->table = construct_table(list->table,1024);
}

int isListEmpty(BufList *list)
{
    return (list->head==NULL);
}

Buf * isBlkInList(unsigned long reqBlk, BufList *list)
{
    Buf *pBufTemp=NULL;

    pBufTemp = (Buf *)lookup(reqBlk,list->table);
    return pBufTemp;
}

//在链表中删除pbuf。
//返回NULL，表示链表中不存在
Buf * deleteFromList( unsigned long reqBlk,BufList *list)
{
    Buf *pBufTemp,*pBuf;
#ifdef MYDEBUG
    printf("%s %d\n",__FUNCTION__,__LINE__);
#endif

    if( list->head == NULL )
    {
        printf("deleting from empty buffer\n");
        return NULL;
    }

    pBuf = (Buf *)lookup(reqBlk, list->table);
    if( pBuf == NULL )
    {
        printf("delete a nonexistent block buffer\n");
        return NULL;
    }

    list->blkNum--;
    pBufTemp = list->head->next;
    if( pBufTemp == NULL )
    {
        /* the only block buffer in the list */
        list->head = NULL;
        list->tail = NULL;
        return (Buf *)del(pBuf->blkno,list->table);
    }

    if( pBuf == list->head )
    {
        list->head = pBuf->next;
        pBuf->next->prev = NULL;
        return (Buf *)del(pBuf->blkno,list->table);
    }

    if( pBuf == list->tail )
    {
        list->tail = pBuf->prev;
        pBuf->prev->next = NULL;
        return (Buf *)del(pBuf->blkno,list->table);
    }

    pBuf->prev->next = pBuf->next;
    pBuf->next->prev = pBuf->prev;
    return (Buf *)del(pBuf->blkno,list->table);
}


//删除链表末尾的pbuf
//
Buf *delListTail(BufList *list)
{
    Buf *pBuf, *tmp;

#ifdef MYDEBUG
    printf("%s %d\n",__FUNCTION__,__LINE__);
#endif
    if( list->head == NULL )
        return NULL;

    pBuf = list->tail;
    del(pBuf->blkno,list->table);
    list->blkNum--;
    if( list->head == list->tail )
    {
        list->head = NULL;
        list->tail = NULL;
    }
    else
    {
        list->tail = pBuf->prev;
        pBuf->prev->next = NULL;
    }

    return pBuf;
}



Buf *delListHead(BufList *list)
{
#ifdef MYDEBUG
    printf("%s %d\n",__FUNCTION__,__LINE__);
#endif
    Buf *pBuf;

    if( list->head == NULL )
        return NULL;

    pBuf = list->head;
    del(pBuf->blkno,list->table);
    list->blkNum--;

    if( list->head == list->tail )
        list->head = list->tail = NULL;
    else
    {
        list->head = pBuf->next;
        pBuf->next->prev = NULL;
    }
    return pBuf;
}


//FIFO队列，新进的队尾，由head指向。tail指向最早入队的请求
//将pBuf插入到list的队首，head指向。
//pBuf本身插入到Buflist的hash_table中
void insertToHead(Buf *pBuf,BufList *list)
{
#ifdef MYDEBUG
    printf("%s %d\n",__FUNCTION__,__LINE__);
#endif

    if( lookup(pBuf->blkno,list->table) )
        deleteFromList( pBuf->blkno, list);
    list->blkNum++;

    pBuf->prev = NULL;
    pBuf->next = list->head;
    if( list->head != NULL )
        list->head->prev = pBuf;
    list->head = pBuf;
    if( list->tail == NULL )
        list->tail = pBuf;

    insert( pBuf->blkno, pBuf, list->table);
}


void moveBufForward(Buf *pBuf,BufList *list)
{
    Buf *pBuf1;
    if( pBuf == list->head )
    {
        printf("can not move head of the list even forward\n");
        exit(1);
    }

    pBuf1 = pBuf->prev;

    pBuf->prev = pBuf1->prev;
    if( pBuf1 == list->head )
        list->head = pBuf;
    else
        pBuf->prev->next = pBuf;

    pBuf1->next = pBuf->next;
    if( pBuf == list->tail )
        list->tail = pBuf1;
    else
        pBuf->next->prev = pBuf1;

    pBuf1->prev = pBuf;
    pBuf->next = pBuf1;
}

