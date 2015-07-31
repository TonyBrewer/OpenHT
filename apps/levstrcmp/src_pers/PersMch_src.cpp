/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT levstrcmp application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
/*
**	Pseudo code, each thread performs an allL-to-1R compare
**	loop through left
**	(strLenL, strLenR, mchAdjD, conIdL, conIdR, ramIdxR, blkSizeL, lineStrCntR, ramBlkIdx)
**
**	int ramIdxR = blkIdxR / LINE_STR_CNT_R + ramBlkIdx * RAM_BLOCK_SIZE;
**	int lineStrMaskR = (1ul << lineStrCntR) - 1;
**
**	for (int blkIdxL = 0; blkIdxL < blkSizeL; blkIdxL += LINE_STR_CNT_L) {
**
**		int lineStrCntL = blkSizeL - blkIdxL > LINE_STR_CNT_L ? LINE_STR_CNT_L : (blkSizeL - blkIdxL);
**		int lineStrMaskL = (1ul << lineStrCntL) - 1;
**
**		int ramIdxL = blkIdxL / LINE_STR_CNT_L + ramBlkIdx * RAM_BLOCK_SIZE;
**
**		cmpLxR( MCH_LOOP, strLenL, strLenR, mchAdjD, lineStrMaskL, lineStrMaskR, ramIdxL, ramIdxR );
**	}
*/

#include "Ht.h"
#include "PersMch.h"

