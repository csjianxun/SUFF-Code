#ifndef SUBGRAPHMATCHING_GENERATEQUERYPLAN_H
#define SUBGRAPHMATCHING_GENERATEQUERYPLAN_H

#include "graph/graph.h"
#include <vector>
class GenerateQueryPlan {
public:
    static void generateGQLQueryPlan(const graph_ptr data_graph, const graph_ptr query_graph, UIntArray &candidates_count,
                                         UIntArray &order, UIntArray &pivot);
    static void generateQSIQueryPlan(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix,
                                         UIntArray &order, UIntArray &pivot);

    static void generateRIQueryPlan(const graph_ptr data_graph, const graph_ptr query_graph, UIntArray &order, UIntArray &pivot);

    static void generateVF2PPQueryPlan(const graph_ptr data_graph, const graph_ptr query_graph, UIntArray &order, UIntArray &pivot);

    static void generateOrderSpectrum(const graph_ptr query_graph, UIntMatrix &spectrum, ui num_spectrum_limit);

    static void
    generateTSOQueryPlan(const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntArray &order, UIntArray &pivot,
                             TreeNodeArray &tree, UIntArray &dfs_order);

    static void
    generateCFLQueryPlan(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix,
                             UIntArray &order, UIntArray &pivot, TreeNodeArray &tree, UIntArray &bfs_order, UIntArray &candidates_count);

    static void
    generateDSPisoQueryPlan(const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntArray &order, UIntArray &pivot,
                                TreeNodeArray &tree, UIntArray &bfs_order, UIntArray &candidates_count, UIntMatrix &weight_array);

    static void generateCECIQueryPlan(const graph_ptr query_graph, TreeNodeArray &tree, UIntArray &bfs_order, UIntArray &order, UIntArray &pivot);
    static void checkQueryPlanCorrectness(const graph_ptr query_graph, UIntArray &order, UIntArray &pivot);

    static void checkQueryPlanCorrectness(const graph_ptr query_graph, UIntArray &order);

    static void printQueryPlan(const graph_ptr query_graph, UIntArray &order);

    static void printSimplifiedQueryPlan(const graph_ptr query_graph, UIntArray &order);
private:
    static VertexID selectGQLStartVertex(const graph_ptr query_graph, UIntArray &candidates_count);
    static std::pair<VertexID, VertexID> selectQSIStartEdge(const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix);

    static void generateRootToLeafPaths(TreeNodeArray &tree_node, VertexID cur_vertex, UIntArray &cur_path,
                                        UIntMatrix &paths);
    static void estimatePathEmbeddsingsNum(UIntArray &path, EdgesPtrMatrix &edge_matrix,
                                           std::vector<size_t> &estimated_embeddings_num);

    static void generateCorePaths(const graph_ptr query_graph, TreeNodeArray &tree_node, VertexID cur_vertex, UIntArray &cur_core_path,
                                  UIntMatrix &core_paths);

    static void generateTreePaths(const graph_ptr query_graph, TreeNodeArray &tree_node, VertexID cur_vertex,
                                  UIntArray &cur_tree_path, UIntMatrix &tree_paths);

    static void generateLeaves(const graph_ptr query_graph, UIntArray &leaves);

    static ui generateNoneTreeEdgesCount(const graph_ptr query_graph, TreeNodeArray &tree_node, UIntArray &path);
    static void updateValidVertices(const graph_ptr query_graph, VertexID query_vertex, BoolArray &visited, BoolArray &adjacent);
};


#endif //SUBGRAPHMATCHING_GENERATEQUERYPLAN_H
