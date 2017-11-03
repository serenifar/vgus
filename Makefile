CC = gcc 
SRC = server.c vgus.c opc_server.c  usr-410s.c modbus_485.c
CLIENT = client.c
ALL: temp_server temperature
FLAGS =	-static -lopen62541 -lpthread -g -DHAVE_OPC_SERVER

temp_server: $(SRC)
	$(CC) -Wall -o  $@ ${SRC} ${FLAGS}

temperature: $(CLIENT)
	$(CC) -Wall -o  $@ ${CLIENT} ${FLAGS}

