#pragma once

#include <vector>
#include <numeric>
#include "Filter.hpp"

namespace suff {

    class FilterSelector {
    public:
        // select k best VFilters for each vertex in q
        // pivot records the parent vertex of each vertex
        static std::vector<VFilter> SelectK(const graph_ptr d, const graph_ptr q,
            const std::vector<VertexID> &matching_order, const std::vector<VertexID> &pivot,
            std::vector<VFilter> &vfilters, long k) {
            // remove useless filters
            std::vector<VFilter> _vfilters;
            for (auto &f : vfilters) {
                f.score = score(q, matching_order, f);
                if (f.score > 0) {
                    _vfilters.emplace_back(f);
                }
            }
            std::sort(_vfilters.begin(), _vfilters.end(), [](VFilter &a, VFilter &b){
                return a.score > b.score;
            });
            std::cout << "after pruning, vfilters: " << _vfilters.size() << std::endl;

            std::vector<VFilter> results;
            for (auto i = 0ul; i < q->getVerticesCount(); i++) {
                auto temp = selectKForLevel(q, matching_order, _vfilters, i, k);
                for (auto &f: temp) {
                    results.emplace_back(f);
                }
            }
            
            std::cout << "resulting vfilters: " << results.size() << std::endl;
            return results;
        }

        // select k random VFilters for each vertex in q
        static std::vector<VFilter> SelectRandomK(const graph_ptr d, const graph_ptr q,
            const std::vector<VertexID> &matching_order, const std::vector<VertexID> &pivot,
            std::vector<VFilter> &vfilters, long k) {
            
            std::vector<VFilter> results;
            for (auto i = 0ul; i < q->getVerticesCount(); i++) {
                auto temp = selectRandomKForLevel(q, matching_order, vfilters, i, k);
                for (auto &f: temp) {
                    results.emplace_back(f);
                }
            }
            
            std::cout << "resulting vfilters: " << results.size() << std::endl;
            return results;
        }

    private:
        // estimate #branches at each level
        // not finished
        static std::vector<unsigned long long> estimated_level_branches(const graph_ptr d,
            const graph_ptr q, const std::vector<VertexID> &matching_order, const std::vector<VertexID> &pivot) {
            std::vector<unsigned long long> result;
            for (auto i = 0ul; i < q->getVerticesCount(); i++) {
                ui v_count;
                const ui* vertices = d->getVerticesByLabel(q->getVertexLabel(i), v_count);
                if (i == 0) {  // first level #branches = #vertices in this label
                    result.emplace_back(v_count);
                } else {  // other levels #branches = #last_level_branches * branching_ratio * neighborhood_filter_ratio
                    // branching ratio, which is #edge(label_i, label_i+1) / #vertices(label_i)
                    long double branching_ratio = 0;
                    for (auto j = 0ul; j < v_count; j++) {
                        ui neighbor_count;
                        d->getNeighborsByLabel(vertices[j], q->getVertexLabel(i+1), neighbor_count);
                        branching_ratio += neighbor_count;
                    }
                    branching_ratio /= v_count;
                    // neighborhood filter ratio
                    ui neighbor_count;
                    const ui* neighbors = q->getVertexNeighbors(i, neighbor_count);
                    std::map<LabelID, ui> neighbor_labels;
                    for (auto j = 0ul; j < neighbor_count; j++) {
                        if (neighbors[j] < i - 1) {  // edge(i-1, i) is considered in branching ratio
                            if (neighbor_labels.count(q->getVertexLabel(neighbors[j])) == 0) {
                                neighbor_labels.emplace(q->getVertexLabel(neighbors[j]), 1);
                            } else {
                                neighbor_labels.at(q->getVertexLabel(neighbors[j])) += 1;
                            }
                        }
                    }

                }
                
            }
            return result;
        }

