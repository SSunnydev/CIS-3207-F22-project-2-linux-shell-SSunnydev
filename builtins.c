#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <dirent.h>
#include <string.h>

#include "myshell.h"

extern char** environ; // an array of environment variables in form of KEY=VALUE, terminated by NULL

void builtins(char** progArgv, int* is_builtinPtr, int* pids, char* line){
	if (strcmp(progArgv[0], "cd") == 0) {
		// handle cd
		cd_func(progArgv[1]);
		*is_builtinPtr = 1;

	}
	if (strcmp(progArgv[0], "clr") == 0) {
		//handle clr
		printf("\e[1;1H\e[2J");
		*is_builtinPtr = 1;
	}
	if (strcmp(progArgv[0], "dir") == 0) {
		//handle dir
		dir_func(progArgv[1]);
		*is_builtinPtr = 1;
	}
	if (strcmp(progArgv[0], "environ") == 0) {
		//handle environ
		//List all of the environ strings
		int i = 0;
		while(environ[i] != NULL) {
			printf("%s\n", environ[i]);
			i++;
		}
		*is_builtinPtr = 1;
	}
	if (strcmp(progArgv[0], "echo") == 0) {
		//handle echo
		*is_builtinPtr = 1; //iterate through argv and print the arguments not on a new line
		int i = 1;
		while (progArgv[i] != NULL){
			if (i == 1) {
				printf("%s", progArgv[i]);
				i++;
				continue;
			}
			printf(" ");
			printf("%s", progArgv[i]);
			i++;
		}
		printf("\n");

	}
	if (strcmp(progArgv[0], "help") == 0) {
		//handle help
		if (progArgv[1] != NULL) {
			help_func(progArgv[1]);
		}
		
		*is_builtinPtr = 1;
	}
	if (strcmp(progArgv[0], "pause") == 0) {
		//handle pause
		*is_builtinPtr = 1;
		printf("%s\n", "Pause\n");
		// read from stdin plsz
		char input[1];
		int n = read(STDIN_FILENO, input, 1);
		while(input[0] != '\n') {
			read(STDIN_FILENO, input, 1);
		}
		//printf("Enter has been Pressed\n");

	}
	if (strcmp(progArgv[0], "quit") == 0) {
		//handle quit
		*is_builtinPtr = 1;
		free(pids);
		int j = 0;
		while (progArgv[j] != NULL) {
			free(progArgv[j]);
			j++;
		}
		free(progArgv);
		free(line);
		exit(0);
	}
}

void help_func(char *command){ //help dir
	if (strcmp(command, "cd") == 0) {
		//print cd
		printf("Changes the directory\n");
	} else if (strcmp(command, "dir") == 0) {
		//print dir
		printf("Lists all of the files in the directory\n");
	} else if (strcmp(command, "clr") == 0) {
		//print clear
		printf("Clears the screen\n");
	} else if (strcmp(command, "environ") == 0) {
		//print environment
		printf("Lists all of the environment strings\n");
	} else if (strcmp(command, "echo") == 0) {
		//print echo
		printf("Displays <comment> on the display and is followed by a new line.\n");
	} else if (strcmp(command, "pause") == 0) {
		//print pause
		printf("Pauses operation of the shell, waiting until 'Enter' is pressed by the user.\n");
	} else if (strcmp(command, "quit") == 0) {
		//print quit
		printf("Quits the shell\n");
	} else {
		char error_message[30] = "An error has occured\n";
		write(STDERR_FILENO, error_message, strlen(error_message));
	}

}

int cd_func(char *cd_dir) {
	//get cwd assign to curr
	char curr[1000] = {0};
    getcwd(curr, 1000);
	//gets string command ex cd directory, check if anything is passed in.
	// change the beat
    if (cd_dir == NULL) {//nothing passed in
        printf("%s\n", curr); //print where we currently are
        return 0;
    }
    if (chdir(cd_dir) < 0) { //check call to cd_dir for error
		char error_message[30] = "An error has occured\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
        return -1;
	}
	
    getcwd(curr, 1000);
    setenv("PWD", curr, 1);
    return 0;
}

void dir_func(char *curr_path) {
    struct dirent **namelist;
    
	//when they dont pass a directory set curr_path = "."
	if (curr_path == NULL) {
		//didnt pass
		curr_path = ".";
	}
	
    int n = scandir(curr_path, &namelist, NULL, alphasort);
    if (n == -1) {
        char error_message[30] = "An error has occured\n";
        write(STDERR_FILENO, error_message, strlen(error_message));
    }

    while (n--) {
        // skip .. and . entries
        if (strcmp(namelist[n]->d_name, "..") == 0 || strcmp(namelist[n]->d_name, ".") == 0) {
            free(namelist[n]);
            continue;
        }
        printf("%s\n", namelist[n]->d_name);

        free(namelist[n]);
    }
    free(namelist);
}
