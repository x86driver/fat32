TARGET = read

all: $(TARGET)

read:read.c fat32.h
	gcc -Wall -o $@ $< -g


clean:
	rm -rf ($TARGET)

