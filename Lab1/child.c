#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>

#include "misc.h"

int main(void) {
    int buffer_size = 64;
    char *input_buffer = malloc(buffer_size);
    char output_buffer[64];

    if (!input_buffer) {
        perror("Failed to allocate memory");
        return EXIT_FAILURE;
    }

    while (1) {
        int bytes_read = read_line_from_file(STDIN_FILENO, &input_buffer, &buffer_size);
        if (bytes_read <= 0) {
            break;
        }

        int sum = 0;
        char *token = strtok(input_buffer, " ");
        while (token) {
            sum += atoi(token);
            token = strtok(NULL, " ");
        }

        int length = convert_integer_to_string(sum, output_buffer, 1);
        output_buffer[length++] = '\n';
        write_string_to_file(STDOUT_FILENO, output_buffer);
    }

    free(input_buffer);
    return EXIT_SUCCESS;
}
