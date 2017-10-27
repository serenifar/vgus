#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "vgus_controller.h"
#include "open62541.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vgus.h"


#define isCMD(c)  (cmd & (1 << c))

#define DEFAULT_IP "10.193.20.217"
#define DEFAULT_PORT "23"
#define DEFAULT_URI "opc.tcp://localhost:16666"



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
#define OPC_SERVER 0x01;
#define PIPE_SERVER 0x02;
#define set_warn_v_valid(v)  ((v) |= 0xffff0000)
#define is_warn_v_valid(v)   (((v) & 0xFFFF0000) == 0xFFFF0000)
#define get_warn_v(v)        ((v) & 0xFFFF)
int main(int argc, char **argv)
{
	char ch;
	opterr = 0;
	char *uri = "opc.tcp://localhost:16666";
	char *ip = DEFAULT_IP;
	char *port = DEFAULT_PORT;
	int cmd = 0;	
	unsigned int data = 0;
	int target_server = 0;
	int latency = 0;
	int high_warn_v, low_warn_v;
	char *usage = "Usage: vgus [options] [date]\n \
	       			-u: set opc-server UA_NODEID_STRING\n \
				-p -i: the modbus port used \n \
				-o : using opc as the server, the default is pipe \n \
				-k: kill current server\n \
				-r: redraw the screen\n\
				-i: print server info\n \
				-s: switch the server (opc | pipe)\n \
				-g: get temperature\n \
				-l: set latency\n \
				-w: set warn temperature \n\
					\"high:low\" : updara the high and low \
						temperature simultaneous.\n \
					 \"high:\" \\ \":low\": only updata high or low.\n\
				-h: print this info\n\
				";

	if (argc > 1){
		while ((ch = getopt(argc, argv, "u:i:p:okis:grl:w:h")) != -1){
			switch(ch){
				case 'u':
					uri = strdup(optarg);
					break;
				case 'i':
					ip = strndup(optarg, strlen("255.255.255.255"));
					break;
				case 'p':
					port = strndup(optarg, strlen("65536"));
					break;
				case 'o':
					cmd |= 1 << OPC_SERVER_CMD;
					break;
				case 'k':
					cmd |= 1 << KILL_SERVER_CMD;
					break;
				case 'i':
					cmd |= 1 << PRINT_INFO_CMD;
					break;
				case 's':
					if (strncmp("opc", optarg, 3) == 0)
						target_server = OPC_SERVER;
				       	else
				       		target_server = PIPE_SERVER;	       
					cmd |= 1 << SWITCH_SERVER_CMD;
					break;
				case 'g':
					cmd |= 1 << GET_TEMPERATURE_CMD;	
					break;
				case 'l':
					latency = atoi(optarg);
					if (latency > Y_x_max || latency < Y_x_min){
						printf("Error: the range of -s is %d - %d\n", Y_x_min, Y_x_max);
						return 0;
					}
				       	cmd |=  1 << SET_LATENCY_CMD;	
					break;
				case 'w':
					char *p = strchr(optarg, ':');
					if (p){
						print("%s", usage);
						return 0;
					}
					
					if (p != optarg){
						*p = '\0';
						high_warn_v = atoi(optarg);
						if (high_warn_v > Y_t_min && high_warn_v < Y_t_max)
							set_warn_v_valid(high_warn_v);
					}
					
					if (*(p + 1) != '\0'){
						low_warn_v = atoi(p + 1);
						if (low_warn_v > Y_t_min && low_warn_v < Y_t_max){
							if (low_warn_v < high_warn_v)
								set_warn_v_valid(low_warn_v);
						}
					}

					if (!is_warn_v_valid(high_warn_v) && 
						!is_warn_v_valid(high_warn_v) ){
						print("%s", usage);
						return 0;
					
					}

					cmd |= 1 << SET_WARN_CMD;
					break;
				default:
					print("%s", usage);
					return 0;
					
								
			}
		}
	}
	else {
		printf("%s", usage);
		return 0;
	}
	
	
	UA_ClientConfig config = UA_ClientConfig_standard;
	config.logger = NULL;
	UA_Client *client = UA_Client_new(config);
	UA_StatusCode retval;

	if (isCMD(SET_IP) || isCMD(SET_PORT) || isCMD(END_SERVER_CMD)){
		retval = UA_Client_connect(client, uri);
		if(retval == UA_STATUSCODE_GOOD) {
			UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "EndServer"), 0, NULL, NULL, NULL );
			sleep(1);
		}
		if ((isCMD(SET_IP) || isCMD(SET_PORT)) && !(isCMD(END_SERVER_CMD))){
			start_opc_server(ip, port);
			sleep(1);
		}
		
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

		UA_Client_disconnect(client);
		UA_Client_delete(client);

	}
	return 0;

}

