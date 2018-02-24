#include <iostream>
#include <set>
#include <queue>
#include <deque>
#include <cassert>
#include "ghost.h"

#define GhostLength 3  //segment number
#define FIRatio 0.5  //forword interaption

using namespace std;

GhostSegment::GhostSegment( int x)    //segment长度
{
    //int x;
    //x = BUFSIZE / SEGNUM;
    segsize = x;
    mu = 0;
}

    GhostCache::GhostCache()
    {

    }

void GhostCache::init( int num)   //segment长度
{
    int i;
    segmentSize = num;

    GhostSegment seg( segmentSize );
    for( i = 0; i < 3; i++ )
    {
        segs.push_back( seg );
    }
}

void GhostCache::insertBufToGhost(unsigned long x)
{
    int i;
    unsigned long blkno, tul;

    blkno = x;
    i = 0;
    while( i < GhostLength )
    {
        if(segs[i].blocks.size() < segs[i].segsize )
        {
            segs[i].blocks.push_front( blkno );
            segs[i].bindex.insert( blkno );
            break;
        }
        tul = segs[i].blocks.back();
        segs[i].blocks.pop_back();
        segs[i].bindex.erase( tul );
        segs[i].blocks.push_front( blkno );
        segs[i].bindex.insert( blkno );
        assert( segs[i].blocks.size() <= segs[i].segsize );
        assert( segs[i].bindex.size() <= segs[i].segsize );
        blkno = tul;
        i += 1;
    }

    return;
}

int GhostCache::isHitInGhost( unsigned long blkno )
{
    int i, ret;
    set<unsigned long>::iterator it;

    ret = -1;
    for( i = 0; i < GhostLength; i++ )
    {
        it = segs[i].bindex.find( blkno );
        if( it != segs[i].bindex.end() )
        {
            ret = i;
            break;
        }
    }

    return ret;
}

void GhostCache::moveSegToLastGhost()
{
    GhostSegment lastItem(segmentSize), titem(segmentSize);
    lastItem = segs.front();
    segs.pop_front();
    titem = segs.back();
    lastItem.mu = titem.mu * FIRatio;
    segs.push_back( lastItem );

    return;
}

void GhostCache::updateMuInGhost( double delay, int num, int pos )
{
    assert( pos < GhostLength );
    segs[pos].mu += delay * num;

    return;
}

double GhostCache::getCmu()
{
    return segs[0].mu;
}

void GhostCache::walkout()
{
    deque<unsigned long>::iterator itq;
    set<unsigned long>::iterator its;
    int i, j;

    for( i = 0; i < GhostLength; i++ )
    {
        cout<<"no."<<i<<"'s ghost: "<<endl;
        cout<<"in queue: ";
        for( itq = segs[i].blocks.begin(); itq != segs[i].blocks.end(); itq++ )
        {
            cout<<*itq<<", ";
        }
        cout<<endl;
        cout<<"in set(index): ";
        for( its = segs[i].bindex.begin(); its != segs[i].bindex.end(); its++ )
        {
            cout<<*its<<", ";
        }
        cout<<endl;
    }

    /*
    for( i = 0; i < GhostLength; i++ )
    {
        cout<<"no."<<i<<"'s ghost: "<<endl;
        cout<<"in queue: ";
        for( j = 0; j < segs[i].blocks.size(); j++ )
        {
            cout<<segs[i].blocks[j]<<", "<<endl;
        }
        cout<<"in set(index): ";
        for( its = segs[i].bindex.begin(); its != segs[i].bindex.end(); its++ )
        {
            cout<<*its<<", "<<endl;
        }
    }
    */
    return;
}
/*
int main()
{
    int i = 1, j, k;
    double value;
    unsigned long blkno;
    Ghost g( SegLength );
    while( i )
    {
        cout<<"input the operations: 1:insert(blkno); 2:find(blkno); 3:move; 4:updateMu(delay,num,pos); 5:getCmu(); 6:walkout()"<<endl;
        cin>>i;
        switch(i)
        {
            case 1:
                cin>>blkno;
                g.insertBufToGhost( blkno );
                break;
            case 2:
                cin>>blkno;
                j = g.isHitInGhost(blkno);
                if( j != -1 )
                {
                    cout<<"find in ghost's segment #"<<j<<endl;
                }
                else cout<<"not in ghost"<<endl;
                break;
            case 3:
                g.moveSegToLastGhost();
                break;
            case 4:
                cin>>value;
                cin>>j;
                cin>>k;
                g.updateMuInGhost(value, j, k );
                break;
            case 5:
                value = g.getCmu();
                cout<<"#0's mu = "<<value<<endl;
                break;

            case 6:
                g.walkout();
                break;

            default:
                cout<<"no this opration"<<endl;
                break;
        }
    }

    return 0;
}
*/








