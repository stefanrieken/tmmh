CC=gcc
CFLAGS=-g -fdata-sections -ffunction-sections -Wall -Os -Wl,-dead_strip -fPIC
SOURCES = tmmh.c types.c stats.c gc.c
OBJECTS = $(SOURCES:.c=.o)
TEST_SOURCES = $(SOURCES) test.c

all: libtmmh.a libdummytmmh.a libtmmh.so libdummytmmh.so

clean:
	rm -f *.o libtmmh.a libtmmh.so test

libtmmh.a: $(OBJECTS)
	ar rcs libtmmh.a $(OBJECTS)

libdummytmmh.a: dummy.o
	ar rcs libdummytmmh.a dummy.o

libtmmh.so: $(OBJECTS)
	gcc -shared -o libtmmh.so $(OBJECTS)

libdummytmmh.so: $(OBJECTS)
	gcc -shared -o libdummytmmh.so dummy.o

%.o: %.c
	$(CC) -D TMMH_HEADER_64 -c $(CFLAGS) $< -o $@

test: $(TEST_SOURCES)
	$(CC) -D TMMH_HEADER_32 -D TMMH_OPTIMIZE_SIZE $(CFLAGS) $(TEST_SOURCES) -o $@
