#ifndef DISPLAY_H_noadvnbzxocuivboivchblkjhasdfoiu
#define DISPLAY_H_noadvnbzxocuivboivchblkjhasdfoiu

#include <igraph/igraph.h>

/**
 * Prints in stdout the number of vertices, edges and the type of the given graph
 * 
 * Arguments:
 * graph: Pointer to the graph object
 */
void cga_print_info(igraph_t *graph);

/**
 * Converts the vertex_ids in the input vector v in as_numbers and prints them in the file.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * v: Vector containing vertex_ids of the graph
 * ostream: File pointer used as output
 */
void cga_print_vector_label(igraph_t *graph, igraph_vector_int_t *v, FILE *ostream);

/**
 * Calculates the degree of freedom of the paths between two nodes using cga_degree_freedom_path
 * and prints its output in stdout
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * vertex1_id: The starting vertex used to calculate the degree of freedom
 * vertex2_id: The ending vertex used to calculate the degree of freedom
 */
void cga_print_degree_freedom_path(igraph_t *graph, igraph_integer_t vertex1_id, igraph_integer_t vertex2_id);

/**
 * Prints the as_numbers of the graph as an adjacency list in stdout
 * 
 * Arguments:
 * graph: Pointer to the graph object
 */
void cga_print_adj(igraph_t *graph);

/**
 * Given a vector containing a list of paths separated by -1 marker, this function prints
 * in the file ostream all the containing paths line by line, converting the vertex_ids of the generic path
 * in as_number. Next to each path this function prints OK if the path is a valley free path, or NOPE
 * if it is a no valley free path.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * res: Pointer to a vector containing a list of paths separated by -1 marker
 * ostream: File pointer used as output
 */
void cga_print_result_label_vfree(igraph_t *graph, igraph_vector_int_t *res, FILE *ostream);

/**
 * Given a vector containing a list of paths separated by -1 marker, this function prints
 * in the file ostream all the containing paths line by line, converting the vertex_ids of the generic path
 * in as_number.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * res: Pointer to a vector containing a list of paths separated by -1 marker
 * ostream: File pointer used as output
 */
void cga_print_result_label(igraph_t *graph, igraph_vector_int_t *res, FILE *ostream);
#endif