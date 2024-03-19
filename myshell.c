#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include "myshell.h"
#include <fcntl.h>

int is_background = 0;

// different file for handling built-in commands, maybe builtins.c

int countTokens(char *str, char **save) {
	char *token;
   	token = strtok_r(str, " \t\r\n", save); //lets any of those characters be a delimiter
    int tokenCount = 0;
   	while (token) {
		// printf("token: %s\n", token);

		if (strcmp(token, "|") == 0) {
			return tokenCount;
		}

		if (strcmp(token, "&") == 0) {
			return tokenCount;
		}

		if (strcmp(token, "<") == 0) {
			token = strtok_r(NULL, " \t\r\n", save);
			token = strtok_r(NULL, " \t\r\n", save);
			continue;
		}

		if (strcmp(token, ">") == 0) {
			token = strtok_r(NULL, " \t\r\n", save);
			token = strtok_r(NULL, " \t\r\n", save);
			continue;
		}
		if (strcmp(token, ">>") == 0) {
			token = strtok_r(NULL, " \t\r\n", save);
			token = strtok_r(NULL, " \t\r\n", save);
			continue;
		}

		tokenCount++;
      	token = strtok_r(NULL, " \t\r\n", save);
    }
	
	return tokenCount;
}

int countCommands(char *str) {
	char *token, *last_token = NULL;
	token = strtok(str, " \t\r\n");
	int count = 1;
	while (token) {
		if (strcmp(token, "|") == 0) {
			count++;
		}

		if (strcmp(token, "&") == 0) {
			count++;
		}

		last_token = token;
		token = strtok(NULL, " \t\r\n");
	}

	// if last token is ampersand, set is background to 1
	if (last_token != NULL && strcmp(last_token, "&") == 0) {
		count--;
		is_background = 1;
	}

	return count;
}

