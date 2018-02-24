#include <iostream>
#include <cassert>
#include <cstdlib>
#include "application.h"

using namespace std;

#define SEQT 4     //sequential threshold
#define LOOPT 2    //looping threshold
#define PrefetchSpeedup 2   //预取的increasing speed rate
#define PrefetchLimit 256

Application::Application( int x )
{
    id = x;
    type = 0;
    CN = 0;
    looplen = 0;
    lastStart = lastEnd = 0;
    prefetchEnd = prefetchDegree = 0;
    looplen = 0;
}
/*
Application::Application()
{

}
*/
//返回预取末尾块，如果0表示没有预取
unsigned long Application::arriveRequest( unsigned long reqStartBlk, int reqBlkNum )
{
    unsigned long reqEndBlk;
    unsigned long prefetchRange = 0;
    map<unsigned long, RangeItem>::iterator itrange;

    reqEndBlk = reqStartBlk + reqBlkNum - 1;
    if( (lastEnd + 1) != reqStartBlk )
    {
        type = 0;
        CN = 1;
        lastStart = reqStartBlk;
        lastEnd = reqEndBlk;
        prefetchEnd = prefetchDegree = 0;
    }
    else
    {
        switch(type)
        {
        case 0:
            CN++;
            assert( CN <= SEQT );
            if( CN == LOOPT )
            {
                itrange = loops.find( lastStart );
                if( (itrange != loops.end()) && (reqEndBlk < itrange->second.addrEnd) )
                {
                    type = 3;
                    //预取looplen长度, looplen = [i].end - [i].start
                    assert( prefetchEnd == 0 );
                    prefetchEnd = prefetchRange = itrange->second.addrEnd;    //等到超过loop长度，变成seq时才改变prefetchDegree
                    looplen = itrange->second.addrEnd - itrange->second.addrStart + 1;
                    break;
                }
                itrange = seqs.find( lastStart );
                if( (itrange != seqs.end()) && (reqEndBlk < itrange->second.addrEnd) )
                {
                    type = 2;
                    RangeItem titem;
                    titem.addrStart = lastStart;
                    titem.addrEnd = reqEndBlk;       //loop length is adding
                    titem.newlpend = itrange->second.addrEnd;
                    titem.num = CN;
                    loops.insert( make_pair(lastStart, titem));
                    looplen = reqEndBlk - lastStart + 1;
                    //预取，首次
                    assert( prefetchEnd == 0 );
                    prefetchDegree = reqBlkNum;
					if( prefetchDegree > PrefetchLimit ) prefetchDegree = PrefetchLimit;
                    prefetchEnd = prefetchRange = reqEndBlk + prefetchDegree;
                    break;
                }
            } //CN < 8, do nothing, move the "lastEnd++" to the last position.
            if( CN >= SEQT )    //identify the sequential accesses (>2)
            {
                type = 1;
                RangeItem titem;
                titem.addrStart = lastStart;
                titem.addrEnd = reqEndBlk;
                titem.newlpend = 0;
                titem.num = CN;
                seqs.insert( make_pair(lastStart, titem) );
                //预取，首次
                assert( prefetchEnd == 0 );
                prefetchDegree = reqBlkNum;
				if( prefetchDegree > PrefetchLimit ) prefetchDegree = PrefetchLimit;
                prefetchEnd = prefetchRange = reqEndBlk + prefetchDegree;
            }
            break;
        case 1:
            itrange = seqs.find( lastStart );   //current sequential access
            assert(itrange->second.addrEnd < reqEndBlk);
            itrange->second.addrEnd = reqEndBlk;
            itrange->second.num++;
            //是否超过之前预取的范围，如果是，那么再次预取
            if( reqEndBlk > prefetchEnd )  //>=异步，>同步
            {
                assert( prefetchDegree != 0 );
                prefetchDegree *= PrefetchSpeedup;
				if( prefetchDegree > PrefetchLimit ) prefetchDegree = PrefetchLimit;
                prefetchEnd = prefetchRange = reqEndBlk + prefetchDegree;
            }
            break;
        case 2:    //looping识别状态, 连续序列之前是sequenc.
            itrange = loops.find( lastStart );
            if( reqStartBlk < (itrange->second.newlpend + 1) )
            {
                itrange->second.addrEnd = reqEndBlk;   //looping is adding
                itrange->second.num++;
                looplen = reqEndBlk - itrange->second.addrStart + 1;
            }
            else
            {
                itrange->second.newlpend = reqEndBlk;
            }
            //是否超过之前预取的范围，如果是，那么再次预取; 没有变成Loop，仍然按照seq处理
            if( reqEndBlk > prefetchEnd )
            {
                assert( prefetchDegree != 0 );
                prefetchDegree *= PrefetchSpeedup;
				if( prefetchDegree > PrefetchLimit ) prefetchDegree = PrefetchLimit;
                prefetchEnd = prefetchRange = reqEndBlk + prefetchDegree;
            }
            break;
        case 3:
            itrange = loops.find( lastStart );  //current looping access
            //if( reqStartBlk < itrange.second.addrEnd + 1
            if( reqStartBlk > (itrange->second.addrEnd ) )
            {
                if( reqStartBlk < (itrange->second.newlpend + 1) ) //looping is adding
                {
                    itrange->second.addrEnd = reqEndBlk;
                    itrange->second.num++;
                    looplen = reqEndBlk - itrange->second.addrStart + 1;
                }
                else //if( reqStartBlk >= (itrange->second.newlpend + 1) )
                {
                    itrange->second.newlpend = reqEndBlk;
                }
            }
            //一起处理，prefetchDegree=0：表示第一次超过loop范围，需要执行预期。否则，表示
            if( reqEndBlk > prefetchEnd )
            {
                if( prefetchDegree == 0 ) prefetchDegree = reqBlkNum;
                else prefetchDegree *= PrefetchSpeedup;
				if( prefetchDegree > PrefetchLimit ) prefetchDegree = PrefetchLimit;
                prefetchEnd = prefetchRange = reqEndBlk + prefetchDegree;
            }
            break;
        default:
            cout<<"error : no this type access pattern"<<endl;
            exit(1);
            break;
        }
        lastEnd = reqEndBlk;
    }
//prefetchRange为0表示不执行预期，或是因为random，或是因为还没有超过之前的预期范围；prefetchRange不为0，表示预取的结束地址
    return prefetchRange;
}
