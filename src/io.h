#ifndef COMPILER_IO_H
#define COMPILER_IO_H
#include <stdio.h>

struct file {
    FILE* file;
    size_t size;
    char* contents;
};

struct file* file_read(const char* filename);

void file_close(struct file* file);

#endif //COMPILER_IO_H
