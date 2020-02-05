#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/dir.h>
#include <string.h>
#include <dirent.h>
#include <error.h>

int main(int argc, char *argv[]){
	DIR *dirp;
	struct dirent *dp;

	if((dirp = opendir(".")) == NULL)
		perror("Dizin acilamadi.");

	do {
		if((dp = readdir(dirp)) != NULL){
			printf("%s ", dp->d_name);
		}
	}while(dp != NULL);
	printf("\n");

	(void) closedir(dirp);

	return 0;
}