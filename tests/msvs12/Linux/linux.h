#pragma once

#include <Winsock2.h>
#include <ws2tcpip.h>
#include <errno.h>
#include "bsd_getopt.h"
//#include "sys/signal.h"
#include <io.h>
#include <process.h>
#include <stdint.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

#if defined(_MT) || defined(_DLL)
# define set_errno(x)    (*_errno()) = (x)
#else
# define set_errno(x)    errno = (x)
#endif

    //#define EADDRINUSE  WSAEADDRINUSE
#define ENDIAN_LITTLE 1

#pragma warning(disable : 4996)

#include <stdint.h>
#ifndef __cplusplus
    typedef enum {false = 0, true = 1} bool;
#endif

#define strtok_r strtok_s

    //#define pid_t int
    //#define EWOULDBLOCK        EAGAIN
    //#define EAFNOSUPPORT       47
    typedef int socklen_t;
    typedef int ssize_t;
    typedef char *	caddr_t;

#define EAI_SYSTEM 11

#define PATH_MAX	1024

#define O_BLOCK 0
#define O_NONBLOCK 1
#define F_GETFL 3
#define F_SETFL 4

#define IOV_MAX 1024
    struct iovec {
        size_t iov_len;  
        void * iov_base;
    };
    struct msghdr
    {
        void	*msg_name;			/* Socket name			*/
        int		 msg_namelen;		/* Length of name		*/
        struct iovec *msg_iov;		/* Data blocks			*/
        size_t	 msg_iovlen;		/* Number of blocks		*/
        void	*msg_accrights;		/* Per protocol magic (eg BSD file descriptor passing) */ 
        int		 msg_accrightslen;	/* Length of rights list */
    };

    int fcntl(SOCKET s, int cmd, int val);
    int inet_aton(register const char *cp, struct in_addr *addr);

//#define inline __inline

#define MSG_DONTWAIT    0

    struct mmsghdr {
        struct msghdr msg_hdr;  // message header
        unsigned int  msg_len;  // number of bytes transmitted
    };

#define SEND_BUF_SIZE   2000

    inline int sendmmsg(int s, struct mmsghdr *mmsgVec, int mmsgCnt, int flags)
    {
        char sendBuf[SEND_BUF_SIZE];
        int mmsgIdx;
        unsigned int iovIdx;
        size_t size;
        int res;

        // mimic linux's multi-messsage send interface
        //   copy data to send into buffer
        assert(mmsgCnt > 0);

        if (mmsgVec[0].msg_hdr.msg_namelen == 0) {
            // TCP
            size_t bufPos = 0;
            for (mmsgIdx = 0; mmsgIdx < mmsgCnt && bufPos < SEND_BUF_SIZE; mmsgIdx += 1) {

                for (iovIdx = 0; iovIdx < mmsgVec[mmsgIdx].msg_hdr.msg_iovlen && bufPos < SEND_BUF_SIZE; iovIdx += 1) {

                    size = min(SEND_BUF_SIZE-bufPos, mmsgVec[mmsgIdx].msg_hdr.msg_iov[iovIdx].iov_len);
                    memcpy(sendBuf + bufPos, mmsgVec[mmsgIdx].msg_hdr.msg_iov[iovIdx].iov_base, size);
                    bufPos += size;
                }
            }

            res = send((SOCKET)s, sendBuf, (int)bufPos, 0);
        } else
            assert(0);

        return res;
    }

    inline int sendmsg(int s, const struct msghdr *msg, int flags)
    {
        char sendBuf[SEND_BUF_SIZE];

        if (msg->msg_namelen == 0) {
            // TCP
            int sentLen = 0;
            size_t i;
            size_t bufPos = 0;

            for (i = 0; i < msg->msg_iovlen && bufPos < SEND_BUF_SIZE; i += 1) {
                size_t len = min(SEND_BUF_SIZE-bufPos, msg->msg_iov[i].iov_len);
                memcpy(sendBuf + bufPos, msg->msg_iov[i].iov_base, len);
                bufPos += len;
            }

            return send((SOCKET)s, sendBuf, (int)bufPos, 0);

        } else {
            // UDP
            DWORD dwBufferCount;
            if(WSASendTo((SOCKET) s,
                (LPWSABUF)msg->msg_iov,
                (DWORD)msg->msg_iovlen,
                &dwBufferCount,
                flags,
                (const struct sockaddr *)msg->msg_name,
                msg->msg_namelen,
                NULL,
                NULL
                ) == 0) {
                    return dwBufferCount;
            }
            if(WSAGetLastError() == WSAECONNRESET) return 0;
            return -1;
        }
    }

#undef errno
#define errno werrno()
    inline int werrno()
    {
        int error = WSAGetLastError();
        if(error == 0) error = *_errno();

        switch(error) {
        default:
            return error;
        case WSAEPFNOSUPPORT:
            set_errno(EAFNOSUPPORT);
            return EAFNOSUPPORT;
        case WSA_IO_PENDING:
        case WSATRY_AGAIN:
            set_errno(EAGAIN);
            return EAGAIN;
        case WSAEWOULDBLOCK:
            set_errno(EWOULDBLOCK);
            return EWOULDBLOCK;
        case WSAEMSGSIZE:
            set_errno(E2BIG);
            return E2BIG;
        case WSAECONNRESET:
            set_errno(0);
            return 0;
        }
    }

#if _MSC_VER < 1300
#define strtoll(p, e, b) ((*(e) = (char*)(p) + (((b) == 10) ? strspn((p), "0123456789") : 0)), _atoi64(p))
#else
#define strtoll(p, e, b) _strtoi64(p, e, b) 
#endif


#ifndef snprintf
#define snprintf _snprintf
#endif

#ifndef strtoull
#define strtoull(p, e, b) _strtoui64(p, e, b)
#endif

    struct sockaddr_un {
        int sun_family;
        char * sun_path;
    };

#define F_OK   0

#define S_ISSOCK(a) 0

    // sys/stat.h
    int __inline umask(int cmask) { assert(0); return 0; }

    int __inline lstat(const char * path, struct stat * buf) { assert(0); return 0; }


    struct timezone {
        int tz_minuteswest;     /* minutes W of Greenwich */
        int tz_dsttime;         /* type of dst correction */
    };

    int gettimeofday(struct timeval *tv, struct timezone *tz);

    inline void sleep(int sec) { Sleep(sec*1000); }

#define __sync_synchronize()
#define __sync_fetch_and_add(a, v)  InterlockedExchangeAdd(a, v)

#define __sync_fetch_and_or(a, v)   CnyInterlockedOr(a, v)
inline int64_t CnyInterlockedOr(volatile int64_t * a, int64_t v) { return InterlockedOr64(a, v); }

#define __builtin_prefetch(a)       _mm_prefetch((const char*)a, _MM_HINT_T0)

#define cny_cp_memset(s, c, l)      memset(s, c, l)

    int __inline posix_memalign(void ** pMem, size_t align, size_t size) {
        *pMem = _aligned_malloc(size, align);
        return *pMem == NULL ? -1 : 0;
    }

    int __inline cny_cp_posix_memalign(void ** pMem, size_t align, size_t size) {
        *pMem = _aligned_malloc(size, align);
        return *pMem == NULL ? -1 : 0;
    }

#ifdef __cplusplus
}
#endif
