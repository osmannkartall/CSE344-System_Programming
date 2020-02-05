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
	int fd = 0;
	char buffer[4096*4];
	ssize_t nread = 0;
	ssize_t nCurrentWritten = 0;
	ssize_t nTotalWritten = 0;

	if((fd = open(argv[1], O_RDONLY)) == -1){
		printf("bash: cd: %s: No such a file or directory\n", argv[1]);
		return -1;
	}

	while((nread = read(fd, buffer, sizeof(buffer))) > 0){
		ssize_t nTotalWritten = 0;
		while(nTotalWritten < nread){
			nCurrentWritten = write(STDOUT_FILENO, buffer+nTotalWritten, nread-nTotalWritten);
			nTotalWritten += nCurrentWritten;
		}
		printf("\n");
	}

	if(close(fd) != 0)
		return -1;
	
	return nread;
}