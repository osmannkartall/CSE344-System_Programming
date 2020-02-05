#include <unistd.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <string.h>
#include <errno.h>
#include <fcntl.h>

char *supported_commands[] = {"8", "ls", "pwd", "cd", "help", "cat", "wc", "exit", "history"};

char *prevCommands[256] = {""};
char *prevParameters[256] = {""};
int currentCommandNum = 0;
static sig_atomic_t handler_flag = 0;

void showAShellPrompt();
void readCommand(char cmd[], char *parameters[]);
int cd(const char *filename);
int cat_fd(int fd);
int cat(const char *filename);
int wc(const char *filename);
int pwd();
int help();
int exitShell();
int runCommand(char command[256], char *parameters[10]);
int searchCommandList(char *command);
int history(int historyIndex);
int executeProcess(char *command, char *parameters[10]);

void catcher(int signal_number){
	int status;

	switch(signal_number){
		case SIGTERM :
			printf("ctrl + c'ye basildi. Islem sonlandiriliyor.\n");
			/* Clean up after the child process. */
			wait(&status);
			handler_flag = status;
			_exit(0);
			break;
		default :
			printf("Handler yok.\n");		
	}	
}


int main(int argc, char** argv){
	int i = 0, last = 0;
	/*Bunlari dinamik yap..*/
	char cmd[256], command[256],*parameters[10];

	struct sigaction sact;
	memset(&sact, 0, sizeof(sact));
	
	/* Set the SIGTERM signal. */
	sact.sa_handler = catcher;
	sigaction(SIGTERM, &sact, NULL);

	pid_t childPid;
	
	/* prevCommands'i initialize et. */
	for(i = 0; i < 256; i++){
		prevCommands[i] = NULL;
		prevParameters[i] = "";
	}

	

	while(strcmp(command,"exit") != 0){
		showAShellPrompt();
		readCommand(command, parameters);
	

		if(searchCommandList(command) == 0){
			runCommand(command, parameters);

			/* yapilan komutu prevCommands array'ine ekle. */
			prevCommands[last] = strdup(command);
			last = (last + 1) % 256;		
		}
		else
			printf("Desteklenmeyen komut.\n");
	}

	return 0;
}	

int searchCommandList(char *command){
	int status = -1, i = 0;
	for (i = 0; i < atoi(supported_commands[0]); ++i)
		if(strcmp(command,supported_commands[i+1]) == 0)
			status = 0;
	if(status == -1){
		if(strstr(command, "history") != NULL)
			status = 0;
	}

	return status;
	
}

int executeProcess(char *command, char *parameters[10]){
	pid_t childPid;
	int status = 0, savedErrno = 0;
	char *argv[5] = {""};

	sigset_t blockMask, origMask;
	struct sigaction saIgnore, saOrigQuit, saOrigInt, saDefault;

	sigemptyset(&blockMask); /* Blok SIGCHLD */
	sigaddset(&blockMask, SIGCHLD);
	sigprocmask(SIG_BLOCK, &blockMask, &origMask);

	saIgnore.sa_handler = SIG_IGN; /* SIGINT ve SIGQUIT i ihmal et. */
	saIgnore.sa_flags = 0;
	sigemptyset(&saIgnore.sa_mask);
	sigaction(SIGINT, &saIgnore, &saOrigInt);
	sigaction(SIGQUIT, &saIgnore, &saOrigQuit);

	switch(childPid = fork()){
		case -1:
			status = -1;
			break;
		case 0: /* child komutu gerceklestirir. */

			saDefault.sa_handler = SIG_DFL;
			saDefault.sa_flags = 0;
			sigemptyset(&saDefault.sa_mask);
			if (saOrigInt.sa_handler != SIG_IGN) sigaction(SIGINT, &saDefault, NULL);
			if (saOrigQuit.sa_handler != SIG_IGN) sigaction(SIGQUIT, &saDefault, NULL);

			sigprocmask(SIG_SETMASK, &origMask, NULL);

			if(strcmp(command, "ls") == 0){
				argv[0] = "./ls.exe";
				argv[1] = "ls";
				argv[2] = NULL;
			}
			else if(strcmp(command, "cat") == 0){
				argv[0] = "./cat.exe";
				argv[1] = parameters[0];
				argv[2] = NULL;
			}
			else if(strcmp(command, "wc") == 0){
				argv[0] = "./wc.exe";
				argv[1] = parameters[0];
				argv[2] = NULL;
			}			

			if(execvp(argv[0], argv) == -1)
				perror("exec yapilamadi.");
			_exit(127); 
		
		default: /* parent: siradaki cocugun islemini sonlandirmasini bekle. */
			while (waitpid(childPid, &status, 0) == -1) {
				if (errno != EINTR) {
					status = -1;
					break; 
				}
			}
			break;		
	}

	/* SICCHILD'in block'unu kaldir ve SIGINT ve SIGQUIT'i resetle. */
	savedErrno = errno;
	sigprocmask(SIG_SETMASK, &origMask, NULL);
	sigaction(SIGINT, &saOrigInt, NULL);
	sigaction(SIGQUIT, &saOrigQuit, NULL);
	errno = savedErrno;

	return status;
}

