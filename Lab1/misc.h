#ifndef __MISC_H__
#define __MISC_H__

#include <stddef.h>

// Преобразует целое число в строку с указанной минимальной длиной
int convert_integer_to_string(long number, char *result, int min_digits);

// Ищет символ в строке с ограничением по длине
char *find_character_in_string(const char *buffer, char target, size_t length);

// Читает строку из файлового дескриптора в буфер
int read_line_from_file(int file_descriptor, char **buffer, int *buffer_size);

// Выводит строку в файловый дескриптор
int write_string_to_file(int file_descriptor, const char *string);

#endif