void
CPersMch::PersMch()
{
  T1_cmpValid = false;
  LineStrCntL_t c_ts_lineStrCntL = 0;

  RamSize_t ramIdxL = P1_ramIdxL;

  if (PR1_htValid) {
      switch (PR1_htInst) {
      case MCH_ENTRY:
      {
          if (SendReturnBusy_MchBlkLx1R()) {
              HtRetry();
              break;
          }

          P1_blkIdxL = 0;
          P1_ramIdxL = (RamSize_t)(P1_ramBlkIdx * RAM_BLOCK_SIZE);

          if (P1_strLenL == 0 && P1_strLenR == 0)
              // end of run indicator, wait until idle
              HtContinue(MCH_WAIT_TIL_IDLE);
          else if (P1_blkSizeL == 0)
              SendReturn_MchBlkLx1R(SR_rsltCnt);		// an empty block, just return
          else
              HtContinue(MCH_BLK_L_LOOP);

          break;
      }
      case MCH_BLK_L_LOOP:
          // perform string compare of one left line of strings by one right line of strings
      {
          if (SendReturnBusy_MchBlkLx1R() || S_lineMatchQue.full(8)) {
              HtRetry();
              break;
          } else
              T1_cmpValid = true;

          c_ts_lineStrCntL = (LineStrCntL_t) (P1_blkSizeL - P1_blkIdxL > LINE_STR_CNT_L
                                              ? LINE_STR_CNT_L : (P1_blkSizeL - P1_blkIdxL));

          // increment loop index and check for loop termination condition
          P1_blkIdxL += LINE_STR_CNT_L;
          P1_ramIdxL += 1;

          if (P1_blkIdxL < P1_blkSizeL)
              HtContinue(MCH_BLK_L_LOOP);
          else
              SendReturn_MchBlkLx1R(SR_rsltCnt);

          break;
      }
      case MCH_WAIT_TIL_IDLE:
      {
          if (SendReturnBusy_MchBlkLx1R()) {
              HtRetry();
              break;
          }

          // we are idle when:
          //   only this thread is present
          //   no cmpValid bits are set
          //   the S_lineMatchQue is empty
          //   S_findNextMatchBusy is false

          if (!T2_cmpValid && !T3_cmpValid && !T4_cmpValid && !T5_cmpValid &&
                  !T6_cmpValid && !T7_cmpValid && !T8_cmpValid &&
                  S_lineMatchQue.empty() && !S_findNextMatchBusy) {

              SendReturn_MchBlkLx1R(SR_rsltCnt);		// an empty block, just return

              S_rsltCnt = 0;	// clear result count for next program run
          } else
              HtContinue(MCH_WAIT_TIL_IDLE);

          break;
      }
      default:
          assert(0);
      }
  }

  // perform a pipelined string compare operation
  // Inputs first clock: r_ts_strSetLRdData.m_data and r_ts_strSetRRdData.m_data
  // each stage handles one loop iterations
  Score_t c_cmpAdjD;
  // if D is greater than the pipe edit distance set it in range
  if (P1_cmpAdjD > (Score_t)EDIT_DIST_W)
      c_cmpAdjD = (Score_t)EDIT_DIST_W;
  else
      c_cmpAdjD = P1_cmpAdjD;
#ifndef _HTV
  if(P1_cmpAdjD >= EDIT_DIST_W)
      printf ("Warning edit distance of %d exceeds pipe, will always hit\n", (int)P1_cmpAdjD);
#endif
  switch (c_cmpAdjD) {
  case 0:
      T1_mchAdjD = (Score_t)SCORE0;
      break;
  case 1:
      T1_mchAdjD = (Score_t)SCORE1;
      break;
  case 2:
      T1_mchAdjD = (Score_t)SCORE2;
      break;
  case 3:
      T1_mchAdjD = (Score_t)SCORE3;
      break;
  default:
      T1_mchAdjD = (Score_t)SCORE4;
      break;
  }

  T1_matchId.m_conIdxL = (ConIdx_t)(P1_conIdxL + ((ramIdxL & (RAM_BLOCK_SIZE-1)) << LINE_STR_CNT_L_W));
  T1_matchId.m_conIdxR = (ConIdx_t)(P1_conIdxR + ((P1_ramIdxR & (RAM_BLOCK_SIZE-1)) << LINE_STR_CNT_R_W));
  T1_matchId.m_conIdL = P1_conIdL;
  T1_matchId.m_conIdR = P1_conIdR;
  T1_lineStrCntL = c_ts_lineStrCntL;

  T1_strLenDiff = (ht_int6)P1_strLenR - (ht_int6)P1_strLenL;

  T2_lineMaskL = (LineStrMaskL_t)((1ul << T2_lineStrCntL)-1);
  T2_lineMaskR = (LineStrMaskR_t)((1ul << T2_lineStrCntR)-1);

  // duplicating registers for FPGA timing
  for (int idxL = 0; idxL < LINE_STR_CNT_L; idxL ++) {
    ht_attrib(equivalent_register_removal, TR2_strL[0][idxL], "no");
    ht_attrib(equivalent_register_removal, TR2_strL[1][idxL], "no");

    T1_strL[0][idxL] = GR1_strSetL[idxL];
    T1_strL[1][idxL] = T1_strL[0][idxL];
  }

  for (int idxR = 0; idxR < LINE_STR_CNT_R; idxR += 1) {
    ht_attrib(equivalent_register_removal, TR2_strR[0][idxR], "no");
    ht_attrib(equivalent_register_removal, TR2_strR[1][idxR], "no");

    T1_strR[0][idxR] = GR1_strSetR[idxR];
    T1_strR[1][idxR] = T1_strR[0][idxR];
  }


  // seed the pipe with initial values
  ScoreStruct c_t;
  for (int i = 0; i< SCORE_WIDTH; i++){
      int l = i+1;
      if (l>SCORE_MAX) l = SCORE_WIDTH-i  ;
      c_t.m_score[i] = (Score_t)(0xf>>l);
  }
  Score_t c_strLenDiffA = (Score_t)((ht_int6)EDIT_DIST_W+T6_strLenDiff);
  bool c_strLenDiffRange = ((ht_int6)T6_strLenDiff > 0-EDIT_DIST_W) &&
          (T6_strLenDiff < EDIT_DIST_W);
  for (int idxL = 0; idxL < LINE_STR_CNT_L; idxL += 1) {
    CStrUnion strL = idxL < LINE_STR_CNT_L/2 ? T2_strL[0][idxL] : T2_strL[1][idxL];

    for (int idxR = 0; idxR < LINE_STR_CNT_R; idxR += 1) {
      CStrUnion strR = idxR < LINE_STR_CNT_R/2 ? T2_strR[0][idxR] : T2_strR[1][idxR];

      ByteCmpMatrix(T2_cmpMatrix[idxL][idxR], strL, strR);

      levenshtein2Band (0, T3_cmpValid && T3_lineMaskL[idxL] && T3_lineMaskR[idxR], TR3_cmpMatrix[idxL][idxR], c_t, T3_score[idxL][idxR]);
      levenshtein2Band (1, T4_cmpValid && T4_lineMaskL[idxL] && T4_lineMaskR[idxR], TR4_cmpMatrix[idxL][idxR], TR4_score[idxL][idxR], T4_score[idxL][idxR]);
      levenshtein2Band (2, T5_cmpValid && T5_lineMaskL[idxL] && T5_lineMaskR[idxR], TR5_cmpMatrix[idxL][idxR], TR5_score[idxL][idxR], T5_score[idxL][idxR]);
      levenshtein2Band (3, T6_cmpValid && T6_lineMaskL[idxL] && T6_lineMaskR[idxR], TR6_cmpMatrix[idxL][idxR], TR6_score[idxL][idxR], T6_score[idxL][idxR]);
      levenshtein2Band (4, T7_cmpValid && T7_lineMaskL[idxL] && T7_lineMaskR[idxR], TR7_cmpMatrix[idxL][idxR], TR7_score[idxL][idxR], T7_score[idxL][idxR]);
      // fanout strLenDiff for timing
      ht_attrib(equivalent_register_removal, TR7_strLenDiffA[idxL][idxR], "no");
      ht_attrib(equivalent_register_removal, TR7_strLenDiffRange[idxL][idxR], "no");
      T6_strLenDiffA[idxL][idxR] = c_strLenDiffA;
      T6_strLenDiffRange[idxL][idxR] = c_strLenDiffRange;
      // length difference and max allowed edit distance are the same for all compares in the same clock
      T7_matchMask[idxR * LINE_STR_CNT_L + idxL] = T7_lineMaskL[idxL] && T7_lineMaskR[idxR] &&
              ((T7_strLenDiffRange[idxL][idxR] ?
                    (Score_t)T7_score[idxL][idxR].m_score[(ht_uint3)T7_strLenDiffA[idxL][idxR]] :
                    (Score_t)SCORE4) | T7_mchAdjD) == T7_mchAdjD;

    }

  }

  // If any strings matched then push the mask and tbl indexes into a queue
  //   we push them on a queue because multiple matches may occur and we
  //   can only push one string match pair to the HIF per clock
  if (T8_cmpValid && T8_matchMask != 0) {
    CLineMatch S_lineMatch;
    S_lineMatch.m_matchMask = T8_matchMask;
    S_lineMatch.m_pair = T8_matchId;

    S_lineMatchQue.push(S_lineMatch);
  }

  if (!S_lineMatchQue.empty() && !SR_findNextMatchBusy) {
    // load compare registers

    S_lineMatch = S_lineMatchQue.front();
    S_lineMatchQue.pop();

    S_processedMask = 0;

    S_findNextMatchBusy = true;
  }

  S_hifOutQuePush = false;
  if (SR_findNextMatchBusy && !SendHostDataFullIn(1)) {
    // Iterate through bits set in matchMask
    //   Start by scanning bit mask from LSB, set mask register after each bit is processed
    bool bZero;
    LineStrCntL_t idxL;
    LineStrCntR_t idxR;
    FindNextMatch(bZero, idxL, idxR, S_processedMask, SR_processedMask, SR_lineMatch.m_matchMask);

    if (bZero) {
      S_findNextMatchBusy = false;
    } else {
      S_hifOutQuePush = true;

      S_strPair.m_conIdxL = (ConIdx_t)(SR_lineMatch.m_pair.m_conIdxL | idxL);
      S_strPair.m_conIdxR = (ConIdx_t)(SR_lineMatch.m_pair.m_conIdxR | idxR);
      S_strPair.m_conIdL = SR_lineMatch.m_pair.m_conIdL;
      S_strPair.m_conIdR = SR_lineMatch.m_pair.m_conIdR;

#ifndef _HTV
      //printf("StrPair(conIdL=%d, conIdR=%d, conIdxL=%d, conIdxR=%d)\n",
      //	S_strPair.m_conIdL, S_strPair.m_conIdR, S_strPair.m_conIdxL, S_strPair.m_conIdxR);
#endif
    }
  }

  if (SR_hifOutQuePush) {

    if (SR_strPair.m_conIdL != SR_strPair.m_conIdR || SR_strPair.m_conIdxL != SR_strPair.m_conIdxR) {
      SendHostData(SR_strPair.m_data64);
      S_rsltCnt += 1;

#ifndef _HTV
      extern FILE *hitFp;
      if (hitFp) fprintf(hitFp, "%d.%dL %d.%dR\n",
             (int)SR_strPair.m_conIdL, (int)SR_strPair.m_conIdxL, (int)SR_strPair.m_conIdR, (int)SR_strPair.m_conIdxR);
#endif
    }
  }

  if (GR_htReset) {
    S_processedMask = 0;
    S_findNextMatchBusy = false;
    S_rsltCnt = 0;
  }
}

