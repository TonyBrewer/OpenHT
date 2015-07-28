/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT memcached application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#include "Ht.h"
#include "PersPkt.h"

void
CPersPkt::PersPkt() {

    if (!RecvMsgBusy_accPop()) {
        ht_uint4 idx = (ht_uint4)RecvMsg_accPop();
        S_accQueCnt[idx] += 1;
    }
    T1_accQueFull = S_accQueCnt[cid2que(P1_connId)] == 0;

    // stage 1
    // ugly hack because nonstandard types don't seem to work with global
    //CAccelWord cR;
    //for (int i=0; i<ACCEL_CONN_SIZE; i++)
    //    cR.m_word[i] = GR1_connList_pConn(i);
    //T1_pConn = cR.m_pConn;
    T1_pConn = GR1_connList;

    // pull these terms out for pipelining
    ht_attrib(register_balancing, CPersPkt, "no");
    ht_attrib(equivalent_register_removal, r_t7__STG__parsePos, "no"); //r_t6_parsePos
    ht_attrib(equivalent_register_removal, r_t7__STG__pConn, "no"); //r_t6_pConn
    T1_parsePos = T1_pConn.m_parsePos;
    T1_parsePosInc = T1_pConn.m_parsePos + 1;
    T1_parsePosDec = T1_pConn.m_parsePos - 1;
    T1_parsePosInc8 = T1_pConn.m_parsePos + 8;
    T1_parsePosIncIs24 = T1_parsePosInc == 24;
    T1_parsePosIncIs8 = T1_parsePosInc == 8;
    T1_parsePosIncIs7 = T1_parsePosInc == 7;
    T1_parsePosIncIs6 = T1_parsePosInc == 6;
    T1_parsePosIncIs5 = T1_parsePosInc == 5;
    T1_parsePosIncIs4 = T1_parsePosInc == 4;
    T1_parsePosIncIs3 = T1_parsePosInc == 3;
    T1_parsePosIs0 = T1_pConn.m_parsePos == 0;
    T1_parsePosIs1 = T1_pConn.m_parsePos == 1;
    T1_parsePosIs2 = T1_pConn.m_parsePos == 2;
    T1_keyLenGtMax =  (P1_keyLen + 1) > KEY_MAX_LENGTH;

    T1_pConnBytes = ntohl(T1_pConn.m_binReqHdr.m_hdr.m_bodyLen) -
            ntohs(T1_pConn.m_binReqHdr.m_hdr.m_keyLen) -
            T1_pConn.m_binReqHdr.m_hdr.m_extraLen;
    T1_bytesEqlZero = T1_pConnBytes == 0;

    uint32_t c_skipLenB_0 = T1_pConn.m_bytes - (uint32_t)T1_pConn.m_parsePos;
    uint16_t c_skipLen = (uint16_t)(P1_pktInfo.m_pktLen - P1_chCnt);
    T1_restartAsciiPrefetch = c_skipLenB_0 > 64 && c_skipLen > 64;
    T1_skipAsciiData = c_skipLenB_0 > 2 && c_skipLen > 0;
    T1_skipBinData = c_skipLenB_0 > 64 && c_skipLen > 64;

    // uglyness in attempt to make timing
    T1_skipLenB = c_skipLen;
    if (c_skipLenB_0 < (uint32_t)c_skipLen)
        T1_skipLenB = (uint16_t)c_skipLenB_0;
    T1_chCntDataIncB = P1_chCnt + T1_skipLenB;
    T1_pChDataIncB = P1_pCh + (ht_uint20)T1_skipLenB;
    // skip data, but not \r\n
    uint32_t c_skipLenA_0 = T1_pConn.m_bytes - (uint32_t)T1_pConn.m_parsePos - 1;
    T1_skipLenA = c_skipLen;
    if (c_skipLenA_0 < (uint32_t)c_skipLen)
        T1_skipLenA = (uint16_t)c_skipLenA_0;
    T1_chCntDataIncA = P1_chCnt + T1_skipLenA;
    T1_pChDataIncA = P1_pCh + (ht_uint20)T1_skipLenA;

    T1_parsePosIsBytes = T1_parsePosInc == T1_pConn.m_bytes;
    T1_parsePosIsBytesDec = T1_parsePosInc == T1_pConn.m_bytes-1;
    T1_parsePosIsKeyLen = T1_parsePosInc == T1_pConn.m_binReqHdr.m_hdr.m_keyLen;


    T1_pChInc = P1_pCh + 1;
    T1_pChInc8 = P1_pCh + 8;
    T1_chCntInc = P1_chCnt + 1;
    T1_chCntInc8 = P1_chCnt + 8;

    T1_chCntIncEqlPktLen = T1_chCntInc == P1_pktInfo.m_pktLen;
    T1_chCntEqlPktLen64 = (((ht_uint20)P1_prefetch.m_lastRead | 0x3f) >= P1_pChMax) &&
            (P1_prefetch.m_prefetch[0] || P1_prefetch.m_prefetch[1] ||
            P1_prefetch.m_valid[0] || P1_prefetch.m_valid[1]);

    T1_recvBufPktIdx = (ht_uint20)((ht_uint20)P1_pktInfo.m_pPkt + P1_chCnt);
    T1_pktLenGr16 = P1_pktInfo.m_pktLen > (uint16_t)16;

    ht_uint36 c_flagsMul10 = (ht_uint36)((ht_uint36)T1_pConn.m_flags<<3)+
            (ht_uint36)((ht_uint36)T1_pConn.m_flags<<1);
    T1_flagsWillOF = (c_flagsMul10>>32) != 0;
    T1_flagsMul10 = (uint32_t)c_flagsMul10;
    ht_uint36 c_expMul10 = (ht_uint36)((ht_uint36)T1_pConn.m_exptime<<3)+
            (ht_uint36)((ht_uint36)T1_pConn.m_exptime<<1);
    T1_expWillOF = (c_expMul10>>32) != 0;
    T1_expMul10 = (uint32_t)c_expMul10;
    ht_uint36 c_bytesMul10 = (ht_uint36)((ht_uint36)T1_pConn.m_bytes<<3)+
            (ht_uint36)((ht_uint36)T1_pConn.m_bytes<<1);
    T1_bytesWillOF = (c_bytesMul10>>32) != 0;
    T1_bytesMul10 = (uint32_t)c_bytesMul10;
    // htv doesn't like biguint so do this the hard way
//    sc_biguint<68> c_deltaMul10 = (sc_biguint<68>)((sc_biguint<68>)T1_pConn.m_delta<<3)+
//            (sc_biguint<68>)((sc_biguint<68>)T1_pConn.m_delta<<1);
//    T1_deltaWillOF = (c_deltaMul10>>64) != 0;
//    T1_deltaMul10 = c_deltaMul10.to_uint64();
    ht_uint33 c_deltaMul10Ls = (ht_uint33)(uint32_t)(T1_pConn.m_delta<<3)+
            (ht_uint33)(uint32_t)(T1_pConn.m_delta<<1);
    ht_uint36 c_deltaMul10 = (ht_uint36)(T1_pConn.m_delta>>29) +
            (ht_uint36)(T1_pConn.m_delta>>31) +
            (ht_uint36)(c_deltaMul10Ls>>32);
    T1_deltaWillOF = (c_deltaMul10>>32) != 0;
    T1_deltaMul10 = (uint64_t)(c_deltaMul10<<32) |
            (uint64_t)(c_deltaMul10Ls & 0x0ffffffffll);
    // these shared memories always read from connAddr
    S_hashResA.read_addr(PR1_connAddr);
    S_hashResB.read_addr(PR1_connAddr);
    S_hashResC.read_addr(PR1_connAddr);

    T1_eval = true;
    T1_ch = 0;
    T1_first64 = (P1_chCnt | (uint16_t)0x03f) == (uint16_t)0x03f;

    T1_prefetchData = GR1_prePktDat;
    if (PR1_htValid) {
      if(PR1_htInst != PKT_ENTRY &&
	 PR1_htInst != PKT_RETURN &&
	 PR1_htInst != PKT_MEMWAIT &&
	 PR1_htInst != PKT_PREFETCH) {
	T1_eval = getNextChar(PR1_pCh,
			      PR1_pktInfo.m_pPkt,
			      T1_first64,
			      PR1_htId,
			      T1_prefetchData,
			      GR1_rdPktDat,
			      P1_prefetch,
			      T1_ch);
      }
    }
    
    ht_attrib(equivalent_register_removal, r_t7__STG__ch, "no"); //r_t6_ch
    ht_attrib(equivalent_register_removal, r_t7__STG__chMisc, "no"); //r_t6_chMisc
    T1_chMisc = T1_ch;
    T1_chIsLF = T1_ch == '\n';
    T1_chIsCR = T1_ch == '\r';
    T1_chIsSP = T1_ch == ' ';
    T1_chIsCee = T1_ch == 'c';
    T1_chIsPee = T1_ch == 'p';
    T1_chIsDigit = isdigit(T1_ch);

    T1_chMatchTouch[0] = T1_ch == 't';
    T1_chMatchTouch[1] = T1_ch == 'o';
    T1_chMatchTouch[2] = T1_ch == 'u';
    T1_chMatchTouch[3] = T1_ch == 'c';
    T1_chMatchTouch[4] = T1_ch == 'h';
    T1_chMatchTouch[5] = T1_ch == ' ';
    T1_chMatchIncr[0] = T1_ch == 'i';
    T1_chMatchIncr[1] = T1_ch == 'n';
    T1_chMatchIncr[2] = T1_ch == 'c';
    T1_chMatchIncr[3] = T1_ch == 'r';
    T1_chMatchIncr[4] = T1_ch == ' ';
    T1_chMatchDelete[0] = T1_ch == 'd';
    T1_chMatchDelete[1] = T1_ch == 'e';
    T1_chMatchDelete[2] = T1_ch == 'l';
    T1_chMatchDelete[3] = T1_ch == 'e';
    T1_chMatchDelete[4] = T1_ch == 't';
    T1_chMatchDelete[5] = T1_ch == 'e';
    T1_chMatchDelete[6] = T1_ch == ' ';
    T1_chMatchDecr[0] = T1_ch == 'd';
    T1_chMatchDecr[1] = T1_ch == 'e';
    T1_chMatchDecr[2] = T1_ch == 'c';
    T1_chMatchDecr[3] = T1_ch == 'r';
    T1_chMatchDecr[4] = T1_ch == ' ';
    T1_chMatchSet[0] = T1_ch == 's';
    T1_chMatchSet[1] = T1_ch == 'e';
    T1_chMatchSet[2] = T1_ch == 't';
    T1_chMatchSet[3] = T1_ch == ' ';
    T1_chMatchAdd[0] = T1_ch == 'a';
    T1_chMatchAdd[1] = T1_ch == 'd';
    T1_chMatchAdd[2] = T1_ch == 'd';
    T1_chMatchAdd[3] = T1_ch == ' ';
    T1_chMatchAppend[0] = T1_ch == 'a';
    T1_chMatchAppend[1] = T1_ch == 'p';
    T1_chMatchAppend[2] = T1_ch == 'p';
    T1_chMatchAppend[3] = T1_ch == 'e';
    T1_chMatchAppend[4] = T1_ch == 'n';
    T1_chMatchAppend[5] = T1_ch == 'd';
    T1_chMatchAppend[6] = T1_ch == ' ';
    T1_chMatchReplace[0] = T1_ch == 'r';
    T1_chMatchReplace[1] = T1_ch == 'e';
    T1_chMatchReplace[2] = T1_ch == 'p';
    T1_chMatchReplace[3] = T1_ch == 'l';
    T1_chMatchReplace[4] = T1_ch == 'a';
    T1_chMatchReplace[5] = T1_ch == 'c';
    T1_chMatchReplace[6] = T1_ch == 'e';
    T1_chMatchReplace[7] = T1_ch == ' ';
    T1_chMatchPrepend[0] = T1_ch == 'p';
    T1_chMatchPrepend[1] = T1_ch == 'r';
    T1_chMatchPrepend[2] = T1_ch == 'e';
    T1_chMatchPrepend[3] = T1_ch == 'p';
    T1_chMatchPrepend[4] = T1_ch == 'e';
    T1_chMatchPrepend[5] = T1_ch == 'n';
    T1_chMatchPrepend[6] = T1_ch == 'd';
    T1_chMatchPrepend[7] = T1_ch == ' ';
    T1_chMatchCas[0] = T1_ch == 'c';
    T1_chMatchCas[1] = T1_ch == 'a';
    T1_chMatchCas[2] = T1_ch == 's';
    T1_chMatchCas[3] = T1_ch == ' ';
    T1_chMatchGet[0] = T1_ch == 'g';
    T1_chMatchGet[1] = T1_ch == 'e';
    T1_chMatchGet[2] = T1_ch == 't';
    T1_chMatchGet[3] = T1_ch == ' ';
    T1_chMatchGet[4] = T1_ch == 'x';
    T1_chMatchGets[0] = T1_ch == 'g';
    T1_chMatchGets[1] = T1_ch == 'e';
    T1_chMatchGets[2] = T1_ch == 't';
    T1_chMatchGets[3] = T1_ch == 's';
    T1_chMatchGets[4] = T1_ch == ' ';
    T1_chMatchNoreply[0] = T1_ch == 'n';
    T1_chMatchNoreply[1] = T1_ch == 'o';
    T1_chMatchNoreply[2] = T1_ch == 'r';
    T1_chMatchNoreply[3] = T1_ch == 'e';
    T1_chMatchNoreply[4] = T1_ch == 'p';
    T1_chMatchNoreply[5] = T1_ch == 'l';
    T1_chMatchNoreply[6] = T1_ch == 'y';
    T1_chMatchNoreply[7] = T1_ch == ' ';

    if (PR1_htValid && PR1_htInst == PKT_ENTRY) {
        // initialize prefetch buffers
        P1_prefetch.m_prefetch[0] = false;
        P1_prefetch.m_prefetch[1] = false;
        P1_prefetch.m_lastRead = P1_pktInfo.m_pPkt;
        P1_prefetch.m_pauseComplete = false;
        P1_prefetch.m_checkReady = false;
        switch ((P1_pktInfo.m_pPkt >> 6) & 0x1) {
        case 0:
        {
            P1_prefetch.m_valid[0] = true;
            P1_prefetch.m_valid[1] = false;
            break;
        }
        case 1:
        {
            P1_prefetch.m_valid[0] = false;
            P1_prefetch.m_valid[1] = true;
            break;
        }
        }
    }

    //
    // Stage 2
    if (PR2_htValid) {
#ifndef _HTV
        extern FILE *tfp;
        if (tfp && false)
            fprintf(tfp, "PKT: htId=%d htInst=%d\n",
                    (int)PR2_htId, (int)PR2_htInst);
#endif
        P2_prefetch.m_pauseComplete=false;
        if(!P2_prefetch.m_checkReady &&
                PR2_htInst != PKT_ENTRY && PR2_htInst != PKT_RETURN &&
                PR2_htInst != PKT_MEMWAIT) {
            prefetch(PR2_pCh, PR2_htId,
                     T2_chCntEqlPktLen64, P2_prefetch);
        }
    }

    if(PR2_htValid && PR2_htInst != PKT_PREFETCH && P2_prefetch.m_checkReady) {
#ifndef _HTV
        extern FILE *tfp;
        if (tfp && false)
            fprintf(tfp, "PKT: checkReady htId=%d htInst=%d\n",
                    (int)PR2_htId, (int)PR2_htInst);
#endif
        if (ReadMemBusy()) {
            HtRetry();
	} else {
            // after coming back from a pause memory will be valid
            P2_prefetch.m_pauseComplete = true;
            ReadMemPause(T2_pConn.m_parseState);
        }

    }

    bool accPushBusy = SendMsgBusy_accPush() || T2_accQueFull;

    T2_hValid = false;
    P2_pktHtId = PR2_htId;
    T2_fValid = false;
    T2_pConnAddr = PR2_connAddr;
    T2_htValid = PR2_htValid;

    // the following uglyness is for timing....
    if (PR2_htInst == CNY_PS_BIN_DATA) {
        //T2_pChDataIncLast = P2_pktInfo.m_pPkt + (MemAddr_t)T2_chCntDataIncB - 64;
        T2_pChDataIncLast = (MemAddr_t)(T2_pChDataIncB - 64) | (P2_pktInfo.m_pPkt & 0xfffffff00000ll);
        T2_parsePosDataInc = T2_parsePos + (uint32_t)T2_skipLenB;
        T2_dataLenInc = (ht_uint14)(P2_dataLen + (ht_uint20)T2_skipLenB);
        T2_parsePosIsBytesData = T2_parsePosDataInc == T2_pConn.m_bytes;
        T2_chCntDataInc = T2_chCntDataIncB - 0;
        T2_pChDataInc = T2_pChDataIncB - 0;
        T2_chCntEqlPktLenData= T2_chCntDataInc == P2_pktInfo.m_pktLen;

    } else {
        //T2_pChDataIncLast = P2_pktInfo.m_pPkt + (MemAddr_t)T2_chCntDataIncA - 65;
        T2_pChDataIncLast = (MemAddr_t)(T2_pChDataIncA - 65) | (P2_pktInfo.m_pPkt & 0xfffffff00000ll);
        T2_parsePosDataInc = T2_parsePosDec + (uint32_t)T2_skipLenA;
        T2_dataLenInc = (ht_uint14)(P2_dataLen + (ht_uint20)T2_skipLenA - 1);
        T2_parsePosIsBytesData = T2_parsePosDataInc == T2_pConn.m_bytes;
        T2_chCntDataInc = T2_chCntDataIncA - 1;
        T2_pChDataInc = T2_pChDataIncA - 1;
        T2_chCntEqlPktLenData= T2_chCntDataInc == P2_pktInfo.m_pktLen;
    }

#ifndef _HTV
    extern FILE *tfp;
    if (tfp && PR2_htValid && 0)
        fprintf(tfp, "PKT: tid=%d inst=%d eval=%d cid=0x%04x blkIdx=%d @ %lld\n",
                (int)PR2_htId, (int)PR2_htInst, (int)T2_eval,
                (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                (long long)sc_time_stamp().value() / 10000);
#endif

    // this seems excessive but most bits are zero and registers will be ripped
    T2_entryRspCmd = 0;
    T2_entryRspCmd.m_cmd.m_connId = P2_connId;
    T2_entryRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_entryRspCmdValid = false;
    T2_bCmdRspCmd = 0;
    T2_bCmdRspCmd.m_cmd.m_connId = P2_connId;
    T2_bCmdRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_bCmdRspCmdValid = false;
    T2_bKeyRspCmd = 0;
    T2_bKeyRspCmd.m_cmd.m_connId = P2_connId;
    T2_bKeyRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_bKeyRspCmdValid = false;
    T2_bDataRspCmd = 0;
    T2_bDataRspCmd.m_cmd.m_connId = P2_connId;
    T2_bDataRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_bDataRspCmdValid = false;
    T2_aKeyRspCmd = 0;
    T2_aKeyRspCmd.m_cmd.m_connId = P2_connId;
    T2_aKeyRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_aKeyRspCmdValid = false;
    T2_aEocRspCmd = 0;
    T2_aEocRspCmd.m_cmd.m_connId = P2_connId;
    T2_aEocRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_aEocRspCmdValid = false;
    T2_aDataRspCmd = 0;
    T2_aDataRspCmd.m_cmd.m_connId = P2_connId;
    T2_aDataRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_aDataRspCmdValid = false;
    T2_retRspCmd = 0;
    T2_retRspCmd.m_cmd.m_connId = P2_connId;
    T2_retRspCmd.m_blkIndex = P2_pktInfo.m_blkIndex;
    T2_retRspCmdValid = false;

    bool bPktInfoPktLenEql0 = P2_pktInfo.m_pktLen == 0;

    if (PR2_htValid && T2_eval) {
#ifndef _HTV
        extern FILE *tfp;
        if (tfp && false)
            fprintf(tfp, "PKT: eval htId=%d htInst=%d\n",
                    (int)PR2_htId, (int)PR2_htInst);
#endif
        switch (PR2_htInst)
        {
        case PKT_ENTRY:
        {
#ifndef _HTV
	    extern FILE *tfp;
	    if (tfp)
            fprintf(tfp, "PKT: entry cid=0x%04x blkIdx=%d htId=0x%x parseState=%d transport=%d protocol=%d prefetch=0x%016llx varAddr=0x%03x @ %lld\n",
                    (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                    (int)PR2_htId,
                    (int)T2_pConn.m_parseState,
                    (int)T2_pConn.m_transport,
                    (int)T2_pConn.m_protocol,
                    (long long)T2_prefetchData,
                    (int)(P2_pktInfo.m_prefetchIdx << 3),
                    (long long)sc_time_stamp().value() / 10000);
#endif
            P2_sentPktDone=false;
            P2_restartPrefetch=false;
            P2_keyInit=false;
            P2_keyLen = 0;

            T2_entryRspCmd.m_cmd.m_keyInit = false;
            T2_entryRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;

            if (accPushBusy && bPktInfoPktLenEql0) {
                HtRetry();
                break;
            }

            if (P2_connId & 0x8000) {
                // new connection
                T2_pConn.m_transport = (Transport_t) (P2_pktInfo.m_pktLen & 0xff);
                T2_pConn.m_protocol = (Protocol_t) (P2_pktInfo.m_pktLen >> 8);
                T2_pConn.m_parsePos = 0;
                T2_pConn.m_bConnClosed = false;
                T2_pConn.m_parseState = PKT_ENTRY;
                T2_pConn.m_hash_c = 0xdeadbeef;
                HtContinue(PKT_RETURN);
#ifndef _HTV
                if (tfp)
                    fprintf(tfp, "PKT: New connection cid=0x%04x blkIdx=%d transport=%d protocol=%d @ %lld\n",
                            (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                            (int)T2_pConn.m_transport, (int)T2_pConn.m_protocol,
                            (long long)sc_time_stamp().value() / 10000);
#endif
            } else if (bPktInfoPktLenEql0) {
                //T2_pConn.m_parseState = PKT_RETURN;

                if (!T2_pConn.m_bConnClosed) {
                    T2_pConn.m_bConnClosed = true;
                    T2_entryRspCmd.m_cmd.m_ascii = false;
                    T2_entryRspCmd.m_cmd.m_cmd = CNY_CMD_ERROR;
                    T2_entryRspCmd.m_cmd.m_errCode = CNY_ERR_CONN_CLOSED;
                    P2_sentPktDone=true;
                    T2_entryRspCmdValid = true;
                    T2_entryRspCmd.m_bPktDone = true;
                    T2_entryRspCmd.m_numCmd = CNY_CMD_ERROR_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_ERROR_DW_CNT, rspOpt, rspOptDwCnt, P2_pktInfo.m_blkIndex, true);
                }
                T2_pConn.m_parseState = PKT_ENTRY;
                HtContinue(PKT_RETURN);
                break;
            } else if (T2_pConn.m_parseState == PKT_ENTRY ||
                       T2_pConn.m_transport == CNY_UDP_TRANSPORT) {
                HtContinue(PKT_NEGOTIATE);
            } else {
                HtContinue(T2_pConn.m_parseState);
            }

            if (T2_pConn.m_transport == CNY_UDP_TRANSPORT) {
                P2_chCnt = 8;
                P2_pCh = (ht_uint20)P2_pktInfo.m_pPkt + 8;
#ifndef _HTV
                if (tfp)
                    fprintf(tfp, "PKT: UDP connection cid=0x%04x blkIdx=%d parseState=%d @ %lld\n",
                            (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                            (int)T2_pConn.m_parseState,
                            (long long)sc_time_stamp().value() / 10000);
#endif
            } else {
                P2_chCnt = 0;
                P2_pCh = (ht_uint20)P2_pktInfo.m_pPkt;
            }

            T2_entryRspCmd.m_cmd.m_ascii = T2_pConn.m_protocol == CNY_ASCII_PROT;
            P2_bAscii = T2_pConn.m_protocol == CNY_ASCII_PROT;

            P2_pChMax = (ht_uint20)P2_pktInfo.m_pPkt + (ht_uint20)P2_pktInfo.m_pktLen;

            P2_keyPos = (ht_uint20)(P2_pktInfo.m_pPkt);
            P2_keyLen = 0;

            P2_dataPos = (ht_uint20)(P2_pktInfo.m_pPkt);
            P2_dataLen = 0;

        }
            break;
        case PKT_NEGOTIATE:
        {
            bool bAscii;

            //P2_pPktEnd = P2_pktInfo.m_pPkt + P2_pktInfo.m_pktLen;
            if (T2_pConn.m_transport == CNY_UDP_TRANSPORT ||
                    T2_pConn.m_protocol == CNY_NEGOTIATING_PROT) {
                uint8_t pPktByte = T2_chMisc;
                bAscii = pPktByte != (uint8_t)PROTOCOL_BINARY_REQ;

                if (!bAscii) {
                    T2_pConn.m_protocol = CNY_BINARY_PROT;
                    T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                } else {
                    T2_pConn.m_protocol = CNY_ASCII_PROT;
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;
                }

                T2_pConn.m_parsePos = 0;
            } else
                bAscii = T2_pConn.m_protocol == CNY_ASCII_PROT;

            T2_entryRspCmd.m_cmd.m_ascii = bAscii;
            P2_bAscii = bAscii;

            HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BINARY_CMD:
        {

            if(T2_parsePosIs0) {
                T2_pConn.m_initial = 0;
                T2_pConn.m_delta = 0;
                T2_pConn.m_flags = 0;
                T2_pConn.m_exptime = 0;
                T2_pConn.m_bytes = 0;
                T2_pConn.m_noreply = false;

                T2_pConn.m_hash_a = 0xdeadbeef;
                T2_pConn.m_hash_b = 0xdeadbeef;
                T2_pConn.m_hash_c = 0xdeadbeef;
                for(int i=0; i<12; i++)
                    T2_pConn.m_key12.m_byte[i] = 0;

                if (T2_chMisc != BINARY_REQ_MAGIC) {
                    HtContinue(CNY_PS_BIN_OTHER);
                    break;

                }
            }
            // FIXME: rework the logic to broadside load qwords
            if (false && T2_parsePosIs0 && (P2_chCnt == 0) && T2_pktLenGr16 &&
                    (T2_pConn.m_transport != CNY_UDP_TRANSPORT)) {
#ifndef _HTV
                if (tfp && false)
                    fprintf(tfp, "PKT: dData cid=0x%04x blkIdx=%d parsePos=%d @ %lld\n",
                            (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                            (int)T2_pConn.m_parsePos,
                            (long long)sc_time_stamp().value() / 10000);
#endif
                CByteData tData;
                // if chCnt is 0 we could do 16 but then we would have to wait for the
                // prefetched data anyway
                //tData.m_qword = P2_pktInfo.m_pktWord1;
                tData.m_qword = 0;
                for (int i=0; i<8; i++)
                    T2_pConn.m_binReqHdr.m_bytes[i] = tData.m_byte[i];
                T2_pConn.m_parsePos = T2_parsePosInc8;
                P2_pCh = T2_pChInc8;
                P2_chCnt = T2_chCntInc8;
            } else {
                T2_pConn.m_binReqHdr.m_bytes[(ht_uint5)T2_pConn.m_parsePos] = T2_chMisc;
                T2_pConn.m_parsePos = T2_parsePosInc;
                P2_pCh = T2_pChInc;
                P2_chCnt = T2_chCntInc;
            }
            if (T2_parsePosIncIs24) {

                T2_pConn.m_binReqHdr.m_hdr.m_keyLen = ntohs(T2_pConn.m_binReqHdr.m_hdr.m_keyLen);
                T2_pConn.m_binReqHdr.m_hdr.m_bodyLen = ntohl(T2_pConn.m_binReqHdr.m_hdr.m_bodyLen);
                T2_pConn.m_binReqHdr.m_hdr.m_cas = ntohll(T2_pConn.m_binReqHdr.m_hdr.m_cas);
#ifndef _HTV
                if (tfp && false)
                    fprintf(tfp, "PKT: cid=0x%04x blkIdx=%d magic=0x%x opcode=0x%x keyLen=0x%x bodyLen=0x%x cas=0x%llx @ %lld\n",
                            (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                            (int)T2_pConn.m_binReqHdr.m_hdr.m_magic,
                            (int)T2_pConn.m_binReqHdr.m_hdr.m_opcode,
                            (int)T2_pConn.m_binReqHdr.m_hdr.m_keyLen,
                            (int)T2_pConn.m_binReqHdr.m_hdr.m_bodyLen,
                            (long long)T2_pConn.m_binReqHdr.m_hdr.m_cas,
                            (long long)sc_time_stamp().value() / 10000);
#endif

                T2_pConn.m_bytes = T2_pConnBytes;

                T2_pConn.m_hashPos = 0;

                T2_pConn.m_noreply = true;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode) {
                case BIN_REQ_CMD_SETQ: break;
                case BIN_REQ_CMD_ADDQ: break;
                case BIN_REQ_CMD_REPLACEQ: break;
                case BIN_REQ_CMD_GETQ: break;
                case BIN_REQ_CMD_GETKQ: break;
                case BIN_REQ_CMD_DELETEQ: break;
                case BIN_REQ_CMD_APPENDQ: break;
                case BIN_REQ_CMD_PREPENDQ: break;
                case BIN_REQ_CMD_INCRQ: break;
                case BIN_REQ_CMD_DECRQ: break;
                case BIN_REQ_CMD_GATQ: break;
                case BIN_REQ_CMD_GATKQ: break;
                case BIN_REQ_CMD_QUITQ: break;
                default:
                    T2_pConn.m_noreply = false;
                }

                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode)
                {
                case BIN_REQ_CMD_SET:
                case BIN_REQ_CMD_ADD:
                case BIN_REQ_CMD_REPLACE:
                case BIN_REQ_CMD_SETQ:
                case BIN_REQ_CMD_ADDQ:
                case BIN_REQ_CMD_REPLACEQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_FLAGS;
                    T2_pConn.m_parsePos = 0;
                    break;
                case BIN_REQ_CMD_GET:
                case BIN_REQ_CMD_GETK:
                case BIN_REQ_CMD_DELETE:
                case BIN_REQ_CMD_GETQ:
                case BIN_REQ_CMD_GETKQ:
                case BIN_REQ_CMD_DELETEQ:
                    if (T2_bytesEqlZero) {
                        T2_pConn.m_parseState = CNY_PS_BIN_KEY;
                        T2_pConn.m_parsePos = 0;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_BIN_OTHER;
                    }
                    break;
                case BIN_REQ_CMD_APPEND:
                case BIN_REQ_CMD_PREPEND:
                case BIN_REQ_CMD_APPENDQ:
                case BIN_REQ_CMD_PREPENDQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_KEY;
                    T2_pConn.m_parsePos = 0;
                    break;
                case BIN_REQ_CMD_INCR:
                case BIN_REQ_CMD_DECR:
                case BIN_REQ_CMD_INCRQ:
                case BIN_REQ_CMD_DECRQ:
                    if (T2_bytesEqlZero) {
                        T2_pConn.m_parseState = CNY_PS_BIN_DELTA;
                        T2_pConn.m_parsePos = 0;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_BIN_OTHER;
                    }
                    break;
                case BIN_REQ_CMD_TOUCH:
                case BIN_REQ_CMD_GAT:
                case BIN_REQ_CMD_GATK:
                case BIN_REQ_CMD_GATQ:
                case BIN_REQ_CMD_GATKQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_EXPTIME;
                    T2_pConn.m_parsePos = 0;
                    break;
                default:
                    T2_pConn.m_binReqHdr.m_hdr.m_keyLen += (uint16_t)T2_pConn.m_binReqHdr.m_hdr.m_bodyLen;
                    if (T2_pConn.m_binReqHdr.m_hdr.m_keyLen > 0) {
                        T2_pConn.m_parseState = CNY_PS_BIN_KEY;
                        T2_pConn.m_parsePos = 0;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_BIN_OTHER;
                    }
                    break;
                }
#ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf(tfp, "PKT: cmd cid=0x%04x blkIdx=%d opcode=%02x done=%d @ %lld\n",
                            (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                            (int)T2_pConn.m_binReqHdr.m_hdr.m_opcode, (int)T2_chCntIncEqlPktLen,
                            (long long)sc_time_stamp().value() / 10000);
                }
#endif
            }
            P2_keyLen = 0;
            P2_keyPos = T2_recvBufPktIdx + 1;

            if (T2_chCntIncEqlPktLen &&
                    T2_pConn.m_parseState != CNY_PS_BIN_OTHER)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_OTHER:
        {
            T2_bCmdRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_bCmdRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;
            T2_bCmdRspCmd.m_cmd.m_opaque = T2_pConn.m_binReqHdr.m_hdr.m_opaque;
            bool otherChCntEqlPktLen = P2_chCnt == P2_pktInfo.m_pktLen;

            if (accPushBusy) {
                HtRetry();
                break;
            }
            if(T2_parsePosIs0) {

                T2_bCmdRspCmd.m_cmd.m_cmd = CNY_CMD_ERROR;
                T2_bCmdRspCmd.m_cmd.m_reqCmd = T2_pConn.m_binReqHdr.m_hdr.m_opcode;
                T2_bCmdRspCmd.m_cmd.m_errCode = CNY_ERR_BINARY_MAGIC;
                T2_bCmdRspCmdValid = true;
                T2_bCmdRspCmd.m_bPktDone = true;
                T2_bCmdRspCmd.m_cmd.m_opaque = 0;

                T2_bCmdRspCmd.m_numCmd = CNY_CMD_ERROR_DW_CNT;
                //WriteRspCmd(P2_rspHdr, CNY_CMD_ERROR_DW_CNT, rspOpt, rspOptDwCnt, P2_pktInfo.m_blkIndex, true);
                P2_sentPktDone=true;

                T2_pConn.m_parseState = CNY_PS_BIN_SWALLOW;
                //T2_pConn.m_bConnClosed = true;

#ifndef _HTV
                extern FILE *tfp;
                if (tfp) {
                    fprintf (stderr, "PKT: ERR_BINARY_MAGIC ch=0x%02x @ %lld\n",
                             (int)T2_ch,
                             (long long)sc_time_stamp().value() / 10000);
                }
#endif
            } else {
                T2_pConn.m_parsePos = 0;
                T2_bCmdRspCmd.m_cmd.m_valueLen = T2_pConn.m_bytes;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode)
                {
                case BIN_REQ_CMD_SET:
                case BIN_REQ_CMD_ADD:
                case BIN_REQ_CMD_REPLACE:
                case BIN_REQ_CMD_SETQ:
                case BIN_REQ_CMD_ADDQ:
                case BIN_REQ_CMD_REPLACEQ:
                    assert(0);
                    break;
                case BIN_REQ_CMD_GET:
                case BIN_REQ_CMD_GETK:
                case BIN_REQ_CMD_DELETE:
                case BIN_REQ_CMD_GETQ:
                case BIN_REQ_CMD_GETKQ:
                case BIN_REQ_CMD_DELETEQ:
                    if (T2_bytesEqlZero) {
                        assert(0);
                    } else {

                        T2_bCmdRspCmd.m_cmd.m_cmd = CNY_CMD_ERROR;
                        T2_bCmdRspCmd.m_cmd.m_reqCmd = T2_pConn.m_binReqHdr.m_hdr.m_opcode;
                        T2_bCmdRspCmd.m_cmd.m_errCode = CNY_ERR_INV_ARG;
                        T2_bCmdRspCmdValid = true;
                        T2_bCmdRspCmd.m_bPktDone = true;
                        T2_bCmdRspCmd.m_numCmd = CNY_CMD_ERROR_DW_CNT;
                        //WriteRspCmd(P2_rspHdr, CNY_CMD_ERROR_DW_CNT, rspOpt, rspOptDwCnt, P2_pktInfo.m_blkIndex, true);
                        P2_sentPktDone=true;

                        T2_pConn.m_parseState = CNY_PS_BIN_SWALLOW;
                    }
                    break;
                case BIN_REQ_CMD_APPEND:
                case BIN_REQ_CMD_PREPEND:
                case BIN_REQ_CMD_APPENDQ:
                case BIN_REQ_CMD_PREPENDQ:
                    assert(0);
                    break;
                case BIN_REQ_CMD_INCR:
                case BIN_REQ_CMD_DECR:
                case BIN_REQ_CMD_INCRQ:
                case BIN_REQ_CMD_DECRQ:
                    if (T2_bytesEqlZero) {
                        assert(0);
                    } else {

                        T2_bCmdRspCmd.m_cmd.m_cmd = CNY_CMD_ERROR;
                        T2_bCmdRspCmd.m_cmd.m_reqCmd = T2_pConn.m_binReqHdr.m_hdr.m_opcode;
                        T2_bCmdRspCmd.m_cmd.m_errCode = CNY_ERR_INV_ARG;
                        T2_bCmdRspCmdValid = true;
                        T2_bCmdRspCmd.m_bPktDone = true;
                        T2_bCmdRspCmd.m_numCmd = CNY_CMD_ERROR_DW_CNT;
                        //WriteRspCmd(P2_rspHdr, CNY_CMD_ERROR_DW_CNT, rspOpt, rspOptDwCnt, P2_pktInfo.m_blkIndex, true);
                        P2_sentPktDone=true;

                        T2_pConn.m_parseState = CNY_PS_BIN_SWALLOW;
                    }
                    break;
                case BIN_REQ_CMD_TOUCH:
                case BIN_REQ_CMD_GAT:
                case BIN_REQ_CMD_GATK:
                case BIN_REQ_CMD_GATQ:
                case BIN_REQ_CMD_GATKQ:
                    assert(0);
                    break;
                default:
                    if (T2_pConn.m_binReqHdr.m_hdr.m_keyLen > 0) {
                        assert(0);
                    } else {

                        T2_bCmdRspCmdValid = true;
                        T2_bCmdRspCmd.m_bPktDone = otherChCntEqlPktLen;
                        T2_bCmdRspCmd.m_numCmd = CNY_BIN_CMD_DW_CNT;
                        T2_bCmdRspCmd.m_cmd.m_keyInit = false;
                        T2_bCmdRspCmd.m_cmd.m_cmd = T2_pConn.m_binReqHdr.m_hdr.m_opcode | BINARY_REQ_MAGIC;
                        T2_bCmdRspCmd.m_cmd.m_cmdMs = T2_pConn.m_binReqHdr.m_hdr.m_opcode >> 7;
                        T2_bCmdRspCmd.m_cmd.m_keyLen = P2_keyLen;
                        T2_bCmdRspCmd.m_cmd.m_keyPos = P2_keyPos - 1;
                        T2_bCmdRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

                        P2_sentPktDone=otherChCntEqlPktLen;
                        P2_keyLen = 0;
                        T2_pConn.m_parseState = CNY_PS_BINARY_CMD;

                    }
                    break;
                }
            }
            P2_keyLen = 0;
            P2_keyPos = T2_recvBufPktIdx + 1;

            if (otherChCntEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_DELTA:
        {
            T2_pConn.m_delta = (uint64_t)(T2_pConn.m_delta << 8) | (uint64_t)T2_chMisc;
            T2_pConn.m_parsePos = T2_parsePosInc;
            if (T2_parsePosIncIs8) {
                T2_pConn.m_parsePos = 0;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode)
                {
                case BIN_REQ_CMD_INCR:
                case BIN_REQ_CMD_DECR:
                case BIN_REQ_CMD_INCRQ:
                case BIN_REQ_CMD_DECRQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_INIT;
                    break;
                default:
                    assert(0);
                }
            }
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_INIT:
        {
            T2_pConn.m_initial = (uint64_t)(T2_pConn.m_initial << 8) | (uint64_t)T2_chMisc;
            T2_pConn.m_parsePos = T2_parsePosInc;
            if (T2_parsePosIncIs8) {
                T2_pConn.m_parsePos = 0;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode)
                {
                case BIN_REQ_CMD_INCR:
                case BIN_REQ_CMD_DECR:
                case BIN_REQ_CMD_INCRQ:
                case BIN_REQ_CMD_DECRQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_EXPTIME;
                    break;
                default:
                    assert(0);
                }
            }
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_FLAGS:
        {
            T2_pConn.m_flags = (uint32_t)(T2_pConn.m_flags << 8) | (uint32_t)T2_chMisc;
            T2_pConn.m_parsePos = T2_parsePosInc;
            if (T2_parsePosIncIs4) {
                T2_pConn.m_parsePos = 0;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode) {
                case BIN_REQ_CMD_SET:
                case BIN_REQ_CMD_ADD:
                case BIN_REQ_CMD_REPLACE:
                case BIN_REQ_CMD_SETQ:
                case BIN_REQ_CMD_ADDQ:
                case BIN_REQ_CMD_REPLACEQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_EXPTIME;
                    break;
                default:
                    assert(0);
                }
            }
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_EXPTIME:
        {
            T2_pConn.m_exptime = (uint32_t)(T2_pConn.m_exptime << 8) | (uint32_t)T2_chMisc;
            T2_pConn.m_parsePos = T2_parsePosInc;
            if (T2_parsePosIncIs4) {
                T2_pConn.m_parsePos = 0;
                T2_pConn.m_hashPos = 0;
                switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode) {
                case BIN_REQ_CMD_SET:
                case BIN_REQ_CMD_ADD:
                case BIN_REQ_CMD_REPLACE:
                case BIN_REQ_CMD_INCR:
                case BIN_REQ_CMD_DECR:
                case BIN_REQ_CMD_TOUCH:
                case BIN_REQ_CMD_GAT:
                case BIN_REQ_CMD_GATK:
                case BIN_REQ_CMD_SETQ:
                case BIN_REQ_CMD_ADDQ:
                case BIN_REQ_CMD_REPLACEQ:
                case BIN_REQ_CMD_INCRQ:
                case BIN_REQ_CMD_DECRQ:
                case BIN_REQ_CMD_GATQ:
                case BIN_REQ_CMD_GATKQ:
                    T2_pConn.m_parseState = CNY_PS_BIN_KEY;
                    break;
                default:
                    assert(0);
                }
            }
            P2_keyLen = 0;
            P2_keyPos = T2_recvBufPktIdx + 1;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_KEY:
        case CNY_PS_BIN_MKEY :
        {
            if (PR2_htInst == CNY_PS_BIN_MKEY) {
                // last interation was a mix, get the result
                T2_pConn.m_hash_a = S_hashResA.read_mem();
                T2_pConn.m_hash_b = S_hashResB.read_mem();
                T2_pConn.m_hash_c = S_hashResC.read_mem();
                T2_pConn.m_parseState = CNY_PS_BIN_KEY;
            }
            P2_keyLen += 1;

            T2_pConn.m_key12.m_byte[T2_pConn.m_hashPos] = T2_chMisc;
            T2_pConn.m_parsePos = T2_parsePosInc;

            uint32_t c_hash_a, c_hash_b, c_hash_c;
            key2word(T2_pConn.m_key12, c_hash_a, c_hash_b, c_hash_c);

            if (T2_parsePosIsKeyLen) {
                // final
                T2_pConn.m_hash_a += c_hash_a;
                T2_pConn.m_hash_b += c_hash_b;
                T2_pConn.m_hash_c += c_hash_c;
                T2_fValid = true;
                // final(T2_pConn->m_hash_a, T2_pConn->m_hash_b, T2_pConn->m_hash_c);
                T2_pConn.m_parseState = CNY_PS_BIN_FKEY;
                HtContinue(T2_pConn.m_parseState);
                break;
            } else if (T2_pConn.m_hashPos == 11) {
                T2_pConn.m_hash_a += c_hash_a;
                T2_pConn.m_hash_b += c_hash_b;
                T2_pConn.m_hash_c += c_hash_c;
                T2_hValid = true;
                //mix(T2_pConn->m_hash_a, T2_pConn->m_hash_b, T2_pConn->m_hash_c);
                T2_pConn.m_parseState = CNY_PS_BIN_MKEY;
                for(int i=0; i<12; i++)
                    T2_pConn.m_key12.m_byte[i] = 0;
            }
            if (++T2_pConn.m_hashPos == 12)
                T2_pConn.m_hashPos = 0;
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;

            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_FKEY:
        {
            if (accPushBusy) {
                HtRetry();
                break;
            }
            T2_pConn.m_hash_c = S_hashResC.read_mem();

            T2_bKeyRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_bKeyRspCmd.m_cmd.m_keyInit = false;
            T2_bKeyRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;
            T2_bKeyRspCmd.m_cmd.m_cmd = T2_pConn.m_binReqHdr.m_hdr.m_opcode | BINARY_REQ_MAGIC;
            T2_bKeyRspCmd.m_cmd.m_keyLen = P2_keyLen;
            T2_bKeyRspCmd.m_cmd.m_keyPos = P2_keyPos;
            T2_bKeyRspCmd.m_cmd.m_valueLen = T2_pConn.m_bytes;
            T2_bKeyRspCmd.m_cmd.m_opaque = T2_pConn.m_binReqHdr.m_hdr.m_opaque;
            T2_bKeyRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;
            ht_uint3 c_numOpt = 0;

            switch (T2_pConn.m_binReqHdr.m_hdr.m_opcode)
            {
            case BIN_REQ_CMD_SET:
            case BIN_REQ_CMD_ADD:
            case BIN_REQ_CMD_REPLACE:
            case BIN_REQ_CMD_APPEND:
            case BIN_REQ_CMD_PREPEND:
            case BIN_REQ_CMD_SETQ:
            case BIN_REQ_CMD_ADDQ:
            case BIN_REQ_CMD_REPLACEQ:
            case BIN_REQ_CMD_APPENDQ:
            case BIN_REQ_CMD_PREPENDQ:
                T2_pConn.m_parseState = CNY_PS_BIN_DATA;
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_flags, 4);
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_exptime, 4);
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_binReqHdr.m_hdr.m_cas, 8);
                break;
            case BIN_REQ_CMD_INCR:
            case BIN_REQ_CMD_DECR:
            case BIN_REQ_CMD_INCRQ:
            case BIN_REQ_CMD_DECRQ:
                T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_exptime, 4);
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_initial, 8);
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_delta, 8);
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_binReqHdr.m_hdr.m_cas, 8);
                break;
            case BIN_REQ_CMD_TOUCH:
                T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_exptime, 4);
                break;
            case BIN_REQ_CMD_DELETE:
            case BIN_REQ_CMD_DELETEQ:
                T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                WriteRspValue(T2_bKeyRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_binReqHdr.m_hdr.m_cas, 8);
                break;
            case BIN_REQ_CMD_GET:
            case BIN_REQ_CMD_GETK:
            case BIN_REQ_CMD_GAT:
            case BIN_REQ_CMD_GATK:
            case BIN_REQ_CMD_GETQ:
            case BIN_REQ_CMD_GETKQ:
            case BIN_REQ_CMD_GATQ:
            case BIN_REQ_CMD_GATKQ:
            default:
                T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                break;
            }
            T2_bKeyRspCmd.m_numOpt = c_numOpt;
            T2_bKeyRspCmdValid = true;
            T2_bKeyRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
            T2_bKeyRspCmd.m_numCmd = CNY_BIN_CMD_DW_CNT;
            //WriteRspCmd(P2_rspHdr, CNY_BIN_CMD_DW_CNT, rspOpt, rspOptDwCnt,
            //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
            P2_sentPktDone=T2_chCntIncEqlPktLen;

            T2_pConn.m_parsePos = 0;
            P2_keyLen = 0;
            P2_dataPos = T2_recvBufPktIdx + 1;
            P2_dataLen = 0;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BIN_DATA:
        {
            if (accPushBusy) {
                HtRetry();
                break;
            }
            T2_bDataRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_bDataRspCmd.m_cmd.m_cmd = CNY_CMD_BIN_DATA;
            T2_bDataRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;
            T2_bDataRspCmd.m_cmd.m_dataPos = P2_dataPos;
            T2_bDataRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;
#ifndef _HTV
        if (tfp && false)
            fprintf(tfp, "PKT: bData cid=0x%04x blkIdx=%d parsePosIsBytesData=%d chCntEqlPktLenData=%d parsePos=%d nextParsePos=%d bytes=%d chCnt=%d pktLen=%d @ %lld\n",
                    (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                    (int)T2_parsePosIsBytesData, (int)T2_chCntEqlPktLenData,
                    (int)T2_pConn.m_parsePos, (int)T2_parsePosDataInc,
                    (int)T2_pConn.m_bytes, (int)P2_chCnt, (int)P2_pktInfo.m_pktLen,
                    (long long)sc_time_stamp().value() / 10000);
#endif
            T2_pConn.m_parsePos = T2_parsePosDataInc;
            P2_dataLen = (ht_uint12)T2_dataLenInc;
            P2_restartPrefetch=false;
            T2_bDataRspCmd.m_cmd.m_dataLen = P2_dataLen;

            if (T2_chCntEqlPktLenData && P2_dataLen != 0 ||
                    T2_parsePosIsBytesData) {
                // partial data parsed, send starting address and length to host
                T2_bDataRspCmdValid = true;
                T2_bDataRspCmd.m_bPktDone = T2_chCntEqlPktLenData;
                T2_bDataRspCmd.m_numCmd = CNY_CMD_DATA_DW_CNT;
                //WriteRspCmd(P2_rspHdr, CNY_CMD_DATA_DW_CNT, rspOpt, 0,
                //            P2_pktInfo.m_blkIndex, T2_chCntEqlPktLenData);
                P2_sentPktDone=T2_chCntEqlPktLenData;
            }

            if (T2_parsePosIsBytesData) {
                T2_pConn.m_parseState = CNY_PS_BINARY_CMD;
                T2_pConn.m_parsePos = 0;
            }

            if (!T2_chCntEqlPktLenData && T2_skipBinData) {
                // dump the prefetch buffer and initialize to start at new position
                P2_restartPrefetch=true;
                P2_prefetch.m_pauseComplete=false;
                P2_prefetch.m_checkReady=false;
                P2_prefetch.m_valid[0] = false;
                P2_prefetch.m_valid[1] = false;
                P2_prefetch.m_prefetch[0] = false;
                P2_prefetch.m_prefetch[1] = false;
                //fprintf(stderr,"PKT: BIN_DATA htId=0x%x pCh=0x%05x lastRead=0x%012llx ",
                //        (int)PR2_htId,
                //        (int)P2_pCh,
                //        (long long)P2_prefetch.m_lastRead);
                P2_prefetch.m_lastRead = T2_pChDataIncLast & (MemAddr_t)0xffffffffffc0ll;
                //fprintf(stderr,"nextRead=0x%012llx\n",
                 //       (long long)P2_prefetch.m_lastRead);
            }

            P2_pCh = T2_pChDataInc;
            P2_chCnt = T2_chCntDataInc;
            if (T2_chCntEqlPktLenData || P2_restartPrefetch)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
#define ASCII 1
#ifdef ASCII
        case CNY_PS_ASCII_CMD:
        {
            switch (T2_ch) {
            case 's':
                T2_pConn.m_parseState = CNY_PS_SET_CMD;
                break;
            case 'a':
                T2_pConn.m_parseState = CNY_PS_ADD_CMD;
                break;
            case 'p':
                T2_pConn.m_parseState = CNY_PS_PREPEND_CMD;
                break;
            case 'c':
                T2_pConn.m_parseState = CNY_PS_CAS_CMD;
                break;
            case 'r':
                T2_pConn.m_parseState = CNY_PS_REPLACE_CMD;
                break;
            case 'g':
                T2_pConn.m_parseState = CNY_PS_GET_CMD;
                break;
            case 'd':
                T2_pConn.m_parseState = CNY_PS_DELETE_CMD;
                break;
            case 'i':
                T2_pConn.m_parseState = CNY_PS_INCR_CMD;
                break;
            case 't':
                T2_pConn.m_parseState = CNY_PS_TOUCH_CMD;
                break;
            default:
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
                break;
            }

            P2_keyLen += 1;
            P2_keyPos = T2_recvBufPktIdx;
            //P2_keyInit=false;
            T2_pConn.m_parsePos = 1;
            T2_pConn.m_delta = 0;
            T2_pConn.m_bytes = 0;
            T2_pConn.m_flags = 0;
            T2_pConn.m_exptime = 0;
            T2_pConn.m_noreply = false;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_OTHER_CMD: {

            T2_pConn.m_cmd = CNY_CMD_OTHER;
            if (T2_chIsCR)
                T2_pConn.m_parseState = CNY_PS_EOC;
            P2_keyLen += 1;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_TOUCH_CMD:
        {
            if (T2_chMatchTouch[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs6) {
                    T2_pConn.m_cmd = CNY_CMD_TOUCH;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_INCR_CMD: {

            if (T2_chMatchIncr[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs5) {
                    T2_pConn.m_cmd = CNY_CMD_INCR;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_DELETE_CMD: {

            if (T2_chMatchDelete[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs7) {
                    T2_pConn.m_cmd = CNY_CMD_DELETE;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else if (T2_chIsCee && T2_parsePosIs2) {
                T2_pConn.m_parseState = CNY_PS_DECR_CMD;
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_DECR_CMD: {

            if (T2_chMatchDecr[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs5) {
                    T2_pConn.m_cmd = CNY_CMD_DECR;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_SET_CMD: {

            if (T2_chMatchSet[(ht_uint2)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs4) {
                    T2_pConn.m_cmd = CNY_CMD_SET;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_ADD_CMD: {

            if (T2_chMatchAdd[(ht_uint2)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs4) {
                    T2_pConn.m_cmd = CNY_CMD_ADD;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else if (T2_parsePosIs1 && T2_chIsPee) {
                T2_pConn.m_parseState = CNY_PS_APPEND_CMD;
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_APPEND_CMD: {

            if (T2_chMatchAppend[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs7) {
                    T2_pConn.m_cmd = CNY_CMD_APPEND;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_REPLACE_CMD: {

            if (T2_chMatchReplace[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs8) {
                    T2_pConn.m_cmd = CNY_CMD_REPLACE;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_PREPEND_CMD: {

            if (T2_chMatchPrepend[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs8) {
                    T2_pConn.m_cmd = CNY_CMD_PREPEND;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_CAS_CMD: {

            if (T2_chMatchCas[(ht_uint2)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs4) {
                    T2_pConn.m_cmd = CNY_CMD_CAS;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_GET_CMD: {

            if (T2_chMatchGet[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs4) {
                    T2_pConn.m_cmd = CNY_CMD_GET;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else if (T2_chMatchGets[(ht_uint3)T2_pConn.m_parsePos]) {
                if (T2_parsePosIncIs5) {
                    T2_pConn.m_cmd = CNY_CMD_GETS;

                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                    T2_pConn.m_parsePos = 0;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_OTHER_CMD;
            }
            P2_keyLen += 1;
            T2_pConn.m_parsePos = T2_parsePosInc;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_KEY:
        case CNY_PS_MKEY:
        {
            if (PR2_htInst == CNY_PS_MKEY) {
                // last interation was a mix, get the result
                T2_pConn.m_hash_a = S_hashResA.read_mem();
                T2_pConn.m_hash_b = S_hashResB.read_mem();
                T2_pConn.m_hash_c = S_hashResC.read_mem();
                T2_pConn.m_parseState = CNY_PS_KEY;
            }
            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    if (T2_chCntIncEqlPktLen)
                        HtContinue(PKT_MEMWAIT);
                    else
                        HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_skipSpace = false;

                P2_keyInit = true;
                P2_keyLen = 0;
                T2_pConn.m_parsePos = 0;
                T2_pConn.m_hashPos = 0;

                T2_pConn.m_parseState = CNY_PS_KEY;
                P2_keyPos = T2_recvBufPktIdx;

                T2_pConn.m_hash_a = 0xdeadbeef;
                T2_pConn.m_hash_b = 0xdeadbeef;
                T2_pConn.m_hash_c = 0xdeadbeef;
                for(int i=0; i<12; i++)
                    T2_pConn.m_key12.m_byte[i] = 0;
            }

            if (!T2_chIsSP && !T2_chIsCR) {
	      if (T2_keyLenGtMax) {
		T2_pConn.m_parseState = CNY_PS_SWALLOW;
		T2_pConn.m_errCode = CNY_ERR_INV_ARG;
		P2_keyLen = 0;
	      } else {
                P2_keyLen += 1;

                T2_pConn.m_key12.m_byte[T2_pConn.m_hashPos] = T2_ch;
                T2_pConn.m_parsePos = T2_parsePosInc;

                uint32_t c_hash_a, c_hash_b, c_hash_c;
                key2word(T2_pConn.m_key12, c_hash_a, c_hash_b, c_hash_c);

                if (T2_pConn.m_hashPos == 11) {
                    T2_pConn.m_hash_a += c_hash_a;
                    T2_pConn.m_hash_b += c_hash_b;
                    T2_pConn.m_hash_c += c_hash_c;
                    T2_hValid = true;
                    //mix(T2_pConn->m_hash_a, T2_pConn->m_hash_b, T2_pConn->m_hash_c);
                    for(int i=0; i<12; i++)
                        T2_pConn.m_key12.m_byte[i] = 0;
                    T2_pConn.m_parseState = CNY_PS_MKEY;
                }
                P2_pCh = T2_pChInc;
                P2_chCnt = T2_chCntInc;
	      }
            } else {
                // final
                uint32_t c_hash_a, c_hash_b, c_hash_c;
                key2word(T2_pConn.m_key12, c_hash_a, c_hash_b, c_hash_c);

                T2_pConn.m_hash_a += c_hash_a;
                T2_pConn.m_hash_b += c_hash_b;
                T2_pConn.m_hash_c += c_hash_c;
                T2_fValid = true;
                //final(T2_pConn->m_hash_a, T2_pConn->m_hash_b, T2_pConn->m_hash_c);
                T2_pConn.m_parseState = CNY_PS_FKEY;
                HtContinue(T2_pConn.m_parseState);
                break;
            }
            if (++T2_pConn.m_hashPos == 12)
                T2_pConn.m_hashPos = 0;

            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_FKEY:
        {
            if (accPushBusy) {
                HtRetry();
                break;
            }
            T2_pConn.m_hash_c = S_hashResC.read_mem();

            T2_aKeyRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;
            T2_aKeyRspCmd.m_cmd.m_keyInit = P2_keyInit;

            T2_aKeyRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_aKeyRspCmd.m_cmd.m_cmd = T2_pConn.m_cmd;
            T2_aKeyRspCmd.m_cmd.m_keyLen = P2_keyLen;
            T2_aKeyRspCmd.m_cmd.m_keyPos = P2_keyPos;
            T2_aKeyRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
            T2_aKeyRspCmd.m_numCmd = CNY_CMD_GET_DW_CNT;

            switch (T2_pConn.m_cmd) {
            case CNY_CMD_SET:
            case CNY_CMD_ADD:
            case CNY_CMD_REPLACE:
            case CNY_CMD_APPEND:
            case CNY_CMD_PREPEND:
            case CNY_CMD_CAS:
                T2_pConn.m_parseState = CNY_PS_FLAG;
                T2_pConn.m_skipSpace = true;
                break;
            case CNY_CMD_DELETE:
                if (T2_chIsSP) {
                    T2_pConn.m_parseState = CNY_PS_NOREPLY;
                    T2_pConn.m_parsePos = 0;
                    T2_pConn.m_skipSpace = true;
                } else if (T2_chIsCR)
                    T2_pConn.m_parseState = CNY_PS_EOC;
                else {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_BAD_FORMAT;
                }
                break;
            case CNY_CMD_INCR:
            case CNY_CMD_DECR:
                T2_pConn.m_parseState = CNY_PS_DELTA;
                T2_pConn.m_skipSpace = true;
                break;
            case CNY_CMD_TOUCH:
                T2_pConn.m_parseState = CNY_PS_EXP;
                T2_pConn.m_skipSpace = true;
                break;
            case CNY_CMD_GET:
            case CNY_CMD_GETS:
                T2_aKeyRspCmdValid = true;
                //WriteRspCmd(P2_rspHdr, CNY_CMD_GET_DW_CNT, rspOpt, 0,
                //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                P2_sentPktDone = T2_chCntIncEqlPktLen;
                P2_keyLen = 0;

                if (T2_chIsSP) {
                    T2_pConn.m_parseState = CNY_PS_KEY;
                    T2_pConn.m_skipSpace = true;
                } else
                    T2_pConn.m_parseState = CNY_PS_EOC;
                break;
            default:
                assert(0);
                break;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_FLAG:
        {
            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                } else if (!T2_chIsDigit) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_FLAG;
                    HtContinue(T2_pConn.m_parseState);
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    break;
                }
                T2_pConn.m_skipSpace = false;
            }

            if (T2_chIsDigit) {
                if (T2_flagsWillOF) {
                    // check for overflow
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_FLAG;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_flags = T2_flagsMul10 + T2_ch - '0';
            } else if (T2_chIsSP) {
                T2_pConn.m_parseState = CNY_PS_EXP;
                T2_pConn.m_skipSpace = true;
                T2_pConn.m_exptime = 0;
            } else {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_INV_FLAG;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_EXP:
        {
            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                } else if (!T2_chIsDigit) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_EXPTIME;
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_skipSpace = false;
            }

            if (T2_chIsDigit) {
                if (T2_expWillOF) {
                    // check for overflow
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_EXPTIME;
                }
                T2_pConn.m_exptime = T2_expMul10 + T2_ch - '0';
            } else {
                switch (T2_pConn.m_cmd) {
                case CNY_CMD_SET:
                case CNY_CMD_ADD:
                case CNY_CMD_REPLACE:
                case CNY_CMD_APPEND:
                case CNY_CMD_PREPEND:
                case CNY_CMD_CAS:
                    if (T2_chIsSP) {
                        T2_pConn.m_parseState = CNY_PS_BYTES;
                        T2_pConn.m_skipSpace = true;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_SWALLOW;
                        T2_pConn.m_errCode = CNY_ERR_INV_EXPTIME;
                    }
                    break;
                case CNY_CMD_TOUCH:
                    if (T2_chIsCR)
                        T2_pConn.m_parseState = CNY_PS_EOC;
                    else if (T2_chIsSP) {
                        T2_pConn.m_skipSpace = true;
                        T2_pConn.m_parsePos = 0;
                        T2_pConn.m_parseState = CNY_PS_NOREPLY;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_SWALLOW;
                        T2_pConn.m_errCode = CNY_ERR_INV_EXPTIME;
                    }
                    break;
                default:
                    assert(0);
                }
            }
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_BYTES:
        {
            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                } else if (!T2_chIsDigit) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_VLEN;
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_skipSpace = false;
            }

            if (T2_chIsDigit) {
                if (T2_bytesWillOF) {
                    // check for overflow
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_VLEN;
                }
                T2_pConn.m_bytes = T2_bytesMul10 + T2_ch - '0';
            } else {
                switch (T2_pConn.m_cmd) {
                case CNY_CMD_SET:
                case CNY_CMD_ADD:
                case CNY_CMD_REPLACE:
                case CNY_CMD_APPEND:
                case CNY_CMD_PREPEND:
                    if (T2_chIsCR) {
                        T2_pConn.m_parseState = CNY_PS_EOC;
                        T2_pConn.m_bytes += 2;
                    } else if (T2_chIsSP) {
                        T2_pConn.m_skipSpace = true;
                        T2_pConn.m_parsePos = 0;
                        T2_pConn.m_parseState = CNY_PS_NOREPLY;
                        T2_pConn.m_bytes += 2;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_SWALLOW;
                        T2_pConn.m_errCode = CNY_ERR_INV_VLEN;
                    }
                    break;
                case CNY_CMD_CAS:
                    if (T2_chIsSP) {
                        T2_pConn.m_parseState = CNY_PS_DELTA;
                        T2_pConn.m_skipSpace = true;
                        T2_pConn.m_bytes += 2;
                    } else {
                        T2_pConn.m_parseState = CNY_PS_SWALLOW;
                        T2_pConn.m_errCode = CNY_ERR_INV_VLEN;
                    }
                    break;
                default:
                    assert(0);
                }
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_DELTA:
        {

            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                } else if (!T2_chIsDigit) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_DELTA;
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_skipSpace = false;
            }

            if (T2_chIsDigit) {
                if (T2_deltaWillOF) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_INV_DELTA;
                }

                T2_pConn.m_delta = T2_deltaMul10 + T2_ch - '0';
            } else if (T2_chIsSP) {
                T2_pConn.m_skipSpace = true;
                T2_pConn.m_parsePos = 0;
                T2_pConn.m_parseState = CNY_PS_NOREPLY;
            } else if (T2_chIsCR) {
                T2_pConn.m_parseState = CNY_PS_EOC;
            } else {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_INV_DELTA;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_NOREPLY:
        {
            if (T2_pConn.m_skipSpace) {
                if (T2_chIsSP) {
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                } else if (T2_chIsCR) {
                    T2_pConn.m_parseState = CNY_PS_EOC;
                    P2_pCh = T2_pChInc;
                    P2_chCnt = T2_chCntInc;
                    HtContinue(T2_pConn.m_parseState);
                    break;
                }
                T2_pConn.m_skipSpace = false;
            }
            if (T2_chMatchNoreply[(ht_uint3)T2_pConn.m_parsePos]) {
                T2_pConn.m_parsePos = T2_parsePosInc;
                if (T2_parsePosIncIs7) {
                    T2_pConn.m_noreply = true;
                    T2_pConn.m_parseState = CNY_PS_EOC_SPACE;
                }
            } else {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_BAD_FORMAT;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_EOC_SPACE:
        {
            if (T2_chIsCR)
                T2_pConn.m_parseState = CNY_PS_EOC;
            else if (!T2_chIsSP) {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_BAD_FORMAT;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_EOC:
        {
            if (accPushBusy) {
                HtRetry();
                break;
            }
            T2_aEocRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_aEocRspCmd.m_cmd.m_keyHash = T2_pConn.m_hash_c;
            T2_aEocRspCmd.m_cmd.m_keyInit = P2_keyInit;
            ht_uint3 c_numOpt = 0;

            if (!T2_chIsLF) {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_BAD_CMD_EOL;
            } else {
                switch (T2_pConn.m_cmd) {
                case CNY_CMD_SET:
                case CNY_CMD_ADD:
                case CNY_CMD_REPLACE:
                case CNY_CMD_APPEND:
                case CNY_CMD_PREPEND:
                case CNY_CMD_CAS:
                {
                    T2_pConn.m_parseState = CNY_PS_DATA;

                    T2_aEocRspCmd.m_cmd.m_cmd = T2_pConn.m_cmd;
                    T2_aEocRspCmd.m_cmd.m_keyLen = P2_keyLen;
                    T2_aEocRspCmd.m_cmd.m_keyPos = P2_keyPos;
                    T2_aEocRspCmd.m_cmd.m_valueLen = T2_pConn.m_bytes;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

                    WriteRspValue(T2_aEocRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_flags, 4);
                    WriteRspValue(T2_aEocRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_exptime, 4);
                    WriteRspValue(T2_aEocRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_delta, 8);
                    T2_aEocRspCmd.m_numOpt = c_numOpt;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_SET_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_SET_DW_CNT, rspOpt, rspOptDwCnt,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_DELETE:
                {
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;

                    T2_aEocRspCmd.m_cmd.m_cmd = T2_pConn.m_cmd;
                    T2_aEocRspCmd.m_cmd.m_keyLen = P2_keyLen;
                    T2_aEocRspCmd.m_cmd.m_keyPos = P2_keyPos;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_DELETE_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_DELETE_DW_CNT, rspOpt, 0,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_GET:
                case CNY_CMD_GETS:
                {
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;

                    T2_aEocRspCmd.m_cmd.m_cmd = CNY_CMD_GET_END;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_GET_END_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_GET_END_DW_CNT, rspOpt, 0,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_DECR:
                case CNY_CMD_INCR:
                {
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;

                    T2_aEocRspCmd.m_cmd.m_cmd = T2_pConn.m_cmd;
                    T2_aEocRspCmd.m_cmd.m_keyLen = P2_keyLen;
                    T2_aEocRspCmd.m_cmd.m_keyPos = P2_keyPos;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

                    WriteRspValue(T2_aEocRspCmd.m_rspOpt, c_numOpt, T2_pConn.m_delta, 8);
                    T2_aEocRspCmd.m_numOpt = c_numOpt;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_ARITH_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_ARITH_DW_CNT, rspOpt, rspOptDwCnt,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_TOUCH:
                {
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;
                    T2_aEocRspCmd.m_cmd.m_cmd = T2_pConn.m_cmd;
                    T2_aEocRspCmd.m_cmd.m_keyLen = P2_keyLen;
                    T2_aEocRspCmd.m_cmd.m_keyPos = P2_keyPos;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

                    WriteRspValue(T2_aEocRspCmd.m_rspOpt, c_numOpt, (uint64_t)T2_pConn.m_exptime, 4);
                    T2_aEocRspCmd.m_numOpt = c_numOpt;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_TOUCH_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_TOUCH_DW_CNT, rspOpt, rspOptDwCnt,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_OTHER:
                {
                    P2_keyLen += 1;
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;
                    T2_aEocRspCmd.m_cmd.m_cmd = CNY_CMD_OTHER;
                    T2_aEocRspCmd.m_cmd.m_keyLen = P2_keyLen;
                    T2_aEocRspCmd.m_cmd.m_keyPos = P2_keyPos;
                    T2_aEocRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_OTHER_DW_CNT;
                    //WriteRspCmd(P2_rspHdr, CNY_CMD_OTHER_DW_CNT, rspOpt, 0,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;
                }
                    break;
                case CNY_CMD_ERROR:
                {
                    T2_aEocRspCmd.m_cmd.m_cmd = CNY_CMD_ERROR;
                    T2_aEocRspCmd.m_cmd.m_errCode = T2_pConn.m_errCode;
                    T2_aEocRspCmdValid = true;
                    T2_aEocRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                    T2_aEocRspCmd.m_numCmd = CNY_CMD_ERROR_DW_CNT;

                    //WriteRspCmd(P2_rspHdr, CNY_CMD_ERROR_DW_CNT, rspOpt, 0,
                    //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                    P2_sentPktDone = T2_chCntIncEqlPktLen;

                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;
                }
                    break;
                default:
                    assert(0);
                }
                P2_keyLen = 0;
                T2_pConn.m_parsePos = 0;

                P2_dataLen = 0;
                P2_dataPos = T2_recvBufPktIdx + 1;
            }

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_DATA:
        {

            P2_restartPrefetch = T2_restartAsciiPrefetch;
            // skip data, but not \r\n
            //fprintf(stderr,"PKT: DATA htId=0x%x pCh=0x%05x bytes=%d parsePos=%d ",
            //        (int)PR2_htId,
            //        (int)P2_pCh, (int)T2_pConn.m_bytes,
            //        (int)T2_pConn.m_parsePos);
            if (T2_skipAsciiData) {
                T2_pConn.m_parsePos = T2_parsePosDataInc;
                P2_dataLen = (ht_uint12)T2_dataLenInc;
                P2_pCh = T2_pChDataInc;
                P2_chCnt = T2_chCntDataInc;
            }
            //fprintf(stderr,"pCh=0x%05x parsePos=%d\n",
            //        (int)P2_pCh,
            //        (int)T2_pConn.m_parsePos);

            // go to the end and check \r\n
            T2_pConn.m_parseState = CNY_PS_DATA2;

            if (P2_restartPrefetch) {
                // dump the prefetch buffer and initialize to start at new position
                P2_prefetch.m_checkReady=false;
                P2_prefetch.m_pauseComplete=false;
                P2_prefetch.m_valid[0] = false;
                P2_prefetch.m_valid[1] = false;
                P2_prefetch.m_prefetch[0] = false;
                P2_prefetch.m_prefetch[1] = false;
                //fprintf(stderr,"PKT: DATA restart htId=0x%x lastRead=0x%012llx ",
                //        (int)PR2_htId,
                //        (long long)P2_prefetch.m_lastRead);
                P2_prefetch.m_lastRead = T2_pChDataIncLast & (MemAddr_t)0xffffffffffc0ll;
                //fprintf(stderr,"nextRead=0x%012llx\n",
                //        (long long)P2_prefetch.m_lastRead);
            }

            if (P2_restartPrefetch)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
        }
            break;
        case CNY_PS_DATA2:
        {
            if (accPushBusy) {
#ifndef _HTV
                extern FILE *tfp;
                if (tfp && false)
                    fprintf(tfp,"PKT: data2 retry htId=%d htInst=%d\n",
                            (int)PR2_htId, (int)PR2_htInst);
#endif
                HtRetry();
                break;
            }
            P2_dataLen += 1;
            T2_aDataRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_aDataRspCmd.m_cmd.m_cmd = CNY_CMD_DATA;
            T2_aDataRspCmd.m_cmd.m_dataPos = P2_dataPos;
            T2_aDataRspCmd.m_cmd.m_dataLen = P2_dataLen;
            T2_aDataRspCmd.m_cmd.m_keyInit = P2_keyInit;
            T2_aDataRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;

            if (T2_parsePosIsBytesDec &&
                    !T2_chIsCR) {
                T2_pConn.m_parseState = CNY_PS_SWALLOW;
                T2_pConn.m_errCode = CNY_ERR_BAD_DATA_EOL;
            }

            if (T2_parsePosIsBytes) {
                if (!T2_chIsLF) {
                    T2_pConn.m_parseState = CNY_PS_SWALLOW;
                    T2_pConn.m_errCode = CNY_ERR_BAD_DATA_EOL;
                } else {
                    T2_pConn.m_parseState = CNY_PS_ASCII_CMD;
                }
            }

            if (T2_chCntIncEqlPktLen && P2_dataLen > 0 ||
                    T2_parsePosIsBytes) {
                // partial data parsed, send starting address and length to host
                T2_aDataRspCmdValid = true;
                T2_aDataRspCmd.m_bPktDone = T2_chCntIncEqlPktLen;
                T2_aDataRspCmd.m_numCmd = CNY_CMD_DATA_DW_CNT;
                //WriteRspCmd(P2_rspHdr, CNY_CMD_DATA_DW_CNT, rspOpt, 0,
                //            P2_pktInfo.m_blkIndex, T2_chCntIncEqlPktLen);
                P2_sentPktDone = T2_chCntIncEqlPktLen;
            }
            if (T2_chCntIncEqlPktLen && !T2_parsePosIsBytes) {
                T2_pConn.m_parseState = CNY_PS_DATA;
            }

            T2_pConn.m_parsePos = T2_parsePosInc;
            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
            //fprintf(stderr,"PKT: data2 htId=%d htInst=%d parseState=%d chCntEqlPktLen=%d pCh=0x%05x pChMax=0x%05x lPrefetch=0x%05x v0=%d v1=%d p0=%d p1=%d\n",
            //        (int)PR2_htId, (int)PR2_htInst,
            //        (int)T2_pConn.m_parseState,
            //        (int)T2_chCntIncEqlPktLen, (int)P2_pCh, (int)P2_pChMax,
            //        (int)(P2_prefetch.m_lastRead & 0x0fffff),
            //        (int)P2_prefetch.m_valid[0], (int)P2_prefetch.m_valid[1],
            //        (int)P2_prefetch.m_prefetch[0], (int)P2_prefetch.m_prefetch[1]);
        }
            break;
        case CNY_PS_SWALLOW:
            // swallow to end of line
            T2_pConn.m_cmd = CNY_CMD_ERROR;
            if (T2_chIsCR)
                T2_pConn.m_parseState = CNY_PS_EOC;
            else
                T2_pConn.m_parseState = CNY_PS_SWALLOW;

            P2_pCh = T2_pChInc;
            P2_chCnt = T2_chCntInc;
            if (T2_chCntIncEqlPktLen)
                HtContinue(PKT_MEMWAIT);
            else
                HtContinue(T2_pConn.m_parseState);
            break;
#endif // ASCII
        case CNY_PS_BIN_SWALLOW:
        case PKT_MEMWAIT    :
        {
            if (ReadMemBusy()) {
                HtRetry();
                break;
            }
            if (P2_restartPrefetch) {
                ReadMemPause(PKT_PREFETCH);
            } else {
                ReadMemPause(PKT_RETURN);
            }
        }
            break;
        case PKT_PREFETCH:
        {
            if (ReadMemBusy()) {
                HtRetry();
                break;
            }
            HtContinue(T2_pConn.m_parseState);
            P2_restartPrefetch = false;
        }
            break;
        case PKT_RETURN:
        {
            bool c_partialKeyCmd = P2_keyLen != 0;
            if (SendReturnBusy_ProcessPkt() ||
                    (accPushBusy && (c_partialKeyCmd || !P2_sentPktDone)) ||
                    SendMsgBusy_pktFinish()) {
                HtRetry();
                break;
            }
            T2_retRspCmd.m_bPktDone = !P2_sentPktDone;
            T2_retRspCmd.m_cmd.m_ascii = P2_bAscii;
            T2_retRspCmd.m_cmd.m_cmd = CNY_CMD_KEY;
            T2_retRspCmd.m_cmd.m_keyLen = P2_keyLen;
            T2_retRspCmd.m_cmd.m_keyPos = P2_keyPos;
            T2_retRspCmd.m_cmd.m_keyInit = P2_keyInit;
            T2_aDataRspCmd.m_cmd.m_noreply = T2_pConn.m_noreply;
            if (c_partialKeyCmd)
                T2_retRspCmd.m_numCmd = CNY_CMD_KEY_DW_CNT;
            if (!P2_sentPktDone || c_partialKeyCmd)
                T2_retRspCmdValid = true;
                //WriteRspCmd(P2_rspHdr, 0, rspOpt, 0,
                //            P2_pktInfo.m_blkIndex, true);
            if (!bPktInfoPktLenEql0 && (P2_connId & 0x8000) == 0)
                SendMsg_pktFinish(P2_pktInfo.m_prefetchIdx);

#ifndef _HTV
	    extern FILE *tfp;
	    if (tfp)
            fprintf(tfp, "PKT: return cid=0x%04x blkIdx=%d htId=0x%x @ %lld\n",
                    (int)P2_connId, (int)P2_pktInfo.m_blkIndex,
                    (int)PR2_htId,
                    (long long)sc_time_stamp().value() / 10000);
#endif

        SendReturn_ProcessPkt();
        }
            break;
        default:
            assert(0);
        }

        P2_prefetchAddr = P2_pktInfo.m_prefetchIdx << 3 | (ht_uint9)(P2_pCh>>3 & 0x7);
        P2_fetchAddr = PR2_htId << 4 | (ht_uint9)(P2_pCh>>3 & 0xf);


    } // if PR2_htValid

    if (T3_htValid) {
        // again, a hack because global can't use a structure
        //CAccelWord cW;
        //cW.m_pConn = T3_pConn;
        // this write doesn't need to occur in stage 2
        //for (int i=0; i<ACCEL_CONN_SIZE; i++) {
        //    GW_connList_pConn(T3_pConnAddr, i, cW.m_word[i]);
        //}
      GW3_connList.write_addr(T3_pConnAddr);
      GW3_connList = T3_pConn;
    }

    // Warning: assuming that time between thread execution is longer than this pipe
    T2_hA = T2_pConn.m_hash_a;
    T2_hB = T2_pConn.m_hash_b;
    T2_hC = T2_pConn.m_hash_c;
    T3_hA -= T3_hC;  T3_hA ^= rot(T3_hC, 4);  T3_hC += T3_hB;
    T3_hB -= T3_hA;  T3_hB ^= rot(T3_hA, 6);  T3_hA += T3_hC;
    T3_hC -= T3_hB;  T3_hC ^= rot(T3_hB, 8);  T3_hB += T3_hA;
    T4_hA -= T4_hC;  T4_hA ^= rot(T4_hC,16);  T4_hC += T4_hB;
    T4_hB -= T4_hA;  T4_hB ^= rot(T4_hA,19);  T4_hA += T4_hC;
    T4_hC -= T4_hB;  T4_hC ^= rot(T4_hB, 4);  T4_hB += T4_hA;
    S_hashResA.write_addr(T5_pConnAddr);
    S_hashResB.write_addr(T5_pConnAddr);
    if (T5_hValid) {
        S_hashResA.write_mem(T5_hA);
        S_hashResB.write_mem(T5_hB);
    }

    // Warning: assuming that time between thread execution is longer than this pipe
    T2_fA = T2_pConn.m_hash_a;
    T2_fB = T2_pConn.m_hash_b;
    T2_fC = T2_pConn.m_hash_c;
    T3_fC ^= T3_fB; T3_fC -= rot(T3_fB,14);
    T3_fA ^= T3_fC; T3_fA -= rot(T3_fC,11);
    T3_fB ^= T3_fA; T3_fB -= rot(T3_fA,25);
    T4_fC ^= T4_fB; T4_fC -= rot(T4_fB,16);
    T4_fA ^= T4_fC; T4_fA -= rot(T4_fC,4);
    T4_fB ^= T4_fA; T4_fB -= rot(T4_fA,14);
    T5_fC ^= T5_fB; T5_fC -= rot(T5_fB,24);
    S_hashResC.write_addr(T6_pConnAddr);
    if (T6_fValid || T6_hValid) {
        S_hashResC.write_mem(T6_fValid ? T6_fC : T6_hC);
    }

    T2_anyRspCmdValid = false;
    if (T2_entryRspCmdValid || T2_bCmdRspCmdValid ||
            T2_bKeyRspCmdValid || T2_bDataRspCmdValid ||
            T2_aKeyRspCmdValid || T2_aEocRspCmdValid ||
            T2_aDataRspCmdValid || T2_retRspCmdValid) {
        T2_anyRspCmdValid = true;
    }
    T2_connId = P2_connId;

    CPktCmd c3_rspCmd;
    c3_rspCmd = 0;
    // this could be a simple bit=wise or but I don't know how to get
    // that through the tools...
    if (T3_entryRspCmdValid)
        c3_rspCmd = T3_entryRspCmd;
    if (T3_bCmdRspCmdValid)
        c3_rspCmd = T3_bCmdRspCmd;
    if (T3_bKeyRspCmdValid)
        c3_rspCmd = T3_bKeyRspCmd;
    if (T3_bDataRspCmdValid)
        c3_rspCmd = T3_bDataRspCmd;
    if (T3_aKeyRspCmdValid)
        c3_rspCmd = T3_aKeyRspCmd;
    if (T3_aEocRspCmdValid)
        c3_rspCmd = T3_aEocRspCmd;
    if (T3_aDataRspCmdValid)
        c3_rspCmd = T3_aDataRspCmd;
    if (T3_retRspCmdValid)
        c3_rspCmd = T3_retRspCmd;
    if(T3_anyRspCmdValid) {
        S_accQueCnt[cid2que(T3_connId)] -= 1;
        SendMsg_accPush(c3_rspCmd);
#ifndef _HTV
        extern FILE *tfp;
        if (tfp) {
            fprintf(tfp, "PKT: cid=0x%04x blkIdx=%d cidQue=%d pktDone=%d ",
                    (int)c3_rspCmd.m_cmd.m_connId, (int)c3_rspCmd.m_blkIndex,
                    (int)((ht_uint4)cid2que(c3_rspCmd.m_cmd.m_connId)), (int)c3_rspCmd.m_bPktDone);
            if (c3_rspCmd.m_numCmd) {
                fprintf(tfp,"header=0x%08x",c3_rspCmd.m_raw.m_word[0]);
                for(uint32_t i = 1; i<c3_rspCmd.m_numCmd; i++)
                    fprintf(tfp, "_%08x", c3_rspCmd.m_raw.m_word[i]);
            }
            fprintf(tfp," @ %lld\n", (long long)sc_time_stamp().value() / 10000);
            fflush(tfp);
        }
#endif
    }

    if (GR_htReset) {
        for (uint32_t i=0; i<SCH_THREAD_CNT; i++)
            S_accQueCnt[i] = ACC_QUE_DEPTH;
    }
}

// this routine is stage one only
bool 
CPersPkt::getNextChar(const ht_uint20 &lsAddr, const MemAddr_t &baseAddr,
                        const bool &first64,
                        const ht_uint4 &htId,
                        const uint64_t &prefetchQw,
                        const uint64_t &fetchQw,
                        CMemPrefetch &m,
                        uint8_t &obyte) {

    CByteData tData;

    ht_uint1 addrIndex, lastIndex;
    addrIndex = (ht_uint1)(lsAddr>>6);
    lastIndex = addrIndex ^ 1;

    // assume we're striding through memory
    // if ls bit of addIndex doesn't match ls bit of memGrp then wrong memReady
    m.m_checkReady = false;

    if (m.m_prefetch[addrIndex] && m.m_pauseComplete) {
        m.m_prefetch[addrIndex] = false;
        m.m_valid[addrIndex] = true;
    }
    // we're never going back
    m.m_valid[lastIndex] = false;

    bool valid = false;
    if (m.m_valid[addrIndex]) {
        valid = true;
    } else {
        m.m_checkReady = m.m_prefetch[addrIndex];
    }

    tData.m_qword = first64 ? prefetchQw : fetchQw;

    obyte = tData.m_byte[lsAddr&0x7];

#ifndef _HTV
    // check that memory fetches are correct
    uint64_t memAddr;
    memAddr =  (uint64_t)(m.m_lastRead & 0xfffffff00000) | (uint64_t)(lsAddr & 0xffff8);

    uint64_t mdata = *(uint64_t*)memAddr;
    if (m.m_valid[(ht_uint1)(memAddr >> 6)] && tData.m_qword != mdata &&
            ((uint8_t)lsAddr & 0x018) == ((uint8_t)memAddr & 0x18)) {
        extern FILE *tfp;
        if (tfp) {
            fprintf(tfp,"PKT: Info: htId=0x%x addr=0x%012llx pdata=0x%016llx mdata=0x%016llx @ %lld\n",
                    (int)htId, (long long)memAddr, (long long)tData.m_qword, (long long)mdata,
                    (long long)sc_time_stamp().value() / 10000);
            fprintf(tfp,"PKT:         lastRead=0x%012llx pCh=%05x ch=%02x valid=%d\n",
                    (long long)m.m_lastRead, (int)lsAddr, (int)obyte, (int)valid);
            fprintf(tfp,"PKT:         v0=%d v1=%d p0=%d p1=%d htInst=%d\n",
                    (int)m.m_valid[0], (int)m.m_valid[1],
                    (int)m.m_prefetch[0], (int)m.m_prefetch[1],
                    (int)PR1_htInst);
        }
        if (valid) {
            assert(0);
        }
    }
#endif
    return(valid);
}

// this routine is stage two only
void
CPersPkt::prefetch(const ht_uint20 &lsAddr,
                     const ht_uint4 &htId,
                     const bool &prefetchDone,
                     CMemPrefetch &m) {

    // assume we're striding through memory, buffers are 1MB aligned
    MemAddr_t thisBuffer = m.m_lastRead & 0xfffffff00000ll;
    ht_uint20 thisOffset = (ht_uint20)(m.m_lastRead & 0xfffffll);
    ht_uint20 nextOffset = thisOffset + 0x40;
    ht_uint1 thisIdx = (ht_uint1)(m.m_lastRead >> 6);
    ht_uint1 nextIdx = thisIdx ^ 1;
    ht_uint9 bufIdx = (ht_uint9)(htId<<4) | (ht_uint9)(nextIdx<< 3);
    if(!ReadMemBusy()) {
        if (!m.m_valid[nextIdx] && !prefetchDone &&
                !m.m_prefetch[0] && !m.m_prefetch[1]) {
            // check that we never increment beyond the 1MB buffer
            assert((0xfff00000 & ((uint32_t)thisOffset + 0x40)) == 0);
            m.m_lastRead = thisBuffer | nextOffset;
            ReadMem_rdPktDat(m.m_lastRead, bufIdx, 8);
            m.m_prefetch[nextIdx] = true;
        }
    }
    //ht_uint1 readBuffer = (ht_uint1)(lsAddr >> 6);
    m.m_checkReady = !m.m_valid[0] && !m.m_valid[1];
}

void
CPersPkt::WriteRspValue(PktOptWords &rspOpt, 
                          ht_uint3 &rspOptDwCnt,
                          const uint64_t &value,
                          const uint8_t &byteSize) {
    uint32_t word0 = (uint32_t)(value >>  0);
    uint32_t word1 = (uint32_t)(value >> 32);
    ht_uint3 cnt = rspOptDwCnt; // workaround for htv error
    if (byteSize == 4 || byteSize == 8) {
        rspOpt.m_word[(ht_uint3)cnt] = word0;
        cnt++;
    }
    if (byteSize == 8) {
        rspOpt.m_word[(ht_uint3)cnt] = word1;
        cnt++;
    }
    rspOptDwCnt = cnt;
#ifndef _HTV
    assert(rspOptDwCnt <= 8);
    assert(byteSize == 0 || byteSize == 4 || byteSize == 8);
#endif

}

uint16_t
CPersPkt::ntohs(const uint16_t &in) {
    uint16_t out =
            ((uint16_t)(in >> 8) & 0x00ff) |
            ((uint16_t)(in << 8) & 0xff00);
    return(out);
}

uint32_t
CPersPkt::ntohl(const uint32_t &in) {
    uint32_t out =
            ((uint32_t)(in >> 24) & 0x000000ff) |
            ((uint32_t)(in >>  8) & 0x0000ff00) |
            ((uint32_t)(in <<  8) & 0x00ff0000) |
            ((uint32_t)(in << 24) & 0xff000000);
    return(out);
}

uint64_t
CPersPkt::ntohll(const uint64_t &in) {
    uint64_t out =
            ((uint64_t)(in >> 56) & (uint64_t)0x00000000000000ffll) |
            ((uint64_t)(in >> 40) & (uint64_t)0x000000000000ff00ll) |
            ((uint64_t)(in >> 24) & (uint64_t)0x0000000000ff0000ll) |
            ((uint64_t)(in >>  8) & (uint64_t)0x00000000ff000000ll) |
            ((uint64_t)(in <<  8) & (uint64_t)0x000000ff00000000ll) |
            ((uint64_t)(in << 24) & (uint64_t)0x0000ff0000000000ll) |
            ((uint64_t)(in << 40) & (uint64_t)0x00ff000000000000ll) |
            ((uint64_t)(in << 56) & (uint64_t)0xff00000000000000ll);
    return(out);
}

uint32_t
CPersPkt::rot(const uint32_t &x, const uint32_t &k) {
    return (x<<k)|(x>>(32-k));
}

void CPersPkt::key2word(const keyField &key12,
                          uint32_t &word_a,
                          uint32_t &word_b,
                          uint32_t &word_c) {
    // I probably have the endian wrong here but I don't think it matters...
    word_a = ((uint32_t)((uint32_t)key12.m_byte[0] << 0) |
              (uint32_t)((uint32_t)key12.m_byte[1] << 8) |
              (uint32_t)((uint32_t)key12.m_byte[2] << 16) |
              (uint32_t)((uint32_t)key12.m_byte[3] << 24));
    word_b = ((uint32_t)((uint32_t)key12.m_byte[4] << 0) |
              (uint32_t)((uint32_t)key12.m_byte[5] << 8) |
              (uint32_t)((uint32_t)key12.m_byte[6] << 16) |
              (uint32_t)((uint32_t)key12.m_byte[7] << 24));
    word_c = ((uint32_t)((uint32_t)key12.m_byte[8] << 0) |
              (uint32_t)((uint32_t)key12.m_byte[9] << 8) |
              (uint32_t)((uint32_t)key12.m_byte[10] << 16) |
              (uint32_t)((uint32_t)key12.m_byte[11] << 24));
}

bool
CPersPkt::isdigit(const uint8_t &ch) {
    return (ch >= (uint8_t)'0' && ch <= (uint8_t)'9');
}
