#include "EvaluateQuery.h"
#include "utility/computesetintersection.h"
#include <vector>
#include <cstring>

#include "utility/pretty_print.h"
#include "suff/FilterManager.hpp"

// Min priority queue.
bool extendable_vertex_compare(std::pair<std::pair<VertexID, ui>, ui> l, std::pair<std::pair<VertexID, ui>, ui> r) {
    if (l.first.second == 1 && r.first.second != 1) {
        return true;
    }
    else if (l.first.second != 1 && r.first.second == 1) {
        return false;
    }
    else
    {
        return l.second > r.second;
    }
}

#if ENABLE_QFLITER == 1
BSRGraph ***EvaluateQuery::qfliter_bsr_graph_;
int *EvaluateQuery::temp_bsr_base1_ = nullptr;
int *EvaluateQuery::temp_bsr_state1_ = nullptr;
int *EvaluateQuery::temp_bsr_base2_ = nullptr;
int *EvaluateQuery::temp_bsr_state2_ = nullptr;
#endif

#ifdef SPECTRUM
bool EvaluateQuery::exit_;
#endif

#ifdef DISTRIBUTION
size_t* EvaluateQuery::distribution_count_;
#endif

void EvaluateQuery::generateBN(const graph_ptr query_graph, UIntArray &order, UIntArray &pivot, UIntMatrix &bn, UIntArray &bn_count) {
    ui query_vertices_num = query_graph->getVerticesCount();
    bn_count.resize(query_vertices_num);
    std::fill(bn_count.begin(), bn_count.begin() + query_vertices_num, 0);
    bn.resize(query_vertices_num);
    for (ui i = 0; i < query_vertices_num; ++i) {
        bn[i].resize(query_vertices_num);
    }

    std::vector<bool> visited_vertices(query_vertices_num, false);
    visited_vertices[order[0]] = true;
    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID vertex = order[i];

        ui nbrs_cnt;
        const ui *nbrs = query_graph->getVertexNeighbors(vertex, nbrs_cnt);
        for (ui j = 0; j < nbrs_cnt; ++j) {
            VertexID nbr = nbrs[j];

            if (visited_vertices[nbr] && nbr != pivot[i]) {
                bn[i][bn_count[i]++] = nbr;
            }
        }

        visited_vertices[vertex] = true;
    }
}

void EvaluateQuery::generateBN(const graph_ptr query_graph, UIntArray &order, UIntMatrix &bn, UIntArray &bn_count) {
    ui query_vertices_num = query_graph->getVerticesCount();
    bn_count.resize(query_vertices_num);
    std::fill(bn_count.begin(), bn_count.begin() + query_vertices_num, 0);
    bn.resize(query_vertices_num);
    for (ui i = 0; i < query_vertices_num; ++i) {
        bn[i].resize(query_vertices_num);
    }

    std::vector<bool> visited_vertices(query_vertices_num, false);
    visited_vertices[order[0]] = true;
    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID vertex = order[i];

        ui nbrs_cnt;
        const ui *nbrs = query_graph->getVertexNeighbors(vertex, nbrs_cnt);
        for (ui j = 0; j < nbrs_cnt; ++j) {
            VertexID nbr = nbrs[j];

            if (visited_vertices[nbr]) {
                bn[i][bn_count[i]++] = nbr;
            }
        }

        visited_vertices[vertex] = true;
    }
}

size_t
EvaluateQuery::exploreGraph(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates,
                            UIntArray &candidates_count, UIntArray &order, UIntArray &pivot, size_t output_limit_num, size_t &call_count) {
    // Generate the bn.
    UIntMatrix bn;
    UIntArray bn_count;
    generateBN(query_graph, order, pivot, bn, bn_count);

    // Allocate the memory buffer.
    UIntArray idx;
    UIntArray idx_count;
    UIntArray embedding;
    UIntArray idx_embedding;
    UIntArray temp_buffer;
    UIntMatrix valid_candidate_idx;
    BoolArray visited_vertices;
    allocateBuffer(data_graph, query_graph, candidates_count, idx, idx_count, embedding, idx_embedding,
                   temp_buffer, valid_candidate_idx, visited_vertices);
    std::vector<bool> found_embedding(query_graph->getVerticesCount(), false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    // Evaluate the query.
    size_t embedding_cnt = 0;
    int cur_depth = 0;
    int max_depth = query_graph->getVerticesCount();
    VertexID start_vertex = order[0];

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    total_count[cur_depth] = candidates_count[start_vertex];

    for (ui i = 0; i < idx_count[cur_depth]; ++i) {
        valid_candidate_idx[cur_depth][i] = i;
    }

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            ui valid_idx = valid_candidate_idx[cur_depth][idx[cur_depth]];
            VertexID u = order[cur_depth];
            VertexID v = candidates[u][valid_idx];

            embedding[u] = v;
            idx_embedding[u] = valid_idx;
            visited_vertices[v] = true;
            idx[cur_depth] += 1;

            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                found_embedding[cur_depth] = true;
                suff::FilterManager::AddMatch(cur_depth, embedding);

                visited_vertices[v] = false;
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
            } else
#ifdef SUFF
                if (suff::FilterManager::PassMatch(cur_depth, embedding))
#endif
             {
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                found_embedding[cur_depth] = false;
                total_count[cur_depth] += 1;
                generateValidCandidateIndex(data_graph, cur_depth, embedding, idx_embedding, idx_count,
                                            valid_candidate_idx,
                                            edge_matrix, visited_vertices, bn, bn_count, order, pivot, candidates);
            }
#ifdef SUFF
             else {
                visited_vertices[v] = false;
            }
#endif
        }

        fail_count[cur_depth] += !found_embedding[cur_depth];
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else {
            found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
#ifdef SUFF
            if (found_embedding[cur_depth+1]) {
                suff::FilterManager::AddMatch(cur_depth, embedding);
            }
#endif
            visited_vertices[embedding[order[cur_depth]]] = false;
        }
    }


    // Release the buffer.
    EXIT:
    releaseBuffer(max_depth, idx, idx_count, embedding, idx_embedding, temp_buffer, valid_candidate_idx,
                  visited_vertices,
                  bn, bn_count);

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void
EvaluateQuery::allocateBuffer(const graph_ptr data_graph, const graph_ptr query_graph, UIntArray &candidates_count, UIntArray &idx,
                              UIntArray &idx_count, UIntArray &embedding, UIntArray &idx_embedding, UIntArray &temp_buffer,
                              UIntMatrix &valid_candidate_idx, BoolArray &visited_vertices) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ui data_vertices_num = data_graph->getVerticesCount();
    ui max_candidates_num = candidates_count[0];

    for (ui i = 1; i < query_vertices_num; ++i) {
        VertexID cur_vertex = i;
        ui cur_candidate_num = candidates_count[cur_vertex];

        if (cur_candidate_num > max_candidates_num) {
            max_candidates_num = cur_candidate_num;
        }
    }

    idx.resize(query_vertices_num);
    idx_count.resize(query_vertices_num);
    embedding.resize(query_vertices_num);
    idx_embedding.resize(query_vertices_num);
    visited_vertices.resize(data_vertices_num);
    temp_buffer.resize(max_candidates_num);
    valid_candidate_idx.resize(query_vertices_num);
    for (ui i = 0; i < query_vertices_num; ++i) {
        valid_candidate_idx[i].resize(max_candidates_num);
    }

    std::fill(visited_vertices.begin(), visited_vertices.begin() + data_vertices_num, false);
}