        // assign a score to a filter according to the query
        // the power of a filter comes from the `future` edges
        static double score(const graph_ptr q, const std::vector<VertexID> &matching_order, const VFilter &f) {
            auto pattern = f.filters[0].pattern;
            double score = pattern->getEdgesCount();
            auto &mapping = f.mappings[0];
            std::set<VertexID> seen_vertices;
            for (auto i = 0ul; i <= f.check_level; i++) {
                seen_vertices.emplace(matching_order[i]);
            }
            // enumerate edges seen in the filter's vertices, and for each deduct score by 1
            for (auto v1 = 0ul; v1 < pattern->getVerticesCount(); v1++) {
                for (auto v2 = v1; v2 < pattern->getVerticesCount(); v2++) {
                    if (pattern->checkEdgeExistence(v1, v2)
                     && (seen_vertices.contains(mapping.at(v1)) || seen_vertices.contains(mapping.at(v2)))) {
                        score -= 1;
                    }
                }
            }
            return score;
        }

        static std::vector<VFilter> selectKForLevel(const graph_ptr q, const std::vector<VertexID> &matching_order,
         std::vector<VFilter> &candidates, ui level, long k) {
            std::vector<VFilter> result;
            std::vector<VFilter> _candidates;
            for (auto &f : candidates) {
                if (f.check_level == level) {
                    _candidates.emplace_back(f);
                }
            }

            // initialize utility of each edge to 0
            std::map<std::pair<ui, ui>, double> current_state;  // utility of each edge
            for (auto u : matching_order) {
                for (auto v: matching_order) {
                    if (q->checkEdgeExistence(u, v)) {
                        if (u < v) {
                            current_state.emplace(std::make_pair(u, v), 0);
                        } else {
                            current_state.emplace(std::make_pair(v, u), 0);
                        }
                    }
                }
            }

            // pre-compute the covered edges of each filter
            std::vector<std::vector<std::pair<ui, ui>>> cover_edges;
            for (auto &f: _candidates) {
                auto pattern = f.filters[0].pattern;
                auto &mapping = f.mappings[0];
                std::set<VertexID> seen_vertices;
                for (auto i = 0ul; i <= f.check_level; i++) {
                    seen_vertices.emplace(matching_order[i]);
                }
                cover_edges.emplace_back();
                for (auto v1 = 0ul; v1 < pattern->getVerticesCount(); v1++) {
                    for (auto v2 = v1+1; v2 < pattern->getVerticesCount(); v2++) {
                        auto u1 = mapping.at(v1);
                        auto u2 = mapping.at(v2);
                        if (pattern->checkEdgeExistence(v1, v2)
                            && (!seen_vertices.contains(u1) || !seen_vertices.contains(u2))) {
                            if (u1 < u2) {
                                cover_edges.back().emplace_back(std::make_pair(u1, u2));
                            } else {
                                cover_edges.back().emplace_back(std::make_pair(u2, u1));
                            }
                        }
                    }
                }
            }

            double current_score = 0.0;
            long selected;
            while (k--) {
                selected = -1;
                double selected_score = 0;
                for (long i = 0; i < _candidates.size(); i++) {
                    auto &f = _candidates[i];
                    
                    double temp_score = current_score;
                    for (auto &e : cover_edges[i]) {
                        if (current_state.at(e) < f.score) {
                            temp_score += f.score - current_state.at(e);
                        }
                    }

                    if (temp_score > selected_score) {
                        selected_score = temp_score;
                        selected = i;
                    }
                }
                if (selected >= 0) {
                    std::cout << "selected vfilter with score " << selected_score << std::endl;
                    result.emplace_back(_candidates[selected]);
                    for (auto &e: cover_edges[selected]) {
                        current_state.at(e) = std::max(current_state.at(e), _candidates[selected].score);
                    }
                }
            }

            return result;
        }

        static std::vector<VFilter> selectRandomKForLevel(const graph_ptr q, const std::vector<VertexID> &matching_order,
         std::vector<VFilter> &candidates, ui level, long k) {
            std::vector<VFilter> result;
            std::vector<VFilter> _candidates;
            for (auto &f : candidates) {
                if (f.check_level == level && result.size() < k) {
                    result.emplace_back(f);
                }
            }
            return result;
        }
    };

}