void shell(FILE* fp) {

	char cwd1[1000];
	char fullpath[1000];
	char underscore[100]; // ./myshell

	//set env var SHELL at beginning of shell
	//set env var PATH at beginning of shell
	setenv("PATH", "/bin", 1);
	//strcat(cwd1, getenv("_"));
	strcpy(underscore, getenv("_")); 

	strcpy(fullpath, getcwd(fullpath, sizeof(fullpath))); 
	strcat(fullpath, "/");
	strcat(fullpath, underscore);

	char real[1000];
	realpath(fullpath, real); 
	
	setenv("SHELL", real, 1);


	// save a copy of stdin and stdout
	int stdinSave = dup(STDIN_FILENO);
	int stdoutSave = dup(STDOUT_FILENO);

	char* input = NULL; //for counting tokens for single command
	size_t n;
	
    while (1) {
		if (fp == stdin) { // if we are in interactive mode, fp is stdin so print prompt
			prompt();
		}

        char copy[1000] = {0}; //used to put things progargV, also to handle dup2 stuff aka redirection
		if (getline(&input, &n, fp) <= 0) {
			break;
		}
        strcpy(copy, input);
		
		// printf("before countCommands\n");
		int amountOfCommands = countCommands(input);
		if (is_background) {
			char *ptr = strrchr(copy, '&');
			if (!ptr) {
				*ptr = '\0'; // replace the background ampersand so strtok doesnt see it
			}
		}
		// printf("before strcpy(input, copy)\n");
		strcpy(input, copy); // restore input
		char* inputSave = NULL;
		char* copySave = NULL;
		int firstUse = 1; //asks is it the first time we do strtok_r on copy
		//why this matters is because we get seg faults if we dont

		// printf("before pids malloc\n");
		int* pids = malloc(sizeof(int) * amountOfCommands);
		memset(pids, 0, amountOfCommands*sizeof(pids[0]));

		int inRedirectionTmp = 0;

		// printf("cmd amt: %d\n", amountOfCommands);
		for (int i = 0; i < amountOfCommands; i++) {

			// printf("current command #: %d\n", i);

			int inRedirection = STDIN_FILENO;
			int outRedirection = STDOUT_FILENO;

			
			//If there was a pipe with prev cmd, then tmp has read end of pipe. we use it for input redirection and then reset tmp
			if (inRedirectionTmp != 0) {
				inRedirection = inRedirectionTmp;
				inRedirectionTmp = 0;
			}
			
			int tokenCount;
			// printf("before countTokens\n");
			if (i == 0) {
				tokenCount = countTokens(input, &inputSave);
			} else {
				tokenCount = countTokens(NULL, &inputSave);
			}

			// printf("tokenCount: %d\n", tokenCount);

			// skip if there is no command
			if (tokenCount == 0) {
				if (firstUse) {
					strtok_r(copy, " \t\r\n", &copySave);
					firstUse = 0;
				} else {
					strtok_r(NULL, " \t\r\n", &copySave);
				}

				continue;
			}

			// printf("before argv malloc\n");
			char **progArgv = malloc(sizeof(char*) * (tokenCount + 1)); //allocate for argv
			char* token;

			// printf("before getting cmd's first token\n");
			if (firstUse) {
				token = strtok_r(copy, " \t\r\n", &copySave);
				firstUse = 0;
			} else {
				token = strtok_r(NULL, " \t\r\n", &copySave);
			}


			int j = 0;
			while (token) {
				//input read from file
				if (strcmp(token, "<") == 0) {
					char* filename = strtok_r(NULL, " \t\r\n", &copySave);
					// now open it with the correct flags and put the fd into inRedirection
					// use open instead of fopen
					int input_fd = 0;
					input_fd = open(filename, O_RDONLY, 0);
					inRedirection = input_fd;
					token = strtok_r(NULL, " \t\r\n", &copySave);
					continue;
				}
				
				// handle >
				//STDOUT written to file
				if (strcmp(token, ">") == 0) {
					char* filename = strtok_r(NULL, " \t\r\n", &copySave);
					int output_fd = 0;
					output_fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0644);
					outRedirection = output_fd;
					token = strtok_r(NULL, " \t\r\n", &copySave);
					continue;
				}

				// handle >> append
				if (strcmp(token, ">>") == 0) {
					char* filename = strtok_r(NULL, " \t\r\n", &copySave);
					int append_fd = 0;
					append_fd = open(filename, O_WRONLY | O_APPEND | O_CREAT, 0644);
					outRedirection = append_fd;
					token = strtok_r(NULL, " \t\r\n", &copySave);
					continue;
				}

				// handle |
				if (strcmp(token, "|") == 0) {
					int pipefd[2];
					pipe(pipefd);
					inRedirectionTmp = pipefd[0]; //next command will use the read end of the pipe
					outRedirection = pipefd[1];
					break;
				}
				
				// handle &
				if (strcmp(token, "&") == 0) {
					break;
				}

				// printf("before strdup\n");
				progArgv[j] = strdup(token);

				token = strtok_r(NULL, " \t\r\n", &copySave);
				j++;
			}
			progArgv[tokenCount] = NULL;

			//We pointed inRedir and outRedir to the file
			// dup2 for input
			// printf("before redirections\n");
			if (inRedirection < 0) {
				// error
				char error_message[30] = "An error has occured\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			// do dup2
			dup2(inRedirection, STDIN_FILENO);

			// dup2 for output
			if (outRedirection < 0) {
				// error
				char error_message[30] = "An error has occured\n";
				write(STDERR_FILENO, error_message, strlen(error_message));
				continue;
			}
			// do dup2
			dup2(outRedirection, STDOUT_FILENO);


			int is_builtin = 0;

			// printf("before builtins\n");
			builtins(progArgv, &is_builtin, pids, input);
			fflush(stdout); //The program was exploding without flushing here, unsure when to flush stdout

			// handle external commands

			if (!is_builtin) {
				// External Commands

				// printf("before fork\n");
				pid_t pid = fork();
				if (pid == 0) {
					// CHILD
					//Set parent env var = <pathname>myshell
					setenv("parent", real, 1);

					// do: make child close up fds
					if (inRedirection != STDIN_FILENO) {
						close(inRedirection);
						// printf("inRedirection: %d\n", inRedirection);
					}
					if (outRedirection != STDOUT_FILENO) {
						// printf("outRedirection: %d\n", outRedirection);
						close(outRedirection);
					}
					close(stdinSave);
					// printf("stdinSave: %d\n", stdinSave);

					close(stdoutSave);
					// printf("stdoutSave: %d\n", stdoutSave);

					// bin --> usrbin --> cwd
					bin_func(progArgv);
					//printf("bin\n");
					userbin_func(progArgv);
					//printf("user\n");
					cwd_func(progArgv);
					//printf("cwd\n");
					
					// error
					exit(1);
				} else if (pid > 0) {
					// PARENT
					pids[i] = pid;
				} else { // pid == -1
					// handle failure
					char error_message[30] = "An error has occured\n";
					write(STDERR_FILENO, error_message, strlen(error_message));
				}
			}

			// close the former redirection
			if (inRedirection != STDIN_FILENO) {
				close(inRedirection);
			}
			if (outRedirection != STDOUT_FILENO) {
				close(outRedirection);
			}
			
			// restore stdin and stdout
			// uses dup2
			dup2(stdinSave, STDIN_FILENO);
			dup2(stdoutSave, STDOUT_FILENO);

			// printf("before clean up progargv\n");
			// clean up malloc
			for (int j = 0; j < tokenCount; j++) {
				free(progArgv[j]);
			}
			free(progArgv);
		}

		// wait for every child to finish
		if (!is_background) {
			for (int i = 0; i < amountOfCommands; i++) {
				if (pids[i] > 0) {
					waitpid(pids[i], NULL, 0); //-1 = any child, null = don't use exit status of children, 0 = set no options

				}
			}
		}
		
		// printf("before general waitpid\n");
		while (1) {
			if (waitpid(-1, NULL, WNOHANG) <= 0) { //-1 = any child, null = don't use exit status of children, WNOHANG = dont block
				break;
			}
		}

		is_background = 0;
		// printf("bouta free %p\n", pids);
		free(pids);
	}
}