void EvaluateQuery::generateValidCandidateIndex(const graph_ptr data_graph, ui depth, UIntArray &embedding, UIntArray &idx_embedding,
                                                UIntArray &idx_count, UIntMatrix &valid_candidate_index, EdgesPtrMatrix &edge_matrix,
                                                BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order, UIntArray &pivot,
                                                UIntMatrix &candidates) {
    VertexID u = order[depth];
    VertexID pivot_vertex = pivot[depth];
    ui idx_id = idx_embedding[pivot_vertex];
    Edges &edge = *edge_matrix[pivot_vertex][u];
    ui count = edge.offset_[idx_id + 1] - edge.offset_[idx_id];
    ui *candidate_idx = &edge.edge_[0] + edge.offset_[idx_id];

    ui valid_candidate_index_count = 0;

    if (bn_cnt[depth] == 0) {
        for (ui i = 0; i < count; ++i) {
            ui temp_idx = candidate_idx[i];
            VertexID temp_v = candidates[u][temp_idx];

            if (!visited_vertices[temp_v])
                valid_candidate_index[depth][valid_candidate_index_count++] = temp_idx;
        }
    } else {
        for (ui i = 0; i < count; ++i) {
            ui temp_idx = candidate_idx[i];
            VertexID temp_v = candidates[u][temp_idx];

            if (!visited_vertices[temp_v]) {
                bool valid = true;

                for (ui j = 0; j < bn_cnt[depth]; ++j) {
                    VertexID u_bn = bn[depth][j];
                    VertexID u_bn_v = embedding[u_bn];

                    if (!data_graph->checkEdgeExistence(temp_v, u_bn_v)) {
                        valid = false;
                        break;
                    }
                }

                if (valid)
                    valid_candidate_index[depth][valid_candidate_index_count++] = temp_idx;
            }
        }
    }

    idx_count[depth] = valid_candidate_index_count;
}

void EvaluateQuery::releaseBuffer(ui query_vertices_num, UIntArray &idx, UIntArray &idx_count, UIntArray &embedding, UIntArray &idx_embedding,
                                  UIntArray &temp_buffer, UIntMatrix &valid_candidate_idx, BoolArray &visited_vertices, UIntMatrix &bn,
                                  UIntArray &bn_count) {

}

size_t
EvaluateQuery::LFTJ(const graph_ptr data_graph, const graph_ptr query_graph, EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates,
                    UIntArray &candidates_count,
                    UIntArray &order, size_t output_limit_num, size_t &call_count) {

#ifdef DISTRIBUTION
    distribution_count_ = new size_t[data_graph->getVerticesCount()];
    memset(distribution_count_, 0, data_graph->getVerticesCount() * sizeof(size_t));
    size_t* begin_count = new size_t[query_graph->getVerticesCount()];
    memset(begin_count, 0, query_graph->getVerticesCount() * sizeof(size_t));
#endif

    // Generate bn.
    UIntMatrix bn;
    UIntArray bn_count;
    generateBN(query_graph, order, bn, bn_count);

    // Allocate the memory buffer.
    UIntArray idx;
    UIntArray idx_count;
    UIntArray embedding;
    UIntArray idx_embedding;
    UIntArray temp_buffer;
    UIntMatrix valid_candidate_idx;
    BoolArray visited_vertices;
    allocateBuffer(data_graph, query_graph, candidates_count, idx, idx_count, embedding, idx_embedding,
                   temp_buffer, valid_candidate_idx, visited_vertices);
    std::vector<bool> found_embedding(query_graph->getVerticesCount(), false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    size_t embedding_cnt = 0;
    int cur_depth = 0;
    int max_depth = query_graph->getVerticesCount();
    VertexID start_vertex = order[0];

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    total_count[cur_depth] = candidates_count[start_vertex];

    for (ui i = 0; i < idx_count[cur_depth]; ++i) {
        valid_candidate_idx[cur_depth][i] = i;
    }

#ifdef ENABLE_FAILING_SET
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> ancestors;
    computeAncestor(query_graph, bn, bn_count, order, ancestors);
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> vec_failing_set(max_depth);
    std::unordered_map<VertexID, VertexID> reverse_embedding;
    reverse_embedding.reserve(MAXIMUM_QUERY_GRAPH_SIZE * 2);
#endif

#ifdef SPECTRUM
    exit_ = false;
#endif

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            ui valid_idx = valid_candidate_idx[cur_depth][idx[cur_depth]];
            VertexID u = order[cur_depth];
            VertexID v = candidates[u][valid_idx];

            if (visited_vertices[v]) {
                idx[cur_depth] += 1;
#ifdef ENABLE_FAILING_SET
                vec_failing_set[cur_depth] = ancestors[u];
                vec_failing_set[cur_depth] |= ancestors[reverse_embedding[v]];
                vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
#endif
                continue;
            }

            embedding[u] = v;
            idx_embedding[u] = valid_idx;
            visited_vertices[v] = true;
            idx[cur_depth] += 1;

#ifdef DISTRIBUTION
            begin_count[cur_depth] = embedding_cnt;
            // printf("Cur Depth: %d, v: %u, begin: %zu\n", cur_depth, v, embedding_cnt);
#endif

#ifdef ENABLE_FAILING_SET
            reverse_embedding[v] = u;
#endif

            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                found_embedding[cur_depth] = true;
                suff::FilterManager::AddMatch(cur_depth, embedding);
                visited_vertices[v] = false;

#ifdef DISTRIBUTION
                distribution_count_[v] += 1;
#endif

#ifdef ENABLE_FAILING_SET
                reverse_embedding.erase(embedding[u]);
                vec_failing_set[cur_depth].set();
                vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
#endif
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
            } else
#ifdef SUFF
                if (suff::FilterManager::PassMatch(cur_depth, embedding))
#endif
             {
                call_count += 1;
                cur_depth += 1;

                idx[cur_depth] = 0;
                found_embedding[cur_depth] = false;
                total_count[cur_depth] += 1;
                generateValidCandidateIndex(cur_depth, idx_embedding, idx_count, valid_candidate_idx, edge_matrix, bn,
                                            bn_count, order, temp_buffer);

#ifdef ENABLE_FAILING_SET
                if (idx_count[cur_depth] == 0) {
                    vec_failing_set[cur_depth - 1] = ancestors[order[cur_depth]];
                } else {
                    vec_failing_set[cur_depth - 1].reset();
                }
#endif
            }
#ifdef SUFF
             else {
                visited_vertices[v] = false;
            }
#endif
        }

