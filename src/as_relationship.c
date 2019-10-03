#include "as_relationship.h"
#include <fcntl.h>
#include <igraph/igraph.h>
#include <limits.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "hashset.h"
#include "hashtable.h"

typedef struct _as_rel {
    unsigned long as1;
    unsigned long as2;
    igraph_integer_t as1_id;
    igraph_integer_t as2_id;
    int relation;
} as_rel_t;

struct tinfo {
    pthread_t t_id;
    char *filename;
    igraph_t *graph;
    igraph_integer_t vertex;
    igraph_integer_t lowerbound;
    igraph_integer_t upperbound;
};

static void cga_dfs_vfree_rec_helper(igraph_t *graph, igraph_integer_t target, igraph_lazy_adjlist_t *adjlist, igraph_vector_int_t *curr_path, cga_hashset_t *used_nodes, igraph_vector_int_t *res);
static int read_line(char **buf, int *size, FILE *file);
static void get_as_rel_parts(char *str, as_rel_t *as_rel);
static igraph_integer_t add_annotated_vertex(igraph_t *graph, cga_hashtable_t *ht, unsigned long as_num, igraph_vector_t *vertex_attr);
static void add_annotated_edges_vect(igraph_vector_t *edges, igraph_vector_t *edges_attr, unsigned int as1_id, unsigned int as2_id, int relation);
static void *cga_as_analysis_job(void *attr);
static void *cga_graph_analysis_job(void *attr);

void cga_load_snapshot(igraph_t *graph, cga_hashtable_t *ht, FILE *instream) {
    if ((fcntl(fileno(instream), F_GETFL) & O_ACCMODE) == O_WRONLY) {
        fprintf(stderr, "cga_load_snapshot permission denied. Have you opened the file in write mode?\n");
        abort();
    }
    int size = 250;
    char *buf = malloc(size);
    as_rel_t as_rel;
    igraph_vector_t edges, edges_attr, vertex_attr;
    igraph_vector_init(&edges, 0);
    igraph_vector_init(&edges_attr, 0);
    igraph_vector_init(&vertex_attr, 0);
    igraph_empty(graph, 0, IGRAPH_DIRECTED);

    while (read_line(&buf, &size, instream) != EOF) {
        if (buf[0] == '#') continue;     // get rid of comments
        buf[strcspn(buf, "\n")] = '\0';  // delete the newline
        get_as_rel_parts(buf, &as_rel);
        as_rel.as1_id = add_annotated_vertex(graph, ht, as_rel.as1, &vertex_attr);
        as_rel.as2_id = add_annotated_vertex(graph, ht, as_rel.as2, &vertex_attr);
        add_annotated_edges_vect(&edges, &edges_attr, as_rel.as1_id, as_rel.as2_id, as_rel.relation);
        if (as_rel.relation == 0)  // if is a p2p relation, also add an inverse direct edge
            add_annotated_edges_vect(&edges, &edges_attr, as_rel.as2_id, as_rel.as1_id, as_rel.relation);
    }
    igraph_add_vertices(graph, cga_ht_nelems(ht), 0);
    igraph_add_edges(graph, &edges, 0);
    for (long i = 0; i < igraph_vector_size(&edges_attr); i++) {
        SETEAN(graph, "type", i, VECTOR(edges_attr)[i]);
    }
    for (long i = 0; i < igraph_vector_size(&vertex_attr); i = i + 2) {
        SETVAN(graph, "label", VECTOR(vertex_attr)[i + 1], VECTOR(vertex_attr)[i]);
    }
    igraph_vector_destroy(&vertex_attr);
    igraph_vector_destroy(&edges);
    igraph_vector_destroy(&edges_attr);
    free(buf);
}

