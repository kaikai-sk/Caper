#include <iostream>
#include <vector>
#include <cstdlib>
#include <cassert>
#include <ctime>
#include "partition.h"

using namespace std;



Partition::Partition( int x )
{
    id = x;
    state = 0;
    //deltaP = 0;
    //deltaPN = 0;
    lastBlkno = -1;
    lastVersion = 0;
    rate = 0;
    adjustP = 0;
    p = g = 0;
    ops = 0;
	ran_n=0;
    seq_n=0;
	ranPG=0;
	seqPG=0;
    //totalHits = 0;
    initBufList( &bufPart );
}

void Partition::cleanCounter()
{
    state = 0;
    ops = 0;
    adjustP = 0;
    p = g = 0;

    posindex.clear();

    return;
}
/*
void Partition::arriveRequest( unsigned long long version, unsigned long size )
{
    reqSizes.insert( make_pair( version, size) );
    return;
}
*/
/*
void Partition::updateOPS()
{
    long tl;

    tl  = adjustP  + deltaP;
    if( tl < 0 ) tl = 0;
    if( 1 == state ) ops = p + tl;
    else if ( 2 == state ) ops = p + g + tl ;
    else
    {
        cout<<"error, invoke the updateOPS() when the cache is locate in random state"<<endl;
        exit(1);
    }

    return;
}
*/
void Partition::updateOPS()
{
    long tl;

    assert( state != 0 );

    if( adjustP < 0 ) adjustP = 0;

    if( 1 == state ) ops = p + adjustP;
    else if ( 2 == state ) ops = p + g + adjustP ;
    while( ops < 0 )
    {
        adjustP /= 2;
        if( 1 == state ) ops = p + adjustP;
        else if ( 2 == state ) ops = p + g + adjustP ;
    }

    return;
}

void Partition::updatePositionDelete( unsigned long blkno )
{
    posindex.erase( blkno );
    /*
                        if( walk2() )
                    {
                        cout<<"id="<<id<<endl;
                        cout<<"出现不一致-d"<<endl;
                        walk();
                        exit(1);
                    }
                    */
}

void Partition::updatePositionInsert( unsigned long blkno )
{
    long long posno;
    map<unsigned long, long long>::iterator it;

//    cout<<"head blkno = "<<bufPart.head->blkno<<endl;
    if( bufPart.head->next == NULL ) posno = 0;
    else
    {
        it = posindex.find( bufPart.head->next->blkno );
        if( it == posindex.end() )
        {
//            cout<<"=0"<<endl;
            posno = 0;
        }
        else
        {
            //cout<<"+1, "<<it->second<<"+1"<<endl;
            posno = it->second + 1;
        }
    }
    //cout<<"insert to posindex, blkno="<<blkno<<", posno="<<posno<<endl;
    posindex.insert( make_pair( blkno, posno) );
    it = posindex.find( bufPart.head->blkno );
    //cout<<"posindex.insert : bufPart.size="<<bufPart.blkNum<<", posno="<<posno<<", headno="<<it->second<<endl;
/*
                    if( walk2() )
                    {
                        cout<<"id="<<id<<endl;
                        cout<<"出现不一致-i"<<endl;
                        walk();
                        exit(1);
                    }
*/
    return;
}
/*
void Partition::walk()
{
    Buf *pb;
    long long txl, txl2, i;
    map<unsigned long, long long>::iterator it;

    i = 0;
    cout<<"blocks(position) : "<<endl;
    for( pb = bufPart.head; pb != NULL; pb = pb->next )
    {
        i++;
        cout<<pb->blkno<<"("<<i<<"), ";
        it = posindex.find( pb->blkno );
        if( it == posindex.end() ) txl = -1;
        else
        {
            txl = it->second;
            it = posindex.find( bufPart.head->blkno );
            txl2 = it->second;
            cout<<pb->blkno<<" : "<<txl2<<" - "<<txl<<", pos=";
            txl = txl2 - txl + 1;
        }
        cout<<txl<<endl;
    }
    cout<<endl;
}
*/
/*
int Partition::walk2()
{
    Buf *pb;
    long long txl, txl2, i, res;
    map<unsigned long, long long>::iterator it;

    res = 0;
    i = 0;
    for( pb = bufPart.head; pb != NULL; pb = pb->next )
    {
        i++;
        it = posindex.find( pb->blkno );
        if( it == posindex.end() ) txl = -1;
        else
        {
            txl = it->second;
            it = posindex.find( bufPart.head->blkno );
            txl2 = it->second;
            txl = txl2 - txl + 1;
        }
        if( i != txl ) res = 1;
    }

    return res;
}
*/
void Partition::updateHitRecord( unsigned long blkno )
{
    long i, j, txl;
    unsigned long tul;
    long long txll1, txll2;
    Buf *pBuf;
    map<unsigned long, long long>::iterator it;
    //CLOCK_START

    //pBuf = isBlkInList( blkno, &bufPart );
    //assert( pBuf != NULL );
        //CLOCK_NODE(1)
/*
    i = 0;
    for( pBuf = bufPart.head; pBuf != NULL; pBuf = pBuf->next )
    {
                i++;
        if( pBuf->blkno == blkno ) break;
    }
    CLOCK_NODE(1)
    assert( pBuf != NULL );
*/
    //另一种position计算方法
    it = posindex.find( blkno );
    assert( it != posindex.end() );
    txll1 = it->second;
    it = posindex.find( bufPart.head->blkno );
    assert( it != posindex.end() );
    txll2 = it->second;
    txl = txll2 - txll1 + 1;
    //CLOCK_NODE(2)
/*
    if( txl != i)
    {
        cout<<"txl="<<txl<<", i="<<i<<endl;
        walk();
        assert( txl == i );
    }
*/
    for( j = hitDist.size(); j < txl; j++ ) hitDist.push_back(0);
    hitDist[txl-1]++;
    //CLOCK_NODE(3)
    //totalHits++;
/*
    tul = 0;
    for( j=0; j<hitDist.size(); j++ )
    tul += hitDist[j];
    */
/*
                for( j = 0; j < hitDist.size(); j++ )
            {
                cout<<hitDist[j]<<", ";
            }
            cout<<endl;
            cout<<tul<<" vs "<<totalHits<<endl;
            */
    //assert( totalHits == tul );

	//CLOCK_END
    return;
}

