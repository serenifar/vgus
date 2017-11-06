#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include "open62541.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vgus.h"
#include "usr-410s.h"
#include "client.h"
#include "server.h"


#define isCMD(c)  (cmd & (1 << c))

#define DEFAULT_IP "10.193.20.217"
#define DEFAULT_PORT "23"
#define DEFAULT_URI "opc.tcp://localhost:16666"

#define set_warn_v_valid(v)  ((v) |= 0xffff0000)
#define is_warn_v_valid(v)   (((v) & 0xFFFF0000) == 0xFFFF0000)
#define get_warn_v(v)        ((v) & 0xFFFF)

int read_pipe(char *buf, int len)
{
	int fd = open(FIFO_FILE_USER_R, O_RDONLY);
	if (fd < 0)
		return -1;

	read(fd, buf, len);
	close(fd);
	return 0;
}

int write_pipe(char *buf)
{
	int fd = open(FIFO_FILE_USER, O_WRONLY);
	if (fd < 0)
		return -1;
	write(fd, buf, strlen(buf) + 1);
	close(fd);
	return 0;
}

int main(int argc, char **argv)
{
	char ch;
	char buf[16];
	opterr = 0;
	char *p;
	int high_warn_v, low_warn_v;
#ifdef HAVE_OPC_SERVER
	char *uri = "opc.tcp://localhost:16666";
	int cmd = 0;	
	int heating = 0;
        int cooling = 0;	
	unsigned int temp = 0;
	UA_ClientConfig config = UA_ClientConfig_standard;
	config.logger = NULL;
	UA_Client *client = UA_Client_new(config);
	UA_StatusCode retval;
#endif
	char *usage = "Usage: vgus [options] [date]\n \
	       			-u: set opc uri\n \
				-k: kill current server\n \
				-r: redraw the screen\n\
				-l: print server info\n \
				-g: get temperature\n \
				-h: heating -- 0: off; 1: 5W; 2 20W; 3: 25W;  \n \
				-c: cooling -- 0: off; 1: 1.8W; 2: 5.6W; 3: 7.4W;\n \
				-w: set warn temperature \n\
					\"high:low\" : updara the high and low \
						temperature simultaneous.\n \
					 \"high:\" \\ \":low\": only updata high or low.\n\
				-h: print this info\n\
				";

	if(argc > 1){
		if(argv[1][1] != 'o'){
			while ((ch = getopt(argc, argv, "u:i:p:kgrw:h:c:")) != (char)(-1)){
				switch(ch){
					case 'k':
						write_pipe("k");
#ifdef HAVE_OPC_SERVER 
						retval = UA_Client_connect(client, uri);
						if(retval == UA_STATUSCODE_GOOD) {
							UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "EndServer"), 0, NULL, NULL, NULL );
						}
#endif
						break;
					case 'g':
						write_pipe("g");
						read_pipe(buf, 16);
						printf("%s", buf);
						break;
					case 'h':
						buf[0] = 'h';
						strcpy(buf + 1, optarg);
						write_pipe(buf);
						break;
					case 'c':
						buf[0] = 'c';
						strcpy(buf + 1, optarg);
						write_pipe(buf);
						break;
					case 'w':
						p = strchr(optarg, ':');
						if (!p){
							printf("%s", usage);
							return 0;
						}
						
						if (p != optarg){
							*p = '\0';
							high_warn_v = atoi(optarg);
							set_warn_v_valid(high_warn_v);
						}
						
						if (*(p + 1) != '\0'){
							low_warn_v = atoi(p + 1);
							if (low_warn_v < high_warn_v)
								set_warn_v_valid(low_warn_v);
						}
	
						if (!is_warn_v_valid(high_warn_v) && 
							!is_warn_v_valid(high_warn_v) ){
							printf("%s", usage);
							return 0;
						
						}
						buf[0] = 'w';
						sprintf(buf + 1,"%u", (high_warn_v << 16 | (low_warn_v & 0xffff)));
						write_pipe(buf);
						break;
	
					default:
						printf("%s", usage);
						return 0;
						
									
				}
			}
		}
