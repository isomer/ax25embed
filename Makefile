CFLAGS?= -Og -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes -pedantic -std=c2x

LIB= \
	 ax25_dl.o \
	 ax25.o \
	 buffer.o \
	 connection.o \
	 debug.o \
	 kiss.o \
	 metric.o \
	 packet.o \
	 ssid.o \
	 time.o \

PLATFORM?=platform-posix.o
SERIAL?=serial-tty.o

all: app-greet app-cli

app-greet: app-greet.o libax25embed.a($(LIB)) $(PLATFORM) $(SERIAL)
app-cli: app-cli.o libax25embed.a($(LIB)) $(PLATFORM) $(SERIAL) app-caseflip.o

fuzz: CFLAGS+=-fsanitize=address,fuzzer
fuzz: CC=clang-18
fuzz: fuzz.c libax25embed.a($(LIB)) platform-null.o serial-dummy.o

clean:
	rm -f *.o *.a app-caseflip app-greet
