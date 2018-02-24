#ifndef PARTITION_H
#define PARTITION_H

#include <deque>
#include <map>
#include "buflist.h"

using namespace std;

class Partition
{
public:
    int id;
    long lowBound;
    unsigned long ranMin;
    unsigned long ranMax;
    double ranPG;
    double seqPG;
    double avgPerf;
    double lastPerf;
    double Tdisk;
    double Tseek;
    double Ttran;
    Buf* current;
    BufList bufPart;
    struct disksim_interface *disksim;
    Partition( int id );
private:

};

#endif












