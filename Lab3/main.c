#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <errno.h>

#define SHM_MAIN "/main_shared_memory"
#define SHM_ERR "/error_shared_memory"
#define SEM_WRITER "/semaphore_writer"
#define SEM_READER "/semaphore_reader"
#define MAX_BUFFER 1024

void display_error(const char *message) {
    write(STDERR_FILENO, message, strlen(message));
}

int main() {
    pid_t process_id;
    char file_name[MAX_BUFFER];
    char user_input[MAX_BUFFER];
    char *main_memory;
    char *err_memory;

    const char prompt[] = "Введите имя файла: ";
    write(STDOUT_FILENO, prompt, sizeof(prompt));
    int file_name_len = read(STDIN_FILENO, file_name, sizeof(file_name) - 1);
    if (file_name_len <= 0) {
        display_error("Ошибка ввода имени файла\n");
        exit(EXIT_FAILURE);
    }
    file_name[file_name_len - 1] = '\0';

    int main_shm_fd = shm_open(SHM_MAIN, O_CREAT | O_RDWR, 0666);
    int err_shm_fd = shm_open(SHM_ERR, O_CREAT | O_RDWR, 0666);
    if (main_shm_fd == -1 || err_shm_fd == -1) {
        display_error("Ошибка при создании shared memory\n");
        exit(EXIT_FAILURE);
    }
    ftruncate(main_shm_fd, MAX_BUFFER);
    ftruncate(err_shm_fd, MAX_BUFFER);
    main_memory = mmap(NULL, MAX_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, main_shm_fd, 0);
    err_memory = mmap(NULL, MAX_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, err_shm_fd, 0);
    if (main_memory == MAP_FAILED || err_memory == MAP_FAILED) {
        display_error("Ошибка при маппинге shared memory\n");
        exit(EXIT_FAILURE);
    }

    sem_t *writer_sem = sem_open(SEM_WRITER, O_CREAT, 0666, 0);
    sem_t *reader_sem = sem_open(SEM_READER, O_CREAT, 0666, 0);
    if (writer_sem == SEM_FAILED || reader_sem == SEM_FAILED) {
        display_error("Ошибка при создании семафоров\n");
        exit(EXIT_FAILURE);
    }

    process_id = fork();
    if (process_id < 0) {
        display_error("Ошибка при создании процесса\n");
        exit(EXIT_FAILURE);
    }

    if (process_id == 0) {
        execlp("./child", "./child", file_name, (char *)NULL);
        display_error("Ошибка при запуске дочернего процесса\n");
        exit(EXIT_FAILURE);
    }

    const char msg[] = "Введите строку (или 'exit' для выхода):\n";
    write(STDOUT_FILENO, msg, sizeof(msg));

    while (1) {
        int input_len = read(STDIN_FILENO, user_input, sizeof(user_input));
        if (input_len <= 0) {
            display_error("Ошибка ввода строки\n");
            break;
        }
        user_input[strcspn(user_input, "\n")] = 0;

        strncpy(main_memory, user_input, MAX_BUFFER);
        sem_post(writer_sem);

        if (strcmp(user_input, "exit") == 0) {
            break;
        }

        sem_wait(reader_sem);

        if (strlen(err_memory) > 0) {
            write(STDERR_FILENO, err_memory, strlen(err_memory));
            memset(err_memory, 0, MAX_BUFFER);
        }
    }

    wait(NULL);
    munmap(main_memory, MAX_BUFFER);
    munmap(err_memory, MAX_BUFFER);
    shm_unlink(SHM_MAIN);
    shm_unlink(SHM_ERR);
    sem_close(writer_sem);
    sem_close(reader_sem);
    sem_unlink(SEM_WRITER);
    sem_unlink(SEM_READER);

    return 0;
}