#ifdef HAVE_OPC_SERVER
		else{
			while ((ch = getopt(argc, argv, "ou:i:p:kgrw:h:c:")) != (char)(-1)){
				switch(ch){
					case 'o' :
						break;
					case 'u':
						uri = strdup(optarg);
						cmd |= 1 < SET_SERVER;
						break;
					case 'k':
						cmd |= 1 << KILL_SERVER_CMD;
						break;
					case 'p':
						cmd |= 1 << PRINT_INFO_CMD;
						break;
					case 'g':
						cmd |= 1 << GET_TEMPERATURE_CMD;	
						break;
					case 'h':
						heating = atoi(optarg);
					       	cmd |=  1 << SET_HEATING_CMD;	
						break;
					case 'c':
						cooling = atoi(optarg);
					       	cmd |=  1 << SET_COOLING_CMD;	
						break;
					case 'w':
						p = strchr(optarg, ':');
						if (!p){
							printf("%s", usage);
							return 0;
						}
						
						if (p != optarg){
							*p = '\0';
							high_warn_v = atoi(optarg);
							set_warn_v_valid(high_warn_v);
						}
						
						if (*(p + 1) != '\0'){
							low_warn_v = atoi(p + 1);
							if (low_warn_v < high_warn_v)
								set_warn_v_valid(low_warn_v);
						}
	
						if (!is_warn_v_valid(high_warn_v) && 
							!is_warn_v_valid(high_warn_v) ){
							printf("%s", usage);
							return 0;
						
						}
	
						cmd |= 1 << SET_WARN_CMD;
						break;
					default:
						printf("%s", usage);
						return 0;
						
									
				}
			}
			
		
			if (isCMD(KILL_SERVER_CMD)){
				retval = UA_Client_connect(client, uri);
				if(retval == UA_STATUSCODE_GOOD) {
					UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "EndServer"), 0, NULL, NULL, NULL );
				}
				return 0;
			}
			
			if (isCMD(REDRAW_CMD) && !(isCMD(KILL_SERVER_CMD))){
				retval = UA_Client_connect(client, uri);
				if(retval != UA_STATUSCODE_GOOD) {
					printf("Can't connect to the ocp_server\n");
					return 0;
				}
				
				UA_Client_call(client, UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER), UA_NODEID_STRING(1, "ReDraw"), 0, NULL, NULL, NULL );
				UA_Client_disconnect(client);
			}
			
			if (isCMD(SET_WARN_CMD) && !(isCMD(KILL_SERVER_CMD))){
				retval = UA_Client_connect(client, uri);
				if(retval != UA_STATUSCODE_GOOD) {
					printf("Can't connect to the ocp_server\n");
					return 0;
				}
				
				UA_Variant *myVariant = UA_Variant_new();
				unsigned int warn_value = (high_warn_v & 0xffff) << 16 | (low_warn_v & 0xffff);
				UA_Variant_setScalarCopy(myVariant, &warn_value, &UA_TYPES[UA_TYPES_INT32]);
				UA_Client_writeValueAttribute(client,
						UA_NODEID_STRING(1, "warn_value"), myVariant);
		
				UA_Client_disconnect(client);
		
			}
			
			if (isCMD(SET_HEATING_CMD) && !(isCMD(KILL_SERVER_CMD))){
				retval = UA_Client_connect(client, uri);
				if(retval != UA_STATUSCODE_GOOD) {
					printf("Can't connect to the ocp_server\n");
					return 0;
				}
				
				UA_Variant *myVariant = UA_Variant_new();
				UA_Variant_setScalarCopy(myVariant, (unsigned int *)(&heating), &UA_TYPES[UA_TYPES_INT32]);
				UA_Client_writeValueAttribute(client,
						UA_NODEID_STRING(1, "heating_score"), myVariant);
		
				UA_Client_disconnect(client);
		
			}
			
			if (isCMD(SET_COOLING_CMD) && !(isCMD(KILL_SERVER_CMD))){
				retval = UA_Client_connect(client, uri);
				if(retval != UA_STATUSCODE_GOOD) {
					printf("Can't connect to the ocp_server\n");
					return 0;
				}	
				UA_Variant *myVariant = UA_Variant_new();
				UA_Variant_setScalarCopy(myVariant, (unsigned int *)(&cooling), &UA_TYPES[UA_TYPES_INT32]);
				UA_Client_writeValueAttribute(client,
						UA_NODEID_STRING(1, "cooling_score"), myVariant);
		
		
				UA_Client_disconnect(client);
			}
			
			if (isCMD(GET_TEMPERATURE_CMD) && !(isCMD(KILL_SERVER_CMD))){
				retval = UA_Client_connect(client, uri);
				if(retval != UA_STATUSCODE_GOOD) {
					printf("Can't connect to the ocp_server\n");
					return 0;
				}
				
				UA_Variant *myVariant = UA_Variant_new();
				retval = UA_Client_readValueAttribute(client, UA_NODEID_STRING(1,"temperature"), myVariant);
				temp = *(UA_Int32*)myVariant->data;
		
				UA_Client_disconnect(client);
				printf("%d", temp);
			}
			UA_Client_delete(client);
			return temp;
		}
#endif
	}
	else {
		printf("%s", usage);
		return 0;
	}
		
}

