//
// Created by ssunah on 6/23/18.
//

#ifndef SUBGRAPHMATCHING_GRAPHOPERATIONS_H
#define SUBGRAPHMATCHING_GRAPHOPERATIONS_H

#include "graph/graph.h"

class GraphOperations {
public:
    static void getKCore(const graph_ptr graph, IntArray &core_table);
    static void match_bfs(IntArray &col_ptrs, IntArray &col_ids, IntArray &match, IntArray &row_match, IntArray &visited,
                          IntArray &queue, IntArray &previous, int n, int m);
    static void bfsTraversal(const graph_ptr graph, VertexID root_vertex, TreeNodeArray &tree, UIntArray &bfs_order);
    static void dfsTraversal(TreeNodeArray &tree, VertexID root_vertex, ui node_num, UIntArray &dfs_order);
private:
    static void old_cheap(IntArray &col_ptrs, IntArray &col_ids, IntArray &match, IntArray &row_match, int n, int m);
    static void dfs(TreeNodeArray &tree, VertexID cur_vertex, UIntArray &dfs_order, ui& count);
};


#endif //SUBGRAPHMATCHING_GRAPHOPERATIONS_H
