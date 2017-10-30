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