#ifdef SPECTRUM
        if (exit_) {
            goto EXIT;
        }
#endif

        fail_count[cur_depth] += !found_embedding[cur_depth];
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else {
            found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
            VertexID u = order[cur_depth];
#ifdef SUFF
            if (found_embedding[cur_depth+1]) {
                suff::FilterManager::AddMatch(cur_depth, embedding);
            }
#endif
            
#ifdef ENABLE_FAILING_SET
            reverse_embedding.erase(embedding[u]);
            if (cur_depth != 0) {
                if (!vec_failing_set[cur_depth].test(u)) {
                    vec_failing_set[cur_depth - 1] = vec_failing_set[cur_depth];
                    idx[cur_depth] = idx_count[cur_depth];
                } else {
                    vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
                }
            }
#endif
            visited_vertices[embedding[u]] = false;

#ifdef DISTRIBUTION
            distribution_count_[embedding[u]] += embedding_cnt - begin_count[cur_depth];
            // printf("Cur Depth: %d, v: %u, begin: %zu, end: %zu\n", cur_depth, embedding[u], begin_count[cur_depth], embedding_cnt);
#endif
        }
    }

    // Release the buffer.

#ifdef DISTRIBUTION
    if (embedding_cnt >= output_limit_num) {
        for (int i = 0; i < max_depth - 1; ++i) {
            ui v = embedding[order[i]];
            distribution_count_[v] += embedding_cnt - begin_count[i];
        }
    }
    delete[] begin_count;
#endif

    EXIT:
    releaseBuffer(max_depth, idx, idx_count, embedding, idx_embedding, temp_buffer, valid_candidate_idx,
                  visited_vertices,
                  bn, bn_count);

#if ENABLE_QFLITER == 1
    delete[] temp_bsr_base1_;
    delete[] temp_bsr_base2_;
    delete[] temp_bsr_state1_;
    delete[] temp_bsr_state2_;

    for (ui i = 0; i < max_depth; ++i) {
        for (ui j = 0; j < max_depth; ++j) {
//            delete qfliter_bsr_graph_[i][j];
        }
        delete[] qfliter_bsr_graph_[i];
    }
    delete[] qfliter_bsr_graph_;
#endif

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void EvaluateQuery::generateValidCandidateIndex(ui depth, UIntArray &idx_embedding, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                                EdgesPtrMatrix &edge_matrix, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order,
                                                UIntArray &temp_buffer) {
    VertexID u = order[depth];
    VertexID previous_bn = bn[depth][0];
    ui previous_index_id = idx_embedding[previous_bn];
    ui valid_candidates_count = 0;

#if ENABLE_QFLITER == 1
    BSRGraph &bsr_graph = *qfliter_bsr_graph_[previous_bn][u];
    BSRSet &bsr_set = bsr_graph.bsrs[previous_index_id];

    if (bsr_set.size_ != 0){
        offline_bsr_trans_uint(bsr_set.base_, bsr_set.states_, bsr_set.size_,
                               (int *) valid_candidate_index[depth]);
        // represent bsr size
        valid_candidates_count = bsr_set.size_;
    }

    if (bn_cnt[depth] > 0) {
        if (temp_bsr_base1_ == nullptr) { temp_bsr_base1_ = new int[1024 * 1024]; }
        if (temp_bsr_state1_ == nullptr) { temp_bsr_state1_ = new int[1024 * 1024]; }
        if (temp_bsr_base2_ == nullptr) { temp_bsr_base2_ = new int[1024 * 1024]; }
        if (temp_bsr_state2_ == nullptr) { temp_bsr_state2_ = new int[1024 * 1024]; }
        int *res_base_ = temp_bsr_base1_;
        int *res_state_ = temp_bsr_state1_;
        int *input_base_ = temp_bsr_base2_;
        int *input_state_ = temp_bsr_state2_;

        memcpy(input_base_, bsr_set.base_, sizeof(int) * bsr_set.size_);
        memcpy(input_state_, bsr_set.states_, sizeof(int) * bsr_set.size_);

        for (ui i = 1; i < bn_cnt[depth]; ++i) {
            VertexID current_bn = bn[depth][i];
            ui current_index_id = idx_embedding[current_bn];
            BSRGraph &cur_bsr_graph = *qfliter_bsr_graph_[current_bn][u];
            BSRSet &cur_bsr_set = cur_bsr_graph.bsrs[current_index_id];

            if (valid_candidates_count == 0 || cur_bsr_set.size_ == 0) {
                valid_candidates_count = 0;
                break;
            }

            valid_candidates_count = intersect_qfilter_bsr_b4_v2(cur_bsr_set.base_, cur_bsr_set.states_,
                                                                 cur_bsr_set.size_,
                                                                 input_base_, input_state_, valid_candidates_count,
                                                                 res_base_, res_state_);

            swap(res_base_, input_base_);
            swap(res_state_, input_state_);
        }

        if (valid_candidates_count != 0) {
            valid_candidates_count = offline_bsr_trans_uint(input_base_, input_state_, valid_candidates_count,
                                                            (int *) valid_candidate_index[depth]);
        }
    }

    idx_count[depth] = valid_candidates_count;

    // Debugging.
#ifdef YCHE_DEBUG
    Edges &previous_edge = *edge_matrix[previous_bn][u];

    auto gt_valid_candidates_count =
            previous_edge.offset_[previous_index_id + 1] - previous_edge.offset_[previous_index_id];
    ui *previous_candidates = previous_edge.edge_ + previous_edge.offset_[previous_index_id];
    ui *gt_valid_candidate_index = new ui[1024 * 1024];
    memcpy(gt_valid_candidate_index, previous_candidates, gt_valid_candidates_count * sizeof(ui));

    ui temp_count;
    for (ui i = 1; i < bn_cnt[depth]; ++i) {
        VertexID current_bn = bn[depth][i];
        Edges &current_edge = *edge_matrix[current_bn][u];
        ui current_index_id = idx_embedding[current_bn];

        ui current_candidates_count =
                current_edge.offset_[current_index_id + 1] - current_edge.offset_[current_index_id];
        ui *current_candidates = current_edge.edge_ + current_edge.offset_[current_index_id];

        ComputeSetIntersection::ComputeCandidates(current_candidates, current_candidates_count,
                                                  gt_valid_candidate_index, gt_valid_candidates_count, temp_buffer,
                                                  temp_count);

        std::swap(temp_buffer, gt_valid_candidate_index);
        gt_valid_candidates_count = temp_count;
    }
    assert(valid_candidates_count == gt_valid_candidates_count);

    cout << "Ret, Level:" << bn_cnt[depth] << ", BSR:"
         << pretty_print_array(valid_candidate_index[depth], valid_candidates_count)
         << "; GT: " << pretty_print_array(gt_valid_candidate_index, gt_valid_candidates_count) << "\n";

    for (auto i = 0; i < valid_candidates_count; i++) {
        assert(gt_valid_candidate_index[i] == valid_candidate_index[depth][i]);
    }
    delete[] gt_valid_candidate_index;
#endif
#else
    Edges& previous_edge = *edge_matrix[previous_bn][u];

    valid_candidates_count = previous_edge.offset_[previous_index_id + 1] - previous_edge.offset_[previous_index_id];
    auto previous_candidates = previous_edge.edge_.begin() + previous_edge.offset_[previous_index_id];

    // memcpy(valid_candidate_index[depth], previous_candidates, valid_candidates_count * sizeof(ui));
    valid_candidate_index[depth].assign(previous_candidates, previous_candidates + valid_candidates_count);

    ui temp_count;
    for (ui i = 1; i < bn_cnt[depth]; ++i) {
        VertexID current_bn = bn[depth][i];
        Edges& current_edge = *edge_matrix[current_bn][u];
        ui current_index_id = idx_embedding[current_bn];

        ui current_candidates_count = current_edge.offset_[current_index_id + 1] - current_edge.offset_[current_index_id];
        ui* current_candidates = &current_edge.edge_[0] + current_edge.offset_[current_index_id];

        ComputeSetIntersection::ComputeCandidates(current_candidates, current_candidates_count, &valid_candidate_index[depth][0], valid_candidates_count,
                        &temp_buffer[0], temp_count);

        valid_candidate_index[depth].assign(temp_buffer.begin(), temp_buffer.begin() + temp_count);
        // std::swap(temp_buffer, valid_candidate_index[depth]);
        valid_candidates_count = temp_count;
    }

    idx_count[depth] = valid_candidates_count;
#endif
}

