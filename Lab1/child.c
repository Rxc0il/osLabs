#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <fcntl.h>

int is_num(char c) {
    return (c >= '0') && (c <= '9');
}

int read_line_of_int(int* res) {
    char c = 0;
    int r;
    int buf = 0;
    int sign = 1;

    while (1) {
        r = read(STDIN_FILENO, &c, sizeof(char));
        if (r < 1) {
            return r;
        }
        if (c == '\n') {
            break;
        }
        if (c == '-') {
            sign = -1;
        } else if (is_num(c)) {
            buf = buf * 10 + (c - '0');
        } else if (c != ' ') {
            return -1;
        }
    }

    *res = buf * sign;
    return 1;
}

int main() {
    int res = 0;
    while (read_line_of_int(&res) > 0) {
        write(STDOUT_FILENO, &res, sizeof(int));
    }
    close(STDOUT_FILENO);
    return 0;
}
