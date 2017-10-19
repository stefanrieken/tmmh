CC=gcc
CFLAGS=-fdata-sections -ffunction-sections -Wall -Os -Wl,--gc-sections
SOURCES = $(wildcard *.c)
OBJECTS = $(SOURCES:.c=.o)

all: tmmh

tmmh: $(OBJECTS)
	$(CC) $(CFLAGS) $(OBJECTS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@

clean:
	rm *.o tmmh