int runCommand(char *command, char *parameters[10]){
	int historyIndex = 0;

	/* Girilen komut pwd.. */
	if(strcmp("pwd",command) == 0)
		pwd(parameters[0]);

	/* Girilen komut cd.. */
	else if(strcmp("cd",command) == 0){
		cd(parameters[0]);
	}

	/* Girilen komut help.. */
	else if(strcmp("help",command) == 0){
		help();
	}

	/* Girilen komut exit.. */
	else if(strcmp("exit",command) == 0){
		exitShell();
	}

	/* Girilen komut history.. */
	else if(strstr(command, "history") != NULL){
		/* command[7] ('i idir. */
		historyIndex = command[8] - '0';
		history(historyIndex);
	}

	else
		executeProcess(command, parameters);

	return 0;
}


int history(int historyIndex){
	int status = 0;
	if(prevCommands[historyIndex] != NULL){
		printf("%s ", prevCommands[historyIndex]);
		printf("%s\n", prevParameters[historyIndex]);
	}
	else{ 
		printf("Henuz %d tane komut girilmedi.\n", historyIndex);
		status = -1;
	}
	return status;	
}

int pwd(){
	printf("%s\n", get_current_dir_name());
	return 0;
}

int help(){
	int i = 0;
	printf("Kullanabileceginiz komutlar:\n");
	for(i = 0; i < atoi(supported_commands[0]); i++)
		printf("--->%s\n", supported_commands[i+1]);
	printf("\nAyrica yukari ok tusuna ve sonra enter'a bastiginiz zaman bir once girmis oldugunuz komutu gorebilirsiniz.\n");
}

int exitShell(){
	return 0;
}

int cd(const char *filename){
	int status = 0;
	if(filename == NULL || chdir(filename) != 0){
		printf("bash: cd: %s: No such a file or directory\n", filename);
		status = -1;
	}
	else
		printf("%s\n", get_current_dir_name());
	return status;
}

void showAShellPrompt(){
	printf("gtusvy@debian:~$ ");
}

void readCommand(char cmd[], char *parameters[]){
	char *line = NULL;
	size_t len = 0;
	ssize_t nread;
	int i = 0, j = 0;
	char *parts[100] , *token;

	
	if((nread = getline(&line, &len, stdin)) != -1){
	/*	printf("%zu uzunlugunda satir okundu.\n", nread); */
	}
	

	/* Ilk token'i al. */
	token = strtok(line, " \n");

	while(token != NULL){
		parts[i++] = strdup(token); 
		token = strtok(NULL, " \n");
	}

	/* ilk kelime komuta denk gelir. */
	strcpy(cmd, parts[0]);

	/* Diger kelimeler komuta ait parametreleri icerir. */
	for (j = 0; j < i-1; ++j)
	{
		parameters[j] = strdup(parts[j+1]); 
		
		/* history'deki komutlari duzgun yazdirmak icin parametre bilgileri saklanmalidir. */
		if(parameters[j] != NULL){
			prevParameters[currentCommandNum] = (char *) malloc(sizeof(parameters[j]));
			strcat(prevParameters[currentCommandNum], parameters[j]);
		}
	}
	currentCommandNum++;
	parameters[j] = NULL; /* parametre listesi NULL ile bitmelidir. */
	free(line);
}