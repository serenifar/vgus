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

#include "vgus.h"
#include "usr-410s.h"

#define pr() //printf("%s %s %d\n", __FILE__, __func__, __LINE__)


int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
	struct flock lock;
	lock.l_type = type;
	lock.l_start = offset;
	lock.l_whence = whence;
	lock.l_len = len;

	return (fcntl(fd, cmd, &lock));

}


pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
	struct flock fl;
	fl.l_type = type;
	fl.l_start = offset;
	fl.l_whence = whence;
	fl.l_len = len;
	
	if (fcntl(fd, F_GETLK, &fl) < 0){
		return -1;
	}
	
	if (fl.l_type == F_UNLCK)
		return 0;
	return (fl.l_pid;)
}

int already_running(char *lockfile)
{
	int fd;
	char buf[16];
	
	fd = open(lockfile, O_RDWR | O_CREAT, LOCK_MODE );

	if (fd < 0){
		return 1;
	}	

	if (lockfile(fd) < 0){
		close(fd);
		return 1;
	}
	ftruncate(fd, 0);
	sprintf(buf, "%ld", (long)getpid());
	write(fd, buf, strlen(buf) + 1);
	return 0;

}

int start_xenomai_server(char *ip, char *port)
{
	if (daemon(0, 1) < 0){
		return -4;
	}

	if (!already_running(LOCK_FILE)){
		return -5;
	}
	
	struct send_info *info;
	info = open_port(ip, port);
	if (!info){
		return 0;
	}
	unsigned int data;	
	int fd; 
	if ((fd = access(FIFO_FILE, F_OK)) < 0){
		fd = mkfifo(FIFO_FILE, 0666);
		if (fd < 0)
			return -1;
	}

	char buf[16];
	int num;
	while (1){
		if ((fd = open(FIFO_FILE, O_RDONLY)) < 0){
			return -6;
		}
		num = read(fd, buf, 16);
		if (strncmp("exit", buf, 4) == 0)
			return 0;
		data = atoi(buf);
		if(data >  || data < Y_min)
			continue;
		update_curve(info, data);
		send_data(info);
		close(fd);
	}

	close_connect(info);
}


