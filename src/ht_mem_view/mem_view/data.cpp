/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT toolset located at:
 *
 * https://github.com/TonyBrewer/OpenHT
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "data.h"
#include <stdio.h>
#include "/usr/include/stdint.h"            // needed for uint64_t etc
#include "../../../ht_lib/sysc/HtMemTrace.h"
#include <QProgressDialog>

int64_t                         DataItem::firstStart = 0x7fffffffffffffff;
int64_t                         DataItem::lastRetire = 0;
map<int64_t, DataItem *>        DataItem::dataMap[CTLR_MAX][BANKS];
map<int64_t, DataItem *>        DataItem::hostMap;
unsigned short                  *DataItem::ctlrBandTimeReq[CTLR_MAX][BANKS];
CMemTrace                       *DataItem::memTraceP = 0;

#include <stdio.h>

void markTime(bool startT, const char *str);

void
DataItem::cleanup()
{
    int                                 c;
    int                                 b;
    map<int64_t, DataItem *>::iterator  itr;
    
    firstStart = 0x7fffffffffffffff;
    lastRetire = 0;
    for(c=0; c<CTLR_MAX; c++) {
        for(b=0; b<BANKS; b++) {
            for(itr = dataMap[c][b].begin(); itr != dataMap[c][b].end(); itr++) {
                delete itr->second;
            }
            dataMap[c][b].clear();
            if(ctlrBandTimeReq[c][b] != 0) {
                free(ctlrBandTimeReq[c][b]);
                ctlrBandTimeReq[c][b] = 0;
            }
        }
    }
    for(itr = hostMap.begin(); itr != hostMap.end(); itr++) {
        delete itr->second;
    }
    hostMap.clear();
}

bool
DataItem::readData(string &fileName, uint64_t &coprocReq, uint64_t &hostReq)
{
    int64_t                                 t;
    int                                     b;
    int                                     c;
    map<int64_t, DataItem *>::iterator      itr;
    int64_t                                 timeMax;
    unsigned short                          *reqP;
    int                                     cntr = 0;
    size_t                                  pos;
    string                                  fn;

    //
    //  load the data
    //
    pos = fileName.find_last_of("/\\");
    if(pos == string::npos) {
        pos = 0;
    } else {
        pos++;              // skip the /
    }
    hostReq = 0;
    fn = fileName.substr(pos);
    if(fn.compare("__internal1.htmt") == 0) {
        coprocReq = generateItems(1);
    } else if(fn.compare("__internal2.htmt") == 0) {
        coprocReq = generateItems(2);
    } else {
        if(readDataFile(fileName, coprocReq, hostReq) == false) {
            return(false);
        }
    }
    //
    //  Now generate the outstanding request map for the data
    //  NOTE: it is 0 based
    //
    StatusData status("Generating Tables...", 0, CTLR_MAX * BANKS);
    markTime(true, "Table Start");
    timeMax = lastRetire - firstStart + 1;
    for(c=0; c<CTLR_MAX; c++) {                     // controller
        for(b=0; b<BANKS; b++) {                    // bank
            status.setValue(++cntr);
            if((cntr%100)==0 && status.wasCanceled()) {
                status.clear();
                cleanup();                          // remove any data
                return(false);
            }
            unsigned short *sp;
            ctlrBandTimeReq[c][b] = sp = (unsigned short *)calloc(timeMax, sizeof(*reqP));
            map<int64_t, DataItem *>::iterator      itrEnd;
            itrEnd = dataMap[c][b].end();
            
            for(itr = dataMap[c][b].begin(); itr != itrEnd; itr++) {
                int64_t tStart = itr->second->getStartTime();
                int64_t tEnd = itr->second->getRetireTime();
                tStart -= firstStart;
                tEnd -= firstStart;
                for(t=tStart; t<= tEnd; t++) {
                    sp[t]++;
                }
            }
            
#ifdef NEVER
            for(itr = dataMap[c][b].begin(); itr != dataMap[c][b].end(); itr++) {
                for(t=itr->second->getStartTime(); t<= itr->second->getRetireTime(); t++) {
                    ctlrBandTimeReq[c][b][t - firstStart]++;
                }
            }
#endif
        }
    }
    markTime(false, "Table Stop");
    status.clear();
    return(true);
}


