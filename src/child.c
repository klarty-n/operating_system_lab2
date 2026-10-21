#include <stdio.h>      
#include <stdlib.h>     
#include <unistd.h>    
#include <sys/wait.h>

#define CHILD_ERR_FD 3

int starts_with_upper(const char *str) {
    if (str == NULL || str[0] == '\0') {
        return 0; 
    }
    return (str[0] >= 'A' && str[0] <= 'Z');
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        dprintf(CHILD_ERR_FD, "Filename was not provided\n");
        return EXIT_FAILURE;
    }

    const char *file_name = argv[1];
    FILE *file = fopen(file_name, "w");
    if (!file) {
        dprintf(CHILD_ERR_FD, "Failed to open file: %s\n", file_name);
        return EXIT_FAILURE;
    }

    int file_descriptor = fileno(file);
    if (file_descriptor == -1) {
        perror("fileno");
        fclose(file);
        return EXIT_FAILURE;
    }

    if (dup2(file_descriptor, STDOUT_FILENO) == -1) {
        perror("dup2");
        fclose(file);
        return EXIT_FAILURE;
    }
    close(file_descriptor);

    char *line = NULL;
    size_t cap = 0;
    ssize_t s_len;
    size_t s_read = 0;

    while ((s_len = getline(&line, &cap, stdin)) != -1) {
        s_read ++;
        if (s_len > 0 && line[s_len - 1] == '\n') {
            line[s_len - 1] = '\0';
        }

        if (starts_with_upper(line)) {
            printf("%s\n", line);
        } else {
            dprintf(CHILD_ERR_FD, "Line %zu does not start with an uppercase letter: %s\n", s_read, line);
        }
    }
    

    free(line);
    fclose(stdout);
    exit(EXIT_SUCCESS);
}