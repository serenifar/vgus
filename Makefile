OPC = "true"
CC = gcc 
SRC = server.c vgus.c usr-410s.c modbus_485.c
CLIENT = client.c
ALL: temp_server temperature
FLAGS =	-static -lpthread -g 
ifeq ($(OPC),"true")
FLAGS += -lopen62541 -DHAVE_OPC_SERVER
SRC += opc_server.c
endif



temp_server: $(SRC)
	$(CC) -Wall -o  $@ ${SRC} ${FLAGS}

temperature: $(CLIENT)
	$(CC) -Wall -o  $@ ${CLIENT} ${FLAGS}

clean:
	rm temperature temp_server
