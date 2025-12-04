#include "ir_gen.h"

#include <stdlib.h>

struct chunk_list* compile_module(struct module* node) {
    struct chunk_list* chunks = malloc(sizeof(struct chunk_list));

    for (int i = 0; i < node->root->children_count; i++) {
        
    }
    
    return chunks;
}