/*
void Partition::updateHitRecord( unsigned long blkno )
{
    int i;
    unsigned long position = 1, deltaSize, lastHit;
    Buf *pBuf;
    map<unsigned long, unsigned long>::iterator it;  //deltasize, hits

//    cout<<"queue ("<<blkno<<") in "<<bufPart.blkNum<<" blocks : ";
//    for( pBuf = bufPart.head; pBuf != NULL; pBuf = pBuf->next )
//    {
//        cout<<pBuf->blkno<<", ";
//    }
//    cout<<endl;

    position = 1;
    pBuf = isBlkInList( blkno, &bufPart );
    assert( pBuf != NULL );
    while( pBuf != bufPart.head )
    {
        pBuf = pBuf->prev;
        position++;
    }
//    if( id == 1 ) cout<<"partition'size = "<<bufPart.blkNum<<", position = "<<position<<endl;
    assert( bufPart.blkNum >= position );
    //deltaSize = bufPart.blkNum - position;
    if( ops >= position )   //TODO, 如果ops < position, 说明ops过小
    {
        deltaSize = ops - position + 1;
        lastHit = 0;
        for( it = hitRecords.begin(); it != hitRecords.end(); it++ )   //deltasize, hits
        {
            if( it->first >= deltaSize ) it->second++;
            else lastHit = it->second;
        }
        it = hitRecords.find( deltaSize );  //如果相等，那么前面已经加过了
        if( it == hitRecords.end() ) hitRecords.insert( make_pair(deltaSize, lastHit+1) );
    }


//    if( id == 1 )
//    {
//        cout<<"hit records : ";
//        for( it = hitRecords.begin(); it != hitRecords.end(); it++ )
//        {
//            cout<<"<"<<it->first<<","<<it->second<<"> ";
//        }
//        cout<<endl;
//    }

    return;
}
*/
/*
void Partition::updateHitRecord( unsigned long blkno )
{
    int i;
    unsigned long position = 1, deltaSize, lastHit;
    unsigned long long cver, smallestv;
    Buf *pBuf;
    map<unsigned long, unsigned long>::iterator it;  //deltasize, hits
    map<unsigned long long, unsigned long>::iterator it2;  //version, size
    vector<unsigned long long> smallvers;

    pBuf = isBlkInList( blkno, &bufPart );
    assert( pBuf != NULL );
    cver = pBuf->version;
    while( pBuf != bufPart.head )
    {
        pBuf = pBuf->prev;
        if( pBuf->version == cver ) position++;
        else break;
    }
    smallestv = bufPart.tail->version;
    for( it2 = reqSizes.begin(); it2 != reqSizes.end(); it2++ )   //version, size
    {
        if( it2->first > cver ) position += it2->second;
        if( it2->first <= smallestv ) smallvers.push_back( it2->first );
    }
    for( i=0; smallvers.size() > 0; i++ )
    {
        reqSizes.erase( smallvers[i] );
    }
    //TODO,可以加入删除version的操作
    assert( bufPart.blkNum > position );
    deltaSize = bufPart.blkNum - position;
    lastHit = 0;
    for( it = hitRecords.begin(); it != hitRecords.end(); it++ )   //deltasize, hits
    {
        if( it->first >= deltaSize ) it->second++;
        else lastHit = it->second;
    }
    it = hitRecords.find( deltaSize );
    if( it == hitRecords.end() ) hitRecords.insert( make_pair(deltaSize, lastHit+1) );

    return;
}
*/
//返回0-10， 10表示性能全部损失掉
int Partition::performanceDown()
{
    //CLOCK_START
    int i;
    unsigned long h1, h2;
    double downP;
    if( ops <= bufPart.blkNum ) return 0;

    h1 = h2 = 0;
    for( i=0; i < bufPart.blkNum; i++ ) h1 += hitDist[i];
    for( ; i < ops; i++ )
    {
        h1 += hitDist[i];
        h2 += hitDist[i];
    }
    downP = ((double)h2) / ((double)h1);
    //CLOCK_END
    return (int)(downP * 10);
}
/*
//返回0-10， 10表示性能全部损失掉
int Partition::performanceDown()
{
    unsigned long prevS, prevH, nextS, nextH, downH, deltaSize;
    double downP;
    map<unsigned long, unsigned long>::iterator it;

    if( ops <= bufPart.blkNum ) return 0;
    deltaSize = ops - bufPart.blkNum;
    it = hitRecords.find( deltaSize );
    if( it == hitRecords.end() )
    {
        nextS = nextH = 0;
        prevS = prevH = 0;
        for( it = hitRecords.begin(); it != hitRecords.end(); it++ )
        {
            if( deltaSize < it->first )
            {
                nextS = it->first;
                nextH = it->second;
                break;
            }
            prevS = it->first;
            prevH = it->second;
        }

        if( it == hitRecords.end() )
        {
            nextS = ops;
            nextH = totalHits;
        }
        assert( nextH != 0 );
        assert( nextH >= prevH );
        assert( nextS >= prevS );
        downH = ( (nextH - prevH)*deltaSize + (nextS * prevH) - (prevS*nextH) )/ (nextS - prevS);
        assert( downH <= totalHits );
        downP = (double)downH / (double)totalHits;
    }
    downP = (double)it->second / (double)totalHits;

    return (int)(downP * 10);
}
*/
void Partition::updateRate( unsigned long long version)
{
    unsigned long long xtime, xtime1;
    unsigned long xsize;

    if( lastVersion == 0 ) xtime = 1;
    else xtime = version - lastVersion;
    assert( xtime > 0 );

    xsize = rateItems.size();
    if( xsize < RATE_NUM )
    {
        rate = ( rate * xsize + (double)xtime ) / (double)( xsize + 1);
    }
    else
    {
        xtime1 = rateItems.back();
        rate = ( rate * xsize - (double)xtime1 ) /(double)(xsize - 1) ;
        rate = ( rate * (xsize - 1) + (double)xtime ) / (double)( xsize );
        rateItems.pop_back();
    }
    rateItems.push_front( xtime );
    lastVersion = version;

    return;
}

