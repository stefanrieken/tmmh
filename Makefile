CC=gcc
CFLAGS=-fdata-sections -ffunction-sections -Wall -Os -Wl,--gc-sections -fPIC
SOURCES = tmmh.c
OBJECTS = $(SOURCES:.c=.o)
TEST_SOURCES = $(wildcard *.c)
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

all: libtmmh.a libtmmh.so test

clean:
	rm *.o libtmmh.a libtmmh.so test

libtmmh.a: $(OBJECTS)
	ar rcs libtmmh.a $(OBJECTS)

libtmmh.so: $(OBJECTS)
	gcc -shared -o libtmmh.so $(OBJECTS)

test: $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(TEST_OBJECTS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
