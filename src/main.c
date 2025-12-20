#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ir.h"
#include "ir_gen.h"
#include "lexer.h"
#include "parser.h"

struct file {
    FILE* file;
    size_t size;
    char* contents;
};

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

#define DEBUG

static double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(int argc, char** argv) {
    double start = get_time_seconds();
    struct file** files = malloc(sizeof(struct file*) * (argc - 1));
    struct lexer** lexers = malloc(sizeof(struct lexer*) * (argc - 1));

#ifdef DEBUG
        printf("compiling... ");
#endif
    
    for (int i = 0; i < argc - 1; i++) {
#ifdef DEBUG
        printf("%s ", argv[i + 1]);
#endif
        files[i] = file_read(argv[i + 1]);
        lexers[i] = lexer_new(files[i]->contents);
    }

#ifdef DEBUG
        printf("\nbuilding ast...\n\n");
#endif
    
    struct ast_module_list* modules = parse(lexers, argc - 1);

    if (modules == NULL) {
        return 1;
    }

#ifdef DEBUG
    printf("\n");
#endif
    
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
#ifdef DEBUG
        printf("--- MODULE %s ---\n", module->name);
        printf("\nSYMBOLS ---\n");
        ast_node_debug(module->definitions);
        printf("\nAST ---\n");
        ast_node_debug(module->root);
        printf("\n");
#endif
    }

#ifdef DEBUG
    printf("\nfinished building ast...\n\n");

    printf("building ir...\n\n");

#endif
    
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        struct ir_module* ir_module = ir_gen_module(module);

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s.dot", module->name);
        
        FILE* cfgdot = fopen(buffer, "w");
#ifdef DEBUG
        printf("--- MODULE %s ---\n", module->name);
        //ir_module_debug(ir_module);
        
        printf("--- COMPILED ---\n");
#endif
        ir_module_debug_graph(ir_module, cfgdot);
        for (int n = 0; n < ir_module->count; n++) {
            ir_compile(ir_module->chunks[n], stdout);
        }
        ir_module_free(ir_module);
        
        fclose(cfgdot);
        
#ifdef DEBUG
        char system_buffer[100];
        snprintf(system_buffer, sizeof(system_buffer), "dot -Tsvg %s.dot > %s.svg", module->name, module->name);
        system(system_buffer);
#endif
    }

    
    ast_module_list_free(modules);

    //close files
    for (int i = 0; i < argc - 1; i++) {
        file_close(files[i]);
        lexer_free(lexers[i]);
    }
    free(files);
    free(lexers);

    double end = get_time_seconds();

    printf("Compile Time: %f", end - start);
    
    return 0;
}