#ifndef CHAR_MASK
#define CHAR_MASK	0x3f
#endif

void
CPersMch::ByteCmpMatrix(CmpMatrix_t &c_cmpMatrix, const CStrUnion &r_strL, const CStrUnion &r_strR)
{
  for (int iL = 0; iL < STR_CMP_LEN; iL += 1) {
    for (int iR = 0; iR < STR_CMP_LEN; iR += 1) {
      c_cmpMatrix[iR*STR_CMP_LEN+iL] = ((r_strL.m_byte[iL] ^ r_strR.m_byte[iR]) & CHAR_MASK) == 0;
    }
  }
}


void
CPersMch::levenshteinBand (int const &level, CmpMatrix_t const &matchArray,
                  ScoreStruct const &l, ScoreStruct &n) {
    // determine which match bits to use
    bool match[SCORE_WIDTH];
    ht_uint7 tStrLen = (ht_uint7)STR_CMP_LEN;

    if (level == 0 || level == STR_CMP_LEN-1) {
      // only bit 0,0 is used
      match[1] = true;
      match[3] = matchArray[(ht_uint7)((level-0)*tStrLen+(level+0))];
      match[5] = true;
    }
   else {
      match[1] = matchArray[(ht_uint7)((level-1)*tStrLen+(level+1))];
      match[3] = matchArray[(ht_uint7)((level-0)*tStrLen+(level+0))];
      match[5] = matchArray[(ht_uint7)((level+1)*tStrLen+(level-1))];
    }
    if (level == STR_CMP_LEN-1) {
      match[0] = true;
      match[2] = true;
      match[4] = true;
      match[6] = true;
    } else if (level == 0 || level == STR_CMP_LEN-2) {
      match[0] = true;
      match[2] = matchArray[(ht_uint7)((level-0)*tStrLen+(level+1))];
      match[4] = matchArray[(ht_uint7)((level+1)*tStrLen+(level-0))];
      match[6] = true;
    } else {
      match[0] = matchArray[(ht_uint7)((level-1)*tStrLen+(level+2))];
      match[2] = matchArray[(ht_uint7)((level-0)*tStrLen+(level+1))];
      match[4] = matchArray[(ht_uint7)((level+1)*tStrLen+(level-0))];
      match[6] = matchArray[(ht_uint7)((level+2)*tStrLen+(level-1))];
    }
    Score_t cost, left, above, diag;
    for (int i=1; i < SCORE_WIDTH; i+=2) {
        cost = (match[i]) ? (Score_t)0 : (Score_t)1;
        diag = (Score_t)(l.m_score[i]<<cost | cost);
        left = (Score_t)(l.m_score[i-1]<<1 | (Score_t)1);
        above =(Score_t)(l.m_score[i+1]<<1 | (Score_t)1);
        n.m_score[i] = diag & left & above;
    }
    cost = (match[0]) ? (Score_t)0 : (Score_t)1;
    left = (Score_t)(SCORE4);
    above = (Score_t)(n.m_score[0+1]<<1 | (Score_t)1);
    diag = (Score_t)(l.m_score[0]<<cost | cost);
    n.m_score[0] = diag & left & above;

    for (int i=2; i < SCORE_WIDTH-2; i+=2) {
        cost = (match[i]) ? (Score_t)0 : (Score_t)1;
        left = (Score_t)(n.m_score[i-1]<<1 | (Score_t)1);
        above = (Score_t)(n.m_score[i+1]<<1 | (Score_t)1);
        diag = (Score_t)(l.m_score[i]<<cost | cost);

        n.m_score[i] = diag & left & above;
    }
    cost = (match[SCORE_WIDTH-1]) ? (Score_t)0 : (Score_t)1;
    left = (Score_t)(n.m_score[SCORE_WIDTH-2]<<1 | (Score_t)1);
    above = (Score_t)(SCORE4);
    diag = (Score_t)(l.m_score[SCORE_WIDTH-1]<<cost | cost);
    n.m_score[SCORE_WIDTH-1] = diag & left & above;

}

