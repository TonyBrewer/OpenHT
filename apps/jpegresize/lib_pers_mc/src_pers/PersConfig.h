/* Copyright (c) 2015 Convey Computer Corporation
 *
 * This file is part of the OpenHT jpegscale application.
 *
 * Use and distribution licensed under the BSD 3-clause license.
 * See the LICENSE file for the complete license text.
 */
#pragma once

#define IHUF2
#define NEW_JOB_PNT_INFO

#if defined(DEC_FIXTURE)

#define DHINFO
#define DEC

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define DPIPE_W 3
#define DPIPE_CNT (1<<DPIPE_W)

#elif defined(HORZ_FIXTURE)

#define DHINFO
#define HIMG
#define HORZ
#define HSMC
#define HSMW
#define HDM

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define CHECK_INBOUND_HORZ_MSG
#define FORCE_HORZ_WRITE_STALLS
#define CHECK_HTB_BUF_CONFLICTS

#elif defined (VERT_FIXTURE)

#define VEINFO
#define VERT
#define VIMG
#define VSM
#define VWM

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define VERT_MCU_ROWS 8

#elif defined(ENC_FIXTURE)

#define VEINFO
#define ENC

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define EPIPE_W 3
#define EPIPE_CNT (1<<EPIPE_W)

#elif defined(DHIMG_FIXTURE)

#define DHINFO
#define DEC
#define HORZ
#define HDM

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define DEC_REPL_MASK 0
#define HORZ_REPL_MASK 0
#define VERT_REPL_MASK 0
#define ENC_REPL_MASK 0

#define DPIPE_W 3
#define DPIPE_CNT 6

#define CHECK_INBOUND_HORZ_MSG

#elif defined (VEIMG_FIXTURE)

#define VSM
#define VEINFO
#define VERT
#define ENC

#define EPIPE_W 2
#define EPIPE_CNT (1<<EPIPE_W)

#define VERT_MCU_ROWS EPIPE_CNT

#define VERT_IMAGES_W 1
#define VERT_IMAGES (1 << VERT_IMAGES_W)
#define VERT_IMAGES_MASK (VERT_IMAGES-1)

#define ENC_IMAGES_W 1
#define ENC_IMAGES (1 << ENC_IMAGES_W)
#define ENC_IMAGES_MASK (ENC_IMAGES-1)

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define DEC_REPL_MASK 0
#define HORZ_REPL_MASK 0
#define VERT_REPL_MASK 0
#define ENC_REPL_MASK 0

#elif defined (IMG_FIXTURE)
// full personality DEC/HORZ/VERT/ENC

#define CHECK_INBOUND_HORZ_MSG

#define IMG

#define DHINFO
#define DEC
#define HORZ

#define VEINFO
#define VERT
#define ENC

#define DPIPE_W 3
#define DPIPE_CNT 6

#define EPIPE_W 2
#define EPIPE_CNT (1<<EPIPE_W)

#define VERT_MCU_ROWS EPIPE_CNT

#define VERT_IMAGES_W 1
#define VERT_IMAGES (1 << VERT_IMAGES_W)
#define VERT_IMAGES_MASK (VERT_IMAGES-1)

#define ENC_IMAGES_W 1
#define ENC_IMAGES (1 << ENC_IMAGES_W)
#define ENC_IMAGES_MASK (ENC_IMAGES-1)

#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define DEC_REPL_MASK 0
#define HORZ_REPL_MASK 0
#define VERT_REPL_MASK 0
#define ENC_REPL_MASK 0

#else
	// full personality DEC/HORZ/VERT/ENC

#define IMG4

#define DHINFO
#define DEC
#define HORZ

#define VEINFO
#define VERT
#define ENC

#define DPIPE_W 3
#define DPIPE_CNT 6

#define EPIPE_W 2
#define EPIPE_CNT (1<<EPIPE_W)

#define VERT_MCU_ROWS 4

#define VERT_IMAGES_W 1
#define VERT_IMAGES (1 << VERT_IMAGES_W)
#define VERT_IMAGES_MASK (VERT_IMAGES-1)

#define ENC_IMAGES_W 1
#define ENC_IMAGES (1 << ENC_IMAGES_W)
#define ENC_IMAGES_MASK (ENC_IMAGES-1)

#define IMG4_REPL4
#ifdef IMG4_REPL4
#define DEC_REPL 4
#define HORZ_REPL 4
#define VERT_REPL 2
#define ENC_REPL 2

#define DEC_REPL_MASK 3
#define HORZ_REPL_MASK 3
#define VERT_REPL_MASK 1
#define ENC_REPL_MASK 1

#else
#define DEC_REPL 1
#define HORZ_REPL 1
#define VERT_REPL 1
#define ENC_REPL 1

#define DEC_REPL_MASK 0
#define HORZ_REPL_MASK 0
#define VERT_REPL_MASK 0
#define ENC_REPL_MASK 0

#endif

//#define FORCE_ENC_STALLS
//#define FORCE_HORZ_WRITE_STALLS

#endif
