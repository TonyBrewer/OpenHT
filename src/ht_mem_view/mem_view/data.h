/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#ifndef DATA_H
#define DATA_H

#include    <map>
#include    <string>
using namespace std;
#include    "MainWindow.h"
#include    "/usr/include/stdint.h"             // needed for uint64_t etc
#include    <stdio.h>                           // HtMemTrace needs this
#include    "../../../ht_lib/sysc/HtMemTrace.h"

#define L_MAX       0
#define L_AVG       1
#define LATENCY     L_MAX

using namespace Ht;



class DataItem {
    public:

    typedef enum {RD, WR } MemOp;

    DataItem(int ctlr, int bank, MemOp op, int64_t startTime, int64_t retireTime,
             int64_t address, int src);


    static bool     readData(string &fileName, uint64_t &coprocReq, uint64_t &hostReq);
    static bool     readDataFile(string &fileName, uint64_t &coprocReq, uint64_t &hostReq);
    static void     cleanup();

    int             getCtlr()       { return(ctlr); }
    int             getBank()       { return(bank); }
    int64_t         getStartTime()  { return(startTime); }
    int64_t         getRetireTime() { return(retireTime); }
    MemOp           getMemOp()      { return(memOp); }
    int64_t         getAddress()    { return(address); }
    void            getSrcFileLine(const char * &file, int &line);
    
    static int64_t  getFirstStart() { return(firstStart); }
    static int64_t  getLastRetire() { return(lastRetire); }
    static int      getDataPoint(int64_t time, int64_t clocks, int ctlr, int bank,
                                 DataItem **item, int &retireBytes, int &latency);
    static void     getHostData(int64_t time, int64_t delta, int &retireBytes, int &latency);
    static DataItem *getNextItem(DataItem *item);
    static DataItem *getPrevItem(DataItem *item);

    private:
        static int  generateItems(int type);
        int         ctlr;               // source memory controller
        int         bank;               // memory bank in controller
        MemOp       memOp;              // read write etc
        int64_t     startTime;          // time arrived in queue ns
        int64_t     retireTime;         // time left queue ns
        int64_t     address;
        int         src;

        static int64_t     firstStart;
        static int64_t     lastRetire;

        static map<int64_t, DataItem *>         dataMap[CTLR_MAX][BANKS];
        static unsigned short                   *ctlrBandTimeReq[CTLR_MAX][BANKS];
        static Ht::CMemTrace                        *memTraceP;
        static map<int64_t, DataItem *>         hostMap;
        static int64_t                          makeHostMapKey(int ctlr, int bank, int64_t time);
};

#endif // DATA_H
