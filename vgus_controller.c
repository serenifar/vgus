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

#define frame_heafer 0xa55a
#define title_vari_addr 0x4000
#define frame_vari_addr 0x4500

#define h_16(var_int16) ((var_int16) >> 8)
#define l_16(var_int16) ((char)((var_int16) & 0xff))
#define set_vari_head(buf, addr) do {buf[0] = h_16(frame_heafer);\
				     buf[1] = l_16(frame_heafer);\
				     buf[4] = h_16(addr);\
				     buf[5] = l_16(addr);} while(0)

#define set_vari_len(buf, len) (buf[2] = len)
#define set_vari_cmd(buf, cmd) (buf[3] = cmd)
#define set_vari_w(buf, len, addr) set_vari_head(buf, addr);\
				   set_vari_len(buf, len); \
       			           set_vari_cmd(buf, 0x82)	
#define get_vari_point(buf)  (buf + 6)
#define get_vari_len(len) (len + 6)

static void set_title_vari(struct send_info *info, char *title)
{
	int len;
	char buf[32];
	len = strlen(title);
        set_vari_w(buf, len, title_vari_addr);
	memcpy(get_vari_point(buf), title, len);
	copy_to_buf(info, buf, get_vari_len(len));

}


#define set_graph_cmd(buf, cmd)  buf[6] = 0x00; buf[7] = cmd
#define set_graph_num(buf, num)  buf[8] = h_16(num); buf[9] = l_16(num)
#define set_graph_color(buf, col) buf[0] = h_16(col); buf[1] = l_16(col)

#define set_pixel(buf,  x, y) (buf)[0] = h_16(x); (buf)[1] = l_16(x); (buf)[2] = h_16(y); (buf)[3] = l_16(y)

#define set_pixel_pair(buf, x0, y0, x1, y1) set_pixel(buf,  x0, y0); set_pixel((buf + 4),  x1, y1)

#define _set_pixel_pair_n(buf,x0, y0, x1, y1, col, n) do {\
				char *b= buf + n * 10; \
				set_graph_color(b, col); b += 2;\
				set_pixel_pair(b,x0, y0, x1, y1);} while(0)

#define set_pixel_pair_n(buf,x0, y0, x1, y1, col, n) _set_pixel_pair_n(buf+10,x0, y0, x1, y1, col,n)

static void set_frame(struct send_info *info)
{
	char buf[255] = {0};
	int len = 0;
	short color0 = 0xfc1f;
	short color1 = 0xecb1;
	set_vari_head(buf, frame_vari_addr);
	set_vari_cmd(buf, 0x82);
        set_graph_cmd(buf, 0x0a);
	unsigned short y = 140;
	unsigned short x = 75;
	int i = 0;
	for (;y < 440; y += 30)
		set_pixel_pair_n(buf, 55, y, 755, y, color0, i++);
//	for (; x <= 755 ; x += 20)
//		set_pixel_pair_n(buf, x, 140, x, 440,  color1, i++);
	len = i * 10 + 10;
	set_vari_len(buf, len - 3);
	set_graph_num(buf, i);
	copy_to_buf(info, buf, len);
		
} 

#define axis_vari_addr 0x4300
static void set_axis(struct send_info *info)
{
	char buf[255] = {0};
	int len = 0;
	short color = 0x522c;
	set_vari_head(buf, axis_vari_addr);
	set_vari_cmd(buf, 0x82);
        set_graph_cmd(buf, 0x0a);
	int i = 0;
	
	set_pixel_pair_n(buf, 55, 440, 55, 130, color, i++);
	set_pixel_pair_n(buf, 56, 440, 56, 130, color, i++);

	set_pixel_pair_n(buf, 55, 440, 765, 440, color, i++);
	set_pixel_pair_n(buf, 55, 441, 765, 441, color, i++);
	
	set_pixel_pair_n(buf, 55, 130, 45, 140, color, i++);
	set_pixel_pair_n(buf, 55, 130, 46, 140, color, i++);
	set_pixel_pair_n(buf, 56, 130, 65, 140, color, i++);
	set_pixel_pair_n(buf, 56, 130, 64, 141, color, i++);

	set_pixel_pair_n(buf, 765, 440, 755, 430, color, i++);
	set_pixel_pair_n(buf, 765, 440, 755, 431, color, i++);
	set_pixel_pair_n(buf, 765, 441, 755, 450, color, i++);
	set_pixel_pair_n(buf, 765, 441, 755, 449, color, i++);
	
	len = i * 10 + 10;
	set_vari_len(buf, (len - 3));
	set_graph_num(buf, i);
	copy_to_buf(info, buf, len);
		
} 


#define axis_value_vari_addr 0x4020
#define set_vari_value(buf, value) (buf)[6] = h_16(value); (buf)[7] = l_16(value)
static void set_axis_value(struct send_info *info)
{
	char buf[255] = {0};
	int len = 0;
	set_vari_cmd(buf, 0x82);
	len = 8;	
	set_vari_len(buf, (len - 3));
	int i;
	unsigned short value = 500;
	unsigned short addr = axis_value_vari_addr;
	for (i = 0; i < 12; i++){
		set_vari_head(buf, (addr + (i * 0x20)));
		set_vari_value(buf, value);
		value += 250;	
		copy_to_buf(info, buf, len);
	}
	value = 350;
	addr = axis_value_vari_addr + ( 11 * 0x20);
	for (i = 0; i < 11; i++){
		set_vari_head(buf, (addr + (i * 0x20)));
		set_vari_value(buf, value);
		value -= 35;	
		copy_to_buf(info, buf, len);
	}
		
} 

