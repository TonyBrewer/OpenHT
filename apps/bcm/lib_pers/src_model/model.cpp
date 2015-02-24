#include <Ht.h>
using namespace Ht;
#include "../src/HtBcmLib.h"

#define BcmByteSwap16(value) \
	((((value) & 0xff) << 8) | (((value) & 0xff00) >> 8))

#define BcmByteSwap32(value) \
	((BcmByteSwap16(value) << 16) | BcmByteSwap16(value >> 16))

static inline void BcmFlip(void *dest_p, const void *src_p, int size8)
{
	assert((size8 & 0x3) == 0);
	uint32_t *dest = (uint32_t *)dest_p;
	const uint32_t *src = (uint32_t const *)src_p;
	int i;

	for (i = 0; i < size8 / 4; i++)
		dest[i] = BcmByteSwap32(src[i]);
}

static void BcmSha2InitState(uint32_t *pState)
{
	pState[0] = 0x6A09E667;
	pState[1] = 0xBB67AE85;
	pState[2] = 0x3C6EF372;
	pState[3] = 0xA54FF53A;
	pState[4] = 0x510E527F;
	pState[5] = 0x9B05688C;
	pState[6] = 0x1F83D9AB;
	pState[7] = 0x5BE0CD19;
}

static void BcmSha2(uint32_t *pState, const unsigned char data[64])
{
	uint32_t temp1, temp2, W[64];
	uint32_t A, B, C, D, E, F, G, H;

	BcmFlip(W, data, 64);

#define  SHR(x, n) ((x & 0xFFFFFFFF) >> n)
#define ROTR(x, n) (SHR(x, n) | (x << (32 - n)))

#define S0(x) (ROTR(x, 7) ^ ROTR(x, 18) ^ SHR(x, 3))
#define S1(x) (ROTR(x, 17) ^ ROTR(x, 19) ^ SHR(x, 10))

#define S2(x) (ROTR(x, 2) ^ ROTR(x, 13) ^ ROTR(x, 22))
#define S3(x) (ROTR(x, 6) ^ ROTR(x, 11) ^ ROTR(x, 25))

#define F0(x, y, z) ((x & y) | (z & (x | y)))
#define F1(x, y, z) (z ^ (x & (y ^ z)))

#define R(t) (                                   \
		W[t] = S1(W[t - 2]) + W[t - 7] + \
		       S0(W[t - 15]) + W[t - 16] \
)

