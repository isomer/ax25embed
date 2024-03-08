CFLAGS=-Os -g -Wall -Wextra -Wmissing-prototypes -Wstrict-prototypes

all: kiss.o metric.o

clean:
	rm -f *.o
