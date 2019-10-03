#ifndef AS_RELATIONSHIP_H_soadifvhodkfasdgashgasgodfvjj
#define AS_RELATIONSHIP_H_soadifvhodkfasdgashgasgodfvjj
#include <igraph/igraph.h>
#include "hashtable.h"

/**
 * This function read an as-rel dataset file provided by CAIDA and load its data into the graph
 * The format of the as-rel file is:
 * <as_num_1>|<as_num_2>|relationship
 * The graph will be initialized as a partially directed graph: it will have directed provider-to-customer edges
 * and undirected peer-to-peer edges.
 * Each vertex has a numeric attribute "label" that identify the as_number of the autonomous system.
 * Each edge has a numeric attribute "type" that identify if the edge is a provider-to-customer edge (-1) or a peer-to-peer edge (0).
 * N.B. In the main program it must be set the igraph attribute table with the following line:
 * igraph_i_set_attribute_table(&igraph_cattribute_table). 
 * Without it this function can’t set the attributes of the graph.
 * To reference the vertex id in the graph with its as_number, this function store in the hashtable ht the association <as_number, vertex_id>.
 * If the given file has no read privileges, this function abort the program with an error printed in stderr.
 * 
 * Arguments:
 * graph: pointer to an uninitialized graph object
 * ht: pointer to an already initialized hashtable. It will be used to store the assotiation <as_number, vertex_id>
 * instream: pointer to a stream. It needs read privilege
 */
void cga_load_snapshot(igraph_t *graph, cga_hashtable_t *ht, FILE *instream);

/**
 * This function evaluates if path is a valley free path.
 * A path is a valley free path iff the following conditions hold true:
 * 1. A provider-to-customer edge can be followed by only provider-to-customer or sibling-to-sibling edges
 * 2. A peer-to-peer edge can be followed by only provider-to-customer or sibling-to-sibling edges
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * path: Pointer to a vector consisting of vertex_ids that forms the path to check
 * 
 * Returns 1 if the path is a valley free path, 0 otherwise
 */
int cga_is_valley_free(igraph_t *graph, igraph_vector_int_t *path);

/**
 * Recursively search all the valley free paths between two nodes.
 * The resulting paths are stored in res. All paths are separated by -1 markers.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * res: Initialized vector, all the resulting paths are stored here, separated by -1 markers.
 *      The paths are included in arbitrary order, as they are found
 * from: The starting vertex_id
 * to: The ending vertex_id
 */
void cga_dfs_vfree_rec(igraph_t *graph, igraph_vector_int_t *res, igraph_integer_t from, igraph_integer_t to);

/**
 * Iteratively search all the valley free paths between two nodes.
 * The resulting paths are stored in res. All paths are separated by -1 markers.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * res: Initialized vector, all the resulting paths are stored here, separated by -1 markers.
 *      The paths are included in arbitrary order, as they are found
 * from: The starting vertex_id
 * to: The ending vertex_id
 */
void cga_dfs_vfree_it(igraph_t *graph, igraph_vector_int_t *res, igraph_integer_t from, igraph_integer_t to);

/**
 * Calculate the cost of the valley free path as an algebraic sum of the relationships between
 * the Autonomous Systems.
 * Customer-to-provider has value 1
 * Provider-to-customer has value -1
 * Peer-to-peer has value 0
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * path: Pointer to a vector of vertex_ids, where each couple has an edge in the graph that connects them.
 *       If it doesn't exists then it will be treated as a customer-to-provider edge.
 * 
 * Returns the algebraic sum of the relationships between the Autonomous Systems
 */
int cga_path_cost(igraph_t *graph, igraph_vector_int_t *path);

/**
 * Calculate the degree of freedom of the paths between two nodes.
 * Given all the paths between two nodes, this function count all the valley free and
 * no valley free paths. The formula for the degree of freedom of the paths between two nodes is
 * calculated as: num_path_vfree / num_path_novfree.
 * If num_vfree and num_novfree pointers are not NULL, this function stores in these pointers the
 * number of valley free and no valley free paths.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * vertex1_id: The starting vertex
 * vertex2_id: The ending vertex
 * num_vfree: Pointer to an unsigned int variable. If not NULL, the number of valley free paths
 *            will be stored here
 * num_nvfree: Pointer to an unsigned int variable. If not NULL, the number of no valley free
 *             paths will be stored here
 * 
 * Returns the degree of freedom of the paths between two nodes.
 */
