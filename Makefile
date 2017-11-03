CC = gcc 
SRC = main.c  server.c vgus.c opc_server.c  usr-410s.c modbus_485.c
ALL: temperature
FLAGS =	-static -lopen62541 -lpthread -g

temperature: $(SRC)
	$(CC) -Wall -o  $@ ${SRC} ${FLAGS}
