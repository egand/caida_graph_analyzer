#include <igraph/igraph.h>
#include <stdio.h>
#include <stdlib.h>
#include "cga.h"

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "%s", "Usage in cli: <filename> <caida_snapshot>\n");
        exit(EXIT_FAILURE);
    }

    cga_hashtable_t *ht = cga_ht_init(3000);

    igraph_t igraph;
    igraph_i_set_attribute_table(&igraph_cattribute_table);

    FILE *fp = fopen("hashtable.txt", "r");
    if (fp != NULL) {
        cga_ht_load_from_file(ht, fp);
        printf("Nelems: %lu\n", cga_ht_nelems(ht));
        fclose(fp);
    }

    fp = fopen(argv[1], "r");
    if (fp == NULL) {
        perror("fopen caida file");
        exit(EXIT_FAILURE);
    }
    cga_load_snapshot(&igraph, ht, fp);
    fclose(fp);

    fp = fopen("hashtable.txt", "w+");
    cga_ht_save_to_file(ht, fp);
    fclose(fp);
    igraph_vector_int_t res;
    igraph_vector_int_init(&res, 0);

    cga_graph_analysis(&igraph, 1, "./output/test/test");
    printf("Finish!\n");
    igraph_vector_int_destroy(&res);
    igraph_destroy(&igraph);
    cga_ht_destroy(ht);

    return 0;
}
