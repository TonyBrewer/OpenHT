#include <stdio.h>
#include <stddef.h>

#include "JobInfo.h"

#define DECSIZEQW(_x)	(int)(sizeof(m_dec._x)/sizeof(uint64_t))
#define DECOFFQW(_x)	(int)(offsetof(struct JobDec, _x)/sizeof(uint64_t))
#define ENCSIZEDW(_x)	(int)(sizeof(m_enc._x)/sizeof(uint32_t))
#define ENCSIZEQW(_x)	(int)(sizeof(m_enc._x)/sizeof(uint64_t))
#define ENCOFFDW(_x)	(int)(offsetof(struct JobEnc, _x)/sizeof(uint32_t))
#define ENCOFFQW(_x)	(int)(offsetof(struct JobEnc, _x)/sizeof(uint64_t))

int main(int argc, char **argv) {
	JobDec m_dec;
	JobEnc m_enc;

	printf("#define DEC_SECTION\t\t0\n");
	printf("\n");
	printf("#define DEC_RSTINFO_QCNT\t%d\n", DECSIZEQW(m_rstInfo));
	printf("#define DEC_RSTINFO_QOFF\t0x%x\n", DECOFFQW(m_rstInfo));
	printf("\n");
	printf("#define DEC_DHT_HUFF_QCNT\t%d\n", DECSIZEQW(m_dcDht[0].m_huffCode));
	printf("#define DEC_DCDHT0_HUFF_QOFF\t0x%x\n", DECOFFQW(m_dcDht[0].m_huffCode));
	printf("#define DEC_DCDHT1_HUFF_QOFF\t0x%x\n", DECOFFQW(m_dcDht[1].m_huffCode));
	printf("#define DEC_ACDHT0_HUFF_QOFF\t0x%x\n", DECOFFQW(m_acDht[0].m_huffCode));
	printf("#define DEC_ACDHT1_HUFF_QOFF\t0x%x\n", DECOFFQW(m_acDht[1].m_huffCode));
	printf("\n");
	printf("#define DEC_DHT_MAX_QCNT\t%d\n", DECSIZEQW(m_dcDht[0].m_maxCode));
	printf("#define DEC_DCDHT0_MAX_QOFF\t0x%x\n", DECOFFQW(m_dcDht[0].m_maxCode));
	printf("#define DEC_DCDHT1_MAX_QOFF\t0x%x\n", DECOFFQW(m_dcDht[1].m_maxCode));
	printf("#define DEC_ACDHT0_MAX_QOFF\t0x%x\n", DECOFFQW(m_acDht[0].m_maxCode));
	printf("#define DEC_ACDHT1_MAX_QOFF\t0x%x\n", DECOFFQW(m_acDht[1].m_maxCode));
	printf("\n");
	printf("#define DEC_DHT_VAL_QCNT\t%d\n", DECSIZEQW(m_dcDht[0].m_valOffset));
	printf("#define DEC_DCDHT0_VAL_QOFF\t0x%x\n", DECOFFQW(m_dcDht[0].m_valOffset));
	printf("#define DEC_DCDHT1_VAL_QOFF\t0x%x\n", DECOFFQW(m_dcDht[1].m_valOffset));
	printf("#define DEC_ACDHT0_VAL_QOFF\t0x%x\n", DECOFFQW(m_acDht[0].m_valOffset));
	printf("#define DEC_ACDHT1_VAL_QOFF\t0x%x\n", DECOFFQW(m_acDht[1].m_valOffset));
	printf("\n");
	printf("#define DEC_DHT_LOOK_QCNT\t%d\n", DECSIZEQW(m_dcDht[0].m_lookup));
	printf("#define DEC_DCDHT0_LOOK_QOFF\t0x%x\n", DECOFFQW(m_dcDht[0].m_lookup));
	printf("#define DEC_DCDHT1_LOOK_QOFF\t0x%x\n", DECOFFQW(m_dcDht[1].m_lookup));
	printf("#define DEC_ACDHT0_LOOK_QOFF\t0x%x\n", DECOFFQW(m_acDht[0].m_lookup));
	printf("#define DEC_ACDHT1_LOOK_QOFF\t0x%x\n", DECOFFQW(m_acDht[1].m_lookup));
	printf("\n");
	printf("#define DEC_DQT_QCNT\t\t%d\n", DECSIZEQW(m_dqt[0]));
	printf("#define DEC_DQT0_QOFF\t\t0x%x\n", DECOFFQW(m_dqt[0]));
	printf("#define DEC_DQT1_QOFF\t\t0x%x\n", DECOFFQW(m_dqt[1]));
	printf("#define DEC_DQT2_QOFF\t\t0x%x\n", DECOFFQW(m_dqt[2]));
	printf("#define DEC_DQT3_QOFF\t\t0x%x\n", DECOFFQW(m_dqt[3]));
	printf("\n");
	printf("#define DEC_DCP0_QOFF\t\t0x%x\n", DECOFFQW(m_dcp[0]));
	printf("#define DEC_DCP1_QOFF\t\t0x%x\n", DECOFFQW(m_dcp[1]));
	printf("#define DEC_DCP2_QOFF\t\t0x%x\n", DECOFFQW(m_dcp[2]));
	printf("\n");
	printf("\n");
	printf("#define ENC_SECTION\t\t3\n");
	printf("#define ENC_OFFSET\t\t0x%lx\n", offsetof(struct JobInfo, m_enc));
	printf("\n");
	printf("#define ENC_DHT_HUFF_QCNT\t%d\n", ENCSIZEQW(m_dcDht[0].m_huffCode));
	printf("#define ENC_DCDHT0_HUFF_QOFF\t0x%x\n", ENCOFFQW(m_dcDht[0].m_huffCode));
	printf("#define ENC_DCDHT1_HUFF_QOFF\t0x%x\n", ENCOFFQW(m_dcDht[1].m_huffCode));
	printf("#define ENC_ACDHT0_HUFF_QOFF\t0x%x\n", ENCOFFQW(m_acDht[0].m_huffCode));
	printf("#define ENC_ACDHT1_HUFF_QOFF\t0x%x\n", ENCOFFQW(m_acDht[1].m_huffCode));
	printf("\n");
	printf("#define ENC_DHT_SIZE_QCNT\t%d\n", ENCSIZEQW(m_dcDht[0].m_huffSize));
	printf("#define ENC_DCDHT0_SIZE_QOFF\t0x%x\n", ENCOFFQW(m_dcDht[0].m_huffSize));
	printf("#define ENC_DCDHT1_SIZE_QOFF\t0x%x\n", ENCOFFQW(m_dcDht[1].m_huffSize));
	printf("#define ENC_ACDHT0_SIZE_QOFF\t0x%x\n", ENCOFFQW(m_acDht[0].m_huffSize));
	printf("#define ENC_ACDHT1_SIZE_QOFF\t0x%x\n", ENCOFFQW(m_acDht[1].m_huffSize));
	printf("\n");
	printf("#define ENC_DQT_QCNT\t\t%d\n", ENCSIZEQW(m_dqt[0]));
	printf("#define ENC_DQT0_QOFF\t\t0x%x\n", ENCOFFQW(m_dqt[0]));
	printf("#define ENC_DQT1_QOFF\t\t0x%x\n", ENCOFFQW(m_dqt[1]));
	printf("#define ENC_DQT2_QOFF\t\t0x%x\n", ENCOFFQW(m_dqt[2]));
	printf("#define ENC_DQT3_QOFF\t\t0x%x\n", ENCOFFQW(m_dqt[3]));
	printf("\n");
	printf("#define ENC_ECP0_QOFF\t\t0x%x\n", ENCOFFQW(m_ecp[0]));
	printf("#define ENC_ECP1_QOFF\t\t0x%x\n", ENCOFFQW(m_ecp[1]));
	printf("#define ENC_ECP2_QOFF\t\t0x%x\n", ENCOFFQW(m_ecp[2]));
	printf("\n");
	printf("#define ENC_RSTCNT_QOFF\t\t0x%x\n", ENCOFFQW(m_pRstBase)-1);
	printf("#define ENC_RSTBASE_QOFF\t0x%x\n", ENCOFFQW(m_pRstBase));
	printf("#define ENC_RSTOFF_QOFF\t\t0x%x\n", ENCOFFQW(m_rstOffset));
	printf("#define ENC_RSTLEN_DCNT\t\t%d\n", ENCSIZEDW(m_rstLength));
	printf("#define ENC_RSTLEN_DOFF\t\t0x%x\n", ENCOFFDW(m_rstLength));

	return 0;
}
