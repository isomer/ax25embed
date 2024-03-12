CFLAGS= -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes

LIB=kiss.o metric.o ax25.o packet.o
PLATFORM?=platform-c.o

all: serial-tcpip serial-tty

serial-tcpip: serial-tcpip.o $(LIB) $(PLATFORM)
serial-tty: serial-tty.o $(LIB) $(PLATFORM)

clean:
	rm -f *.o *.a