size_t EvaluateQuery::exploreGraphQLStyle(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                          UIntArray &candidates_count, UIntArray &order,
                                          size_t output_limit_num, size_t &call_count) {
    size_t embedding_cnt = 0;
    int cur_depth = 0;
    ui max_depth = query_graph->getVerticesCount();
    VertexID start_vertex = order[0];

    // Generate the bn.
    UIntMatrix bn;
    UIntArray bn_count;

    bn.resize(max_depth);
    for (ui i = 0; i < max_depth; ++i) {
        bn[i].resize(max_depth);
    }

    bn_count.assign(max_depth, 0);
    // std::fill(bn_count, bn_count + max_depth, 0);

    std::vector<bool> visited_query_vertices(max_depth, false);
    visited_query_vertices[start_vertex] = true;
    for (ui i = 1; i < max_depth; ++i) {
        VertexID cur_vertex = order[i];
        ui nbr_cnt;
        const VertexID *nbrs = query_graph->getVertexNeighbors(cur_vertex, nbr_cnt);

        for (ui j = 0; j < nbr_cnt; ++j) {
            VertexID nbr = nbrs[j];

            if (visited_query_vertices[nbr]) {
                bn[i][bn_count[i]++] = nbr;
            }
        }

        visited_query_vertices[cur_vertex] = true;
    }

    // Allocate the memory buffer.
    UIntArray idx(max_depth);
    UIntArray idx_count(max_depth);
    UIntArray embedding(max_depth);
    UIntMatrix valid_candidate(max_depth);
    BoolArray visited_vertices(data_graph->getVerticesCount(), false);

    std::vector<bool> found_embedding(max_depth, false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    for (ui i = 0; i < max_depth; ++i) {
        VertexID cur_vertex = order[i];
        ui max_candidate_count = candidates_count[cur_vertex];
        valid_candidate[i].resize(max_candidate_count);
    }

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    total_count[cur_depth] = candidates_count[start_vertex];

    std::copy(candidates[start_vertex].begin(), candidates[start_vertex].begin() + candidates_count[start_vertex],
              valid_candidate[cur_depth].begin());

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            VertexID u = order[cur_depth];
            VertexID v = valid_candidate[cur_depth][idx[cur_depth]];
            embedding[u] = v;
            visited_vertices[v] = true;
            idx[cur_depth] += 1;

            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                found_embedding[cur_depth] = true;
                suff::FilterManager::AddMatch(cur_depth, embedding);

                visited_vertices[v] = false;
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
            } else
#ifdef SUFF
                if (suff::FilterManager::PassMatch(cur_depth, embedding))
#endif
             {
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                found_embedding[cur_depth] = false;
                total_count[cur_depth] += 1;
                generateValidCandidates(data_graph, cur_depth, embedding, idx_count, valid_candidate,
                                        visited_vertices, bn, bn_count, order, candidates, candidates_count);
            }
#ifdef SUFF
             else {
                visited_vertices[v] = false;
            }
#endif
        }

        fail_count[cur_depth] += !found_embedding[cur_depth];
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else {
            found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
#ifdef SUFF
            if (found_embedding[cur_depth+1]) {
                suff::FilterManager::AddMatch(cur_depth, embedding);
            }
#endif
            visited_vertices[embedding[order[cur_depth]]] = false;
        }
    }

    // Release the buffer.
    EXIT:

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void EvaluateQuery::generateValidCandidates(const graph_ptr data_graph, ui depth, UIntArray &embedding, UIntArray &idx_count,
                                            UIntMatrix &valid_candidate, BoolArray &visited_vertices, UIntMatrix &bn, UIntArray &bn_cnt,
                                            UIntArray &order, UIntMatrix &candidates, UIntArray &candidates_count) {
    VertexID u = order[depth];

    idx_count[depth] = 0;

    for (ui i = 0; i < candidates_count[u]; ++i) {
        VertexID v = candidates[u][i];

        if (!visited_vertices[v]) {
            bool valid = true;

            for (ui j = 0; j < bn_cnt[depth]; ++j) {
                VertexID u_nbr = bn[depth][j];
                VertexID u_nbr_v = embedding[u_nbr];

                if (!data_graph->checkEdgeExistence(v, u_nbr_v)) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                valid_candidate[depth][idx_count[depth]++] = v;
            }
        }
    }
}