#define P(a, b, c, d, e, f, g, h, x, K) {                \
		temp1 = h + S3(e) + F1(e, f, g) + K + x; \
		temp2 = S2(a) + F0(a, b, c);             \
		d += temp1; h = temp1 + temp2;           \
}

	A = pState[0];
	B = pState[1];
	C = pState[2];
	D = pState[3];
	E = pState[4];
	F = pState[5];
	G = pState[6];
	H = pState[7];

	P(A, B, C, D, E, F, G, H, W[0], 0x428A2F98);
	P(H, A, B, C, D, E, F, G, W[1], 0x71374491);
	P(G, H, A, B, C, D, E, F, W[2], 0xB5C0FBCF);
	P(F, G, H, A, B, C, D, E, W[3], 0xE9B5DBA5);
	P(E, F, G, H, A, B, C, D, W[4], 0x3956C25B);
	P(D, E, F, G, H, A, B, C, W[5], 0x59F111F1);
	P(C, D, E, F, G, H, A, B, W[6], 0x923F82A4);
	P(B, C, D, E, F, G, H, A, W[7], 0xAB1C5ED5);
	P(A, B, C, D, E, F, G, H, W[8], 0xD807AA98);
	P(H, A, B, C, D, E, F, G, W[9], 0x12835B01);
	P(G, H, A, B, C, D, E, F, W[10], 0x243185BE);
	P(F, G, H, A, B, C, D, E, W[11], 0x550C7DC3);
	P(E, F, G, H, A, B, C, D, W[12], 0x72BE5D74);
	P(D, E, F, G, H, A, B, C, W[13], 0x80DEB1FE);
	P(C, D, E, F, G, H, A, B, W[14], 0x9BDC06A7);
	P(B, C, D, E, F, G, H, A, W[15], 0xC19BF174);
	P(A, B, C, D, E, F, G, H, R(16), 0xE49B69C1);
	P(H, A, B, C, D, E, F, G, R(17), 0xEFBE4786);
	P(G, H, A, B, C, D, E, F, R(18), 0x0FC19DC6);
	P(F, G, H, A, B, C, D, E, R(19), 0x240CA1CC);
	P(E, F, G, H, A, B, C, D, R(20), 0x2DE92C6F);
	P(D, E, F, G, H, A, B, C, R(21), 0x4A7484AA);
	P(C, D, E, F, G, H, A, B, R(22), 0x5CB0A9DC);
	P(B, C, D, E, F, G, H, A, R(23), 0x76F988DA);
	P(A, B, C, D, E, F, G, H, R(24), 0x983E5152);
	P(H, A, B, C, D, E, F, G, R(25), 0xA831C66D);
	P(G, H, A, B, C, D, E, F, R(26), 0xB00327C8);
	P(F, G, H, A, B, C, D, E, R(27), 0xBF597FC7);
	P(E, F, G, H, A, B, C, D, R(28), 0xC6E00BF3);
	P(D, E, F, G, H, A, B, C, R(29), 0xD5A79147);
	P(C, D, E, F, G, H, A, B, R(30), 0x06CA6351);
	P(B, C, D, E, F, G, H, A, R(31), 0x14292967);
	P(A, B, C, D, E, F, G, H, R(32), 0x27B70A85);
	P(H, A, B, C, D, E, F, G, R(33), 0x2E1B2138);
	P(G, H, A, B, C, D, E, F, R(34), 0x4D2C6DFC);
	P(F, G, H, A, B, C, D, E, R(35), 0x53380D13);
	P(E, F, G, H, A, B, C, D, R(36), 0x650A7354);
	P(D, E, F, G, H, A, B, C, R(37), 0x766A0ABB);
	P(C, D, E, F, G, H, A, B, R(38), 0x81C2C92E);
	P(B, C, D, E, F, G, H, A, R(39), 0x92722C85);
	P(A, B, C, D, E, F, G, H, R(40), 0xA2BFE8A1);
	P(H, A, B, C, D, E, F, G, R(41), 0xA81A664B);
	P(G, H, A, B, C, D, E, F, R(42), 0xC24B8B70);
	P(F, G, H, A, B, C, D, E, R(43), 0xC76C51A3);
	P(E, F, G, H, A, B, C, D, R(44), 0xD192E819);
	P(D, E, F, G, H, A, B, C, R(45), 0xD6990624);
	P(C, D, E, F, G, H, A, B, R(46), 0xF40E3585);
	P(B, C, D, E, F, G, H, A, R(47), 0x106AA070);
	P(A, B, C, D, E, F, G, H, R(48), 0x19A4C116);
	P(H, A, B, C, D, E, F, G, R(49), 0x1E376C08);
	P(G, H, A, B, C, D, E, F, R(50), 0x2748774C);
	P(F, G, H, A, B, C, D, E, R(51), 0x34B0BCB5);
	P(E, F, G, H, A, B, C, D, R(52), 0x391C0CB3);
	P(D, E, F, G, H, A, B, C, R(53), 0x4ED8AA4A);
	P(C, D, E, F, G, H, A, B, R(54), 0x5B9CCA4F);
	P(B, C, D, E, F, G, H, A, R(55), 0x682E6FF3);
	P(A, B, C, D, E, F, G, H, R(56), 0x748F82EE);
	P(H, A, B, C, D, E, F, G, R(57), 0x78A5636F);
	P(G, H, A, B, C, D, E, F, R(58), 0x84C87814);
	P(F, G, H, A, B, C, D, E, R(59), 0x8CC70208);
	P(E, F, G, H, A, B, C, D, R(60), 0x90BEFFFA);
	P(D, E, F, G, H, A, B, C, R(61), 0xA4506CEB);
	P(C, D, E, F, G, H, A, B, R(62), 0xBEF9A3F7);
	P(B, C, D, E, F, G, H, A, R(63), 0xC67178F2);

	pState[0] += A;
	pState[1] += B;
	pState[2] += C;
	pState[3] += D;
	pState[4] += E;
	pState[5] += F;
	pState[6] += G;
	pState[7] += H;
}