unsigned long long Partition::getRate()
{
    return rate;
}
//0，不连续； 1, 连续
int Partition::isSequency( unsigned long reqblk )
{
    if( lastBlkno == -1 ) return 0;
    else if( reqblk == ( lastBlkno + 1 ) ) return 1;
    else return 0;
}

/*
unsigned long Partition::findInGhost( unsigned long reqBlk )  //返回block在ghost中的位置, 不再的话返回0
{
    int i;
    for( i=0; i<ghost.size(); i++)
    {
        if( ghost[i].blkno == reqBlk ) return ( ghost.front().version - ghost[i].version + 1 );
    }
    return 0;
}

//找队首的block的version，然后将version+1赋值为新来的block，然后插入到队列中
void Partition::insertToGhost( unsigned long reqBlk )
{
    GBlock gblk;

    if( ghost.size() >= MAX_GHOST_SIZE ) ghost.pop_back();
    gblk.blkno = reqBlk;
    if( ghost.empty() ) gblk.version = 1;   //计数从1开始
    else gblk.version = (ghost.front().version + 1);
    ghost.push_front( gblk );
    return;
}
void Partition::updateGhostTracker( unsigned long position )  //更新deltaPart
{
    deltaP = (deltaP*deltaPN + position) / (deltaPN +1);
    deltaPN += 1;
    return;
}
unsigned long Partition::getDeltaPartition()      //返回当前的delta{P}
{
    return deltaP;
}
*/
