#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>
#include <wait.h>

int print_int(const int num) {
    char buf[16];
    char res[32];
    int n = 0;
    int sign = (num < 0);
    int x = abs(num);
    if (x == 0) {
        write(STDOUT_FILENO, "0\n", 2);
        return 0;
    }
    while (x) {
        buf[n] = (x % 10) + '0';
        x = x / 10;
        n++;
    }
    if (sign) {
        res[0] = '-';
    }
    for (int i = 0; i < n; i++) {
        res[i + sign] = buf[n - i - 1];
    }
    res[n + sign] = '\n';
    write(STDOUT_FILENO, res, (n + sign + 1));
    return 0;
}

int read_line(char** buf, int* n){
    char c;
    int i = 0;
    while(1) {
        if (read(STDIN_FILENO, &c, sizeof(char)) == -1) {
            return -1;
        }
        (*buf)[i] = c;
        i++;
        if (i >= *n) {
            char* new_buf = realloc(*buf, (*n) * 2 * sizeof(char));
            if (!new_buf) {
                return -1;
            }
            *buf = new_buf;
            *n = *n * 2;
        }
        if (c == '\n') {
            (*buf)[i - 1] = '\0';
            break;
        }
    }
    return 0;
}

int main() {
    char* buf = malloc(128 * sizeof(char));
    if (!buf) {
        char* msg = "fail to allocate memory\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        exit(-1);
    }
    int n = 128;
    if (read_line(&buf, &n) == -1) {
        char* msg = "fail to read file name\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        free(buf);
        exit(-1);
    }

    int file_fd = open(buf, O_RDONLY);
    if (file_fd == -1) {
        char* msg = "fail to open file\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        free(buf);
        exit(-1);
    }

    int pipe_fd[2];
    if (pipe(pipe_fd) == -1) {
        char* msg = "fail to create pipe\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        free(buf);
        close(file_fd);
        exit(-1);
    }

    __pid_t pid = fork();
    if (pid == -1) {
        char* msg = "fail to fork\n";
        write(STDOUT_FILENO, msg, strlen(msg));
        free(buf);
        close(file_fd);
        close(pipe_fd[0]);
        close(pipe_fd[1]);
        exit(-1);
    }

    if (pid == 0) {
        close(pipe_fd[0]);
        if (dup2(file_fd, STDIN_FILENO) == -1) {
            char* msg = "fail to reassign file descriptor\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            free(buf);
            close(file_fd);
            close(pipe_fd[1]);
            exit(-1);
        }
        close(file_fd);
        if (dup2(pipe_fd[1], STDOUT_FILENO) == -1) {
            char* msg = "fail to reassign file descriptor\n";
            write(STDOUT_FILENO, msg, strlen(msg));
            free(buf);
            close(pipe_fd[1]);
            exit(-1);
        }
        close(pipe_fd[1]);
        char *arg[] = {"./child.out", NULL};
        if (execv("./child.out", arg) == -1) {
            char* msg = "fail to replace process image\n";
            write(STDOUT_FILENO, msg,
