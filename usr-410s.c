#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <sys/time.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>
#include <pthread.h>

#include "usr-410s.h"

#define pr()  printf("%s %s %d\n", __FILE__, __func__, __LINE__)
#define DEF_BUFF_SIZE 1024


static int create_sk_cli(char *addr, char *port)
{
	int skfd;
	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	int is_first = 0;	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	ret = getaddrinfo(addr, port, &hints, &res);
	if(ret != 0){
		printf("getaddrinfo : %s\n", gai_strerror(ret));
		return -1;
	}
try_connect:
	for(p = res; p != NULL; p = p->ai_next){
		skfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
		if(skfd == -1){
			continue;
		}

		ret = connect(skfd, p->ai_addr, p->ai_addrlen);
		if(ret == -1){
			close(skfd);
			continue;
		}
		break;
	}

	if(p == NULL){
		if (skfd > 0)
			close(skfd);
		if (is_first == 0)
			printf("Fail to connect %s:%s , after 5s, i  will have a try \n", addr, port);
		is_first = 1;
		sleep(5);
		goto try_connect;
	}

	freeaddrinfo(res);
	
	return skfd;
}

static struct send_info *create_connect(char *ip, char *port)
{
	struct info_header *info_header = malloc(sizeof(struct info_header));
	if (!info_header){
		return NULL;
	}

	struct send_info *info = malloc(sizeof(struct send_info));
	if (!info){
		free(info_header);
		return NULL;
	}

	memset(info, 0 , sizeof(*info));
	info->buf = malloc(DEF_BUFF_SIZE);
	if (!(info->buf)){
		free(info_header);
		free(info);
		return NULL;
	}

	info->buf_len = DEF_BUFF_SIZE;
	info->ip = ip;
	info->port = port;
	info->skfd = create_sk_cli(ip, port);
	pthread_rwlock_init(&(info->rwlock), NULL);
	info->next = NULL;
	info->prev = NULL;
	if(info->skfd < 0){
		free(info_header);
		free(info->buf);
		free(info);
		return NULL;
	}
	info_header->info_header = info;
	info->header = info_header;
	pthread_rwlock_init(&(info_header->rwlock), NULL);
	return info;
}

struct send_info *dup_send_info(struct send_info *info)
{
	struct send_info *info_n = malloc(sizeof(struct send_info));
	if (!info_n){
		return NULL;
	}
	memset(info_n, 0 , sizeof(*info));
	info_n->buf = malloc(DEF_BUFF_SIZE);
	if (!(info_n->buf)){
		free(info_n);
		return NULL;
	}
	info_n->buf_len = DEF_BUFF_SIZE;
	pthread_rwlock_init(&(info_n->rwlock), NULL);
	
	pthread_rwlock_wrlock(&(info->header->rwlock));
	while(1){
		if(info->next)
			info = info->next;
		else
			break;
			
	}
	info_n->ip = info->ip;
	info_n->port = info->port;
	info_n->skfd = 0;
	info_n->prev = info;
	info->next = info_n;
	info_n->header = info->header;
	pthread_rwlock_unlock(&(info->header->rwlock));
	return info_n;	
}

int copy_to_buf(struct send_info *info, char *buffer, int len)
{
	char *buf;
	pthread_rwlock_wrlock(&(info->rwlock));
	if (info->buf_len - info->len < len){
		int n = (info->buf_len + len) / DEF_BUFF_SIZE + 1;
		buf = malloc(DEF_BUFF_SIZE * n);
		if (!buf)
			return -1;
		memcpy(buf, info->buf, info->len);
		free(info->buf);
		info->buf = buf;
		info->buf_len = n * DEF_BUFF_SIZE;
	}
	memcpy(info->buf + info->len, buffer, len);
	info->len += len;
	pthread_rwlock_unlock(&(info->rwlock));
	return len;	
}

void del_send_info(struct send_info *info)
{
	pthread_rwlock_wrlock(&(info->header->rwlock));
	if (info->prev){
		info->prev->next = info->next;
	}

	if (info->next){
		info->next->prev = info->prev;
	}
	pthread_rwlock_destroy(&(info->rwlock));
	
	pthread_rwlock_unlock(&(info->header->rwlock));
	
	free(info->buf);
	if (info->skfd)
		close(info->skfd);
	free(info);
}

void close_connect(struct send_info *info)
{
	struct info_header *header = info->header;
	struct send_info *i;
	while(header->info_header->next){
		i = header->info_header->next;
		del_send_info(i);
	}
	del_send_info(header->info_header);
	free(header);
}

int send_all_data(struct send_info *info)
{
	int wlen;
	 struct send_info *i;
	 i= info->header->info_header;
	 int skfd = i->skfd;
	 int try;
	 while(!i){ 
		pthread_rwlock_rdlock(&(i->rwlock));
		for(try = 0; (try < 3 && i->len > 0); try++){
         		wlen = send(skfd, i->buf, i->len, MSG_NOSIGNAL);
			if (wlen == -1){
				printf("disconnect ...\n");
				exit(-1);
			}
	 		i->len -= wlen;
		}
		if (i->len){
			printf("Data will be drop\n");
			i->len = 0;
		}
		pthread_rwlock_unlock(&(i->rwlock));
		i = i->next;
	 }
	return wlen;
}	

int send_data(struct send_info *info)
{
	int wlen;
	int skfd = info->header->info_header->skfd;
	pthread_rwlock_rdlock(&(info->rwlock));
        wlen = send(skfd, info->buf, info->len, MSG_NOSIGNAL);
	if (wlen == -1){
		printf("disconnect ...\n");
		exit(-1);
	}
	info->len -= wlen;
	pthread_rwlock_unlock(&(info->rwlock));
	return wlen;
}

int send_and_recv_data(struct send_info *info, char *buf, int len)
{
	int wlen;
	int skfd = info->header->info_header->skfd;
	pthread_rwlock_rdlock(&(info->rwlock));
        wlen = send(skfd, info->buf, info->len, MSG_NOSIGNAL);
	if (wlen == -1){
		printf("disconnect ...\n");
		exit(-1);
	}
	info->len -= wlen;
	pthread_rwlock_unlock(&(info->rwlock));
       	wlen = recv(skfd, buf, len, 0);	
	if (wlen == 0 && errno != EINTR){
		printf("disconnect ...\n");
		exit(-1);
	}
	return wlen;
}	

struct send_info* open_port(char *ip, char *port)
{

	struct send_info  *info;
	if ((info = create_connect(ip, port)) == NULL){
		printf("Can't create a connect to %s:%s\n", ip, port);
		return NULL;
	}
	struct timeval tv;
	tv.tv_usec = 200000;
	setsockopt(info->skfd, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(struct timeval));
	return info;
}
