TARGET = radix

all: $(TARGET)

read:read.c fat32.h
	gcc -Wall -o $@ $< -g

radix:radix.c radix.h mm.h
	gcc -Wall -o $@ $< -g -std=c99

clean:
	rm -rf ($TARGET)

