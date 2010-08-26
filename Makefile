TARGET = read

all: $(TARGET)

read:read.c
	gcc -Wall -o $@ $< -g


clean:
	rm -rf ($TARGET)