size_t EvaluateQuery::exploreQuickSIStyle(const graph_ptr data_graph, const graph_ptr query_graph, UIntMatrix &candidates,
                                          UIntArray &candidates_count, UIntArray &order,
                                          UIntArray &pivot, size_t output_limit_num, size_t &call_count) {
    size_t embedding_cnt = 0;
    int cur_depth = 0;
    ui max_depth = query_graph->getVerticesCount();
    VertexID start_vertex = order[0];

    // Generate the bn.
    UIntMatrix bn;
    UIntArray bn_count;
    generateBN(query_graph, order, pivot, bn, bn_count);

    // Allocate the memory buffer.
    UIntArray idx(max_depth);
    UIntArray idx_count(max_depth);
    UIntArray embedding(max_depth);
    UIntMatrix valid_candidate(max_depth);
    BoolArray visited_vertices(data_graph->getVerticesCount(), false);

    std::vector<bool> found_embedding(query_graph->getVerticesCount(), false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    ui max_candidate_count = data_graph->getGraphMaxLabelFrequency();
    for (ui i = 0; i < max_depth; ++i) {
        valid_candidate[i].resize(max_candidate_count);
    }

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    total_count[cur_depth] = candidates_count[start_vertex];
    std::copy(candidates[start_vertex].begin(), candidates[start_vertex].begin() + candidates_count[start_vertex],
              valid_candidate[cur_depth].begin());

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            VertexID u = order[cur_depth];
            VertexID v = valid_candidate[cur_depth][idx[cur_depth]];
            embedding[u] = v;
            visited_vertices[v] = true;
            idx[cur_depth] += 1;

            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                found_embedding[cur_depth] = true;
                suff::FilterManager::AddMatch(cur_depth, embedding);

                visited_vertices[v] = false;
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
            } else
#ifdef SUFF
                if (suff::FilterManager::PassMatch(cur_depth, embedding))
#endif
             {
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                found_embedding[cur_depth] = false;
                total_count[cur_depth] += 1;
                generateValidCandidates(query_graph, data_graph, cur_depth, embedding, idx_count, valid_candidate,
                                        visited_vertices, bn, bn_count, order, pivot);
            }
#ifdef SUFF
             else {
                visited_vertices[v] = false;
            }
#endif
        }

        fail_count[cur_depth] += !found_embedding[cur_depth];
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else {
            found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
#ifdef SUFF
            if (found_embedding[cur_depth+1]) {
                suff::FilterManager::AddMatch(cur_depth, embedding);
            }
#endif
            visited_vertices[embedding[order[cur_depth]]] = false;
        }
    }

    // Release the buffer.
    EXIT:

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void EvaluateQuery::generateValidCandidates(const graph_ptr query_graph, const graph_ptr data_graph, ui depth, UIntArray &embedding,
                                            UIntArray &idx_count, UIntMatrix &valid_candidate, BoolArray &visited_vertices, UIntMatrix &bn,
                                            UIntArray &bn_cnt, UIntArray &order, UIntArray &pivot) {
    VertexID u = order[depth];
    LabelID u_label = query_graph->getVertexLabel(u);
    ui u_degree = query_graph->getVertexDegree(u);

    idx_count[depth] = 0;

    VertexID p = embedding[pivot[depth]];
    ui nbr_cnt;
    const VertexID *nbrs = data_graph->getVertexNeighbors(p, nbr_cnt);

    for (ui i = 0; i < nbr_cnt; ++i) {
        VertexID v = nbrs[i];

        if (!visited_vertices[v] && u_label == data_graph->getVertexLabel(v) &&
            u_degree <= data_graph->getVertexDegree(v)) {
            bool valid = true;

            for (ui j = 0; j < bn_cnt[depth]; ++j) {
                VertexID u_nbr = bn[depth][j];
                VertexID u_nbr_v = embedding[u_nbr];

                if (!data_graph->checkEdgeExistence(v, u_nbr_v)) {
                    valid = false;
                    break;
                }
            }

            if (valid) {
                valid_candidate[depth][idx_count[depth]++] = v;
            }
        }
    }
}

