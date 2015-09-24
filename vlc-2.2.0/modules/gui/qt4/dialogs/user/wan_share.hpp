#ifndef __WAN_SHARE_H__
#define __WAN_SHARE_H__

#include <pthread.h>


extern void open_wan_share(int uid);

extern void close_wan_share(void);

extern int read_wan_share_status(void);


#endif
