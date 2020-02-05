#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

int main(int argc, char *argv[]){
	int fd = 0, i = 0, numLine = 1;
	char buffer[4096*4];

	if((fd = open(argv[1], O_RDONLY)) == -1){
		printf("bash: cd: %s: No such a file or directory\n", argv[1]);
		return -1;
	}

	if(read(fd, buffer, sizeof(buffer)) < 0)
		return -1;

	for(i = 0; buffer[i] != '\0'; i++)
		if(buffer[i] == '\n')
			numLine++;
	printf("%d %s\n", numLine, argv[1]);

	if(close(fd) != 0)
		return -1;

	return numLine;
}