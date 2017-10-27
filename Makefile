CC = gcc 
SRC = opc_client_updata.c opc_server.c vgus_controller.c 
ALL: latency
FLAGS =	-static -lopen62541

latency: $(SRC)
	$(CC) -Wall -o  $@ ${SRC} ${FLAGS}
