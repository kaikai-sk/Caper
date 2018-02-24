#ifndef APPLICATION_H
#define APPLICATION_H
#include <map>

using namespace std;

typedef struct addrange_T
{
    unsigned long addrStart;
    unsigned long addrEnd;
    unsigned long newlpend;   //new loopend
    int num;
} RangeItem;

class Application
{
public:
    int id;
    int type;
    long looplen;
    Application( int id );
    unsigned long arriveRequest( unsigned long reqStartBlk, int reqBlkNum );
private:
    int CN;
    unsigned long lastStart;
    unsigned long lastEnd;
    unsigned long prefetchEnd;
    unsigned long prefetchDegree;
    map<unsigned long, RangeItem> seqs;   //start-address, item
    map<unsigned long, RangeItem> loops;
};

#endif
