#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h> 
#include <string.h> 
#include <sys/types.h> 
#include <sys/wait.h> 
#include <fcntl.h> 
 
#define MAX_COMMANDS 10 
#define MAX_TOKENS 10 
#define MAX_TOKEN_LENGTH 256 
#define MAX_COMMAND_LENGTH 1024 
 
void execute_command_line(char *tokens[MAX_TOKENS], int num_tokens) { 
    pid_t pid; 
    int status; 
 
    pid = fork(); 
    if (pid < 0) { 
        perror("fork"); 
        exit(EXIT_FAILURE); 
    } else if (pid == 0) { // child process 
        // Execute the command 
        execvp(tokens[0], tokens); 
        // If execvp returns, an error occurred 
        perror("execvp"); 
        exit(EXIT_FAILURE); 
    } else { // parent process 
        // Wait for child process to finish 
        waitpid(pid, &status, 0); 
    } 
} 
 
void execute_pipeline(char *commands[MAX_COMMANDS], int num_commands) { 
    int fds[num_commands - 1][2]; 
 
    // Create pipes 
    for (int i = 0; i < num_commands - 1; i++) { 
        if (pipe(fds[i]) == -1) { 
            perror("pipe"); 
            exit(EXIT_FAILURE); 
        } 
    } 
 
    for (int i = 0; i < num_commands; i++) { 
        char *tokens[MAX_TOKENS]; 
        int num_tokens = 0; 
        char *token = strtok(commands[i], " "); 
        while (token != NULL) { 
            tokens[num_tokens++] = token; 
            token = strtok(NULL, " "); 
        } 
        tokens[num_tokens] = NULL; // Null-terminate the array 
 
        pid_t pid = fork(); 
        if (pid < 0) { 
            perror("fork"); 
            exit(EXIT_FAILURE); 
        } else if (pid == 0) { // child process 
            // Connect pipes 
            if (i != 0) { 
                if (dup2(fds[i - 1][0], STDIN_FILENO) == -1) { 
                    perror("dup2"); 
                    exit(EXIT_FAILURE); 
                } 
            } 
            if (i != num_commands - 1) { 
                if (dup2(fds[i][1], STDOUT_FILENO) == -1) { 
                    perror("dup2"); 
                    exit(EXIT_FAILURE); 
                } 
            } 
 
            // Close all pipe file descriptors 
            for (int j = 0; j < num_commands - 1; j++) { 
                close(fds[j][0]); 
                close(fds[j][1]); 
            } 
 
            // Execute the command 
            execvp(tokens[0], tokens); 
            // If execvp returns, an error occurred 
            perror("execvp"); 
            exit(EXIT_FAILURE); 
        } else { // parent process 
            // Close unused pipe file descriptors 
            if (i != 0) { 
                close(fds[i - 1][0]); 
                close(fds[i - 1][1]); 
            } 
        } 
    } 
 
    // Close all pipe file descriptors 
    for (int i = 0; i < num_commands - 1; i++) { 
        close(fds[i][0]); 
        close(fds[i][1]); 
    } 
 
    // Wait for all child processes to finish 
    for (int i = 0; i < num_commands; i++) { 
        wait(NULL); 
    } 
} 
 
void execute_redirect(char *tokens[MAX_TOKENS], int num_tokens, char *filename, int append) { 
    int fd; 
    mode_t mode = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH; // rw-r--r-- 
    if (append) { 
        fd = open(filename, O_WRONLY | O_CREAT | O_APPEND, mode); 
    } else { 
        fd = open(filename, O_WRONLY | O_CREAT | O_TRUNC, mode); 
    } 
    if (fd == -1) { 
        perror("open"); 
        return; 
    } 
 
    pid_t pid = fork(); 
    if (pid < 0) { 
        perror("fork"); 
        close(fd); 
        return; 
    } else if (pid == 0) { // child process 
        // Redirect output to the file 
        if (dup2(fd, STDOUT_FILENO) == -1) { 
            perror("dup2"); 
            close(fd); 
            exit(EXIT_FAILURE); 
        } 
        close(fd); 
 
        // Execute the command 
        execvp(tokens[0], tokens); 
        // If execvp returns, an error occurred 
        perror("execvp"); 
        exit(EXIT_FAILURE); 
    } else { // parent process 
        close(fd); 
        // Wait for child process to finish 
        waitpid(pid, NULL, 0); 
    } 
} 
int
main(void)
{
	const size_t buf_size = 1024;
	char buf[buf_size];
	int rc;
	struct parser *p = parser_new();
	while ((rc = read(STDIN_FILENO, buf, buf_size)) > 0) {
		parser_feed(p, buf, rc);
		struct command_line *line = NULL;
		while (true) {
			enum parser_error err = parser_pop_next(p, &line);
			if (err == PARSER_ERR_NONE && line == NULL)
				break;
			if (err != PARSER_ERR_NONE) {
				printf("Error: %d\n", (int)err);
				continue;
			}
			execute_command_line(line);
			command_line_delete(line);
		}
	}
	parser_delete(p);
	return 0;
}
