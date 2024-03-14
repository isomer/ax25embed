CFLAGS= -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes

LIB= \
	 ax25_dl.o \
	 ax25.o \
	 buffer.o \
	 connection.o \
	 kiss.o \
	 metric.o \
	 packet.o \
	 ssid.o \

PLATFORM?=platform-c.o

all: serial-tcpip serial-tty

serial-tcpip: serial-tcpip.o $(LIB) $(PLATFORM)
serial-tty: serial-tty.o $(LIB) $(PLATFORM)

clean:
	rm -f *.o *.a
