#include "misc.h"
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

// Преобразует целое число в строку с указанной минимальной длиной
int convert_integer_to_string(long number, char *result, int min_digits) {
    int is_negative = 0;
    char *start = result;

    // Обработка отрицательных чисел
    if (number < 0) {
        is_negative = 1;
        number = -number;
        *result++ = '-';
    }

    // Преобразуем число в строку в обратном порядке
    int index = 0;
    do {
        result[index++] = '0' + (number % 10);
        number /= 10;
    } while (number > 0);

    // Дополняем нулями, если нужно
    while (index < min_digits) {
        result[index++] = '0';
    }

    // Разворачиваем строку
    for (int i = 0; i < index / 2; i++) {
        char temp = result[i];
        result[i] = result[index - i - 1];
        result[index - i - 1] = temp;
    }

    // Завершаем строку нулевым символом
    result[index] = '\0';

    // Возвращаем длину строки (включая знак минуса, если есть)
    return index + is_negative;
}

// Ищет символ в строке с ограничением по длине
char *find_character_in_string(const char *buffer, char target, size_t length) {
    for (size_t i = 0; i < length; i++) {
        if (buffer[i] == target) {
            return (char *)(buffer + i);
        }
        if (buffer[i] == '\0') {
            break;
        }
    }
    return NULL;
}

// Читает строку из файлового дескриптора в буфер
int read_line_from_file(int file_descriptor, char **buffer, int *buffer_size) {
    int count = 0;
    int bytes_read;
    char *newline_pos;

    // Ищем конец текущих данных в буфере
    if (*buffer) {
        newline_pos = find_character_in_string(*buffer, '\n', *buffer_size);
        if (newline_pos) {
            count = newline_pos - *buffer + 1;
            memmove(*buffer, *buffer + count, *buffer_size - count);
        }
    }

    // Читаем данные из файлового дескриптора
    do {
        // Увеличиваем буфер, если нужно
        if (count + 1 >= *buffer_size) {
            int new_size = *buffer_size * 2;
            char *temp = realloc(*buffer, new_size);
            if (!temp) {
                return count;
            }
            *buffer = temp;
            *buffer_size = new_size;
        }

        // Читаем данные
        bytes_read = read(file_descriptor, *buffer + count, *buffer_size - count - 1);
        if (bytes_read < 0) {
            return -1; // Ошибка чтения
        }

        count += bytes_read;
        (*buffer)[count] = '\0';

        // Проверяем, есть ли символ новой строки
        newline_pos = find_character_in_string(*buffer, '\n', count);
    } while (bytes_read > 0 && !newline_pos);

    // Если нашли символ новой строки, обрезаем строку
    if (newline_pos) {
        count = newline_pos - *buffer;
        (*buffer)[count] = '\0';
        return count + 1;
    }

    return count;
}

// Выводит строку в файловый дескриптор
int write_string_to_file(int file_descriptor, const char *string) {
    int length = strlen(string);
    return write(file_descriptor, string, length);
}