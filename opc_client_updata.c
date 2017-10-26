#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "vgus_controller.h"
#include "open62541.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


#define END_SERVER_CMD 0x01
#define START_SERVER_CMD 0x02
#define REDRAW_CMD 0x03
#define UPDATA_CMD 0x04
#define SET_URI  0x05
#define SET_IP  0x06
#define SET_PORT  0x08
#define USE_CONF_URI_CMD 0x09

#define isCMD(c)  (cmd & (1 << c))

#define DEFAULT_IP "10.193.20.217"
#define DEFAULT_PORT "23"

static char *find_str(char *s, char *b, int str_num)
{
	int index = 0;
	int i;
	for (i = 0; i < str_num; i++){
		if (strncmp(s, b + index, strlen(s)) == 0){
			return b + index + strlen(s);
		}
	}
	return NULL;

}

int main(int argc, char **argv)
{
	char ch;
	opterr = 0;
	char *uri = "opc.tcp://localhost:16666";
	char *ip = DEFAULT_IP;
	char *port = DEFAULT_PORT;
	int cmd = 0;	
	unsigned int data = 0;
	char *usage = "Usage: draw_latency  [-u uri] [-p port] [-i ip] -s latency [-c] [-r] \
	       			-u: set opc-server uri to connect\n \
				-p -i: the modbus port used when opc-server start \n \
				-s : updata the latency\n \
				-c: end the opc-server\n \
				-r: redraw the screen\n";
	if (argc > 2){
		while ((ch = getopt(argc, argv, "u:i:p:cs:r")) != -1){
			switch(ch){
				case 'u':
					uri = strdup(optarg);
					cmd |= 1 << SET_URI;
					break;
				case 'i':
					ip = strndup(optarg, strlen("255.255.255.255"));
					cmd |= 1 << SET_IP;
					break;
				case 'p':
					port = strndup(optarg, strlen("65536"));
					cmd |= 1 << SET_PORT;
					break;
				case 'c':
					cmd |= END_SERVER_CMD;
					break;
				case 's':
					data = atoi(argv[1]);
					if (data > Y_max || data < Y_min){
						printf("Error: the range of -s is %d - %d\n", Y_min, Y_max);
						return 0;
					}
				       cmd |=  1 << USE_CONF_URI_CMD;	
					break;
				case 'r':
					cmd = 1 << REDRAW_CMD;
								
				default:
					printf("%s", usage);
					return 0;
			}
		}
	}else if (argc == 2){
		cmd |= 1 << UPDATA_CMD; 
		data = atoi(argv[1]);
		if (data > Y_max || data < Y_min){
			printf("Error: the range of -s is %d - %d\n", Y_min, Y_max);
			return 0;
		}
	}	

	if(cmd == 0){
		printf("%s", usage);
		return 0;
	}
	
	int config_fd = 0;
	if (isCMD(SET_URI)){
		char buf[128] = "uri:";
		char buf1[128] = {0}; 
		char *p = NULL;
		int index = 0;
		config_fd = open("./latency.con", O_RDWR);
		if (config_fd < 0 ){
			config_fd = open("./latency.con", O_RDWR | O_CREAT);
			strcpy(buf + 4, uri);
			write(config_fd, buf, sizeof(buf));
			close(config_fd);
		}
		else{
			read(config_fd, buf, 128);
			p = strstr("ip:", buf);
			if(p){
				strcpy(buf1, p);
				index += strlen(p);
			}
			 p = strstr("port:", buf);
			 if(p){
				strcpy(buf1 + index, p);
				index += strlen(p);
			}
			 strcpy(buf1 + index, "uri:");
			 index += 4;
			 strcpy(buf1 + index, uri);
			 write(config_fd, buf, sizeof(buf));
			 close(config_fd);
		}
		return 0;
	}
	
	if (isCMD(SET_PORT)){
		char buf[128] = "port:";
		char buf1[128] = {0}; 
		char *p = NULL;
		int index = 0;
		config_fd = open("./latency.con", O_RDWR);
		if (config_fd < 0 ){
			config_fd = open("./latency.con", O_RDWR | O_CREAT);
			strncpy(buf + 5, port, 64);
			write(config_fd, buf, sizeof(buf));
			close(config_fd);
		}
		else{
			read(config_fd, buf, 128);
			p = strstr("ip:", buf);
			if(p){
				strcpy(buf1, p);
				index += strlen(p);
			}
			 p = strstr("uri:", buf);
			 if(p){
				strcpy(buf1 + index, p);
				index += strlen(p);
			}
			 strcpy(buf1 + index, "port:");
			 index += 5;
			 strncpy(buf1 + index, uri, 64);
			 write(config_fd, buf, sizeof(buf));
			 close(config_fd);
		}
	}

	if (isCMD(SET_IP)){
		char buf[128] = "ip:";
		char buf1[128] = {0}; 
		char *p = NULL;
		int index = 0;
		config_fd = open("./latency.con", O_RDWR);
		if (config_fd < 0 ){
			config_fd = open("./latency.con", O_RDWR | O_CREAT);
			strncpy(buf + 3, port, 64);
			write(config_fd, buf, sizeof(buf));
			close(config_fd);
		}
		else{
			read(config_fd, buf, 128);
			p = strstr("port:", buf);
			if(p){
				strcpy(buf1, p);
				index += strlen(p);
			}
			 p = strstr("uri:", buf);
			 if(p){
				strcpy(buf1 + index, p);
				index += strlen(p);
			}
			 strcpy(buf1 + index, "port:");
			 index += 3;
			 strncpy(buf1 + index, uri, 64);
			 write(config_fd, buf, sizeof(buf));
			 close(config_fd);
		}
	}


	if (isCMD(USE_CONF_URI_CMD)){
		if ((config_fd = open("./latency.con", O_RDWR)) < 0){
			config_fd = open("./latency.con", O_RDWR | O_CREAT);
			char buf[128] = {0};
			int index = 0;
			strcpy(buf + index, "ip:");
			index += 3;
			strcpy(buf + index, ip);
			index += strlen(ip);
			strcpy(buf + index, "port:");
			index += 5;
			strcpy(buf + index, port);
			index += strlen(port);
			strcpy(buf + index, "uri:");
			index += 4;
			strcpy(buf + index, uri);
			write(config_fd, buf, sizeof(buf));
			close(config_fd);
		}
		else{
			char buf[128] = {0};
			char *b;
			read(config_fd, buf, 128);
			if ((b = find_str("uri:", b, 3)) == NULL)
				return 0;
			if (isCMD(SET_URI))
				free(uri);
			uri = strdup(b);
			
			if ((b = find_str("ip:", b, 3)) == NULL)
				return 0;
			if (isCMD(SET_IP))
				free(ip);
			ip = strdup(b);
			
			if ((b = find_str("port:", b, 3)) == NULL)
				return 0;
			if (isCMD(SET_PORT))
				free(port);
			port = strdup(port);
		}
	}
	
	UA_ClientConfig config = UA_ClientConfig_standard;
	config.logger = NULL;
	UA_Client *client = UA_Client_new(config);
	UA_StatusCode retval;

	if (isCMD(SET_IP) || isCMD(SET_PORT) || isCMD(END_SERVER_CMD)){
		retval = UA_Client_connect(client, uri);
		if(retval == UA_STATUSCODE_GOOD) {
			UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "EndOpcServer"), 0, NULL, NULL, NULL );
			sleep(1);
		}
		if ((isCMD(SET_IP) || isCMD(SET_PORT)) && !(isCMD(END_SERVER_CMD)))
			start_opc_server(ip, port);
			sleep(3);
		
	}
	
	if (isCMD(REDRAW_CMD) && !(isCMD(END_SERVER_CMD))){
		retval = UA_Client_connect(client, uri);
		if(retval != UA_STATUSCODE_GOOD) {
			start_opc_server(ip, port);
			sleep(3);
			int i = 0;
			for ( i = 0; i < 3; i++){
				retval = UA_Client_connect(client, uri);
				if(retval == UA_STATUSCODE_GOOD) {
					break;
				}
				sleep(3);
			}
			
			if(retval != UA_STATUSCODE_GOOD) {
				return 0;
			}
		}
		
		UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "ReDraw"), 0, NULL, NULL, NULL );
	}
	
	if (isCMD(UPDATA_CMD) && !(isCMD(END_SERVER_CMD))){
		retval = UA_Client_connect(client, uri);
		if(retval != UA_STATUSCODE_GOOD) {
			start_opc_server(ip, port);
			sleep(3);
			int i = 0;
			for ( i = 0; i < 3; i++){
				retval = UA_Client_connect(client, uri);
				if(retval == UA_STATUSCODE_GOOD) {
					break;
				}
				sleep(3);
			}
			
			if(retval != UA_STATUSCODE_GOOD) {
				return 0;
			}

		}
		
		UA_Variant *myVariant = UA_Variant_new();
		UA_Variant_setScalarCopy(myVariant, &data, &UA_TYPES[UA_TYPES_INT32]);
		UA_Client_writeValueAttribute(client,
				UA_NODEID_STRING(1, "latency"), myVariant);

	}
	return 0;

}

