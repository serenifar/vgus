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
#include "vgus_controller.h"

#define pr() //printf("%s %s %d\n", __FILE__, __func__, __LINE__)
#define DEF_BUFF_SIZE 1024

static int create_sk_cli(char *addr, char *port)
{
	int skfd;
	int ret;
	struct addrinfo hints;
	struct addrinfo *res;
	struct addrinfo *p;
	
	memset(&hints, 0, sizeof(hints));
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;
	hints.ai_protocol = 0;

	ret = getaddrinfo(addr, port, &hints, &res);
	if(ret != 0){
		printf("getaddrinfo : %s\n", gai_strerror(ret));
		return -1;
	}

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
		printf("Fail to connect\n");
 		return -1;
	}

	freeaddrinfo(res);
	
	return skfd;
}

static struct send_info *create_connect(char *ip, char *port)
{
	struct send_info *info = malloc(sizeof(struct send_info));
	if (!info){
		return NULL;
	}
	memset(info, 0 , sizeof(*info));
	info->buf = malloc(DEF_BUFF_SIZE);
	if (!(info->buf)){
		free(info);
		return NULL;
	}
	info->buf_len = DEF_BUFF_SIZE;
	info->ip = ip;
	info->port = port;
	info->skfd = create_sk_cli(ip, port);
	if(info->skfd < 0){
		free(info->buf);
		free(info);
		return NULL;
	}
	return info;
}

static int copy_to_buf(struct send_info *info, char *buffer, int len)
{
	char *buf;
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

	return 0;	
}

void close_connect(struct send_info *info)
{
	close(info->skfd);
	free(info->buf);
	free(info);
}


int send_data(struct send_info *info)
{
	int wlen;

         wlen = send(info->skfd, info->buf, info->len, MSG_NOSIGNAL);
	 if (wlen != info->len){
		return -1;
	 }
	 info->len = 0;

	return wlen;
}	

struct send_info* open_port(char *ip, char *port)
{

	struct send_info *info;
	if ((info = create_connect(ip, port)) == NULL){
		printf("Can't create a connect to %s:%s\n", ip, port);
		return NULL;
	}
	struct timeval tv;
	tv.tv_usec = 100000;
	setsockopt(info->skfd, SOL_SOCKET, SO_RCVTIMEO,(char *)&tv,sizeof(struct timeval));
	return info;
}


static void set_title_var(struct send_info *info, struct text_variable  *text, char *title)
{
	int len;
	char buf[32];
	len = strlen(title);
        len = write_text_var(buf, len, text->variable_addr, title);
	copy_to_buf(info, buf, len);

}

static void set_grid(struct send_info *info, struct grid_frame *grid)
{
	char buf[255] = {0};
	int len = 0;
	struct curve_frame * curve = grid->curve_frame; 
	unsigned short color0 = grid->color;

	write_wire_header(buf, grid->variable_addr);
	unsigned short y = curve->apex[1];
	unsigned short x = curve->apex[0];
	unsigned short interval = curve->width / curve->number;
	int i = 0;
	for (;y <= curve->apex[1] + curve->width; y += interval)
		set_pixel_pair_n(buf, x, y, x + curve->length, y, color0, i++);
	len = i * 10 + 10;
	set_vari_len(buf, (len - 3));
	set_graph_num(buf, i);
	copy_to_buf(info, buf, len);
		
} 

static void set_axis(struct send_info *info, struct axis_frame *axis)
{
	char buf[255] = {0};
	int len = 0;
	int i = 0;
	struct curve_frame * curve = axis->curve_frame; 
	unsigned short color = axis->color;
	write_wire_header(buf, axis->variable_addr);
	unsigned short x0 = curve->apex[0], y0 = curve->apex[1], 
		 length = curve->length, width = curve->width,
		 x1 = x0 + length, y1 = y0,
		 x2 = x0, y2 = y0 + width,
		 x3 = x1, y3 = y2;
	/**
	 *  (x0, y0)    (x1, y1)
	 * 
	 *
	 *  (x2, y2)    (x3, y3)
	 */


	set_pixel_pair_n(buf, x2, y2, x0, y0, color, i++);
	set_pixel_pair_n(buf, x2 + 1, y2, x0 + 1, y0, color, i++);

	set_pixel_pair_n(buf, x2, y2, x3, y3, color, i++);
	set_pixel_pair_n(buf, x2, y2 + 1, x3, y3 + 1, color, i++);
	
	set_pixel_pair_n(buf, x0, y0, x0 - 10, y0 + 10, color, i++);
	set_pixel_pair_n(buf, x0, y0, x0 - 9, y0 + 10, color, i++);
	set_pixel_pair_n(buf, x0 + 1, y0, x0 + 10, y0 + 10, color, i++);
	set_pixel_pair_n(buf, x0 + 1, y0, x0 + 9, y0 + 10, color, i++);

	set_pixel_pair_n(buf, x3, y3, x3 - 10, y3 - 10, color, i++);
	set_pixel_pair_n(buf, x3, y3, x3 - 10, y3 - 9, color, i++);
	set_pixel_pair_n(buf, x3, y3 + 1, x3 - 10, y3 + 10, color, i++);
	set_pixel_pair_n(buf, x3, y3 + 1, x3 - 10, y3 + 9, color, i++);
	
	len = i * 10 + 10;
	set_vari_len(buf, (len - 3));
	set_graph_num(buf, i);
	copy_to_buf(info, buf, len);
		
} 