DataItem::DataItem(int ctlr, int bank, MemOp memOp, int64_t startTime, int64_t retireTime,
                   int64_t address, int src)
    : ctlr(ctlr), bank(bank), memOp(memOp), startTime(startTime), retireTime(retireTime),
            address(address), src(src)
{
    if(startTime < firstStart) {
        firstStart = startTime;
    }
    if(retireTime > lastRetire) {
        lastRetire = retireTime;
    }
}
//
//  generate a host data point across a time range (clocks)
//  retireBytes = # bytes that retire (divide by delta to get bw)
//  latency = sum of all that retire in this time window
//
void
DataItem::getHostData(int64_t time, int64_t delta, int &retireBytes, int &latency)
{
    map<int64_t, DataItem *>::iterator      itr;
    int64_t                                 hostKey;
    int64_t                                 stopTime = time + delta;
    int64_t                                 latencyCntr = 0;
    DataItem                                *itemP;
    
    retireBytes = 0;
    latency = 0;
    hostKey = makeHostMapKey(0, 0, time);           // left edge of time window
    itr = hostMap.lower_bound(hostKey);             // first >= time
    while(1) {
        if(itr == hostMap.end()) {
            break;                                  // at right edge of data
        }
        itemP = itr->second;
        if(itemP->getRetireTime() > stopTime) {
            break;                                  // not in time window
        }
        retireBytes += 8;                           // # bytes retired
        latency += itemP->getRetireTime() - itemP->getStartTime();
        latencyCntr++;
        itr++;
    }
    if(latencyCntr != 0) {
        latency /= latencyCntr;
    }
    
}


//
//  generate a data point across a time range (clocks)
//  if ctlr is 8, generating a point using data from all 8 controllers, b0-15 is ctlr0 b0-127
//  if ctlr 0-7, generating a point using data from a specific ctlr
//  rtn = number of requests outstanding
//  item = first item found
//  the first item found that matches is returned
//  returns the average latency
//
int                     // return requests outstanding
DataItem::getDataPoint(int64_t time, int64_t clocks, int ctlr, int bank, DataItem **item,
                       int &retireBytes, int &latency)
{
    int                                     bankStart;
    int                                     requests = 0;
    int                                     ctlrRequests = 0;
    int64_t                                 t;
    int64_t                                 maxRt;
    int64_t                                 latencyR;
    DataItem                                *itemP = 0;
    DataItem                                *ctlrItemP = 0;
    map<int64_t, DataItem *>::iterator      itr;
    int                                     localBR;
    int                                     localLA;
    int                                     latencyCtr;
    
    requests = 0;
    retireBytes = 0;
    latency = 0;
    latencyCtr = 0;
    
    if(lastRetire == 0) {                   // no file loaded yet
       itemP = 0;
       requests = 0;
    } else if(ctlr <= 7) {
        //
        //  getting data for a single memory controller dataMap[CTLR_MAX][BANKS]
        //  must get the max across clocks
        //  return the first item
        //
        maxRt = -1;
        unsigned short *fp = ctlrBandTimeReq[ctlr][bank];

        int64_t tm = time - firstStart;
        int64_t tst = tm;
        int64_t tsp = tm + clocks;
        
        for(t=tst; t<tsp; t++) {
            if(fp[t] > requests) {
                requests = fp[t];
                maxRt = t - tm + time;
            }
        }
#ifdef NEVER
        for(t=0; t<clocks; t++) {
            if(fp[t + time - firstStart] > requests) {
                requests = fp[t + time - firstStart];
                maxRt = t + time;
            }
        }
#endif
        //
        //  now generate the latency
        //  start at end time and work backwards
        //
        map<int64_t, DataItem *>::iterator      itr;
        map<int64_t, DataItem *>                *map;
        
        map = &dataMap[ctlr][bank];
        itr = map->lower_bound(time + clocks);              // first >= t
        if(itr == map->end() || itr->second->getRetireTime() > time + clocks) {
            if(itr != map->begin()) {
                itr--;                                      // point to a valid one
            }
        }
        while(itr != map->begin() && itr->second->getRetireTime() > time) {
            retireBytes += 8;
            latencyR = itr->second->getRetireTime() - itr->second->getStartTime();
            if(LATENCY == L_MAX) {
                if(latencyR > latency) {
                    latency = latencyR;                     // return max
                }
                latencyCtr = 1;
            } else {
                latency += latencyR;                        // return avg
                latencyCtr++;
            }
            itr--;
        }
        if(latencyCtr) {
            latency /= latencyCtr;                          // cntr=1 for max
        }

        if(maxRt >= 0) {
            itr = dataMap[ctlr][bank].upper_bound(maxRt - 1);     // need minus, only finds >
        }
        if(maxRt < 0 || itr == dataMap[ctlr][bank].end()) {
            itemP = 0;
        } else {
            itemP = itr->second;
        }
    } else if(ctlr == CTLR_ALL_B) {                      // all_b
        //
        //  getting data for all memory controllers, merge 128 lines into 16 (8:1)
        //  bank0=ctlr_0-7 bank 0   bank1=ctlr_0-7 bank 1 ...
        //
        for(ctlr=0; ctlr<8; ctlr++) {
            ctlrRequests = getDataPoint(time, clocks, ctlr, bank, &ctlrItemP, localBR, localLA);
            if(ctlrRequests > requests) {
                requests = ctlrRequests;
                itemP = ctlrItemP;
            }
            retireBytes += localBR;
            if(localLA) {
                latencyCtr++;
            }
            latency += localLA;
        }
        if(latencyCtr != 0) {
            latency /= latencyCtr;
        }
    } else {                                    // all_c
        //
        //  getting data for all memory controllers, merge 128 lines into 16 (8:1)
        //  bank0=ctlr_0 banks0-7   bank1=ctlr_0 banks 8-15 ...
        //
        ctlr = bank / 16;
        bankStart = (bank % 16) * 8;            // what bank to start with
        for(bank=0; bank<8; bank++) {
            ctlrRequests = getDataPoint(time, clocks, ctlr, bank + bankStart, &ctlrItemP, localBR, localLA);
            if(ctlrRequests > requests) {
                requests = ctlrRequests;
                itemP = ctlrItemP;
            }
            retireBytes += localBR;
            if(localLA) {
                latencyCtr++;
            }
            latency += localLA;
        }
        if(latencyCtr != 0) {
            latency /= latencyCtr;
        }
    }
    *item = itemP;
    return(requests);
}