float cga_degree_freedom_path(igraph_t *graph, igraph_integer_t vertex1_id, igraph_integer_t vertex2_id, unsigned int *num_vfree, unsigned int *num_novfree);

/**
 * This function analyze and print in n files all the valley free paths from a node to
 * all other nodes, where n is the number of threads used to compute the analysis.
 * The search for paths is done through the use of cga_dfs_vfree_it function.
 * nthreads must be at least 1.  Specifying a number greater than 1 will use more
 * threads to compute the analysis.
 * Ideally the number of threads should be a divisor of the graph’s vertices number,
 * or at least less than that number. Each thread will have a range of vertices of the
 * graph to analyze, and the results will be stored in a file that uses the following
 * naming convention:
 * For a generic thread n, given the name of the file filename, the file name will
 * be filename_n.csv.
 * filename shouldn’t have the extension of the file (it will be added automatically
 * as .csv) and can be written as a path. If a path is given, and not only a filename,
 * the folders forming the path must already exist.
 * The files’ output consist of a header and the content.
 * The header <from,to,length,cost> represents the starting autonomous system,
 * the target autonomous system, the path length and the cost given as the algebraic
 * sum  of  the  AS  relationship  in  the  path.   Each  line  of  the  content  represents
 * the  analysis  of  a  valley  free  path  between  two  nodes,  with  each  information
 * separated by a comma.
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * vertex: The vertex_id to analyze. It will be the starting vertex from where the paths are calculated
 * nthreads: The number of threads used to analyze the vertex's paths. The given value must be
 *           at least greater or equal to 1
 * filename: Part of the filename used to compose the name of the output file. It should not have
 *           the extension and can be a path (in this case the folders that compose the path must
 *           already exists)
 */
cga_status_t cga_as_analysis(igraph_t *graph, igraph_integer_t vertex, unsigned int nthreads, char *filename);

/**
 * This function analyze and print in n files all the valley free paths from all nodes 
 * to all other nodes, where n is the number of threads used to compute the analysis.
 * The search for paths is done through the use of cga_dfs_vfree_it function.
 * nthreads must be at least 1.  Specifying a number greater than 1 will use more
 * threads to compute the analysis.
 * Ideally the number of threads should be a divisor of the graph’s vertices number,
 * or at least less than that number. Each thread will have a range of vertices of the
 * graph to analyze, and the results will be stored in a file that uses the following
 * naming convention:
 * For a generic thread n, given the name of the file filename, the file name will be
 * filename_n.csv.
 * filename shouldn’t have the extension of the file (it will be added automatically
 * as .csv) and can be written as a path. If a path is given, and not only a filename,
 * the folders forming the path must already exist.
 * The files’ output consist of a header and the content.
 * Since  there  can  be  multiple  valley  free  paths  between  two  nodes,  the  header
 * <from, to, avg length, min length, max length, avg cost, min cost, max cost>
 * represents  the  starting  autonomous  system,  the  target  autonomous  system,  
 * the  average  path  length,  the  minimum  path  length,  the  maximum  path length,  
 * the average cost,  the minimum and the maximum cost of all the paths between the two nodes.
 * The cost is given as the algebraic sum of the AS relationship in the path.  Each
 * line  of  the  content  represents  the  analysis  of  all  the  valley  free  paths  between
 * two nodes, with each information separated by a comma
 * 
 * Arguments:
 * graph: Pointer to the graph object
 * nthreads: The number of threads used to analyze the vertex's paths. The given value must be
 *           at last greater or equal to 1
 * filename: Part of the name used to compose the name of the output file. It should not have
 *           the extension and can be a path (in this case the folders that compose the path
 *           must already exists)
 */
cga_status_t cga_graph_analysis(igraph_t *graph, unsigned int nthreads, char *filename);
#endif