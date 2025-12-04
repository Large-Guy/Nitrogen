#include "ast.h"

#include <assert.h>
#include <stdlib.h>


struct ast_node* ast_node_new(enum ast_node_type type, struct token token) {
    struct ast_node* node = malloc(sizeof(struct ast_node));
    assert(node);
    node->type = type;
    node->token = token;
    node->children_count = 0;
    node->children_capacity = 1;
    node->children = malloc(sizeof(struct ast_node*) * node->children_capacity);
    return node;
}

void ast_node_free(struct ast_node* node) {
    for (size_t i = 0; i < node->children_count; i++) {
        ast_node_free(node->children[i]);
    }
    free(node->children);
    free(node);
}

void ast_node_append_child(struct ast_node* node, struct ast_node* child) {
    if (node->children_count >= node->children_capacity) {
        node->children_capacity *= 2;
        node->children = realloc(node->children, node->children_capacity * sizeof(struct ast_node*));
        assert(node->children);
    }
    node->children[node->children_count++] = child;
}

static void debug(struct ast_node* node, int32_t depth) {
    // Generate a color based on the type number
    const char* colors[] = {
        "\033[31m", // red
        "\033[32m", // green
        "\033[33m", // yellow
        "\033[34m", // blue
        "\033[35m", // magenta
        "\033[36m", // cyan
        "\033[37m", // white
    };
    
    for (int i = 0; i < depth; i++) {
        printf("%s| ", colors[i % (sizeof(colors) / sizeof(colors[0]))]);
    }
    
    int color_index = node->type % (sizeof(colors)/sizeof(colors[0]));

    // Print node with type-colored bracket
    printf("%snode\033[0m: [%d] %s %.*s\n",
           "\033[1;37m",           // "node" in bright white
           node->type,
           colors[color_index],    // color based on type
           (int32_t)node->token.length,
           node->token.start);

    for (int i = 0; i < node->children_count; i++) {
        debug(node->children[i], depth + 1);
    }
}

void ast_node_debug(struct ast_node* node) {
    debug(node, 0);
}