bool BcmTargetCmp(uint32_t *pState, uint32_t *pTarget)
{
	for (int i = 7; i >= 0; i -= 1) {
		if (pState[i] < pTarget[i])
			return true;
		else if (pState[i] > pTarget[i])
			return false;
	}

	return true;
}

void FinishTask(queue<CHtBcmTask *> &rdyTaskQue, bool &bInitTask, CHtModelAuUnit *pUnit)
{
	if (rdyTaskQue.empty())
		return;

	CHtBcmTask *pBcmTask = rdyTaskQue.front();
	rdyTaskQue.pop();

	bInitTask = true;

	while (!pUnit->SendReturn_bcm(pBcmTask))
		usleep(1000);
}

void CHtModelAuUnit::UnitThread()
{
	queue<CHtBcmTask *> rdyTaskQue;
	queue<CHtBcmRslt *> avlRsltQue;

	CHtBcmTask *pBcmTask;

	uint32_t nonce = 0;
	bool bInitTask = true;
	uint32_t target[8];
	bool bHalt = false;

	do {
		ERecvType eRecvType = RecvPoll();

		switch (eRecvType) {
		case eRecvIdle:
			break;
		case eRecvHostMsg:
		{
			uint8_t msgType;
			uint64_t msgData;
			RecvHostMsg(msgType, msgData);

			if (msgType == BCM_FORCE_RETURN) {
				FinishTask(rdyTaskQue, bInitTask, this);
			} else if (msgType == BCM_RSLT_BUF_AVL) {
				CHtBcmRslt *pBcmRslt = (CHtBcmRslt *)msgData;
				avlRsltQue.push(pBcmRslt);
			} else if (msgType == BCM_ZERO_INPUT) {
				// Do nothing with this right now... no real place in the model
			} else {
				assert(0);
			}
		}
		break;
		case eRecvCall:
		{
			void *pVoid = 0;
			RecvCall_bcm(pVoid);
			CHtBcmTask *pBcmTask = (CHtBcmTask *)pVoid;

			rdyTaskQue.push(pBcmTask);
		}
		break;
		case eRecvHalt:
			bHalt = true;
			break;
		default:
			assert(0);
		}

		// process active task
		if (!rdyTaskQue.empty()) {
			pBcmTask = rdyTaskQue.front();

			if (bInitTask) {
				nonce = pBcmTask->m_initNonce;

				memcpy(target + 5, pBcmTask->m_target, 12);
				memset(target, 0x00, 20);

				bInitTask = false;
			}

			// re-establish midstate
			uint32_t state1[8];

			//BcmFlip( state1, pBcmTask->m_midState, 32 );
			memcpy(state1, pBcmTask->m_midState, 32);

			// create 64-byte data buffer for first pass
			unsigned char buffer1[64] = { 0 };
			BcmFlip(buffer1, pBcmTask->m_data, 12);
			BcmFlip(buffer1 + 12, &nonce, 4);
			buffer1[16] = 0x80;
			buffer1[62] = 2;
			buffer1[63] = 0x80;

			// first pass sha256
			BcmSha2(state1, buffer1);

			// second pass, initialize state to constant value
			uint32_t state2[8];
			BcmSha2InitState(state2);

			// create 64-byte data buffer for second pass
			uint8_t buffer2[64] = { 0 };
			BcmFlip(buffer2, state1, 32);
			buffer2[32] = 0x80;
			buffer2[62] = 1;

			// second pass sha256
			BcmSha2(state2, buffer2);

			BcmFlip(state2, state2, 32);

			if (BcmTargetCmp(state2, target)) {
				while (avlRsltQue.empty())
					usleep(1000);

				CHtBcmRslt *pBcmRslt = avlRsltQue.front();
				avlRsltQue.pop();

				memcpy(pBcmRslt->m_midState, ((CHtBcmTask *)pBcmTask)->m_midState, 32);
				memcpy(pBcmRslt->m_data, ((CHtBcmTask *)pBcmTask)->m_data, 12);
				memcpy(&pBcmRslt->m_nonce, &nonce, 4);

				SendHostMsg(BCM_RSLT_BUF_RDY, (uint64_t)pBcmRslt);
			}

			if (nonce++ == pBcmTask->m_lastNonce)
				FinishTask(rdyTaskQue, bInitTask, this);
		} else {
			usleep(1000);
		}
	} while (!bHalt);
}
