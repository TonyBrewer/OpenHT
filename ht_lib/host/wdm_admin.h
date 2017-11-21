/*
 * Copyright (C) 2013-2015 Convey Computer Corp. All Rights Reserved.
 */
#ifndef __WDM_ADMIN_H__
#define __WDM_ADMIN_H__

/*
 * Include file dependencies.
 */
#include <errno.h>
#include <time.h>
#include <signal.h>
#include <stdint.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/uio.h>

#ifdef __cplusplus
extern "C"
{
#endif

/*
 * This header file defines the APIs for administrative utilities.  Typically,
 * these APIs require root privilege and a file descriptor to the firmware
 * device.
 */

/*
**============================================================================*
**============================================================================*
**
**	Wolverine Admin Runtime Library API prototypes:
**
**============================================================================*
**============================================================================*
*/

typedef uint64_t   wdm_admin_t;
#define WDM_ADM_INVALID    ((wdm_admin_t)0)

/*
 * Firmware Logical Device
 */
wdm_admin_t     wdm_attach_fw(int cpid_fw);
int             wdm_detach_fw(wdm_admin_t admin);

/*
 * Physical Logical Device
 */
wdm_admin_t     wdm_attach_phy(int cpid_phy);
int             wdm_detach_phy(wdm_admin_t admin);

/*
 * Coprocessor FPGA CSR Access:
 */
typedef enum {
	WDM_HIX = 1,
	WDM_MGMT,
	WDM_AEMC0,
	WDM_AEMC1,
	WDM_HIX_XADC,
	WDM_AEMC0_XADC,
	WDM_AEMC1_XADC,
	WDM_ALL_AEMC,
} wdm_fpga_t;

int     wdm_fpga_read_csr(wdm_admin_t coproc,
			  wdm_fpga_t  fpga,
			  uint64_t    offset,
			  uint64_t    *data);
int     wdm_fpga_write_csr(wdm_admin_t coproc,
			   wdm_fpga_t  fpga,
			   uint64_t    offset,
			   uint64_t    data);
int     wdm_fpga_rmw_csr(wdm_admin_t coproc,
			 wdm_fpga_t  fpga,
			 uint64_t    offset,
			 uint64_t    data,
			 uint64_t    mask,
			 uint64_t    *result);

/*
 * Coprocessor FPGA Loading:
 * =========================
 */
int     wdm_adm_load_fpga(wdm_admin_t admin,
			  char	      *fw_dev,
			  const char  *tgz_file);


#ifdef __cplusplus
}
#endif

#endif
