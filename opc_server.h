#ifndef __OPC_SERVER_________H____
#define __OPC_SERVER_________H____
#include "usr-410s.h"
extern unsigned char *running;
extern unsigned int blocking;
extern pthread_mutex_t blocking_lock;
int start_opc_server(struct send_info *info_458, struct send_info *info_232);
#endif