#define set_curve_head(buf, curve) do {buf[0] = h_16(frame_heafer);\
				     buf[1] = l_16(frame_heafer);\
				     buf[4] = curve;} while(0)
int current_number = 0;

static void up_current(int n)
{
	if (current_number < 0)
		return ;
	current_number += n;
}
#define current_clear()  current_number = 0
static int set_curve_values(struct send_info *info, int *buffer, int len, int curve)
{
	char buf[255] = {0};
	char *b = buf + 5;
	set_vari_cmd(buf, 0x84);
	set_curve_head(buf, curve);
	int i;
	len > 250 ? 250 : len;
	for (i = 0; i < len; i++){
		b[0] = h_16(buffer[i] & 0xFFFF);
		b[1] = l_16(buffer[i] & 0xFFFF);
		b += 2;
	}		
	
	set_vari_len(buf, (len + 2));
	copy_to_buf(info, buf, len + 5);
	up_current(len);
	return len;

}

static int set_curve_value(struct send_info *info, int data, int curve)
{
	char buf[32] = {0};
	char *b = buf + 5;
	set_vari_cmd(buf, 0x84);
	set_curve_head(buf, curve);
		b[0] = h_16(data & 0xFFFF);
		b[1] = l_16(data & 0xFFFF);
	
	set_vari_len(buf, 4);
	copy_to_buf(info, buf, 7);
	up_current(1);
	return 1;

}
#define current_value_addr 0x42e0
#define current_value_attr 0x0300
#define Y_min   500
#define Y_max   3000
#define Y_start_pixel  140
#define Y_hight_pixel  300
#define X_start_pixel  55
#define X_width_pixel  700
#define X_interval     20
#define curve_data_source 1

static void set_current_value(struct send_info *info, int data)
{
	unsigned short x, y;
	int len = 0;
	float y1;  
	if (current_number > X_width_pixel / X_interval){
		x = (X_start_pixel + X_width_pixel);
	}
	else {
		x = (current_number - 1) * (X_interval) + X_start_pixel;
	}

//	y = (data & 0xffff - Y_min) / ((Y_max - Y_min) / Y_hight_pixel) ;
	y1 = (((float)(data & 0xffff ) -Y_min) * (Y_hight_pixel)) / (Y_max - Y_min);
	y = Y_hight_pixel - (unsigned short)y1 + Y_start_pixel;
//	y -= 8;
	x -= 8;
        char buf[32] = {0};
	set_vari_cmd(buf, 0x82);
	len = 8;	
	set_vari_len(buf, (len - 3));
	set_vari_head(buf, current_value_addr);
	set_vari_value(buf, (data & 0xffff));
	copy_to_buf(info, buf, len);
	len = 10;	
	set_vari_len(buf, (len - 3));
	set_vari_head(buf, current_value_attr + 0x01);
	set_vari_value(buf, x);
	buf[8] = y >> 8;
	buf[9] = y & 0xff;

	copy_to_buf(info, buf, len);	
}

void update_curve(struct send_info *info, int data)
{
	set_curve_value(info, data, curve_data_source);
	set_current_value(info, data);
	
} 
#define curve_attr_addr 0x20
static void curve_init(struct send_info *info)
{
        char buf[64] = {0};
	set_vari_cmd(buf, 0x82);
	int len = 8;	
	set_vari_len(buf, len - 3);
	set_vari_head(buf, curve_attr_addr + 0x09);
	set_vari_value(buf, ( 1 < 8 | X_interval));
	copy_to_buf(info, buf, len);

	len = 8;	
	set_vari_len(buf, len - 3);
	set_vari_head(buf, curve_attr_addr + 0x08);
	unsigned short nul_y = 0;
	nul_y = ((Y_hight_pixel) << 8) / (Y_max - Y_min);  
	set_vari_value(buf, nul_y);
	copy_to_buf(info, buf, len);	

	len = 8;	
	set_vari_len(buf, len - 3);
	set_vari_head(buf, curve_attr_addr + 0x05);
	unsigned short y_centre = Y_hight_pixel / 2 + Y_start_pixel;
	set_vari_value(buf, y_centre);
	copy_to_buf(info, buf, len);	
	
	len = 8;	
	set_vari_len(buf, len - 3);
	set_vari_head(buf, curve_attr_addr + 0x06);
	unsigned short y_centre_v = (Y_max - Y_min) / 2 + Y_min ;
	set_vari_value(buf, y_centre_v);
	copy_to_buf(info, buf, len);	
}

void curve_clear_data(struct send_info *info, int ch)  // ch = 0 : all ch; 
{
	char buf[] = {0xa5, 0x5a, 0x03, 0x80, 0xeb, 0x55};
	buf[5] += ch;
	copy_to_buf(info, buf, 6);
}


#define mdelay() usleep(15*1000)
#define FIFO_FILE "/tmp/latency"
int main(int argc, char **argv)
{
	struct send_info *info;
	info = open_port("10.193.20.217", "23");
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
	else {
	//	struct stat buf;
	//	if((fd = lstat(argv[1],&buf)) < 0)
	//	       return -2;
	//	if(S_ISFIFO(buf.st_mode) == 0)
	//		return -3;	
	}


	if (daemon(0, 1) < 0){
		return -4;
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

