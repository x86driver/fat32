TARGET = read

all: $(TARGET)
#CC = arm-none-linux-gnueabi-gcc
CC = gcc
CFLAGS = -fno-stack-protector -std=c99 -g

namei.o:namei.c
	$(CC) -Wall -c -o $@ $< $(CFLAGS)
radix.o:radix.c radix.h mm.h mm.o
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

dir.o:dir.c dir.h page.h io.h fat32.h
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

superblock.o:superblock.c superblock.h page.h
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

io.o:io.c io.h
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

mm.o:mm.c mm.h page.h radix.h
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

buffer.o: buffer.c buffer.h mm.h page.h fat32.h io.h radix.h superblock.h dir.h lib.h
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

read.o:read.c
	$(CC) -Wall -c -o $@ $< $(CFLAGS)

read:read.o buffer.o dir.o radix.o superblock.o io.o read.o mm.o namei.o
	$(CC) -Wall -o $@ $^ $(CFLAGS)

clean:
	rm -rf $(TARGET) *.o