static void set_axis_values(struct send_info *info, struct axis_values *axis_v)
{
	char buf[255] = {0};
	int len = 0;

	int i;
	unsigned short value = axis_v->init;
	unsigned short addr = axis_v->variable_addr;
	for (i = 0; i < axis_v->num; i++){
		write_var(buf, addr, value);
		value += axis_v->interval;
		addr += interval_addr;
	}
	
	copy_to_buf(info, buf, 7 * axis_v->num);
		
} 

static void up_vernier(struct vernier_show *vernier, unsigned short n)
{
	if (vernier->vernier < vernier->length)
		return ;
	vernier-> += n;
}

static int set_curve_values(struct send_info *info, unsigned short *buffer, int len, int channel)
{
	char buf[255] = {0};
	int i;
	int len > 250 ? 250 : len;
	write_curve_values(buf, len, channel, buffer);
	
	copy_to_buf(info, buf, (len * 2 + 5));
	return len;

}

static int set_curve_value(struct send_info *info, int data, int channel)
{
	char buf[16] = {0};
	write_curve_value(buf, channel, data);
	copy_to_buf(info, buf, 7);
	return 1;

}

static void set_vernier_value(struct send_info *info,struct xenomai_screen *xenomai, unsigned short data)
{
	unsigned short x, y;
	int len = 0;
	struct curve * = xenomai->curve;
	float y1;  
	if ( xenomai->vernier.vernier > curve->curve_frame.width / curve.x_interval){
		x = ( curve->curve_frame.apex[0] + curve->curve_frame.length);
	}
	else {
		x = (xenomai->vernier.vernier - 1) * (curve.x_interval) + curve->curve_frame.apex[0];
	}

//	y = (data & 0xffff - Y_min) / ((Y_max - Y_min) / Y_hight_pixel) ;
	y1 = (((float)(data & 0xffff ) - curve->y_min) * (curve->curve_frame->width)) / (curve->y_max - curve->y_min);
	y = curve_frame->width - (unsigned short)y1 + curve->curve_frame.apex[1];
	x -= 8;
        char buf[32] = {0};
	len = 8;
	write_var(buf, xenomai->vernier.var.variable_addr, data);
	len += 10;	
	unsigned short temp[2];
	temp[0] = x; temp[1] = y;
	write_vars((buf + 8),(xenomai->vernier.var.describe_addr + 0x01), 2, temp );
	copy_to_buf(info, buf, len);	
}

void xenomai_update_curve(struct send_info *info, struct xenomai_screen *xenomai ,unsigned data)
{
	set_curve_value(info, xenomai, data);
	up_vernier(xenomai->vernier, 1);
	set_vernier_value(info, xenomai, data);
} 

static void realtime_curve_init(struct send_info *info, struct realtime_curve *curve)
{
        char buf[64] = {0};
	int len;
	len = write_var(buf, curve->describe_addr + 0x09, (curve->channel << 8) | curve->x_interval);

	unsigned short nul_y = 0;
	nul_y = ((curve->curve_frame.width) << 8) / (curve->y_max - curve->y_min);  
	len += write_var(buf + len, curve->describe_addr + 0x08, nul_y);

	unsigned short y_centre = curve->curve_frame.width / 2 + curve->curve_frame.apex[1];
	len += write_var(buf + len, curve->describe_addr + 0x05, y_centre);

	unsigned short y_centre_v = (curve->y_max - curve->y_min) / 2 + curve->y_min;
	len += write_var(buf + len, curve->describe_addr + 0x05, y_centre);
	
	copy_to_buf(info, buf, len);	
}

void curve_clear_data(struct send_info *info, struct realtime_curve *curve )  // ch = 0 : all ch; 
{
	char buf[] = {0xa5, 0x5a, 0x03, 0x80, 0xeb, 0x55};
	buf[5] += curve->channel;
	copy_to_buf(info, buf, 6);
}

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
	set_title_vari(info, "Xenomai Interrupt Latency");
	send_data(info);
	curve_init(info);
	send_data(info);
	set_axis(info);
	set_frame(info);
	set_axis_value(info);
	send_data(info);
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
		if(data > Y_max || data < Y_min)
			continue;
		update_curve(info, data);
		send_data(info);
		close(fd);
	}

	close_connect(info);
}

void vgus_init(struct send_info *info)
{
	set_title_vari(info, "Xenomai Interrupt Latency");
	send_data(info);
	curve_init(info);
	send_data(info);
	set_axis(info);
	set_frame(info);
	set_axis_value(info);
	send_data(info);
	current_clear();
	curve_clear_data(info, 1);
	send_data(info);
	update_curve(info, 1500);
	send_data(info);

}

