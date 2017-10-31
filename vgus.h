#ifndef __VGUS__H____
#define __VGUS__H____
#include <string.h>
#include "usr-410s.h"
#define frame_heafer 0xa55a
extern struct temperature_screen *t_screen;
extern struct xenomai_screen *x_screen;

struct numerical_variable
{
	unsigned short describe_addr;
	unsigned short variable_addr;
};

struct icon_variable
{
	unsigned short describe_addr;
	unsigned short variable_addr;
};
 
struct curve_frame
{
	unsigned short length;
	unsigned short apex[2];
        unsigned short width;	
};

struct axis_frame
{
	unsigned short describe_addr;
	unsigned short variable_addr;
	struct curve_frame *curve_frame;
	unsigned short color;
};

struct grid_frame
{
	unsigned short describe_addr;
	unsigned short variable_addr;
	unsigned short number;
	struct curve_frame *curve_frame;
	unsigned short color;
};

struct warn_frame
{
	unsigned short describe_addr;
	unsigned short variable_addr;
	struct curve_frame *curve_frame;
	unsigned short color_max;
	unsigned short color_min;
};
struct text_variable
{
	unsigned short describe_addr;
	unsigned short variable_addr;
};

struct realtime_curve
{
	unsigned short describe_addr;
	struct curve_frame *curve_frame;
	unsigned short channel;
	unsigned short y_max;
	unsigned short y_min;
	unsigned short x_interval;
	unsigned short color;
};

struct axis_values
{
	int number;
	short init;
	short interval;
	unsigned short variable_addr;
	unsigned short interval_addr;
};

struct temperature_screen
{
	struct numerical_variable temp;
	struct numerical_variable warn_max;
	struct numerical_variable warn_min;
	struct icon_variable warn_icon;
	struct realtime_curve curve;
	struct warn_frame warn;
	struct grid_frame grid;
	struct axis_values x_axis;
	struct axis_values y_axis;
	struct axis_frame axis;
	unsigned short screen_id;
};

struct vernier 
{
	struct numerical_variable var;
	unsigned short  vernier;
	struct curve_frame *curve_frame;
	
};

struct xenomai_screen
{
	struct vernier vernier;
	struct text_variable title;
	struct realtime_curve curve;
	struct grid_frame grid;
	struct axis_values x_axis;
	struct axis_values y_axis;
	struct axis_frame axis;
	unsigned short screen_id;
};


#define h_16(var_int16) ((char)((var_int16) >> 8))
#define l_16(var_int16) ((char)((var_int16) & 0xff))

// write variable
#define set_var_head(buf, addr) do {buf[0] = h_16(frame_heafer);\
				     buf[1] = l_16(frame_heafer);\
				     buf[4] = h_16(addr);\
				     buf[5] = l_16(addr);} while(0)

#define set_var_len(buf, len) ((buf)[2] = (len))
#define set_var_cmd(buf, cmd) ((buf)[3] = (cmd))
#define set_value(buf, data) (buf)[0] = h_16(data); (buf)[1] = l_16(data)

#define write_var(buf, addr, data) write_vars(buf, 0x01, addr, &(data))


// draw realtime curve						   
#define write_curve_value(buf, channel, data) write_curve_values(buf, 1, channel, &data)



// draw wire
#define set_graph_cmd(buf, cmd)  buf[6] = 0x00; buf[7] = cmd
#define set_graph_num(buf, num)  buf[8] = h_16(num); buf[9] = l_16(num)
#define set_graph_color(buf, col) buf[0] = h_16(col); buf[1] = l_16(col)

#define set_pixel(buf,  x, y) (buf)[0] = h_16(x); (buf)[1] = l_16(x); (buf)[2] = h_16(y); (buf)[3] = l_16(y)

#define set_pixel_pair(buf, x0, y0, x1, y1) set_pixel(buf,  x0, y0); set_pixel((buf + 4),  x1, y1)

#define _set_pixel_pair_n(buf,x0, y0, x1, y1, col, n) do {\
				char *b= buf + n * 10; \
				set_graph_color(b, col); b += 2;\
				set_pixel_pair(b,x0, y0, x1, y1);} while(0)

#define write_pixel_pair_n(buf,x0, y0, x1, y1, col, n) _set_pixel_pair_n(buf+10,x0, y0, x1, y1, col,n)

#define write_wire_header(buf, addr) do {\
		set_var_head(buf, addr);\
		set_var_cmd(buf, 0x82); \
		set_graph_cmd(buf, 0x0a);} while(0)
// write/read registers 

#define set_reg_head(buf, addr) do {buf[0] = h_16(frame_heafer);\
				     buf[1] = l_16(frame_heafer);\
				     buf[4] = (addr);} while(0)
#define set_reg_len(buf, len) buf[2] = (len)
#define set_reg_cmd(buf, cmd) buf[3] = (cmd)

#define write_reg(buf, addr, data) write_regs(buf, addr, &data, 1)
// multiple processes lock 
#define write_lock(fd, offset, whence, len) \
		lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))

#define lockfile(fd) write_lock((fd), 0, SEEK_SET, 0)

#define is_write_lockable(fd, offset, whence, len) \
		(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

#define is_lockfile(fd) is_write_lockable(fd, 0, SEEK_SET, 0)



#define mdelay() usleep(15*1000)
#define FIFO_FILE "/tmp/latency"
#define LOCK_FILE "/tmp/vgus.pid"
#define LOCK_MODE (S_IRUSR | S_IWUSR| S_IRGRP | S_IROTH)

void update_curve(struct send_info *info, int data);
void curve_clear_data(struct send_info *info, struct realtime_curve *curve); // ch = 0 : all ch; 
void xenomai_update_curve(struct send_info *info, unsigned int data);
void temperature_update_curve(struct send_info *info, unsigned int data);
void vgus_init(struct send_info *info);
void set_warn_icon(struct send_info *info, int red);
int temperature_draw_warn(struct send_info *info, unsigned int max, unsigned int min);
#endif
