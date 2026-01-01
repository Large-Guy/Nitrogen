#include "io.h"

#include <stdlib.h>

struct file* file_read(const char* filename) {
    struct file* file = malloc(sizeof(struct file));
    file->file = fopen(filename, "r");
    if (file->file == NULL) {
        fprintf(stderr, "Failed to open file %s\n", filename);
        exit(EXIT_FAILURE);
    }

    fseek(file->file, 0L, SEEK_END);
    file->size = ftell(file->file);

    rewind(file->file);
    file->contents = malloc(file->size + 1);
    if (file->contents == NULL) {
        fprintf(stderr, "Failed to allocate memory for file contents\n");
        exit(EXIT_FAILURE);
    }

    fread(file->contents, sizeof(char), file->size, file->file);
    file->contents[file->size] = '\0';

    return file;
}

void file_close(struct file* file) {
    if (file->contents != NULL) {
        free(file->contents);
    }
    fclose(file->file);
    free(file);
}