int cga_is_valley_free(igraph_t *graph, igraph_vector_int_t *path) {
    int state = 0, type_val;
    igraph_integer_t eid;
    long i = 0;
    while (state >= 0 && i < igraph_vector_int_size(path) - 1) {
        igraph_get_eid(graph, &eid, VECTOR(*path)[i], VECTOR(*path)[i + 1], igraph_is_directed(graph), 0);
        if (eid == -1)
            type_val = 1;  // customer to provider edge
        else
            type_val = (int)EAN(graph, "type", eid);
        i++;
        switch (state) {
            case 0:
                if (type_val == -1)
                    state = 1;
                else if (type_val == 0)
                    state = 2;
                break;
            case 1:
                if (type_val == 1 || type_val == 0) state = -1;
                break;
            case 2:
                if (type_val == -1)
                    state = 1;
                else
                    state = -1;
                break;
        }
    }
    return state >= 0 ? 1 : 0;
}

int cga_valley_free_state(igraph_t *graph, int current_state, igraph_integer_t old_node, igraph_integer_t new_node) {
    igraph_integer_t eid;
    igraph_get_eid(graph, &eid, old_node, new_node, igraph_is_directed(graph), 0);
    int arc_type;
    if (eid == -1)
        arc_type = 1;  // customer to provider edge
    else
        arc_type = (int)EAN(graph, "type", eid);
    switch (current_state) {
        case 0:
            if (arc_type == -1 || arc_type == 0)
                current_state = 1;
            break;
        case 1:
            if (arc_type == 1 || arc_type == 0)
                current_state = -1;
            break;
    }
    return current_state;
}

void cga_dfs_vfree_rec(igraph_t *graph, igraph_vector_int_t *res, igraph_integer_t from, igraph_integer_t to) {
    cga_hashset_t *hs = cga_hs_init(40);
    igraph_vector_int_t curr_path;
    igraph_lazy_adjlist_t adjlist;
    igraph_lazy_adjlist_init(graph, &adjlist, IGRAPH_ALL, 1);
    igraph_vector_int_init(&curr_path, 0);
    igraph_vector_int_push_back(&curr_path, from);
    cga_hs_insert(hs, from);
    cga_dfs_vfree_rec_helper(graph, to, &adjlist, &curr_path, hs, res);
    cga_hs_destroy(hs);
    igraph_lazy_adjlist_destroy(&adjlist);
    igraph_vector_int_destroy(&curr_path);
}

/**
 * Helper function for get_all_vfree_paths.
 * It uses a DFS search to search all possible paths between 2 nodes, and retrieve only valley free paths
 */
static void cga_dfs_vfree_rec_helper(igraph_t *graph, igraph_integer_t target, igraph_lazy_adjlist_t *adjlist, igraph_vector_int_t *curr_path, cga_hashset_t *used_nodes, igraph_vector_int_t *res) {
    igraph_integer_t last_node = igraph_vector_int_tail(curr_path);
    if (!cga_is_valley_free(graph, curr_path)) return;
    if (last_node == target) {
        igraph_vector_int_append(res, curr_path);
        igraph_vector_int_push_back(res, -1);
    } else {
        igraph_vector_t *neighbors = igraph_lazy_adjlist_get(adjlist, last_node);
        for (long i = 0; i < igraph_vector_size(neighbors); i++) {
            if (!cga_hs_contains(used_nodes, (igraph_integer_t)VECTOR(*neighbors)[i])) {
                igraph_vector_int_push_back(curr_path, (igraph_integer_t)VECTOR(*neighbors)[i]);
                cga_hs_insert(used_nodes, (igraph_integer_t)VECTOR(*neighbors)[i]);
                cga_dfs_vfree_rec_helper(graph, target, adjlist, curr_path, used_nodes, res);
                cga_hs_delete(used_nodes, (igraph_integer_t)VECTOR(*neighbors)[i]);
                igraph_vector_int_pop_back(curr_path);
            }
        }
    }
}

