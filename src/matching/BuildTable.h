//
// Created by ssunah on 11/20/18.
//

#ifndef SUBGRAPHMATCHING_BUILDTABLE_H
#define SUBGRAPHMATCHING_BUILDTABLE_H

#include "graph/graph.h"
#include "utility/QFliter.h"
#include <vector>
class BuildTable {
public:
    static void buildTables(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates, UIntArray &candidates_count,
                            EdgesPtrMatrix &edge_matrix);

    static void printTableCardinality(const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix);
    static void printTableCardinality(const graph_ptr query_graph, TreeNodeArray &tree, UIntArray &order,
                                         std::vector<std::unordered_map<VertexID, std::vector<VertexID >>> &TE_Candidates,
                                         std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates);
    static void printTableInfo(const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix);
    static void printTableInfo(VertexID begin_vertex, VertexID end_vertex, EdgesPtrMatrix &edge_matrix);
    static size_t computeMemoryCostInBytes(const graph_ptr query_graph, UIntArray &candidates_count, EdgesPtrMatrix &edge_matrix);
    static size_t computeMemoryCostInBytes(const graph_ptr query_graph, UIntArray &candidates_count, UIntArray &order, TreeNodeArray &tree,
                                               std::vector<std::unordered_map<VertexID, std::vector<VertexID >>> &TE_Candidates,
                                               std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates);

#if ENABLE_QFLITER == 1
    static BSRGraph*** qfliter_bsr_graph_;
#endif
};


#endif //SUBGRAPHMATCHING_BUILDTABLE_H
