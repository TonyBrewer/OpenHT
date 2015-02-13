#include "linux.h"

int fcntl(SOCKET s, int cmd, int val)
{
    u_long imode = 1;
    switch(cmd) {
        case F_SETFL:
            switch(val) {
                case O_NONBLOCK:
                    imode = 1;
                    if(ioctlsocket(s, FIONBIO, &imode) == SOCKET_ERROR)
                        return -1;
                    break;
                case O_BLOCK:
                    imode = 0;
                    if(ioctlsocket(s, FIONBIO, &imode) == SOCKET_ERROR)
                        return -1;
                    break;
                default:
                    return -1;
            }
        case F_GETFL:
            return 0;
        default:
            return -1;
    }
}
