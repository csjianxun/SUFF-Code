//
// Created by ssunah on 11/20/18.
//

#ifndef SUBGRAPHMATCHING_FILTERVERTICES_H
#define SUBGRAPHMATCHING_FILTERVERTICES_H

#include "graph/graph.h"
#include <vector>

class FilterVertices {
public:
    static bool LDFFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count);
    static bool NLFFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count);
    static bool GQLFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count);
    static bool TSOFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count, UIntArray &order, TreeNodeArray &tree);
    static bool CFLFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count, UIntArray &order, TreeNodeArray &tree);
    static bool DPisoFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count, UIntArray &order, TreeNodeArray &tree);

    static bool CECIFilter(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count, UIntArray &order, TreeNodeArray &tree,
                                    std::vector<std::unordered_map<VertexID, std::vector<VertexID >>> &TE_Candidates,
                           std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates);

    // static bool VCFilter(const Graph* data_graph, const Graph* query_graph, ui **&candidates, ui *&candidates_count);

    static void computeCandidateWithNLF(const graph_ptr data_graph, const graph_ptr query_graph, VertexID query_vertex,
                                        ui &count, UIntArray &buffer);

    static void computeCandidateWithLDF(const graph_ptr data_graph, const graph_ptr query_graph, VertexID query_vertex,
                                        ui &count, UIntArray &buffer);

    static void generateCandidates(const graph_ptr data_graph, const graph_ptr query_graph, VertexID query_vertex,
                                      UIntArray &pivot_vertices, ui pivot_vertices_count, UIntMatrix &candidates,
                                      UIntArray &candidates_count, UIntArray &flag, UIntArray &updated_flag);

    static void pruneCandidates(const graph_ptr data_graph, const graph_ptr query_graph, VertexID query_vertex,
                                   UIntArray &pivot_vertices, ui pivot_vertices_count, UIntMatrix &candidates,
                                   UIntArray &candidates_count, UIntArray &flag, UIntArray &updated_flag);


    static void
    printCandidatesInfo(const graph_ptr query_graph, UIntArray &candidates_count, std::vector<ui> &optimal_candidates_count);

    static void sortCandidates(UIntMatrix &candidates, UIntArray &candidates_count, ui num);

    static double computeCandidatesFalsePositiveRatio(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                                          UIntArray &candidates_count, std::vector<ui> &optimal_candidates_count);
private:
    static void allocateBuffer(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                    UIntArray &candidates_count);
    static bool verifyExactTwigIso(const graph_ptr data_graph, const graph_ptr query_graph, ui data_vertex, ui query_vertex,
                                   BoolMatrix &valid_candidates, IntArray &left_to_right_offset, IntArray &left_to_right_edges,
                                   IntArray &left_to_right_match, IntArray &right_to_left_match, IntArray &match_visited,
                                   IntArray &match_queue, IntArray &match_previous);
    static void compactCandidates(UIntMatrix &candidates, UIntArray &candidates_count, ui query_vertex_num);
    static bool isCandidateSetValid(UIntMatrix &candidates, UIntArray &candidates_count, ui query_vertex_num);
};


#endif //SUBGRAPHMATCHING_FILTERVERTICES_H