void
CPersMch::levenshtein2Band (int const &level, bool const &valid, CmpMatrix_t const &matchArray,
                                   ScoreStruct const &l, ScoreStruct &n) {
    ScoreStruct m;
    levenshteinBand ((uint32_t)(level<<1), matchArray, l, m);
    levenshteinBand ((uint32_t)((level<<1)+1), matchArray, m, n);
//#define DEBUG
#ifdef DEBUG
    if (valid) {
        printf ("%d: ", level<<1);
        for (int i = 0; i<SCORE_WIDTH; i++)
            printf("%01x ",(int)l.m_score[i]);
        printf ("\n%d: ",(level<<1)+1);
        for (int i = 0; i<SCORE_WIDTH; i++)
            printf("%01x ", (int)n.m_score[i]);
        printf ("\n");
    }

#endif
}
void
CPersMch::FindNextMatch(bool &bZero, LineStrCntL_t &idxL, LineStrCntR_t &idxR, MatchMask_t &processedMask,
              MatchMask_t const &SR_processedMask, MatchMask_t const &r_matchMask)
{
  MatchMask_t activeMask = r_matchMask & ~SR_processedMask;

  // find non-zero lineStrL
  LineStrMaskR_t c_lineMaskR;
  for (int iR = 0; iR < LINE_STR_CNT_R; iR += 1)
    c_lineMaskR[iR] = activeMask((iR+1)*LINE_STR_CNT_L-1, iR*LINE_STR_CNT_L) != 0;

  uint16_t inR = (uint16_t) c_lineMaskR;
  ht_uint4 outR;
  Tzc(outR, inR);
  idxR = (LineStrCntR_t)outR;

  LineStrMaskL_t c_lineMaskL = (activeMask >> (idxR*LINE_STR_CNT_L)) & ((1ul << LINE_STR_CNT_L)-1);

  uint16_t inL = (uint16_t) c_lineMaskL;
  ht_uint4 outL;
  Tzc(outL, inL);
  idxL = (LineStrCntL_t)outL;

  bZero = c_lineMaskR == 0;

  processedMask = SR_processedMask | (MatchMask_t)((1ull << idxL) << (idxR*LINE_STR_CNT_L));
}

void
CPersMch::Tzc(ht_uint4 &cnt, uint16_t const &m)
{
  sc_uint<16> m0 = m;
  sc_uint<4> m1;
  m1[0] = m0(3,0) != 0;
  m1[1] = m0(7,4) != 0;
  m1[2] = m0(11,8) != 0;
  m1[3] = m0(15,12) != 0;

  if (m1[0])
    cnt(3,2) = 0;
  else if (m1[1])
    cnt(3,2) = 1;
  else if (m1[2])
    cnt(3,2) = 2;
  else
    cnt(3,2) = 3;

  sc_uint<4> m2;
  switch (cnt(3,2)) {
  case 0: m2 = m0(3,0); break;
  case 1: m2 = m0(7,4); break;
  case 2: m2 = m0(11,8); break;
  case 3: m2 = m0(15,12); break;
  }

  if (m2[0])
    cnt(1,0) = 0;
  else if (m2[1])
    cnt(1,0) = 1;
  else if (m2[2])
    cnt(1,0) = 2;
  else
    cnt(1,0) = 3;
}
