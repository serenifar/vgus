#ifndef __VGUS__CONTROLER____H____
#define __VGUS__CONTROLER____H____
struct send_info
{
	int skfd;
	int buf_len;
	int len;
	char *buf;
	char *ip;
	char *port;
};

#define Y_min   500
#define Y_max   3000
#define Y_start_pixel  140
#define Y_hight_pixel  300
#define X_start_pixel  55
#define X_width_pixel  700
#define X_interval     20
#define curve_data_source 1

#define mdelay() usleep(15*1000)
#define FIFO_FILE "/tmp/latency"
#define LOCK_FILE "/tmp/vgus.pid"
#define LOCK_MODE (S_IRUSR | S_IWUSR| S_IRGRP | S_IROTH)

void close_connect(struct send_info *info);

int send_data(struct send_info *info);
struct send_info* open_port(char *ip, char *port);
void update_curve(struct send_info *info, int data);
void curve_clear_data(struct send_info *info, int ch); // ch = 0 : all ch; 
int start_opc_server(char *ip, char *port);
int start_xenomai_server(char *ip, char *port);
int already_running(char *lockfile);

void vgus_init(struct send_info *info);
#endif