void cga_dfs_vfree_it(igraph_t *graph, igraph_vector_int_t *res, igraph_integer_t from, igraph_integer_t to) {
    cga_hashset_t *used_nodes = cga_hs_init(40);
    igraph_lazy_adjlist_t adjlist;
    igraph_vector_int_t curr_path;
    igraph_stack_int_t stack;
    igraph_vector_int_t dfa_state;
    igraph_lazy_adjlist_init(graph, &adjlist, IGRAPH_ALL, 1);
    igraph_stack_int_init(&stack, 0);
    igraph_vector_int_init(&curr_path, 0);
    igraph_vector_int_init(&dfa_state, 0);

    // init stack with from and his neighbors
    igraph_stack_int_push(&stack, from);
    igraph_vector_int_push_back(&curr_path, from);
    igraph_vector_int_push_back(&dfa_state, 0);  // dfa state starts from 0
    cga_hs_insert(used_nodes, from);
    igraph_vector_t *initial_neighbors = igraph_lazy_adjlist_get(&adjlist, from);
    for (long i = 0; i < igraph_vector_size(initial_neighbors); i++) {
        igraph_stack_int_push(&stack, VECTOR(*initial_neighbors)[i]);
    }
    while (!igraph_stack_int_empty(&stack)) {
        igraph_integer_t curr_node = igraph_stack_int_top(&stack);
        if (curr_node == igraph_vector_int_tail(&curr_path)) {  // his neighbors are already explored, delete the node from the stack
            igraph_stack_int_pop(&stack);
            igraph_vector_int_pop_back(&curr_path);
            igraph_vector_int_pop_back(&dfa_state);
            cga_hs_delete(used_nodes, curr_node);
            continue;
        }
        int state = cga_valley_free_state(graph, igraph_vector_int_tail(&dfa_state), igraph_vector_int_tail(&curr_path), curr_node);
        if (state == -1) {  // not valley free
            igraph_stack_int_pop(&stack);
            continue;
        } else {
            igraph_vector_int_push_back(&curr_path, curr_node);
            igraph_vector_int_push_back(&dfa_state, state);
        }

        if (curr_node == to) {  // found a solution
            igraph_vector_int_append(res, &curr_path);
            igraph_vector_int_push_back(res, -1);
            igraph_stack_int_pop(&stack);
            igraph_vector_int_pop_back(&curr_path);
            igraph_vector_int_pop_back(&dfa_state);
            continue;
        }
        cga_hs_insert(used_nodes, curr_node);
        igraph_vector_t *neighbors = igraph_lazy_adjlist_get(&adjlist, curr_node);
        for (long i = 0; i < igraph_vector_size(neighbors); i++) {
            if (!cga_hs_contains(used_nodes, (igraph_integer_t)VECTOR(*neighbors)[i])) {
                igraph_stack_int_push(&stack, (igraph_integer_t)VECTOR(*neighbors)[i]);
            }
        }
    }
    igraph_stack_int_destroy(&stack);
    cga_hs_destroy(used_nodes);
    igraph_lazy_adjlist_destroy(&adjlist);
    igraph_vector_int_destroy(&curr_path);
    igraph_vector_int_destroy(&dfa_state);
}

int cga_path_cost(igraph_t *graph, igraph_vector_int_t *path) {
    int total = 0;
    igraph_integer_t eid;
    for (long i = 0; i < igraph_vector_int_size(path) - 1; i++) {
        igraph_get_eid(graph, &eid, VECTOR(*path)[i], VECTOR(*path)[i + 1], igraph_is_directed(graph), 0);
        if (eid == -1)
            total += 1;  // customer to provider edge
        else
            total += (int)EAN(graph, "type", eid);
    }
    return total;
}

/**
 * This function uses a dynamic buffer to read an entire line (\n included).
 * The content of the line is stored in the buf variable. 
 * The size of the buffer is changed iff the line length is greater than the size of the buffer. (the value is doubled every time this happens)
 * 
 * Arguments:
 * buf: pointer to string. After the reading operation the line is stored here.
 * size: pointer to integer that represent the size of the buffer. 
 *      This value change iff the line length is greater than the size of the buffer (the value is doubled every time this happens).
 * file: pointer to a stream. It should be readable.
 */
