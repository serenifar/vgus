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

#include "vgus.h"
#include "usr-410s.h"
#include "opc_server.h"
#include "modbus_485.h"

#define pr() printf("%s %s %d\n", __FILE__, __func__, __LINE__)

int lock_file(int fd)
{
	struct flock fl;
	fl.l_type = F_WRLCK;
	fl.l_start = 0;
	fl.l_whence = SEEK_SET;
	fl.l_len = 0;
	return (fcntl(fd, F_SETLK, &fl));
}

int is_already_running(char *lockfile)
{
	return open(lockfile, O_RDWR | O_CREAT, LOCK_MODE );
}

int lock_running(int fd)
{
	char buf[16];
	if (lock_file(fd) < 0){
		close(fd);
		return 1;
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;

}

void *start_xenomai(void *arg)
{
	struct send_info *info = arg;
	unsigned int data;	
	int fd; 
	if ((fd = access(FIFO_FILE, F_OK)) < 0){
		fd = mkfifo(FIFO_FILE, 0666);
		if (fd < 0)
			return (void *)(0);
	}

	char buf[16];
	int num;
	while (*running){
		if ((fd = open(FIFO_FILE, O_RDONLY)) < 0){
			return (void *)(0);
		}
		num = read(fd, buf, 16);
		close(fd);

		if (strncmp("exit", buf, 4) == 0)
			return (void *)(0);
		data = atoi(buf);
		if(data > x_screen->curve.y_max  || data <  x_screen->curve.y_min)
			continue;
		xenomai_update_curve(info, data);
		send_data(info);
	}
	pthread_exit((void *)0);
}

unsigned int blocking = 0;
pthread_mutex_t blocking_lock;

#define block_opc_client() pthread_mutex_lock(&blocking_lock); blocking = 1
#define unblock_opc_client() blocking = 0; pthread_mutex_unlock(&blocking_lock)

#define COORD_SAVE_MAX 10
struct {
	unsigned short screen_id;
	struct{
		unsigned short x;
		unsigned short y;
	}coord[COORD_SAVE_MAX];
	int len;
	int index;
	unsigned int average_x;
	int direction_trend; //  0 unknown, >0 right, <0 lift.  

}touch;

int get_touch_instructions(struct send_info *info)
{
	char rbuf[16] = {0};
	int i;
	int rlen = 0;
	int try = 50;  //wait 1s
	int skfd = info->header->info_header->skfd;
	unsigned int coord;
	unsigned int average_x = 0;
	unsigned int average_y = 0;
	unsigned int variance = 0;
	memset(&touch, 0 , sizeof(touch));
	while (1){
		rlen = recv(skfd, rbuf, 16, 0);
		if (strncmp(rbuf, "press0", 6) == 0){
			touch.screen_id = t_screen->screen_id;
		}
		else if(strncmp(rbuf, "press1", 6) == 0){
			touch.screen_id = x_screen->screen_id;
		}
		else
			continue;	
		break;	
	}
	
	while (try--){
		coord = get_touch_coord(info);
		if(coord == 0xFFFFFFFF){
			break;
		}
		if (!coord){
		
			usleep(100000);
			continue;
		}
		touch.coord[touch.index].x =(unsigned short)(coord >> 16);	
		touch.coord[touch.index].y =(unsigned short)(coord & 0xFFFF);
		
		touch.len++;
		if(touch.len > 2){
			if (touch.index == 0){
				average_x = touch.coord[COORD_SAVE_MAX -1].x +
					touch.coord[COORD_SAVE_MAX - 2].x;
			}else if (touch.index == 1){
				average_x = touch.coord[0].x +
					touch.coord[COORD_SAVE_MAX -1].x;
			
			}else {
				average_x = touch.coord[touch.index - 1].x +
					touch.coord[touch.index - 2].x;
			}
			average_x >>= 2;
			if (average_x > (touch.coord[touch.index].x >> 1)){
//				printf("moving to lift\n");
				touch.direction_trend--;
			}
			else if(average_x < (touch.coord[touch.index].x >> 1)){
//				printf("moving to right\n");
				touch.direction_trend++;
			
			}	
			else{
//				printf("no moving\n");
			}
		}

		if (touch.index++ >= COORD_SAVE_MAX)
			touch.index = 0;
		usleep(20000);
		try++;

	}
	
	touch.len = touch.len > COORD_SAVE_MAX ? COORD_SAVE_MAX : touch.len;

	if (touch.len == 0)
		return 0;
	for (i = 0; i < touch.len; i++){
		average_y += touch.coord[i].y;	
	}
	
	average_y /= i;
	
	for (i = 0; i < touch.len; i++){
		variance += average_y > touch.coord[i].y ?
			(average_y - touch.coord[i].y) : (touch.coord[i].y - average_y);
	}
//	printf("variance = %d\n", variance);
//	printf("touch.direction_trend = %d\n", touch.direction_trend);

	return touch.direction_trend;
}
void *start_touch(void *arg)
{
	struct send_info *info = arg;
	int ret;
	int is_in_temperature = 1;
	blocking = 0;
	pthread_mutex_init(&blocking_lock, NULL);
	while(*running){
		ret = get_touch_instructions(info);
		if (ret < 0 && touch.screen_id == x_screen->screen_id){ // we are in xenomai and silde to lift 
			switch_screen(info, t_screen->screen_id);
			unblock_opc_client();
		}
		else if (ret > 0 && is_in_temperature == t_screen->screen_id){
			block_opc_client();
			switch_screen(info, x_screen->screen_id);
		}
		else 
			continue;
		send_data(info);
	}
	pthread_exit((void *)0);
}

void *start_send_data(void *arg)
{
	struct send_info **info = arg;
	struct send_info *info_485 = info[0];
	struct send_info *info_232 = info[1];
	while (*running){
		modbus_callback(info_485, info_232);
		send_data(info_232);
		usleep(200000);
	}
	pthread_exit((void *)0);
}

int start_server(char *ip)
{
	int fd;
	if ((fd = is_already_running(LOCK_FILE)) < 0)
		return 0;
	
//	if (daemon(0, 1) < 0){
//		return -1;
///	}
	
	if (lock_running(fd) < 0){
		return -1;
	}

	pthread_t p_xeno, p_touch, p_send;
	int retval;
	struct send_info *info_485 = open_port(ip, "26");
	if (!info_485){
		return -2;
	}
	
	struct send_info *info_232 = open_port(ip, "23");
	if (!info_232){
		return -3;
	}
	
	vgus_init(info_232);	
	send_data(info_232);
	struct send_info *info[2] = {info_485, info_232};	

	retval = pthread_create(&p_send, NULL, start_send_data, (void *)info);
	if (retval){
		fprintf(stderr,"Error - pthread_create() return code: %d\n",retval);
		return -5;
	}
	
	struct send_info *info_xenomai = dup_send_info(info_232);

	if (!info_xenomai){
		return -4;
	}

	retval = pthread_create(&p_xeno, NULL, start_xenomai, (void *)info_xenomai);
	if (retval){
		fprintf(stderr,"Error - pthread_create() return code: %d\n",retval);
		return -5;
	}
	
	struct send_info *info_touch = dup_send_info(info_232);

	if (!info_touch){
		return -6;
	}

	retval = pthread_create(&p_touch, NULL, start_touch, (void *)info_touch);
	if (retval){
		fprintf(stderr,"Error - pthread_create() return code: %d\n",retval);
		return -7;
	}
	
	start_opc_server(info_485, info_232);
	
	pthread_join(p_touch, NULL);	
	pthread_join(p_xeno, NULL);	
	pthread_join(p_send, NULL);	

	return 0;
}

int main()
{
	start_server("10.193.20.217");
	return 0;
}
