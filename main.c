#include <stdio.h>                  // Provides standard input/output functions
#include <string.h>                 // Provides string processing support functions
#include <stdlib.h>                 // Provides memory management and program control functions
#include <unistd.h>                 // Contains functions related to UNIX/Linux systems
#include <sys/types.h>              // Defines data types such as pid_t (process ID)
#include <sys/wait.h>               // Support for child process control functions
#include <readline/readline.h>      // Support reading commands from users with advanced features
#include <readline/history.h>       // Manage input command history

// Clearing the shell screen using escape sequences
#define clear() printf("\033[H\033[J")

// Start the Shell
void init_shell() {
    clear();
    char *user = getenv("USER");    // Get the current username from the USER environment variable on Linux systems.
    printf("\n| ------------------------------------------------------------------- |");
    printf("\n|                                                                     |");
    printf("\n| ___________________ Welcome to Astatine's Shell ____________________|");
    printf("\n|                                                                     |");
    printf("\n|                      Have a nice day at work!                       |");
    printf("\n|                                                                     |");
    printf("\n|                       **********************                        |");
    printf("\n|                                                                     |");
    printf("\n| ------------------------------------------------------------------- |");
    printf("\n_________________________  User is : @%s  _________________________\n",user);
    printf("\n***********************************************************************\n\n\n");
    sleep(1);
}

// Function to take input and store to str
int take_input(char* str) {
    char* buff = readline("\033[0;32m\n>>> \033[0m");     // Read input from keyboard, support editing, autocomplete, and history
    
    // Check for NULL for buff to avoid error when readline() returns NULL
    if (!buff) {
        printf("\nError reading input...\n");
        return -1;
    }

    if (strlen(buff) != 0) { 
        add_history(buff);                          // Save commands entered by the user to history
        strcpy(str, buff);                          // Copy commands entered by the user to str
        free(buff);
        return 0;
    } else {
        free(buff);
        return 1;
    }
}

// Function to print current directory
void current_dir() { 
    char cur_dir[1024];
    if( getcwd(cur_dir, sizeof(cur_dir)) != NULL ){                                             // Get "Current Working Directory" and save to cur_dir
        printf("\n\033[0;32mCurrent Working Directory:\033[0m \033[0;34m%s\033[0m\n", cur_dir); // Print directory path value from cur_dir
    } else{
        perror("getcwd");
    }
}

// Function where the system command is executed
void exec_syscmd(char *parsed[]) {
    pid_t process_pid = fork();

    if (process_pid >= 0) {
        if (process_pid == 0) {                                     // Child Process
            if (execvp(parsed[0], parsed) < 0) {
                perror("\nError: could not execute command...");
                exit(EXIT_FAILURE);
            }
        }
        else {                                                      // Parent Process
            wait(NULL);
            return;
        }
    } 
    else {
        perror("fork() is failed\n");
        return;
    }
}

// Function where the piped system commands is executed
void exec_pipecmd(char **cmd1, char **cmd2) {
    int fds[2];
    int status;
    pid_t pid1, pid2;

    if (pipe(fds) < 0) {
        perror("pipe() unsuccessfully\n");
        return;
    }

    if ( (pid1 = fork()) < 0) {
        perror("\nCould not fork pid1");
        return;
    } else if (pid1 == 0) {
        // Child 1 executing ...
        // Write at the write end
        close(fds[0]);                  // Close the reader (not in use)
        dup2(fds[1], STDOUT_FILENO);    // Switch stdout to fds[1]
        close(fds[1]);                  // Close the writer after copying

        if (execvp(cmd1[0], cmd1) < 0) {
            perror("\nCould not execute command 1 ..."); 
            exit(EXIT_FAILURE); 
        }
    } else {
        // Parent executing
        if ( (pid2 = fork()) < 0) {
            perror("\nCould not fork pid2");
            return;
        }

        // Child 2 executing ...
        // Read at the read end
        if (pid2 == 0) {
            close(fds[1]);                  // Close the writer (not in use)
            dup2(fds[0], STDIN_FILENO);     // Switch stdin to fds[0]
            close(fds[0]);                  // Close the reader after copying

            if (execvp(cmd2[0], cmd2) < 0) {
                perror("\nCould not execute command 2 ...");    
                exit(EXIT_FAILURE); 
            }
        }
        else {
            close(fds[0]); close(fds[1]);
            waitpid(pid1, &status, 0);
            waitpid(pid2, &status, 0);
        }
    }
}

// Function for check pipe and separate string
int parse_pipe(char *str, char **strpipe) {
    // Cut the string str at the |, save the part before | to strpipe[i],
    // and update str pointing to the remaining part
    strpipe[0] = strsep(&str, "|");
    strpipe[1] = strsep(&str, "|");

    // returns 0 if no pipe is found, or 1 otherwise
    return (strpipe[1] != NULL);
}

// Function for parsing command words
void parse_space(char *str, char **parsed) {
    for (int i = 0; i < 100; i++) {
        parsed[i] = strsep(&str, " ");

        if (parsed[i] == NULL) {
            break;
        }
        if (strlen(parsed[i]) == 0) {       // Remove extra spaces
            i--;
        }
    }
}

// Function for process input command 1 and command 2
int process_string(char *str, char **cmd1, char **cmd2) {
    char *strpipe[2];

    int check = parse_pipe(str, strpipe);

    if (check) {
        // if it is including a pipe
        parse_space(strpipe[0], cmd1);
        parse_space(strpipe[1], cmd2);
    }
    else {
        // if it is a simple command
        parse_space(str, cmd1);
    }

    if (strcmp(cmd1[0], "cd")  == 0) {
        if( (chdir(cmd1[1]) != 0) ||  (cmd1[1] == NULL) ){
            perror("cd failed\n");
        }
        return 0;
    }
    else {
        return check + 1;
    }
}

// Clear memory command
void clear_mem(char **cmd) {
    memset(cmd, 0, sizeof(char*) * 100);    // Set a memory area to a unique value
}

int main() {
    char input[1000];
    char *cmd1[100];
    char *cmd2[100];

    init_shell();

    while(1) {
        current_dir();
        if (take_input(input) == -1) break;

        int flag = process_string(input, cmd1, cmd2);

        if (flag == 1) {
            exec_syscmd(cmd1);
        }
        else if (flag == 2) {
            exec_pipecmd(cmd1, cmd2);
        }

        clear_mem(cmd1);
        clear_mem(cmd2);
    }

    return 0;
}