size_t EvaluateQuery::exploreDPisoStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                        EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates, UIntArray &candidates_count,
                                        UIntMatrix &weight_array, UIntArray &order, size_t output_limit_num,
                                        size_t &call_count) {
    ui max_depth = query_graph->getVerticesCount();

    UIntArray extendable(max_depth);
    for (ui i = 0; i < max_depth; ++i) {
        extendable[i] = tree[i].bn_count_;
    }

    // Generate backward neighbors.
    UIntMatrix bn;
    UIntArray bn_count;
    generateBN(query_graph, order, bn, bn_count);

    // Allocate the memory buffer.
    UIntArray idx;
    UIntArray idx_count;
    UIntArray embedding;
    UIntArray idx_embedding;
    UIntArray temp_buffer;
    UIntMatrix valid_candidate_idx;
    BoolArray visited_vertices;
    allocateBuffer(data_graph, query_graph, candidates_count, idx, idx_count, embedding, idx_embedding,
                   temp_buffer, valid_candidate_idx, visited_vertices);
    std::vector<bool> found_embedding(query_graph->getVerticesCount(), false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    // Evaluate the query.
    size_t embedding_cnt = 0;
    int cur_depth = 0;

#ifdef ENABLE_FAILING_SET
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> ancestors;
    computeAncestor(query_graph, tree, order, ancestors);
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> vec_failing_set(max_depth);
    std::unordered_map<VertexID, VertexID> reverse_embedding;
    reverse_embedding.reserve(MAXIMUM_QUERY_GRAPH_SIZE * 2);
#endif

    VertexID start_vertex = order[0];
    std::vector<dpiso_min_pq> vec_rank_queue;
    total_count[cur_depth] = candidates_count[start_vertex];

    for (ui i = 0; i < candidates_count[start_vertex]; ++i) {
        VertexID v = candidates[start_vertex][i];
        embedding[start_vertex] = v;
#ifdef SUFF
                // if (!suff::FilterManager::PassMatch(cur_depth, start_vertex, v)) {
                //     continue;
                // }
#endif
        idx_embedding[start_vertex] = i;
        visited_vertices[v] = true;

#ifdef ENABLE_FAILING_SET
        reverse_embedding[v] = start_vertex;
#endif
        vec_rank_queue.emplace_back(dpiso_min_pq(extendable_vertex_compare));
        updateExtendableVertex(idx_embedding, idx_count, valid_candidate_idx, edge_matrix, temp_buffer, weight_array,
                               tree, start_vertex, extendable,
                               vec_rank_queue, query_graph);

        VertexID u = vec_rank_queue.back().top().first.first;
        vec_rank_queue.back().pop();

#ifdef ENABLE_FAILING_SET
        if (idx_count[u] == 0) {
            vec_failing_set[cur_depth] = ancestors[u];
        } else {
            vec_failing_set[cur_depth].reset();
        }
#endif

        call_count += 1;
        cur_depth += 1;
        found_embedding[cur_depth] = false;
        total_count[cur_depth] += 1;
        order[cur_depth] = u;
        idx[u] = 0;
        while (cur_depth > 0) {
            while (idx[u] < idx_count[u]) {
                ui valid_idx = valid_candidate_idx[u][idx[u]];
                v = candidates[u][valid_idx];

                if (visited_vertices[v]) {
                    idx[u] += 1;
#ifdef ENABLE_FAILING_SET
                    vec_failing_set[cur_depth] = ancestors[u];
                    vec_failing_set[cur_depth] |= ancestors[reverse_embedding[v]];
                    vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
#endif
                    continue;
                }
                embedding[u] = v;
                idx_embedding[u] = valid_idx;
                visited_vertices[v] = true;
                idx[u] += 1;

#ifdef ENABLE_FAILING_SET
                reverse_embedding[v] = u;
#endif

                if (cur_depth == max_depth - 1) {
                    embedding_cnt += 1;
                    found_embedding[cur_depth] = true;
#ifdef SUFF
                    // suff::FilterManager::AddMatch(cur_depth, u, v);
#endif
                    visited_vertices[v] = false;
#ifdef ENABLE_FAILING_SET
                    reverse_embedding.erase(embedding[u]);
                    vec_failing_set[cur_depth].set();
                    vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];

#endif
                    if (embedding_cnt >= output_limit_num) {
                        goto EXIT;
                    }
                } else
#ifdef SUFF
                // if (suff::FilterManager::PassMatch(cur_depth, u, v))
#endif
                 {
                    call_count += 1;
                    cur_depth += 1;
                    found_embedding[cur_depth] = false;
                    total_count[cur_depth] += 1;
                    vec_rank_queue.emplace_back(vec_rank_queue.back());
                    updateExtendableVertex(idx_embedding, idx_count, valid_candidate_idx, edge_matrix, temp_buffer,
                                           weight_array, tree, u, extendable,
                                           vec_rank_queue, query_graph);

                    u = vec_rank_queue.back().top().first.first;
                    vec_rank_queue.back().pop();
                    idx[u] = 0;
                    order[cur_depth] = u;

#ifdef ENABLE_FAILING_SET
                    if (idx_count[u] == 0) {
                        vec_failing_set[cur_depth - 1] = ancestors[u];
                    } else {
                        vec_failing_set[cur_depth - 1].reset();
                    }
#endif
                }
#ifdef SUFF
                // else {
                    // visited_vertices[v] = false;
                // }
#endif
            }

            fail_count[cur_depth] += !found_embedding[cur_depth];
            cur_depth -= 1;
            vec_rank_queue.pop_back();
            u = order[cur_depth];
            visited_vertices[embedding[u]] = false;
            restoreExtendableVertex(tree, u, extendable);

            if (cur_depth >= 0) {
                found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
#ifdef SUFF
                // if (found_embedding[cur_depth+1]) {
                //     suff::FilterManager::AddMatch(cur_depth, u, embedding[u]);
                // }
#endif
            }

#ifdef ENABLE_FAILING_SET
            reverse_embedding.erase(embedding[u]);
            if (cur_depth != 0) {
                if (!vec_failing_set[cur_depth].test(u)) {
                    vec_failing_set[cur_depth - 1] = vec_failing_set[cur_depth];
                    idx[u] = idx_count[u];
                } else {
                    vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
                }
            }
#endif
        }
    }

    // Release the buffer.
    EXIT:
    releaseBuffer(max_depth, idx, idx_count, embedding, idx_embedding, temp_buffer, valid_candidate_idx,
                  visited_vertices,
                  bn, bn_count);

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void EvaluateQuery::updateExtendableVertex(UIntArray &idx_embedding, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                           EdgesPtrMatrix &edge_matrix, UIntArray &temp_buffer, UIntMatrix &weight_array,
                                           TreeNodeArray &tree, VertexID mapped_vertex, UIntArray &extendable,
                                           std::vector<dpiso_min_pq> &vec_rank_queue, const graph_ptr query_graph) {
    TreeNode &node = tree[mapped_vertex];
    for (ui i = 0; i < node.fn_count_; ++i) {
        VertexID u = node.fn_[i];
        extendable[u] -= 1;
        if (extendable[u] == 0) {
            generateValidCandidateIndex(u, idx_embedding, idx_count, valid_candidate_index[u], edge_matrix, tree[u].bn_,
                                        tree[u].bn_count_, temp_buffer);

            ui weight = 0;
            for (ui j = 0; j < idx_count[u]; ++j) {
                ui idx = valid_candidate_index[u][j];
                weight += weight_array[u][idx];
            }
            vec_rank_queue.back().emplace(std::make_pair(std::make_pair(u, query_graph->getVertexDegree(u)), weight));
        }
    }
}

void EvaluateQuery::restoreExtendableVertex(TreeNodeArray &tree, VertexID unmapped_vertex, UIntArray &extendable) {
    TreeNode &node = tree[unmapped_vertex];
    for (ui i = 0; i < node.fn_count_; ++i) {
        VertexID u = node.fn_[i];
        extendable[u] += 1;
    }
}

void
EvaluateQuery::generateValidCandidateIndex(VertexID u, UIntArray &idx_embedding, UIntArray &idx_count, UIntArray &valid_candidate_index,
                                           EdgesPtrMatrix &edge_matrix, UIntArray &bn, ui bn_cnt, UIntArray &temp_buffer) {
    VertexID previous_bn = bn[0];
    Edges &previous_edge = *edge_matrix[previous_bn][u];
    ui previous_index_id = idx_embedding[previous_bn];

    ui previous_candidates_count =
            previous_edge.offset_[previous_index_id + 1] - previous_edge.offset_[previous_index_id];
    ui *previous_candidates = &previous_edge.edge_[0] + previous_edge.offset_[previous_index_id];

    ui valid_candidates_count = 0;
    for (ui i = 0; i < previous_candidates_count; ++i) {
        valid_candidate_index[valid_candidates_count++] = previous_candidates[i];
    }

    ui temp_count;
    for (ui i = 1; i < bn_cnt; ++i) {
        VertexID current_bn = bn[i];
        Edges &current_edge = *edge_matrix[current_bn][u];
        ui current_index_id = idx_embedding[current_bn];

        ui current_candidates_count =
                current_edge.offset_[current_index_id + 1] - current_edge.offset_[current_index_id];
        ui *current_candidates = &current_edge.edge_[0] + current_edge.offset_[current_index_id];

        ComputeSetIntersection::ComputeCandidates(current_candidates, current_candidates_count, &valid_candidate_index[0],
                                                  valid_candidates_count,
                                                  &temp_buffer[0], temp_count);

        valid_candidate_index.assign(temp_buffer.begin(), temp_buffer.begin() + temp_count);
        // std::swap(temp_buffer, valid_candidate_index);
        valid_candidates_count = temp_count;
    }

    idx_count[u] = valid_candidates_count;
}

void EvaluateQuery::computeAncestor(const graph_ptr query_graph, TreeNodeArray &tree, UIntArray &order,
                                    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ancestors.resize(query_vertices_num);

    // Compute the ancestor in the top-down order.
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID u = order[i];
        TreeNode &u_node = tree[u];
        ancestors[u].set(u);
        for (ui j = 0; j < u_node.bn_count_; ++j) {
            VertexID u_bn = u_node.bn_[j];
            ancestors[u] |= ancestors[u_bn];
        }
    }
}

