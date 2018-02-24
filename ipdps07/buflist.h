#ifndef BUFLIST_H
#define BUFLIST_H

#include "hash.h"

struct buf
{
	unsigned  		blkno;
	int 			dirty;
	int 			valid;
	int             old;
    struct buf      *nextW;
	struct buf 		*next;
	struct buf 		*prev;
};

typedef struct buf Buf;

struct bufList
{
	Buf *head;
	Buf *tail;
	int blkNum;                //当前的使用大小?
	hash_table *table;
};

typedef struct bufList BufList;


void initBufList(BufList *list);
//0 非空。 1 true 空
int isListEmpty(BufList *list);

Buf * isBlkInList(unsigned long reqBlk, BufList *list);

Buf * deleteFromList( unsigned long reqBlk,BufList *list);

Buf *delListTail(BufList *list);

Buf *delListHead(BufList *list);

void insertToHead(Buf *pBuf,BufList *list);

void moveBufForward(Buf *pBuf,BufList *list);

#endif