static int read_line(char **buf, int *size, FILE *file) {
    int old_size;
    char *offset;
    if (fgets(*buf, *size, file) == NULL)
        return EOF;

    if (strchr(*buf, '\n') != NULL)
        return strlen(*buf);

    do {
        old_size = *size;
        *size *= 2;
        char *temp = realloc(*buf, *size);
        if (temp == NULL) {
            free(*buf);
            fprintf(stderr, "%s", "Out of memory while allocating buffer for reading. Aborting process...");
            fclose(file);
            abort();
        }
        *buf = temp;
        offset = &((*buf)[old_size - 1]);
    } while (fgets(offset, old_size + 1, file) && strchr(offset, '\n') == NULL);

    return strlen(*buf);
}

/**
 * This function fetch as_num1, as_num2 and the relationship from the string str, and store their values in as_rel
 * 
 * Arguments:
 * str: string to fetch
 * as_rel: pointer to a struct defined internally
 */
static void get_as_rel_parts(char *str, as_rel_t *as_rel) {
    char *str_part;
    str_part = strtok(str, "|");
    as_rel->as1 = (unsigned int)strtoul(str_part, NULL, 10);
    str_part = strtok(NULL, "|");
    as_rel->as2 = (unsigned int)strtoul(str_part, NULL, 10);
    str_part = strtok(NULL, "|");
    as_rel->relation = (int)strtol(str_part, NULL, 10);
}

/**
 * This function insert into the vertex_attr the as_num and the id of a vertex, following this pattern:
 * <AS_NUM, ID> 
 * It also stores the pairs <as_num, vertex_id> into the hashtable if not already inside.
 * 
 * Arguments:
 * graph: The graph.
 * ht: pointer to a hash table.
 * as_num: the autonomous system number to store into the graph
 * vertex_attr: vector in which the as_num and id of the vertex are stored
 * 
 * Returns the vertex id associated with the given as_num
 */
static igraph_integer_t add_annotated_vertex(igraph_t *graph, cga_hashtable_t *ht, unsigned long as_num, igraph_vector_t *vertex_attr) {
    igraph_vector_push_back(vertex_attr, as_num);
    igraph_integer_t *res = cga_ht_search(ht, as_num);

    if (res == NULL) {
        size_t id = cga_ht_nelems(ht);
        cga_ht_insert(ht, as_num, id);
        igraph_vector_push_back(vertex_attr, id);
        return (igraph_integer_t)id;
    }
    igraph_vector_push_back(vertex_attr, *res);
    return *res;
}

/**
 * This function stores to the vector "edges" the starting and ending vertex of the edge 
 * and stores the attribute type provided by relation into the vector edges_attr.
 * 
 * Arguments:
 * edges: pointer to a vector where the starting and ending vertices of the edge are stored
 * edges_attr: pointer to a vector where the type of the edge is stored
 * as1_id: the first autonomous system id 
 * as2_id: the second autonomous system id 
 * relation: the type of the edge (p2c or p2p)
 */
static void add_annotated_edges_vect(igraph_vector_t *edges, igraph_vector_t *edges_attr, unsigned int as1_id, unsigned int as2_id, int relation) {
    igraph_vector_push_back(edges, as1_id);
    igraph_vector_push_back(edges, as2_id);
    igraph_vector_push_back(edges_attr, relation);
}

float cga_degree_freedom_path(igraph_t *graph, igraph_integer_t vertex1_id, igraph_integer_t vertex2_id, unsigned int *num_vfree, unsigned int *num_novfree) {
    igraph_vector_int_t res, v;
    unsigned int vfree = 0, nvfree = 0;
    igraph_vector_int_init(&res, 0);
    igraph_vector_int_init(&v, 0);
    igraph_get_all_simple_paths(graph, &res, vertex1_id, igraph_vss_1(vertex2_id), -1, IGRAPH_ALL);

    for (long i = 0; i < igraph_vector_int_size(&res); i++) {
        if (VECTOR(res)[i] != -1) {
            igraph_vector_int_push_back(&v, VECTOR(res)[i]);
        } else {
            cga_is_valley_free(graph, &v) ? vfree++ : nvfree++;
            igraph_vector_int_clear(&v);
        }
    }
    if (num_vfree != NULL) *num_vfree = vfree;
    if (num_novfree != NULL) *num_novfree = nvfree;
    igraph_vector_int_destroy(&res);
    igraph_vector_int_destroy(&v);
    return vfree / (float)nvfree;
}