DataItem *
DataItem::getPrevItem(DataItem *itemP)
{
    int                                     ctlr;
    int                                     bank;
    int64_t                                 rTime;
    map<int64_t, DataItem *>::iterator      itr;

    ctlr = itemP->ctlr;
    bank = itemP->bank;
    rTime = itemP->retireTime;
    itr = dataMap[ctlr][bank].find(rTime);
    if(itr == dataMap[ctlr][bank].end()) {
        printf("curr item not found(<)\n"); fflush(stdout);
    }
    itr--;
    itemP = (itr == dataMap[ctlr][bank].begin()) ? 0 : itr->second;
    return(itemP);
}

DataItem *
DataItem::getNextItem(DataItem *itemP)
{
    int                                     ctlr;
    int                                     bank;
    int64_t                                 rTime;
    map<int64_t, DataItem *>::iterator      itr;

    ctlr = itemP->ctlr;
    bank = itemP->bank;
    rTime = itemP->retireTime;
    itr = dataMap[ctlr][bank].find(rTime);
    if(itr == dataMap[ctlr][bank].end()) {
        printf("curr item not found(>)\n"); fflush(stdout);
    }
    itr++;
    itemP = (itr == dataMap[ctlr][bank].end()) ? 0 : itr->second;

    return(itemP);
}

