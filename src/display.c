#include <igraph/igraph.h>
#include "as_relationship.h"

void cga_print_info(igraph_t *graph) {
    printf("Vertices: %d\n", (int)igraph_vcount(graph));
    printf("Edges: %d\n", (int)igraph_ecount(graph));
    printf("Is directed? %s\n", igraph_is_directed(graph) ? "TRUE" : "FALSE");
}

void cga_print_vector_label(igraph_t *graph, igraph_vector_int_t *v, FILE *ostream) {
    for (long i = 0; i < igraph_vector_int_size(v); i++) {
        if(i == (igraph_vector_int_size(v) -1))
            fprintf(ostream,"%lu\n", (unsigned long) VAN(graph, "label", VECTOR(*v)[i]));
        else
            fprintf(ostream,"%lu ", (unsigned long) VAN(graph, "label", VECTOR(*v)[i]));
    }
}

void cga_print_degree_freedom_path(igraph_t *graph, igraph_integer_t vertex1_id, igraph_integer_t vertex2_id) {
    unsigned int vfree = 0, nvfree = 0;
    float res = cga_degree_freedom_path(graph, vertex1_id, vertex2_id, &vfree, &nvfree);
    printf("Grado di libertá: %.2f, vfree: %u, nvfree: %u\n", res, vfree, nvfree);
}

void cga_print_result_label(igraph_t *graph, igraph_vector_int_t *res, FILE *ostream) {
    igraph_vector_int_t v;
    igraph_vector_int_init(&v, 0);
    for(long i = 0; i < igraph_vector_int_size(res); i++) {
        
        if(VECTOR(*res)[i] != -1) {
            igraph_vector_int_push_back(&v, VECTOR(*res)[i]);
        }
        else { // path is complete, is valley free?
            cga_print_vector_label(graph, &v, ostream);
            igraph_vector_int_clear(&v);
        }
    }
    igraph_vector_int_destroy(&v);
}

void cga_print_result_label_vfree(igraph_t *graph, igraph_vector_int_t *res, FILE *ostream) {
    igraph_vector_int_t v;
    igraph_vector_int_init(&v, 0);
    for(long i = 0; i < igraph_vector_int_size(res); i++) {
        
        if(VECTOR(*res)[i] != -1) {
            igraph_vector_int_push_back(&v, VECTOR(*res)[i]);
        }
        else { // path is complete, is valley free?
            for (long j = 0; j < igraph_vector_int_size(&v); j++) {
                fprintf(ostream,"%lu ", (unsigned long) VAN(graph, "label", VECTOR(v)[j]));
            }
            fprintf(ostream,"- %s\n", cga_is_valley_free(graph, &v) ? "OK" : "NOPE");
            igraph_vector_int_clear(&v);
        }
    }
    igraph_vector_int_destroy(&v);
}

void cga_print_adj(igraph_t *graph) {
    igraph_lazy_adjlist_t adj;
    igraph_lazy_adjlist_init(graph, &adj, IGRAPH_ALL, 1);
    for(long i = 0; i < igraph_vcount(graph); i++) {
        printf("%lu -", (unsigned long) VAN(graph, "label", i));
        igraph_vector_t *v = igraph_lazy_adjlist_get(&adj, i);
        for (long j = 0; j < igraph_vector_size(v); j++) {
            printf(" %lu", (unsigned long) VAN(graph, "label", VECTOR(*v)[j]));
        }
        printf("\n");
    }
    igraph_lazy_adjlist_destroy(&adj);
}