void interactiveMode() {
	shell(stdin);
}

void cwd_func(char** progArgv) {
	if (access(progArgv[0], X_OK) == 0) {
		execvp(progArgv[0], progArgv);
	}
}


void userbin_func(char** progArgv) {
	char array[1000]; 
	strcpy(array, "/usr");
	strcat(array, "/"); 
	strcat(array, "bin");
	strcat(array, "/"); 
	strcat(array, progArgv[0]);

	if (access(array, X_OK) == 0) {
		execvp(array, progArgv);
	}
}

void bin_func(char** progArgv) { //cd 
	// we need bin/cd
	// strcat("Bin" + command)
	//Tells us if we can use a binary file
	char array[1000]; 
	strcpy(array, "/bin");
	strcat(array, "/"); 
	strcat(array, progArgv[0]);

	// check bin -> userbin -> cwd
	if (access(array, X_OK) == 0) { // bin/ls exists & able to be executed on, we will execvp it 
		execvp(array, progArgv);
	}
	
}
void prompt(){
	// environment varialbes can be accessed via getenv()
	// build up a string that is [CWD]/[shell-prompt]
	char cwd[1000];
	// getcwd();
	// strcpy(prompt, PWD);
	// strcat(prompt,"/");
	if (getcwd(cwd, sizeof(cwd)) != NULL) { 
		fprintf(stdout, "%s/", cwd);
		printf("myshell> ");
	} else {
		perror("getcwd() error");
	}
	
}

void nonInteractiveMode(char* filename) {
	// basically the same thing as interactive mode code but instead
	// get a file pointer from filename, pass into shell()
	FILE *fp;
	fp = fopen(filename, "r");
	if (fp == NULL) {
		char error_message[30] = "An error has occured\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	}
	shell(fp);
	fclose(fp);
	
}

int main(int argc, char *argv[]) {
    if (argc == 1) {
        interactiveMode();
    } else if (argc > 2) {
		char error_message[30] = "An error has occured\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
		exit(1);
	} else {
        nonInteractiveMode(argv[1]); // argv[1] is the first argument to ./myshell
	}
}

/*
bucket list
handle errors correctly
*/
