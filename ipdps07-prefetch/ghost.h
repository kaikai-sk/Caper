#ifndef GHOST_H
#define GHOST_H

#include <set>
#include <queue>
#include <deque>

using namespace std;

class GhostSegment
{
public:
    int segsize;
    double mu;
    deque<unsigned long> blocks;
    set<unsigned long> bindex;
    GhostSegment( int x);
};

class GhostCache
{
public:
    GhostCache();    //segment长度
    void init(int num);
    void insertBufToGhost(unsigned long x);
    int isHitInGhost( unsigned long blkno );
    void moveSegToLastGhost();
    void updateMuInGhost( double delay, int num, int pos );
    double getCmu();
    void walkout();
private:
    deque<GhostSegment> segs;
    int segmentSize;
};

#endif
