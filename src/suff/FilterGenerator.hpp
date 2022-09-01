#pragma once

#include <vector>
#include <algorithm>
#include "configuration/types.hpp"
#include "graph/graph.h"
#include "Filter.hpp"

namespace suff {
    
    class FilterGenerator {
    public:
        struct VertexSetCandidate {
            std::vector<VertexID> vertex_set;
            double score;

            VertexSetCandidate(std::vector<VertexID> &vertex_set, double score): vertex_set(vertex_set), score(score) {}
        };

        // given a query graph, select some vertex sets to generate filters
        static std::vector<Filter> GenerateNewFilters(const std::string &query_path, const graph_ptr query,
         const std::vector<VertexID> &matching_order) {
            std::vector<Filter> filters;
            for (auto i = 0ul; i < query->getVerticesCount(); i++) {
                // because of the dynamic order, only use 1 vertex to check
                filters.emplace_back(query_path, i);
            }
            return filters;
        }

    private:
        // enumerate all subsets of `vertices`
        static void EnumerateSubSets(const graph_ptr query, const std::vector<VertexID> &matching_order,
         VertexID current, const size_t &max_level, std::vector<VertexID> &temp,
          std::vector<VertexSetCandidate> &vertex_sets) {

            if (temp.size() == max_level) {
                // vertex_sets.emplace_back(temp, Score(query, matching_order, temp));
                vertex_sets.emplace_back(temp, query->getVerticesCount() - current);
                return;
            }
            if (current < query->getVerticesCount()) {
                EnumerateSubSets(query, matching_order, current + 1, max_level, temp, vertex_sets);
                temp.push_back(matching_order[current]);
                EnumerateSubSets(query, matching_order, current + 1, max_level, temp, vertex_sets);
                temp.pop_back();
            }
        }

        // assign a preference score to each vertex set to select the best ones
        static double Score(const graph_ptr query, const std::vector<VertexID> &matching_order, std::vector<VertexID> &vertex_set) {
            double score = 0.0;
            for (auto id : vertex_set) {
                score += query->getVertexDegree(id);
            }
            return score;
        }
    };

}