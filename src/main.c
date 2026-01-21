#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>

#include "ast_debug.h"
#include "unit.h"
#include "unit_module_gen.h"
#include "lexer.h"
#include "ast_gen.h"
#include "chunk.h"
#include "io.h"
#include "ssa_gen.h"
#include "unit_debug.h"
#include "x86_64.h"

static double get_time_seconds() {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (double)ts.tv_sec + (double)ts.tv_nsec / 1000000000.0;
}

int main(int argc, char** argv) {
    double start = get_time_seconds();
    struct file** files = malloc(sizeof(struct file*) * (argc - 1));

    printf("compiling... ");
    
#pragma region lexing
    
    struct lexer** lexers = malloc(sizeof(struct lexer*) * (argc - 1));
    
    for (int i = 0; i < argc - 1; i++) {
        printf("%s ", argv[i + 1]);
        files[i] = file_read(argv[i + 1]);
        lexers[i] = lexer_new(files[i]->contents);
    }

#pragma endregion
    
#pragma region ast_gen
    
    printf("\nbuilding ast...\n\n");
    
    struct ast_module_list* modules = parse(lexers, argc - 1);

    if (modules == NULL) {
        goto cleanup;
    }

    // debugging
    
    printf("\n");
    
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        printf("--- MODULE %s ---\n", module->name);
        printf("\nSYMBOLS ---\n");
        ast_node_debug(stdout, module->symbols);
        printf("\nAST ---\n");
        ast_node_debug(stdout, module->root);
        printf("\n");
    }

    printf("\nfinished building ast...\n\n");
    
#pragma endregion

#pragma region ssa_gen
    
    printf("building unit...\n\n");
    
    for (int i = 0; i < modules->module_count; i++) {
        struct ast_module* module = modules->modules[i];
        struct unit_module* unit_module = unit_module_forward(module);

        unit_module_build(unit_module);

        char buffer[100];
        snprintf(buffer, sizeof(buffer), "%s.dot", module->name);
        
        FILE* cfgdot = fopen(buffer, "w");
        printf("--- MODULE %s ---\n", module->name);
        //unit_module_debug(unit_module);
        
        printf("--- COMPILED ---\n");
        unit_module_debug_graph(unit_module, cfgdot);

        struct backend x64 = x86_64_backend; // default for now
        x64.out = chunk_new();

        for (int n = 0; n < unit_module->unit_count; n++) {
            unit_compile(&x64, unit_module->units[n]);
        }
        
        FILE* out = fopen("out.asm", "w");
        debug_x86_64_bytecode(out, x64.out);
        fclose(out);

        unit_module_free(unit_module);
        
        fclose(cfgdot);
        
        char system_buffer[100];
        snprintf(system_buffer, sizeof(system_buffer), "dot -Tsvg %s.dot > %s.svg", module->name, module->name);
        system(system_buffer);
    }
    
#pragma endregion

    // ast cleanup
    ast_module_list_free(modules);

    cleanup:
    
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
