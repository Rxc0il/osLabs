#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <string.h>
#include <semaphore.h>
#include <sys/mman.h>

#define SHM_MAIN "/main_shared_memory"
#define SHM_ERR "/error_shared_memory"
#define SEM_WRITER "/semaphore_writer"
#define SEM_READER "/semaphore_reader"
#define MAX_BUFFER 1024

void log_error(const char *error_storage, const char *message) {
    strncpy((char *)error_storage, message, MAX_BUFFER);
}

int has_valid_ending(const char *text) {
    int length = strlen(text);
    return (length > 0 && (text[length - 1] == '.' || text[length - 1] == ';'));
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        write(STDERR_FILENO, "Файл не указан\n", 18);
        return -1;
    }

    int main_shm_fd = shm_open(SHM_MAIN, O_RDWR, 0666);
    int err_shm_fd = shm_open(SHM_ERR, O_RDWR, 0666);
    if (main_shm_fd == -1 || err_shm_fd == -1) {
        write(STDERR_FILENO, "Ошибка при доступе к shared memory\n", 37);
        return -1;
    }

    char *main_memory = mmap(NULL, MAX_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, main_shm_fd, 0);
    char *err_memory = mmap(NULL, MAX_BUFFER, PROT_READ | PROT_WRITE, MAP_SHARED, err_shm_fd, 0);
    if (main_memory == MAP_FAILED || err_memory == MAP_FAILED) {
        write(STDERR_FILENO, "Ошибка при маппинге shared memory\n", 36);
        return -1;
    }

    sem_t *writer_sem = sem_open(SEM_WRITER, 0);
    sem_t *reader_sem = sem_open(SEM_READER, 0);
    if (writer_sem == SEM_FAILED || reader_sem == SEM_FAILED) {
        write(STDERR_FILENO, "Ошибка при открытии семафоров\n", 31);
        return -1;
    }

    int file_desc = open(argv[1], O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (file_desc < 0) {
        log_error(err_memory, "Ошибка при открытии файла\n");
        sem_post(reader_sem);
        return -1;
    }

    while (1) {
        sem_wait(writer_sem);

        char buffer[MAX_BUFFER];
        strncpy(buffer, main_memory, MAX_BUFFER);

        if (strcmp(buffer, "exit") == 0) {
            break;
        }

        if (has_valid_ending(buffer)) {
            write(file_desc, buffer, strlen(buffer));
            write(file_desc, "\n", 1);
        } else {
            log_error(err_memory, "Ошибка: строка не соответствует правилу!\n");
        }

        sem_post(reader_sem);
    }

    close(file_desc);
    munmap(main_memory, MAX_BUFFER);
    munmap(err_memory, MAX_BUFFER);
    sem_close(writer_sem);
    sem_close(reader_sem);

    return 0;
}
