#include <stdio.h> 
#include <stdlib.h>  
#include <unistd.h>  
#include <sys/wait.h>  
#include <errno.h>
#include <signal.h>

#define CHILD_PATH "./child"  
#define CHILD_ERR_FD 3   

static int write_all(int fd, const char *buf, size_t len) {
    size_t written = 0;
    while (written < len) {
        ssize_t n = write(fd, buf + written, len - written);
        if (n == -1) {
            if (errno == EINTR) {
                continue; 
            }
            if (errno == EPIPE) {
                fprintf(stderr, "error: child closed the pipe, cannot send data\n");
            } else {
                perror("write to pipe1");
            }
            return -1;
        }
        written += (size_t)n;
    }
    return 0;
}

int main(void)
{

    signal(SIGPIPE, SIG_IGN);

    char *file_name = NULL;
    size_t cap = 0;
    ssize_t s_len = getline(&file_name, &cap, stdin);

    if (s_len <=0) {
        fprintf(stderr, "error: reading input\n");
        return EXIT_FAILURE;
    }

    if (file_name[s_len - 1] == '\n') {
        file_name[s_len - 1] = '\0';
    }
    if (file_name[0] == '\0') {
        fprintf(stderr, "error: empty file name\n");
        free(file_name);
        return EXIT_FAILURE;
    }

    int pipe1[2];
    int pipe2[2];

    if (pipe(pipe1) == -1) {
        perror("pipe1");
        free(file_name);
        return EXIT_FAILURE;
    }

    if (pipe(pipe2) == -1) {
        perror("pipe2");
        free(file_name);
        close(pipe1[0]);
        close(pipe1[1]);
        return EXIT_FAILURE;
    }

    pid_t pid = fork();
    if (pid == -1) {
        perror("fork");
        free(file_name);
        close(pipe1[0]);
        close(pipe1[1]);
        close(pipe2[0]);
        close(pipe2[1]);
        return EXIT_FAILURE;
    }

    if (pid == 0) {
        //child process
        close(pipe1[1]); 
        close(pipe2[0]); 
        if (dup2(pipe1[0], STDIN_FILENO) == -1) {
            perror("child process, dup2");
            exit(EXIT_FAILURE);
        }
        close(pipe1[0]);

        if (dup2(pipe2[1], CHILD_ERR_FD) == -1) {
            perror("child process, dup2");
            exit(EXIT_FAILURE);
        }

        close(pipe2[1]); 

        char *argv[] = {"child", file_name, NULL};
        execv(CHILD_PATH, argv);
        perror("execv");
        exit(EXIT_FAILURE);

    } else {
        //parent process
        close(pipe1[0]); 
        close(pipe2[1]); 

        printf("enter string to send to child process:\n");

        char *line = NULL;
        size_t cap = 0;
        ssize_t s_len;

        while ((s_len = getline(&line, &cap, stdin)) != -1) {
            if (write_all(pipe1[1], line, (size_t)s_len) == -1) {
                break;                
            }
        }

        if (ferror(stdin)) {
            perror("getline");
        }

        free(line);
        close(pipe1[1]);
        
        char buffer[4096];
        ssize_t bytes_read;

        while (1) {
            bytes_read = read(pipe2[0], buffer, sizeof(buffer));
            
            if (bytes_read == -1) {
                if (errno == EINTR) {
                    continue;
                }
                perror("read from pipe2");
                break;
            }
            
            if (bytes_read == 0) {
                break;
            }
            
            if (write_all(STDOUT_FILENO, buffer, bytes_read) == -1) {
                break;
            }
        }

        close(pipe2[0]);

        int status;
        if (waitpid(pid, &status, 0) == -1) {
            perror("waitpid");
            free(file_name);
            return EXIT_FAILURE;
        }

        if (WIFEXITED(status) && WEXITSTATUS(status) == 0) {
            printf("child process exited successfully\n");
        } else if (WIFEXITED(status)){
            printf("child process exited with status %d\n", WEXITSTATUS(status));
        } else{
            printf("child process exited abnormally\n");
        }

        free(file_name);
        return EXIT_SUCCESS;
    }
}