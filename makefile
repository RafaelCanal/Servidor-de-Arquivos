# difene paths

INCLUDE = ../../../usr/include
SYS = ../../../usr/sys
NETINET = ../../../usr/netinet/in.h
ARPA = ../../../usr/arpa/inet.h

# define the C compiler to use
CC = gcc

#compile
#$(CC)  -I $(INCLUDE) -I $(SYS) -I $(NETINET) -I $(ARPA) -o servidor servidor.c
#$(CC)  -I $(INCLUDE) -I $(SYS) -I $(NETINET) -I $(ARPA) -o cliente cliente.c
all:
	$(CC)  -o servidor servidor.c
	$(CC)  -o cliente cliente.c	

#run server
runservidor:
	./servidor.exe

#run client
runcliente:
	./cliente.exe

#clean executables
clean:
	rm *.exe