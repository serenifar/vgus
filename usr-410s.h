#ifndef __VGUS__CONTROLER____H____
#define __VGUS__CONTROLER____H____
#include <pthread.h>
struct info_header{
	struct send_info *info_header;  //= NULL;
	pthread_rwlock_t rwlock; //= PTHREAD_RWLOCK_INITIALIZER;
};

struct send_info
{
	int skfd;
	int buf_len;
	int len;
	char *buf;
	char *ip;
	char *port;
	struct send_info *next;
	struct send_info *prev;
	struct info_header *header;
	pthread_rwlock_t rwlock;
};

void close_connect(struct send_info *info);
struct send_info *open_port(char *ip, char *port);
int send_all_data(struct send_info *info);
void del_send_info(struct send_info *info);
int copy_to_buf(struct send_info *info, char *buffer, int len);
struct send_info *dup_send_info(struct send_info *info);
int send_and_recv_data(struct send_info *info, char *buf, int len);
int send_data(struct send_info *info);

#endif
