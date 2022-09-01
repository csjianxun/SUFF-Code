#pragma once

#include <vector>
#include <map>
#include <set>
#include <string>
#include "Filter.hpp"
#include "graph/graph.h"

namespace suff {

    class VFilterGenerator {
    public:
        static std::vector<VFilter> Generate(const graph_ptr q, const std::vector<VertexID> &matching_order,
        std::map<std::string, std::vector<Filter>> &groups) {
            std::vector<VFilter> results;
            // process each group (contains filters having the same pattern graph) separately
            for (auto &group: groups) {
                if (group.second.size() == 0) {
                    continue;
                }
                const graph_ptr filter_pattern = group.second[0].pattern;
                if (filter_pattern->getVerticesCount() > q->getVerticesCount()) {
                    continue;
                }
                // stores matches of filter_pattern in q
                std::vector<VMapping> mappings = Match(q, filter_pattern);
                // for each mapping and each Filter, generate one VFilter
                for (auto &f : group.second) {
                    for (auto &m: mappings) {
                        // decide the filter vertex
                        auto u = m.at(f.vid);
                        // add to results
                        results.emplace_back(f, m, u);
                    }
                }
            }
            return results;
        }

        // recursive call of Match below
        static void Match(const graph_ptr d, const graph_ptr p,
                VMapping &current_match, std::set<VertexID> &mapped, VertexID level, std::vector<VMapping> &results) {
            if (level == p->getVerticesCount()) {
                results.emplace_back(current_match);
                return;
            } else {
                for (VertexID v = 0; v < d->getVerticesCount(); v++) {
                    if (mapped.count(v) == 0 && d->getVertexLabel(v) == p->getVertexLabel(level)) {
                        current_match.insert_or_assign(level, v);
                        bool can_match = true;
                        for (auto &pair1: current_match) {
                            for (auto &pair2: current_match) {
                                if (p->checkEdgeExistence(pair1.first, pair2.first) && !d->checkEdgeExistence(pair1.second, pair2.second)) {
                                    can_match = false;
                                }
                            }
                        }
                        if (can_match) {
                            mapped.insert(v);
                            Match(d, p, current_match, mapped, level + 1, results);
                            mapped.erase(v);
                        }
                        current_match.erase(level);
                    }
                }
            }
        }

        // find all matches of p in d (d,p are small graphs)
        static std::vector<VMapping> Match(const graph_ptr d, const graph_ptr p) {
            std::vector<VMapping> matches;
            VMapping current_match;
            std::set<VertexID> mapped;
            Match(d, p, current_match, mapped, 0, matches);
            return matches;
        }
    };

}