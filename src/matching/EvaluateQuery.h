//
// Created by ssunah on 11/20/18.
//

#ifndef SUBGRAPHMATCHING_EVALUATEQUERY_H
#define SUBGRAPHMATCHING_EVALUATEQUERY_H

#include "graph/graph.h"
#include "utility/QFliter.h"
#include <vector>
#include <queue>
#include <bitset>
#include <functional>

// Min priority queue.
bool extendable_vertex_compare(std::pair<std::pair<VertexID, ui>, ui> l, std::pair<std::pair<VertexID, ui>, ui> r);

typedef std::priority_queue<std::pair<std::pair<VertexID, ui>, ui>, std::vector<std::pair<std::pair<VertexID, ui>, ui>>,
        std::function<bool(std::pair<std::pair<VertexID, ui>, ui>, std::pair<std::pair<VertexID, ui>, ui>)>> dpiso_min_pq;

class EvaluateQuery {
public:
    static size_t exploreGraph(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates,
                                  UIntArray &candidates_count, UIntArray &order, UIntArray &pivot, size_t output_limit_num, size_t &call_count);

    static size_t LFTJ(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates, UIntArray &candidates_count,
                           UIntArray &order, size_t output_limit_num, size_t &call_count);

    static size_t
    exploreGraphQLStyle(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates, UIntArray &candidates_count, UIntArray &order,
                            size_t output_limit_num, size_t &call_count);

    static size_t
    exploreQuickSIStyle(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates, UIntArray &candidates_count, UIntArray &order,
                            UIntArray &pivot, size_t output_limit_num, size_t &call_count);

    static size_t exploreDPisoStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                    EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates, UIntArray &candidates_count,
                                    UIntMatrix &weight_array, UIntArray &order, size_t output_limit_num,
                                    size_t &call_count);

    static size_t exploreDPisoRecursiveStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                             EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates, UIntArray &candidates_count,
                                             UIntMatrix &weight_array, UIntArray &order, size_t output_limit_num,
                                             size_t &call_count);

    static size_t exploreCECIStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree, UIntMatrix &candidates,
                                      UIntArray &candidates_count,
                                      std::vector<std::unordered_map<VertexID, std::vector<VertexID>>> &TE_Candidates,
                                      std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates,
                                      UIntArray &order, size_t &output_limit_num, size_t &call_count);

#if ENABLE_QFLITER == 1
    static BSRGraph*** qfliter_bsr_graph_;
    static int* temp_bsr_base1_;
    static int* temp_bsr_state1_;
    static int* temp_bsr_base2_;
    static int* temp_bsr_state2_;
#endif

#ifdef SPECTRUM
    static bool exit_;
#endif

#ifdef DISTRIBUTION
    static size_t* distribution_count_;
#endif
private:
    static void generateBN(const graph_ptr query_graph, UIntArray &order, UIntArray &pivot, UIntMatrix &bn, UIntArray &bn_count);
    static void generateBN(const graph_ptr query_graph, UIntArray &order, UIntMatrix &bn, UIntArray &bn_count);
    static void allocateBuffer(const graph_ptr query_graph, const graph_ptr data_graph, UIntArray &candidates_count, UIntArray &idx,
                                   UIntArray &idx_count, UIntArray &embedding, UIntArray &idx_embedding, UIntArray &temp_buffer,
                                   UIntMatrix &valid_candidate_idx, BoolArray &visited_vertices);
    static void releaseBuffer(ui query_vertices_num, UIntArray &idx, UIntArray &idx_count, UIntArray &embedding, UIntArray &idx_embedding,
                                  UIntArray &temp_buffer, UIntMatrix &valid_candidate_idx, BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_count);

    static void generateValidCandidateIndex(const graph_ptr data_graph, ui depth, UIntArray &embedding, UIntArray &idx_embedding,
                                            UIntArray &idx_count, UIntMatrix &valid_candidate_index, EdgesPtrMatrix &edge_matrix,
                                            BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order, UIntArray &pivot,
                                            UIntMatrix &candidates);

    static void generateValidCandidateIndex(ui depth, UIntArray &idx_embedding, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                                EdgesPtrMatrix &edge_matrix, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order, UIntArray &temp_buffer);

    static void generateValidCandidates(const graph_ptr data_graph, ui depth, UIntArray &embedding, UIntArray &idx_count, UIntMatrix &valid_candidate,
                                        BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order, UIntMatrix &candidates, UIntArray &candidates_count);

    static void generateValidCandidates(const graph_ptr query_graph, const graph_ptr data_graph, ui depth, UIntArray &embedding,
                                            UIntArray &idx_count, UIntMatrix &valid_candidate, BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_cnt,
                                            UIntArray &order, UIntArray &pivot);
    static void generateValidCandidates(ui depth, UIntArray &embedding, UIntArray &idx_count, UIntMatrix &valid_candidates, UIntArray &order,
                                            UIntArray &temp_buffer, TreeNodeArray &tree,
                                            std::vector<std::unordered_map<VertexID, std::vector<VertexID>>> &TE_Candidates,
                                            std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates);

    static void updateExtendableVertex(UIntArray &idx_embedding, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                          EdgesPtrMatrix &edge_matrix, UIntArray &temp_buffer, UIntMatrix &weight_array,
                                          TreeNodeArray &tree, VertexID mapped_vertex, UIntArray &extendable,
                                          std::vector<dpiso_min_pq> &vec_rank_queue, const graph_ptr query_graph);

    static void restoreExtendableVertex(TreeNodeArray &tree, VertexID unmapped_vertex, UIntArray &extendable);
    static void generateValidCandidateIndex(VertexID vertex, UIntArray &idx_embedding, UIntArray &idx_count, UIntArray &valid_candidate_index,
                                            EdgesPtrMatrix &edge_matrix, UIntArray &bn, ui bn_cnt, UIntArray &temp_buffer);

    static void computeAncestor(const graph_ptr query_graph, TreeNodeArray &tree, UIntArray &order,
                                std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors);

    static void computeAncestor(const graph_ptr query_graph, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order,
                                std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors);

    static void computeAncestor(const graph_ptr query_graph, UIntArray &order, std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors);

    static std::bitset<MAXIMUM_QUERY_GRAPH_SIZE> exploreDPisoBacktrack(ui max_depth, ui depth, VertexID mapped_vertex, TreeNodeArray &tree, UIntArray &idx_embedding,
                                                     UIntArray &embedding, std::unordered_map<VertexID, VertexID> &reverse_embedding,
                                                     BoolArray &visited_vertices, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                                     EdgesPtrMatrix &edge_matrix,
                                                     std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors,
                                                     dpiso_min_pq rank_queue, UIntMatrix &weight_array, UIntArray &temp_buffer, UIntArray &extendable,
                                                     UIntMatrix &candidates, size_t &embedding_count, size_t &call_count,
                                                     const graph_ptr query_graph);
};


#endif //SUBGRAPHMATCHING_EVALUATEQUERY_H
