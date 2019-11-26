CC=gcc
CFLAGS=-g -fdata-sections -ffunction-sections -Wall -Os -Wl,-dead_strip -fPIC
SOURCES = tmmh.c types.c stats.c gc.c
OBJECTS = $(SOURCES:.c=.o)
TEST_SOURCES = $(SOURCES) test.c
TEST_OBJECTS = $(TEST_SOURCES:.c=.o)

all: libtmmh.a libdummytmmh.a libtmmh.so libdummytmmh.so test

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

test: $(TEST_OBJECTS)
	$(CC) $(CFLAGS) $(TEST_OBJECTS) -o $@

%.o: %.c
	$(CC) -c $(CFLAGS) $< -o $@
	# For test:
	#$(CC) -D TMMH_OPTIMIZE_SIZE -c $(CFLAGS) $< -o $@
