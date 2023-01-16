//
// Created by ssunah on 11/21/18.
//

#ifndef SUBGRAPHMATCHING_GENERATEFILTERINGPLAN_H
#define SUBGRAPHMATCHING_GENERATEFILTERINGPLAN_H


#include "graph/graph.h"
#include "configuration/types.hpp"

class GenerateFilteringPlan {
public:
    static void generateTSOFilterPlan(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                           UIntArray &order);
    static void generateCFLFilterPlan(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                      UIntArray &order, int &level_count, UIntArray &level_offset);
    static void generateDPisoFilterPlan(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                        UIntArray &order);
    static void generateCECIFilterPlan(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                       UIntArray &order);
private:
    static VertexID selectTSOFilterStartVertex(const graph_ptr data_graph, const graph_ptr query_graph);
    static VertexID selectCFLFilterStartVertex(const graph_ptr data_graph, const graph_ptr query_graph);
    static VertexID selectDPisoStartVertex(const graph_ptr data_graph, const graph_ptr query_graph);
    static VertexID selectCECIStartVertex(const graph_ptr data_graph, const graph_ptr query_graph);
};


#endif //SUBGRAPHMATCHING_GENERATEFILTERINGPLAN_H
