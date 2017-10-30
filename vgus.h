#ifndef __VGUS__H____
#define __VGUS__H____

#define KILL_SERVER_CMD 0x01
#define REDRAW_CMD  0x02
#define OPC_SERVER_CMD  0x03
#define PRINT_INFO_CMD 0x04
#define SWITCH_SERVER_CMD 0x05
#define GET_TEMPERATURE_CMD 0x06
#define SET_LATENCY_CMD 0x07
#define SET_WARN_CMD 0x08

#define frame_heafer 0xa55a

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
	int num;
	unsigned short init;
	unsigned short interval;
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
	struct grid_frame grid;
	struct axis_values x_axis;
	struct axis_values y_axis;
	struct axis_frame axis;
};

struct vernier 
{
	struct numerical_variable var;
	unsigned short  vernier;
	
}; 
struct xenomai_screen
{
	struct vernier vernier;
	struct text_variable title;
	struct icon_variable warn_icon;
	struct realtime_curve curve;
	struct grid_frame grid;
	struct axis_values x_axis;
	struct axis_values y_axis;
	struct axis_frame axis;
};


#define h_16(var_int16) ((var_int16) >> 8)
#define l_16(var_int16) ((char)((var_int16) & 0xff))

// write variable
#define set_var_head(buf, addr) do {buf[0] = h_16(frame_heafer);\
				     buf[1] = l_16(frame_heafer);\
				     buf[4] = h_16(addr);\
				     buf[5] = l_16(addr);} while(0)

#define set_var_len(buf, len) ((buf)[2] = (len))
#define set_var_cmd(buf, cmd) ((buf)[3] = (cmd))
#define set_value(buf, data) (buf)[0] = h_16(data); (buf)[1] = l_16(data)

#define write_vars(buf, len, addr, src ) do {set_var_head(buf, addr);\
				   set_var_len(buf, (len << 1) + 0x03); \
       			           set_var_cmd(buf, 0x82); \
				   {int i; for (i = 0; i < len; i++){ \
					     set_value(buf + 6 + i * 2, src[i])}}\
				   }while(0); (len * 2) + 6;

#define write_var(buf, addr, data) write_vars(buf, 0x01, addr, &data)

#define write_text_var(buf, len, addr, str ) do {set_var_head(buf, addr);\
				   set_var_len(buf, (len  + 0x03); \
       			           set_var_cmd(buf, 0x82); \
				   memcpy(buf + 6, str, len);\
				   } while(0); (len + 6)

// draw realtime curve						   
#define write_curve_values(buf, len, channel, src) \
				do { buf[0] = h_16(frame_heafer); \
				     buf[1] = l_16(frame_heafer); \
				     buf[4] = 1 << channel; \
				     set_var_len(buf, len * 2 + 2);\
				     set_var_cmd(buf, 0x84); \
				     {int i; for (i = 0; i < len; i+){\
					set_value(buf + 5 + i * 2, src[i])}} \
				} while(0); ((len * 2) + 5)

#define write_curve_value(buf, channel, data) write_curve_values(buf, 2, channel, &data)



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
		set_vari_head(buf, addr);\
		set_vari_cmd(buf, 0x82); \
		set_graph_cmd(buf, 0x0a);} while(0)


// multiple processes lock 
#define write_lock(fd, offset, whence, len) \
		lock_reg((fd), F_SETLK, F_WRLCK, (offset), (whence), (len))

#define lockfile(fd) write_lock((fd), 0, SEEK_SET, 0)

#define is_write_lockable(fd, offset, whence, len) \
		(lock_test((fd), F_WRLCK, (offset), (whence), (len)) == 0)

#define is_lockfile(fd) is_write_lockable(fd, 0, SEEK_SET, 0)

#endif
