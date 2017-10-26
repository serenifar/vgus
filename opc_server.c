#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "open62541.h"
#include "vgus_controller.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>




UA_Boolean running = true;
char *ip, *port;

struct send_info *info;
static void end_opc_server()
{
	running = false;
	close_connect(info);
}

static void 
afterUpdate(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 latency;
	UA_Variant value;
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "latency");
	UA_Server_readValue(server, tempNodeId, &value);
	latency = *(UA_Int32 *)(value.data);
	update_curve(info, latency);
	send_data(info);
}


static void
addUpdateCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "latency");
	UA_Int32 temperature = 500;
	UA_Variant_setScalar(&attr.value, &temperature, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "latency");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "latency");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = NULL;
	callback.onWrite = afterUpdate;
	UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
}

static UA_StatusCode 
endServerMethod(void *handle, const UA_NodeId nodeId, size_t inputSize,
		       const UA_Variant *input, size_t optputSize, UA_Variant *output)
{
	end_opc_server();	
	return UA_STATUSCODE_GOOD;
}

static void
addEndServerMethod(UA_Server *server)
{
	UA_MethodAttributes get_temp;
	UA_MethodAttributes_init(&get_temp);

	get_temp.description = UA_LOCALIZEDTEXT("en_US","End this opc server");
	get_temp.displayName = UA_LOCALIZEDTEXT("en_US", "End Server");
	get_temp.executable = true;
	get_temp.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_STRING(1,"EndServer"),
				UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
				UA_QUALIFIEDNAME(1, "EndServer"), get_temp, 
				&endServerMethod, NULL, 0, NULL, 0, NULL, NULL);
}
static UA_StatusCode 
reDrawMethod(void *handle, const UA_NodeId nodeId, size_t inputSize,
		       const UA_Variant *input, size_t optputSize, UA_Variant *output)
{
	vgus_init(info);
	return UA_STATUSCODE_GOOD;
}

static void
addReDrawMethod(UA_Server *server)
{
	UA_MethodAttributes get_temp;
	UA_MethodAttributes_init(&get_temp);

	get_temp.description = UA_LOCALIZEDTEXT("en_US","ReDraw the screen");
	get_temp.displayName = UA_LOCALIZEDTEXT("en_US", "ReDraw");
	get_temp.executable = true;
	get_temp.userExecutable = true;
	UA_Server_addMethodNode(server, UA_NODEID_STRING(1,"ReDraw"),
				UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER),
				UA_NODEID_NUMERIC(0, UA_NS0ID_HASORDEREDCOMPONENT),
				UA_QUALIFIEDNAME(1, "ReDraw"), get_temp, 
				&reDrawMethod, NULL, 0, NULL, 0, NULL, NULL);
}
int start_opc_server(char *ip, char *port)
{
	UA_ServerNetworkLayer nl;
	UA_ServerConfig config = UA_ServerConfig_standard;
        
	nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16666);
	
	config.networkLayers = &nl;
	config.networkLayersSize = 1;
	UA_Server *server = UA_Server_new(config);

	info = open_port(ip, port);
	vgus_init(info);

	addUpdateCallback(server);

	addReDrawMethod(server);
	addEndServerMethod(server);

	pid_t pid = fork();
	if (pid < 0)
		return -1;
	if (pid > 0)
		return(EXIT_SUCCESS);

	umask(0);  
	pid_t sid = setsid();  
	if (sid < 0) {  
		exit(EXIT_FAILURE);  
	}
	
	if ((chdir("/")) < 0) {  
		exit(EXIT_FAILURE);  
	} 
	close(STDIN_FILENO);  
	close(STDOUT_FILENO);  
	close(STDERR_FILENO); 
	int fd = open( "/dev/null", O_RDWR );
	dup2(fd, 0);
	dup2(fd, 1);
	dup2(fd, 2);
	UA_StatusCode retval = UA_Server_run(server, &running);
	UA_Server_delete(server);
	return (int)retval;
}


//int main()
//{
//	start_opc_server("10.193.20.217", "23");
//	return 0;
//}