size_t EvaluateQuery::exploreDPisoRecursiveStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree,
                                                 EdgesPtrMatrix &edge_matrix, UIntMatrix &candidates, UIntArray &candidates_count,
                                                 UIntMatrix &weight_array, UIntArray &order, size_t output_limit_num,
                                                 size_t &call_count) {
    ui max_depth = query_graph->getVerticesCount();

    UIntArray extendable(max_depth);
    for (ui i = 0; i < max_depth; ++i) {
        extendable[i] = tree[i].bn_count_;
    }

    // Generate backward neighbors.
    UIntMatrix bn;
    UIntArray bn_count;
    generateBN(query_graph, order, bn, bn_count);

    // Allocate the memory buffer.
    UIntArray idx;
    UIntArray idx_count;
    UIntArray embedding;
    UIntArray idx_embedding;
    UIntArray temp_buffer;
    UIntMatrix valid_candidate_idx;
    BoolArray visited_vertices;
    allocateBuffer(data_graph, query_graph, candidates_count, idx, idx_count, embedding, idx_embedding,
                   temp_buffer, valid_candidate_idx, visited_vertices);

    // Evaluate the query.
    size_t embedding_cnt = 0;

    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> ancestors;
    computeAncestor(query_graph, tree, order, ancestors);

    std::unordered_map<VertexID, VertexID> reverse_embedding;
    reverse_embedding.reserve(MAXIMUM_QUERY_GRAPH_SIZE * 2);
    VertexID start_vertex = order[0];

    for (ui i = 0; i < candidates_count[start_vertex]; ++i) {
        VertexID v = candidates[start_vertex][i];
        embedding[start_vertex] = v;
        idx_embedding[start_vertex] = i;
        visited_vertices[v] = true;
        reverse_embedding[v] = start_vertex;
        call_count += 1;

        exploreDPisoBacktrack(max_depth, 1, start_vertex, tree, idx_embedding, embedding, reverse_embedding,
                              visited_vertices, idx_count, valid_candidate_idx, edge_matrix,
                              ancestors, dpiso_min_pq(extendable_vertex_compare), weight_array, temp_buffer, extendable,
                              candidates, embedding_cnt,
                              call_count, nullptr);

        visited_vertices[v] = false;
        reverse_embedding.erase(v);
    }

    // Release the buffer.
    releaseBuffer(max_depth, idx, idx_count, embedding, idx_embedding, temp_buffer, valid_candidate_idx,
                  visited_vertices,
                  bn, bn_count);

    return embedding_cnt;
}

std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>
EvaluateQuery::exploreDPisoBacktrack(ui max_depth, ui depth, VertexID mapped_vertex, TreeNodeArray &tree, UIntArray &idx_embedding,
                                     UIntArray &embedding, std::unordered_map<VertexID, VertexID> &reverse_embedding,
                                     BoolArray &visited_vertices, UIntArray &idx_count, UIntMatrix &valid_candidate_index,
                                     EdgesPtrMatrix &edge_matrix,
                                     std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors,
                                     dpiso_min_pq rank_queue, UIntMatrix &weight_array, UIntArray &temp_buffer, UIntArray &extendable,
                                     UIntMatrix &candidates, size_t &embedding_count, size_t &call_count,
                                     const graph_ptr query_graph) {
    // Compute extendable vertex.
    TreeNode &node = tree[mapped_vertex];
    for (ui i = 0; i < node.fn_count_; ++i) {
        VertexID u = node.fn_[i];
        extendable[u] -= 1;
        if (extendable[u] == 0) {
            generateValidCandidateIndex(u, idx_embedding, idx_count, valid_candidate_index[u], edge_matrix, tree[u].bn_,
                                        tree[u].bn_count_, temp_buffer);

            ui weight = 0;
            for (ui j = 0; j < idx_count[u]; ++j) {
                ui idx = valid_candidate_index[u][j];
                weight += weight_array[u][idx];
            }
            rank_queue.emplace(std::make_pair(std::make_pair(u, query_graph->getVertexDegree(u)), weight));
        }
    }

    VertexID u = rank_queue.top().first.first;
    rank_queue.pop();

    if (idx_count[u] == 0) {
        restoreExtendableVertex(tree, mapped_vertex, extendable);
        return ancestors[u];
    } else {
        std::bitset<MAXIMUM_QUERY_GRAPH_SIZE> current_fs;
        std::bitset<MAXIMUM_QUERY_GRAPH_SIZE> child_fs;

        for (ui i = 0; i < idx_count[u]; ++i) {
            ui valid_index = valid_candidate_index[u][i];
            VertexID v = candidates[u][valid_index];

            if (!visited_vertices[v]) {
                embedding[u] = v;
                idx_embedding[u] = valid_index;
                visited_vertices[v] = true;
                reverse_embedding[v] = u;
                if (depth != max_depth - 1) {
                    call_count += 1;
                    child_fs = exploreDPisoBacktrack(max_depth, depth + 1, u, tree, idx_embedding, embedding,
                                                     reverse_embedding, visited_vertices, idx_count,
                                                     valid_candidate_index, edge_matrix,
                                                     ancestors, rank_queue, weight_array, temp_buffer, extendable,
                                                     candidates, embedding_count,
                                                     call_count, query_graph);
                } else {
                    embedding_count += 1;
                    child_fs.set();
                }
                visited_vertices[v] = false;
                reverse_embedding.erase(v);

                if (!child_fs.test(u)) {
                    current_fs = child_fs;
                    break;
                }
            } else {
                child_fs.reset();
                child_fs |= ancestors[u];
                child_fs |= ancestors[reverse_embedding[v]];
            }

            current_fs |= child_fs;
        }

        restoreExtendableVertex(tree, mapped_vertex, extendable);
        return current_fs;
    }
}