bool
DataItem::readDataFile(string &fileName, uint64_t &coprocReq, uint64_t &hostReq)
{
    DataItem    *dataItem;
    const char        *file;
    int         ae, unit, mc, bank, op, src, line, host;
    uint64_t    startTime, compTime, addr;
    uint64_t    reqCnt;
    uint64_t    actCnt = 0;
    uint64_t    hostKey;
   
    coprocReq = 0;
    hostReq = 0;
    if(memTraceP == 0) {
        memTraceP = new Ht::CMemTrace;
    }
    memTraceP->Open(CMemTrace::Read, fileName.c_str());
    reqCnt = memTraceP->GetMemReqCount();

    StatusData status("Reading Memory Trace...", 0, reqCnt);

    while (memTraceP->ReadReq(ae, unit, mc, bank, op, src, host, startTime, compTime, addr)) {
	if(0 && !memTraceP->LookupFileLine(src, file, line)) {
	    printf("bad LookupFileLine\n");
	}
	if(0) {
	    printf("ae=%d, uint=%d, mc=%d, bank=%d, op=%d, host=%d, startTime=%ld, compTime=%ld, addr=%016lx\n",
		ae, unit, mc, bank, op, host, startTime, compTime, addr);
	    printf("    file=%s, line=%d  req=%ld\n", file, line, reqCnt);
	    fflush(stdout);
	}
	actCnt++;
	if((actCnt % 1000) == 0) {
	    status.setValue(actCnt);
	    if(status.wasCanceled()) {
		cleanup();		    // remove any data
		return(false);
	    }
	}
	if(llabs(compTime) > 1000000
		|| llabs(startTime) > 1000000
		|| startTime > compTime) {
	    //printf("data dropped\n");
	    continue;			    // ignore bad data
	}
	if(host != 0) {
	    hostReq++;
	    dataItem = new DataItem(mc, bank, (MemOp)op, startTime, compTime, addr, src);
	    hostKey = makeHostMapKey(mc, bank, compTime);
	    hostMap[hostKey] = dataItem;
	} else {
	    coprocReq++;
	    dataItem = new DataItem(mc, bank, (MemOp)op, startTime, compTime, addr, src);
	    dataMap[mc][bank][compTime] = dataItem;
	}
    }
    status.clear();
    memTraceP->Close();
    return(true);
}

int64_t
DataItem::makeHostMapKey(int ctlr, int bank, int64_t time)
{
    int64_t     hostKey;
    
    hostKey = (time << 9) | (ctlr << 7) | (bank & 0x7f);
    return(hostKey);
}

void
DataItem::getSrcFileLine(const char * &fileName, int &line)
{
    static char    *internal = (char *)"__internal";
    static char   *error = (char *)"bad file line lookup";
    
    if(memTraceP == 0) {
        fileName = internal;
        line = 0;
    } else if(memTraceP->LookupFileLine(src, fileName, line) == false) {
        fileName = error;
        line = 0;
    }
}


#include <math.h>
DataItem    *generateDataItem(int ctlr, int bank, int64_t time, int type);

int
DataItem::generateItems(int type)
{
    int         c;
    int         b;
    int         t;
    DataItem    *dataItem;
    int         items = 0;
    
    for(c=0; c<CTLR_MAX; c++) {
        for(b=0; b<BANKS; b++) {
            for(t=0; t<=1000; t++) {
                dataItem = generateDataItem(c, b, t, type);
                if(dataItem != 0) {
                    dataMap[c][b][dataItem->retireTime] = dataItem;
                    items++;
                }
            }
        }
    }
    return(items);
}
DataItem *
generateDataItem(int ctlr, int bank, int64_t time, int type)
{
    DataItem        *dataItem = 0;
    static int64_t address = 0;
    int             k;                  // k=0,1 no overlap, k=2 overlap 1  k=3 overlap 2 ...
    float           f, v;
    DataItem::MemOp memOp;

    if(type == 1) {
        memOp = (address & 0x01) ? DataItem::RD : DataItem::WR;
        k = 10;
        f = v = 10;
        if((bank == 0 || bank == 127) && (time == 10 || time == 1000) && ctlr == 0) {
            dataItem = new DataItem(ctlr, bank, memOp, time-k, time, address++, 2);
        } else if(bank >= 10 && bank <= 20 && time >= 500 && time <= 520 && ctlr == 2) {
           dataItem = new DataItem(ctlr, bank, memOp, time-k, time, address++, 2);
        } else if(ctlr == 3 && (bank%16) == 15 && time < 20) {
            dataItem = new DataItem(ctlr, bank, memOp, time-k, time, address++, 2);
        } else {
            dataItem = 0;
        }

    } else if(type == 2) {
    
        f = (3.1415 / (float)X_MAX) * (time + bank + ctlr * 8);
        v = sinf(f);                      // 0->1->0->-1->0
        v *= 10.0;                        // 0->10->0->-10->0
        if(v < 0) {
            k = -1;
        } else if(v < 2) {
            k = 0;
        } else if(v < 4) {
            k = 1;
        } else if(v < 6) {
            k = 2;
        } else {
            k = 3;
        }
        k += 1;
    
        if(k > 0) {
            memOp = (address & 0x01) ? DataItem::RD : DataItem::WR;
            dataItem = new DataItem(ctlr, bank, memOp, time, time+k, address++, 2);
        } else {
            dataItem = 0;
        }
    }

    return(dataItem);
}

