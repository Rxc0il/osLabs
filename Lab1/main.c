#include <errno.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <stdio.h>

#include "misc.h"

int main(int argc, char *argv[]) {
    // Определяем имя программы для запуска (по умолчанию "./child.out")
    char *child_program = (argc >= 2) ? argv[1] : "./child.out";

    // Создаем pipe для обмена данными между процессами
    int pipefd[2];
    if (pipe(pipefd) == -1) {
        write_string_to_file(STDERR_FILENO, "ERROR: failed to create pipe\n");
        exit(EXIT_FAILURE);
    }

    // Чтение имени файла из стандартного ввода
    int buffer_size = 64;
    char *filename = malloc(buffer_size);
    if (!filename) {
        write_string_to_file(STDERR_FILENO, "ERROR: failed to allocate memory\n");
        exit(EXIT_FAILURE);
    }

    // Чтение строки с именем файла
    if (read_line_from_file(STDIN_FILENO, &filename, &buffer_size) <= 0) {
        write_string_to_file(STDERR_FILENO, "ERROR: failed to read filename from standard input\n");
        free(filename);
        exit(EXIT_FAILURE);
    }

    // Открываем файл для чтения
    int file_descriptor = open(filename, O_RDONLY);
    if (file_descriptor == -1) {
        write_string_to_file(STDERR_FILENO, "ERROR: failed to open file: \"");
        write_string_to_file(STDERR_FILENO, filename);
        write_string_to_file(STDERR_FILENO, "\"\n");
        write_string_to_file(STDERR_FILENO, strerror(errno));
        write_string_to_file(STDERR_FILENO, "\n");
        free(filename);
        exit(EXIT_FAILURE);
    }
    free(filename); // Освобождаем память, так как имя файла больше не нужно

    // Создаем дочерний процесс
    pid_t pid = fork();
    if (pid == 0) {
        // Дочерний процесс
        dup2(file_descriptor, STDIN_FILENO); // Перенаправляем ввод из файла
        close(file_descriptor); // Закрываем оригинальный файловый дескриптор

        close(pipefd[0]); // Закрываем чтение из pipe
        dup2(pipefd[1], STDOUT_FILENO); // Перенаправляем вывод в pipe
        dup2(pipefd[1], STDERR_FILENO); // Перенаправляем ошибки в pipe
        close(pipefd[1]); // Закрываем оригинальный pipe

        // Запускаем дочернюю программу
        char *child_argv[] = {child_program, NULL};
        if (execv(child_program, child_argv) == -1) {
            write_string_to_file(STDERR_FILENO, "ERROR: failed to launch process \"");
            write_string_to_file(STDERR_FILENO, child_program);
            write_string_to_file(STDERR_FILENO, "\"\n");
            write_string_to_file(STDERR_FILENO, strerror(errno));
            write_string_to_file(STDERR_FILENO, "\n");
            exit(EXIT_FAILURE);
        }
    } else if (pid == -1) {
        // Ошибка при создании дочернего процесса
        write_string_to_file(STDERR_FILENO, "ERROR: failed to fork process\n");
        write_string_to_file(STDERR_FILENO, strerror(errno));
        write_string_to_file(STDERR_FILENO, "\n");
        exit(EXIT_FAILURE);
    } else {
        // Родительский процесс
        close(pipefd[1]); // Закрываем запись в pipe

        // Чтение данных из pipe и вывод в стандартный вывод
        char buffer[128];
        while (1) {
            int bytes_read = read(pipefd[0], buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                break; // Завершаем цикл, если данных больше нет
            }
            write(STDOUT_FILENO, buffer, bytes_read);
        }

        // Ожидание завершения дочернего процесса
        int status;
        waitpid(pid, &status, 0);

        close(pipefd[0]); // Закрываем чтение из pipe
    }

    return EXIT_SUCCESS;
}