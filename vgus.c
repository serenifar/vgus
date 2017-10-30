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
	short value = axis_v->init;
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
	set_curve_value(info, data, xenomai->curve.channel);
	up_vernier(xenomai->vernier, 1);
	set_vernier_value(info, xenomai, data);
} 

void temperature_update_curve(struct send_info *info, struct temperature_screen *temp ,unsigned data)
{
	set_curve_value(info, data, temp->curve.channel);
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

void switch_screen(struct send_info *info,  unsigned short screen_id) 
{
	char buf[16];
	int len;
	len = write_regs_short(buf, 0x03, screen_id);
	copy_to_buf(info, buf, len);
}

struct temperature_screen *t_screen;
struct curve_frame temperature_curve_frame;
struct xenomai_screen *x_screen;
int temperature_screen_init(struct send_info *info)
{
	t_screen = malloc(sizeof(struct temperature_screen));
	if (!t_screen){
		return -1;
	}
	
	temperature_curve_frame.length = 700;
	temperature_curve_frame.width = 265;
	temperature_curve_frame.apex[0] = 55;
	temperature_curve_frame.apex[1] = 175;

	t_screen->temp.describe_addr = 0x1400;
	t_screen->temp.variable_addr = 0x5400;
	t_screen->warn_max.describe_addr = 0x1420;
	t_screen->warn_max.variable_addr = 0x5420;
	t_screen->warn_min.describe_addr = 0x1440;
	t_screen->warn_min.variable_addr = 0x5440;
	t_screen->warn_icon.describe_addr = 0x1000;
	t_screen->warn_icon.variable_addr = 0x5000;
	
	t_screen->curve.describe_addr = 0x1020;
	t_screen->curve.curve_frame = &temperature_curve_frame;
	t_screen->channel = 0x02;
	t_screen->y_max = 80;
	t_screen->y_min = 20;
	t_screen->x_interval = 20;
	t_screen->color = 0xF810;
	t_screen->grid.describe_addr = 0x13c0;
	t_screen->grid.variable_addr = 0x53c0;
	t_screen->grid.number = 7;
        t_screen->grid.curve_frame = &temperature_curve_frame;
	t_screen->grid.color = 0xFC00;

	t_screen->x_axis.number = 7;
	t_screen->x_axis.init = 20;
	t_screen->x_axis.interval = 10;
        t_screen->x_axis.variable_addr = 0x5180;
	t_screen->x_axis.interval_addr = 0x20;

	t_screen->y_axis.number = 11;
	t_screen->y_axis.init = 350;
	t_screen->y_axis.interval = -35;
        t_screen->y_axis.variable_addr = 0x5260;
	t_screen->y_axis.interval_addr = 0x20;

	t_screen->axis.describe_addr = 0x13e0;		
	t_screen->axis.variable_addr = 0x53e0;
	t_screen->axis.curve_frame = &temperature_curve_frame;
	t_screen->axis.color = 0x4008;

	t_screen->screen_id = 0x01;
}

int xenomai_screen_init(struct send_info *info){
	x_screen = malloc(sizeof(struct xenomai_screen));
	if (!x_screen){
		free(t_screen);
		return -1;
	}
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

