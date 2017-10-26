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

void close_connect(struct send_info *info);

int send_date(struct send_info *info);
struct send_info* open_port(char *ip, char *port);
void update_curve(struct send_info *info, int data);
void curve_clear_data(struct send_info *info, int ch); // ch = 0 : all ch; 

void vgus_init(struct send_info *info);
#endif