cga_status_t cga_as_analysis(igraph_t *graph, igraph_integer_t vertex, unsigned int nthreads, char *filename) {
    struct tinfo *ti = calloc(nthreads, sizeof(struct tinfo));
    if (ti == NULL)
        return NOMEM;
    igraph_integer_t split = igraph_vcount(graph) / nthreads;
    for (unsigned int i = 0; i < nthreads; i++) {
        printf("Thread %d init\n", i);
        ti[i].vertex = vertex;
        ti[i].graph = graph;
        int size = snprintf(NULL, 0, "%s_%u.csv", filename, i);
        ti[i].filename = malloc(size + 1);
        if (ti[i].filename == NULL) return NOMEM;
        snprintf(ti[i].filename, size + 1, "%s_%u.csv", filename, i);
        ti[i].lowerbound = split * i;
        ti[i].upperbound = (i == (nthreads - 1)) ? igraph_vcount(graph) : split * (i + 1);
        pthread_create(&ti[i].t_id, NULL, cga_as_analysis_job, &ti[i]);
    }
    for (unsigned int i = 0; i < nthreads; i++) {
        pthread_join(ti[i].t_id, NULL);
        printf("Thread T%d finished\n", i);
        free(ti[i].filename);
        printf("Freed%d\n", i);
    }
    printf("Job finished\n");
    free(ti);
    return SUCCESS;
}

static void *cga_as_analysis_job(void *attr) {
    struct tinfo *ti = (struct tinfo *)attr;
    igraph_lazy_adjlist_t adjlist;
    igraph_vector_int_t res, path;
    igraph_lazy_adjlist_init(ti->graph, &adjlist, IGRAPH_ALL, 1);
    igraph_vector_int_init(&res, 0);
    igraph_vector_int_init(&path, 0);

    FILE *fp = fopen(ti->filename, "w+");
    if (fp == NULL) {
        printf("No output\n");
        exit(EXIT_FAILURE);
    }
    printf("Open file \n");
    fprintf(fp, "from, to, length, cost\n");
    for (igraph_integer_t i = ti->lowerbound; i < ti->upperbound; i++) {
        igraph_vector_t *neighbors = igraph_lazy_adjlist_get(&adjlist, i);
        if ((igraph_vector_size(neighbors) == 0) || (i == ti->vertex)) continue;
        cga_dfs_vfree_it(ti->graph, &res, ti->vertex, i);
        for (long j = 0; j < igraph_vector_int_size(&res); j++) {
            if (VECTOR(res)[j] != -1) {
                igraph_vector_int_push_back(&path, VECTOR(res)[j]);
            } else {
                if (igraph_vector_int_size(&path) == 0) continue;  // no path between the two nodes
                int pathc = cga_path_cost(ti->graph, &path);
                fprintf(fp, "%lu,%lu,%li,%d\n", (unsigned long)VAN(ti->graph, "label", ti->vertex), (unsigned long)VAN(ti->graph, "label", i), igraph_vector_int_size(&path) - 1, pathc);
                igraph_vector_int_clear(&path);
            }
        }
        igraph_vector_int_clear(&res);
    }
    fclose(fp);
    printf("file close\n");
    igraph_lazy_adjlist_destroy(&adjlist);
    printf("prova1\n");
    igraph_vector_int_destroy(&path);
    printf("prova2\n");
    igraph_vector_int_destroy(&res);
    printf("prova3\n");
    return NULL;
}