size_t
EvaluateQuery::exploreCECIStyle(const graph_ptr data_graph, const graph_ptr query_graph, TreeNodeArray &tree, UIntMatrix &candidates,
                                UIntArray &candidates_count,
                                std::vector<std::unordered_map<VertexID, std::vector<VertexID>>> &TE_Candidates,
                                std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates,
                                UIntArray &order, size_t &output_limit_num, size_t &call_count) {

    int max_depth = query_graph->getVerticesCount();
    ui data_vertices_count = data_graph->getVerticesCount();
    // temporal results may be large, use maximum possible size
    ui max_valid_candidates_count = data_graph->getGraphMaxLabelFrequency();
    // Allocate the memory buffer.
    UIntArray idx(max_depth);
    UIntArray idx_count(max_depth);
    UIntArray embedding(max_depth);
    UIntArray temp_buffer(max_valid_candidates_count);
    UIntMatrix valid_candidates(max_depth);
    for (int i = 0; i < max_depth; ++i) {
        valid_candidates[i].resize(max_valid_candidates_count);
    }
    BoolArray visited_vertices(data_vertices_count, false);
    // std::fill(visited_vertices, visited_vertices + data_vertices_count, false);
    std::vector<bool> found_embedding(query_graph->getVerticesCount(), false);
    std::vector<u_int64_t> fail_count(query_graph->getVerticesCount(), 0);
    std::vector<u_int64_t> total_count(query_graph->getVerticesCount(), 0);

    // Evaluate the query.
    size_t embedding_cnt = 0;
    int cur_depth = 0;
    VertexID start_vertex = order[0];

    idx[cur_depth] = 0;
    idx_count[cur_depth] = candidates_count[start_vertex];
    total_count[cur_depth] = candidates_count[start_vertex];
    
    for (ui i = 0; i < idx_count[cur_depth]; ++i) {
        valid_candidates[cur_depth][i] = candidates[start_vertex][i];
    }

#ifdef ENABLE_FAILING_SET
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> ancestors;
    computeAncestor(query_graph, order, ancestors);
    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> vec_failing_set(max_depth);
    std::unordered_map<VertexID, VertexID> reverse_embedding;
    reverse_embedding.reserve(MAXIMUM_QUERY_GRAPH_SIZE * 2);
#endif

    while (true) {
        while (idx[cur_depth] < idx_count[cur_depth]) {
            VertexID u = order[cur_depth];
            VertexID v = valid_candidates[cur_depth][idx[cur_depth]];
            idx[cur_depth] += 1;

            if (visited_vertices[v]) {
#ifdef ENABLE_FAILING_SET
                vec_failing_set[cur_depth] = ancestors[u];
                vec_failing_set[cur_depth] |= ancestors[reverse_embedding[v]];
                vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
#endif
                continue;
            }

            embedding[u] = v;
            visited_vertices[v] = true;

#ifdef ENABLE_FAILING_SET
            reverse_embedding[v] = u;
#endif
            if (cur_depth == max_depth - 1) {
                embedding_cnt += 1;
                found_embedding[cur_depth] = true;

                visited_vertices[v] = false;
#ifdef ENABLE_FAILING_SET
                reverse_embedding.erase(embedding[u]);
                vec_failing_set[cur_depth].set();
                vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
#endif
                if (embedding_cnt >= output_limit_num) {
                    goto EXIT;
                }
            } else
#ifdef SUFF
                if (suff::FilterManager::PassMatch(cur_depth, embedding))
#endif
             {
                call_count += 1;
                cur_depth += 1;
                idx[cur_depth] = 0;
                found_embedding[cur_depth] = false;
                total_count[cur_depth] += 1;
                generateValidCandidates(cur_depth, embedding, idx_count, valid_candidates, order, temp_buffer, tree,
                                        TE_Candidates,
                                        NTE_Candidates);
#ifdef ENABLE_FAILING_SET
                if (idx_count[cur_depth] == 0) {
                    vec_failing_set[cur_depth - 1] = ancestors[order[cur_depth]];
                } else {
                    vec_failing_set[cur_depth - 1].reset();
                }
#endif
            }
#ifdef SUFF
             else {
                visited_vertices[v] = false;
            }
#endif
        }

        fail_count[cur_depth] += !found_embedding[cur_depth];
        cur_depth -= 1;
        if (cur_depth < 0)
            break;
        else {
            found_embedding[cur_depth] = found_embedding[cur_depth] || found_embedding[cur_depth+1];
            VertexID u = order[cur_depth];
#ifdef SUFF
            if (found_embedding[cur_depth+1]) {
                suff::FilterManager::AddMatch(cur_depth, embedding);
            }
#endif
            
#ifdef ENABLE_FAILING_SET
            reverse_embedding.erase(embedding[u]);
            if (cur_depth != 0) {
                if (!vec_failing_set[cur_depth].test(u)) {
                    vec_failing_set[cur_depth - 1] = vec_failing_set[cur_depth];
                    idx[cur_depth] = idx_count[cur_depth];
                } else {
                    vec_failing_set[cur_depth - 1] |= vec_failing_set[cur_depth];
                }
            }
#endif
            visited_vertices[embedding[u]] = false;
        }
    }

    // Release the buffer.
    EXIT:

    std::cout << "fail count: " << fail_count << std::endl;
    std::cout << "total count: " << total_count << std::endl;
    return embedding_cnt;
}

void EvaluateQuery::generateValidCandidates(ui depth, UIntArray &embedding, UIntArray &idx_count, UIntMatrix &valid_candidates, UIntArray &order,
                                            UIntArray &temp_buffer, TreeNodeArray &tree,
                                            std::vector<std::unordered_map<VertexID, std::vector<VertexID>>> &TE_Candidates,
                                            std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> &NTE_Candidates) {

    VertexID u = order[depth];
    // TreeNode &u_node = tree[u];
    idx_count[depth] = 0;
    ui valid_candidates_count = 0;
    {
        VertexID u_p = tree[u].parent_;
        VertexID v_p = embedding[u_p];

        auto iter = TE_Candidates[u].find(v_p);
        if (iter == TE_Candidates[u].end() || iter->second.empty()) {
            return;
        }

        valid_candidates_count = iter->second.size();
        VertexID *v_p_nbrs = iter->second.data();

        for (ui i = 0; i < valid_candidates_count; ++i) {
            valid_candidates[depth][i] = v_p_nbrs[i];
        }
    }
    ui temp_count;
    for (ui i = 0; i < tree[u].bn_count_; ++i) {
        VertexID u_p = tree[u].bn_[i];
        VertexID v_p = embedding[u_p];
        if (NTE_Candidates.size() <= u || NTE_Candidates.at(u).size() <= u_p) {
            continue;
        }
        
        auto iter = NTE_Candidates[u][u_p].find(v_p);
        if (iter == NTE_Candidates[u][u_p].end() || iter->second.empty()) {
            return;
        }

        ui current_candidates_count = iter->second.size();
        UIntArray &current_candidates = iter->second;

        ComputeSetIntersection::ComputeCandidates(&current_candidates[0], current_candidates_count,
                                                  &valid_candidates[depth][0], valid_candidates_count,
                                                  &temp_buffer[0], temp_count);

        valid_candidates[depth].assign(temp_buffer.begin(), temp_buffer.begin() + temp_count);
        // std::swap(temp_buffer, valid_candidates[depth]);
        valid_candidates_count = temp_count;
    }

    idx_count[depth] = valid_candidates_count;
}

void EvaluateQuery::computeAncestor(const graph_ptr query_graph, UIntMatrix &bn, UIntArray &bn_cnt, UIntArray &order,
                                    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ancestors.resize(query_vertices_num);

    // Compute the ancestor in the top-down order.
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID u = order[i];
        ancestors[u].set(u);
        for (ui j = 0; j < bn_cnt[i]; ++j) {
            VertexID u_bn = bn[i][j];
            ancestors[u] |= ancestors[u_bn];
        }
    }
}

void EvaluateQuery::computeAncestor(const graph_ptr query_graph, UIntArray &order,
                                    std::vector<std::bitset<MAXIMUM_QUERY_GRAPH_SIZE>> &ancestors) {
    ui query_vertices_num = query_graph->getVerticesCount();
    ancestors.resize(query_vertices_num);

    // Compute the ancestor in the top-down order.
    for (ui i = 0; i < query_vertices_num; ++i) {
        VertexID u = order[i];
        ancestors[u].set(u);
        for (ui j = 0; j < i; ++j) {
            VertexID u_bn = order[j];
            if (query_graph->checkEdgeExistence(u, u_bn)) {
                ancestors[u] |= ancestors[u_bn];
            }
        }
    }
}


