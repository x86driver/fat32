#include <stdio.h>
#include <unistd.h>

int main(int argc, char **argv)
{
	char number[32];
	int i;
	for (i = 0; i < 262144; i+=8192) {
		snprintf(number, sizeof(number), "./buffer %d", i);
		system(number);
	}
	perror("execve");
	return 0;
}