cga_status_t cga_graph_analysis(igraph_t *graph, unsigned int nthreads, char *filename) {
    struct tinfo *ti = calloc(nthreads, sizeof(struct tinfo));
    if (ti == NULL)
        return NOMEM;
    igraph_integer_t split = igraph_vcount(graph) / nthreads;
    for (unsigned int i = 0; i < nthreads; i++) {
        ti[i].graph = graph;
        int size = snprintf(NULL, 0, "%s_%u.csv", filename, i);
        ti[i].filename = malloc(size + 1);
        if (ti[i].filename == NULL) return NOMEM;
        snprintf(ti[i].filename, size + 1, "%s_%u.csv", filename, i);
        ti[i].lowerbound = split * i;
        ti[i].upperbound = (i == (nthreads - 1)) ? igraph_vcount(graph) : split * (i + 1);
        pthread_create(&ti[i].t_id, NULL, cga_graph_analysis_job, &ti[i]);
    }
    for (int i = 0; i < nthreads; i++) {
        pthread_join(ti[i].t_id, NULL);
        free(ti[i].filename);
    }
    free(ti);
    return SUCCESS;
}

static void *cga_graph_analysis_job(void *attr) {
    struct tinfo *ti = (struct tinfo *)attr;
    igraph_lazy_adjlist_t adjlist;
    igraph_vector_int_t res, path;
    igraph_lazy_adjlist_init(ti->graph, &adjlist, IGRAPH_ALL, 1);
    igraph_vector_int_init(&res, 0);
    igraph_vector_int_init(&path, 0);

    FILE *fp = fopen(ti->filename, "w+");
    if (fp == NULL) {
        printf("No output\n");
        exit(EXIT_FAILURE);
    }
    printf("Open file \n");
    fprintf(fp, "from, to, avg length, min length, max length, avg cost, min cost, max cost\n");
    for (igraph_integer_t i = ti->lowerbound; i < ti->upperbound; i++) {
        igraph_vector_t *outervect = igraph_lazy_adjlist_get(&adjlist, i);
        if (igraph_vector_size(outervect) == 0) continue;  // the node is unreachable
        for (igraph_integer_t j = 0; j < igraph_vcount(ti->graph); j++) {
            if (j == i) continue;  // same node, not needed for analysis
            igraph_vector_t *innervect = igraph_lazy_adjlist_get(&adjlist, j);
            if (igraph_vector_size(innervect) == 0) continue;  // the node is unreachable
            int cost_sum = 0, cost_min = INT_MAX, cost_max = INT_MIN, length_sum = 0, length_min = INT_MAX, length_max = INT_MIN, count = 0;
            cga_dfs_vfree_it(ti->graph, &res, i, j);
            for (long k = 0; k < igraph_vector_int_size(&res); k++) {
                if (VECTOR(res)[k] != -1) {
                    igraph_vector_int_push_back(&path, VECTOR(res)[k]);
                } else {
                    int pathc = cga_path_cost(ti->graph, &path);
                    cost_sum += pathc;
                    count++;
                    if (pathc < cost_min) cost_min = pathc;
                    if (pathc > cost_max) cost_max = pathc;
                    int size = igraph_vector_int_size(&path) - 1;
                    length_sum += size;
                    if (size < length_min) length_min = size;
                    if (size > length_max) length_max = size;
                    igraph_vector_int_clear(&path);
                }
            }
            if (count != 0) {  // if count is 0 there's no paths between two nodes
                fprintf(fp, "%lu,%lu,%.3f,%d,%d,%.3f,%d,%d\n", (unsigned long)VAN(ti->graph, "label", i), (unsigned long)VAN(ti->graph, "label", j),
                        length_sum / (float)count, length_min, length_max, cost_sum / (float)count, cost_min, cost_max);
            }
            igraph_vector_int_clear(&res);
        }
    }
    fclose(fp);
    igraph_lazy_adjlist_destroy(&adjlist);
    igraph_vector_int_destroy(&path);
    igraph_vector_int_destroy(&res);
    return NULL;
}
