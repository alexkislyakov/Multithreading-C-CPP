#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/wait.h>

#define TOK_BUFSIZE 64
#define TOK_DELIM " \t\r\n\a\f\v"
#define COM_DELIM "|"

char **parse_arg(char *src) {
    int position = 0;
    char **args = (char**)malloc(sizeof(char) * TOK_BUFSIZE);
    
    char *arg = strtok(src, TOK_DELIM);

    while (arg != NULL) {
	args[position] = arg;
	++position;
	arg = strtok(NULL, TOK_DELIM);
    }

    args[position] = NULL;
    return args;
}

int parse_commands(char *src, char **commands) {
    int position = 0;
    char *token = strtok(src, COM_DELIM);

    while (token != NULL) {
	commands[position] = token;
	++position;
	token = strtok(NULL, COM_DELIM);
    }

    return position;
}

//who | sort | uniq -c | sort -nk1
//x --- (stdout) ---> [pfd[1] === pfd[0]] ---> (stdin)---> y
void cmd_launch(char *src) {
    char **commands = (char**)malloc(sizeof(char) * TOK_BUFSIZE);
    int nCmd = parse_commands(src, commands);

    int in = -1;
    int exit_code = 0;
    for (int i = nCmd - 1; i >= 0; --i) {
	 int pfd[2];
         pipe(pfd);
	 int pid = fork();

	 if (!pid) {
	     if (i == 0 && i != nCmd - 1) {
	        close(STDOUT_FILENO);
	    
	        dup2(in, STDOUT_FILENO);

		close(in);//
	    
	        char **args = parse_arg(commands[i]);
	        execvp(args[0], args);
	    
	     } else if(i > 0 && i < nCmd - 1) {
	        close(STDOUT_FILENO);
	        dup2(in, STDOUT_FILENO);
	    
	        close(STDIN_FILENO);
	        dup2(pfd[0], STDIN_FILENO);

	        close(pfd[1]);
		close(pfd[0]);//
		close(in); //
	    
	        char **args = parse_arg(commands[i]);
	        execvp(args[0], args);
	    
	     } else if (i == nCmd - 1) {
	        close(STDIN_FILENO);
	        close(STDOUT_FILENO);
	    
	        dup2(pfd[0], STDIN_FILENO);
	        int fd = open("result.txt", O_RDWR | O_CREAT | O_TRUNC, 0666);
                dup2(fd, STDOUT_FILENO);

	        close(pfd[1]);
		close(pfd[0]); //
	    
	        char **args = parse_arg(commands[i]);
	        execvp(args[0], args);
	     }
	 }
	 
	 in = pfd[1];
    }
    
    waitpid(-1, &exit_code, 0);
    return;
}

int main(int argc, char *argv[argc])
{

    char buffer[1024];
    read(STDIN_FILENO, buffer, 1024);
    cmd_launch(buffer);
    
    return 0;
}
