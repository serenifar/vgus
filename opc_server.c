#include <signal.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "open62541.h"
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "vgus.h"
#include "opc_server.h"
#include "modbus_485.h"


UA_Boolean opc_running = true;
unsigned char *running = (unsigned char *)&opc_running;

void block_client()
{
	if(blocking){
		pthread_mutex_lock(&blocking_lock);		
		pthread_mutex_unlock(&blocking_lock);		
	}
}

struct send_info *opc_info_458, *opc_info_232;
static void end_opc_server()
{
	opc_running = false;
}

static void 
get_temperature(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	block_client();
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 temperature = modbus_get_temperature();
	UA_Variant value;
	UA_Variant_setScalarCopy(&value, &temperature,
				 &UA_TYPES[UA_TYPES_INT32]);
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "temperature");
	UA_Server_writeValue(server, tempNodeId, value);
}


static void
addGetTemperatureCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "temperature");
	UA_Int32 temperature = 243;
	UA_Variant_setScalar(&attr.value, &temperature, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "temperature");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "temperature");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = get_temperature;
	callback.onWrite = NULL;
	UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
}

static void 
set_warn_value(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	block_client();
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 warn;
	UA_Variant value;
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "warn_value");
	UA_Server_readValue(server, tempNodeId, &value);
	warn = *(UA_Int32 *)(value.data);
	modbus_update_warn_vaules(opc_info_232, warn >> 16, warn | 0xffff);
}


static void
addSetWarnCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "warn_vaule");
	UA_Int32 warn = (75 << 16) + 25;
	UA_Variant_setScalar(&attr.value, &warn, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "warn_vaule");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "warn_vaule");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = NULL;
	callback.onWrite = set_warn_value;
	UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
}

static void 
set_heating_score(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	block_client();
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 score;
	UA_Variant value;
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "heating_score");
	UA_Server_readValue(server, tempNodeId, &value);
	score = *(UA_Int32 *)(value.data);
	modbus_update_heating_score(score);
}


static void
addSetHeatingCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "heating_score");
	UA_Int32 score = 0;
	UA_Variant_setScalar(&attr.value, &score, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "heating_score");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "heating_score");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = NULL;
	callback.onWrite = set_heating_score;
	UA_Server_setVariableNode_valueCallback(server, newNodeId, callback);
}

static void 
set_cooling_score(void *handle, const UA_NodeId nodeId,
		      const UA_Variant *data, const UA_NumericRange *range){
	block_client();
	UA_Server *server = (UA_Server *)handle;
	UA_Int32 score;
	UA_Variant value;
	UA_NodeId tempNodeId = UA_NODEID_STRING(1, "cooling_score");
	UA_Server_readValue(server, tempNodeId, &value);
	score = *(UA_Int32 *)(value.data);
	printf("%d\n",score);
	modbus_update_cooling_score(score);
}


static void
addSetCoolingCallback(UA_Server *server)
{
	/*1. Define the node attributes */
	UA_VariableAttributes attr;
	UA_VariableAttributes_init(&attr);
	attr.displayName = UA_LOCALIZEDTEXT("en_US", "cooling_score");
	UA_Int32 score = 0;
	UA_Variant_setScalar(&attr.value, &score, &UA_TYPES[UA_TYPES_INT32]);
	
	/*2. Define where the node shall be added with which browsename*/
	UA_NodeId newNodeId = UA_NODEID_STRING(1, "cooling_score");
	UA_NodeId parentNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_OBJECTSFOLDER);
	UA_NodeId parentReferenceNodeId = UA_NODEID_NUMERIC(0, UA_NS0ID_ORGANIZES);
	UA_NodeId variableType = UA_NODEID_NULL;
	UA_QualifiedName browseName = UA_QUALIFIEDNAME(1, "cooling_score");

	/*3. Add the node*/
	UA_Server_addVariableNode(server, newNodeId, parentNodeId,
			          parentReferenceNodeId,browseName,
				  variableType, attr, NULL, NULL);
	
	/*4. add callBack func*/
	UA_ValueCallback callback;
	callback.handle = server;
	callback.onRead = NULL;
	callback.onWrite = set_cooling_score;
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
	block_client();
	vgus_init(opc_info_232);
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

int start_opc_server(struct send_info *info_458, struct send_info *info_232)
{

	opc_info_232 = info_232;
	opc_info_458 = info_458;
	UA_ServerNetworkLayer nl;
	UA_ServerConfig config = UA_ServerConfig_standard;
        
	nl = UA_ServerNetworkLayerTCP(UA_ConnectionConfig_standard, 16666);
	
	config.networkLayers = &nl;
	config.networkLayersSize = 1;
	config.logger = NULL;
	UA_Server *server = UA_Server_new(config);


	addReDrawMethod(server);
	addEndServerMethod(server);
	addGetTemperatureCallback(server);
	addSetWarnCallback(server);
	addSetCoolingCallback(server);
	addSetHeatingCallback(server);

	UA_StatusCode retval = UA_Server_run(server, &opc_running);
	UA_Server_delete(server);
	return (int)retval;
}
