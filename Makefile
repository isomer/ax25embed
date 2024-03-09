CFLAGS= -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes

all: embedax25

embedax25: embedax25.c kiss.o metric.o ax25.o driver-tcpip.o packet.o

clean:
	rm -f *.o
