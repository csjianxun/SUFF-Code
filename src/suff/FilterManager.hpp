#pragma once

#include <map>
#include <string>
#include <vector>
#include <filesystem>
#include <atomic>
#include <algorithm>
#include "configuration/types.hpp"
#include "graph/graph.h"
#include "Filter.hpp"
#include "FilterGenerator.hpp"
#include "VFilterGenerator.hpp"
#include "FilterSelector.hpp"

namespace fs = std::filesystem;

namespace suff {

    class filter_record {
    public:
        long vf_id;
        std::vector<long> f_ids;

        explicit filter_record(long id, std::vector<long> fids): vf_id(id), f_ids(move(fids)) {};
        filter_record(const filter_record &other) = default;
        filter_record &operator=(filter_record const &other) = default;
    };

    class FilterManager {
    public:
        static long NUM_THREADS;
        static double FILTER_FPR;
        static fs::path root_folder;
        static fs::path graph_name;
        static ui pattern_size;
        static ui data_size;  // data graph size
        // for creating filters
        static fs::path query_name;
        static std::vector<Filter> new_filters;
        static std::vector<ui> buffer;
        static ui add_level;
        static std::vector<ui> matching_order;
        static ui FILTER_SIZE;  // filter bit array size in KB, 0 means do not create filters
        static std::string SELECTOR;
        // for using filters
        static std::map<std::string, std::vector<Filter>> loaded_filters;
        static std::vector<VFilter> vfilters;  // vfilters[i] stores vfilter at level i
        static std::vector<bool> is_check_level;  // check if level i has filter
        static std::vector<ui> vid2level;  // if vertex v has filter, this gives the level, otherwise gives -1
        static bool stop;
        // for statistics
        static std::vector<std::atomic_llong> total_check;
        static std::vector<std::atomic_llong> total_fail;
        static std::vector<u_int64_t> fail_count;
        static std::vector<u_int64_t> total_count;
        static graph_ptr data_graph;
        static graph_ptr query_graph;

        static void Init(const fs::path &root_folder, const fs::path &graph_name, ui filter_size = 0,
            double fpr = 0.01, std::string selector = "greedy") {

            FilterManager::root_folder = root_folder;
            FilterManager::graph_name = graph_name;
            FILTER_SIZE = filter_size;
            FILTER_FPR = fpr;
            SELECTOR = selector;
            pattern_size = 0;
            data_size = 0;
            query_name = "";
            add_level = 0;
            new_filters.clear();
            loaded_filters.clear();
            vfilters.clear();
            is_check_level.clear();
            vid2level.clear();
            total_check.clear();
            total_fail.clear();
            fail_count.clear();
            total_count.clear();
            data_graph = nullptr;
            query_graph = nullptr;
            matching_order.clear();
        }

        // create new empty filters to be filled later
        static void CreateFilters(const graph_ptr d, const std::string &query_path, const graph_ptr q, const std::vector<VertexID> &order) {
            data_graph = d;
            query_graph = q;
            pattern_size = q->getVerticesCount();
            data_size = d->getVerticesCount();
            matching_order.assign(order.begin(), order.end());
            FilterManager::query_name = query_path.substr(query_path.rfind('/') + 1);
            std::cout << "generating new filters..." << std::endl;
            new_filters = FilterGenerator::GenerateNewFilters(query_path, q, matching_order);
            for (auto &f: new_filters) {
                f.init_filter(FILTER_SIZE);
            }
            // std::cout << "creating temp vectors..." << std::endl;
            // for (std::size_t i = 0; i < q->getVerticesCount(); i++) {
            //     temp_valid_mapping.emplace_back(d->getVerticesCount(), false);
            //     temp_valid_mapping_count.emplace_back(0);
            // }
            
            fail_count = std::vector<u_int64_t>(pattern_size, 0);
            total_count = std::vector<u_int64_t>(pattern_size, 0);
            add_level = std::min(matching_order.size(), 3ul);
            buffer.resize(add_level);
        }

        // load some existing filters to be used later
        static void PrepareFilters(const graph_ptr d, const graph_ptr q, const std::vector<VertexID> &matching_order,
            const std::vector<VertexID> &pivot, long k) {

            pattern_size = q->getVerticesCount();
            is_check_level.resize(pattern_size, false);  // max check level is #vertices - 1
            vid2level.resize(pattern_size, -1);
            vfilters.resize(pattern_size);
            total_check = std::vector<std::atomic_llong>(pattern_size);
            total_fail = std::vector<std::atomic_llong>(pattern_size);
            for (ui i = 0; i < pattern_size; i++) {
                total_check[i] = 0;
                total_fail[i] = 0;
            }
            if (loaded_filters.size() == 0) {
                return;
            }
            std::cout << "generating vfilters..." << std::endl;
            for (ui i = 0; i < matching_order.size(); i++) {
                vid2level[matching_order[i]] = i;
            }
            auto initial_vfilters = VFilterGenerator::Generate(q, matching_order, loaded_filters, vid2level);
            std::cout << "# initial vfilters: " << initial_vfilters.size() << std::endl;
            std::vector<VFilter> selected_vfilters;
            if (SELECTOR == "greedy") {
                std::cout << "selecting " << k << " best filters..." << std::endl;
                selected_vfilters = FilterSelector::SelectK(d, q, matching_order, pivot, initial_vfilters, k);
            } else {
                std::cout << "selecting " << k << " random filters..." << std::endl;
                selected_vfilters = FilterSelector::SelectRandomK(d, q, matching_order, pivot, initial_vfilters, k);
            }
            std::cout << "# selected vfilters: " << selected_vfilters.size() << std::endl;
            // group vfilters by level
            std::cout << "grouping and loading filter data..." << std::endl;
            for (auto &f: selected_vfilters) {
                is_check_level[f.check_level] = true;
            }
            std::cout << "check levels: [";
            for (auto i = 0ul; i < pattern_size; i++) {
                if (is_check_level[i]) {
                    std::cout << " " << i;
                    std::vector<Filter> filters;
                    std::vector<VMapping> mappings;
                    for (auto &f: selected_vfilters) {
                        if (f.check_level == i) {
                            filters.emplace_back(f.filters[0]);
                            mappings.emplace_back(f.mappings[0]);
                        }
                    }
                    vfilters[i] = VFilter(filters, mappings, i);
                    // load filter data
                    vfilters[i].load_data();
                }
            }
            std::cout << "]" << std::endl << std::flush;

        }

        static inline bool PassMatch(ui level, UIntArray &embedding) {
            if (!is_check_level[level] || level >= pattern_size - 5) {
                return true;
            }

            bool pass = vfilters[level].contains(embedding);
            
            total_check[level]++;
            total_fail[level] += !pass;
            return pass;
        }

        // for dynamic matching order, using vid(u) to check
        // static inline bool PassMatch(long level, UIntArray &order, UIntArray &embedding) {
        //     if (level >= pattern_size - 5) {
        //         return true;
        //     }

        //     bool pass = vfilters[u].contains(v);
            
        //     total_check[level]++;
        //     total_fail[level]+= !pass;
        //     return pass;
        // }

        // add embeddings to filters
        static inline void AddMatch(ui level, UIntArray &embedding) {
            if (level == add_level) {
                for (auto &f: new_filters) {
                    for (ui i = 0; i < f.v_num; i++) {
                        buffer[i] = embedding[f.vids[i]];
                    }
                    f.add(buffer);
                }
            }
        }

        static void LoadFilters(bool use_cache=true) {
            long num_loaded = 0;
            if (use_cache && ReadFilterCache()) {
                for (auto &[k, v] : loaded_filters) {
                    num_loaded += v.size();
                }
                std::cout << "loaded filters from cache: " << num_loaded << std::endl;
                return;
            }
            std::cout << "loading filter configurations..." << std::endl;
            if (fs::is_directory(root_folder / graph_name)) {
                for (auto &p : fs::directory_iterator(root_folder / graph_name)) {
                    if (fs::is_directory(p)
                        && p.path().filename().string().find("_") != std::string::npos  // pre-defined patterns
                        ) {
                        // std::cout << "folder: " << p.path() << std::endl;
                        auto filters = std::vector<Filter>();
                        for (auto &fp : fs::directory_iterator(p)) {
                            // use .bf to test whether the filter is fully dumped
                            // if .bf file exists, then .cfg file is ready to read
                            auto path = fp.path().string();
                            if (path.rfind(".bf") != std::string::npos) {
                                auto prefix = path.substr(0, path.size() - 3);
                                if (fs::exists(prefix + ".lock")) {
                                    continue;
                                }
                                num_loaded++;
                                filters.emplace_back(Filter(prefix));
                            }
                        }
                        if (filters.size() > 0) {
                            loaded_filters.insert_or_assign(p.path().filename(), std::move(filters));
                        }
                    }
                }
            }
            std::cout << "# loaded filters: " << num_loaded << std::endl;
            if (FILTER_SIZE == 0) {  // not creating filters
                WriteFilterCache();
            }
        }

        static bool ReadFilterCache() {
            if (fs::is_directory(root_folder / graph_name) && fs::exists(root_folder / graph_name / fs::path("filter_cache"))) {
                std::ifstream file(root_folder / graph_name / fs::path("filter_cache"), std::ios::binary);
                size_t map_size;
                file.read((char *)&map_size, sizeof(map_size));
                while (map_size--) {
                    std::string key;
                    size_t key_size;
                    file.read((char *)&key_size, sizeof(key_size));
                    key.resize(key_size);
                    file.read(&key[0], key_size);
                    size_t array_size;
                    file.read((char *)&array_size, sizeof(array_size));
                    std::vector<Filter> filters;
                    while (array_size--) {
                        filters.emplace_back(file);
                    }
                    loaded_filters.insert_or_assign(key, std::move(filters));
                }
                file.close();
                return true;
            } else {
                return false;
            }
        }

        static void WriteFilterCache() {
            std::ofstream file(root_folder / graph_name / fs::path("filter_cache"), std::ios::binary);
            size_t map_size = loaded_filters.size();
            file.write((char *)&map_size, sizeof(map_size));
            for (auto &[key, filters]: loaded_filters) {
                size_t key_size = key.size();
                file.write((char *)&key_size, sizeof(key_size));
                file.write(&key[0], key_size);
                size_t array_size = filters.size();
                file.write((char *)&array_size, sizeof(array_size));
                for (auto &f: filters) {
                    f.dump_b(file);
                }
            }
            file.close();
        }

        static void Dump() {
            if (FILTER_SIZE == 0) {
                return;
            }
            std::string sub_folder = root_folder / graph_name / query_name;
            if (!fs::is_directory(sub_folder) || !fs::exists(sub_folder)) {
                fs::create_directories(sub_folder);
            }

            // for (auto i = 0ul; i < temp_valid_mapping.size(); i++) {
            //     new_filters[i].init_filter(FILTER_SIZE);
            //     new_filters[i].filtering_ratio = ((double) temp_valid_mapping_count[i]) / data_graph->getLabelsFrequency(query_graph->getVertexLabel(i));
            //     for (auto v = 0ul; v < data_size; v++) {
            //         if (temp_valid_mapping[i][v]) {
            //             new_filters[i].add(v);
            //         }
            //     }
            // }
            if (FILTER_SIZE > 0) {
                ui count = 0;
                for (auto &f: new_filters) {
                    std::string prefix = std::to_string(count++);
                    if (f.bf.effective_fpp() <= FILTER_FPR) {
                        f.dump(sub_folder / fs::path(prefix));
                        std::cout << "fpp is ok: " << f.bf.effective_fpp() << std::endl;
                    } else {
                        std::cout << "fpp is too large: " << f.bf.effective_fpp() << std::endl;
                    }
                }

            }
        }

        // pruning filters
        // pruned filters will not be used (but will be retained on disk)
        // the removed filters will be recorded in a cache file, so you need to remove it for a new run
        static void FilterRemoval(double alpha=0.1, bool force_run=false) {
            if (loaded_filters.size() == 0) {
                return;
            }
            fs::path cache_file_path = root_folder / graph_name / fs::path("removal_cache");
            // if a cache file exists, load the cache file directly
            std::set<std::string> removed;
            if (fs::exists(cache_file_path) && !force_run) {
                std::ifstream cache_file(cache_file_path, std::ios::in);
                ui count;
                std::string temp;
                cache_file >> count;
                for (ui i = 0; i < count; i++) {
                    cache_file >> temp;
                    removed.emplace(temp);
                }
                cache_file.close();
                
            } else {  // else identify filters to be removed
                std::vector<suff::Filter> all_filters;
                for (auto &[k, v] : loaded_filters) {
                    for (auto &f: v) {
                        all_filters.emplace_back(f);
                    }
                }
                std::cout << "before removal, database size: " << all_filters.size() << std::endl;
                // sort filters by the pattern size
                std::sort(all_filters.begin(), all_filters.end(), [](suff::Filter &a, suff::Filter &b){
                    return a.pattern->getVerticesCount() > b.pattern->getVerticesCount();
                });
                for (ui i = 0; i < all_filters.size(); i++) {
                    Filter &fr = all_filters[i];
                    for (ui j = i+1; j < all_filters.size(); j++) {  // fo is smaller
                        // check if the false-positive condition is satisfied
                        Filter &fo = all_filters[j];
                        double upper = fo.fp_rate + (fo.element_count - fr.element_count) * (1 - fo.fp_rate) / alpha / fr.element_count;
                        if (upper > FILTER_FPR) {
                            // std::cout << "upper bound too large: " << std::endl;
                            // std::cout << "fo: " << fo.pattern->getVerticesCount() << " " << fo.fp_rate << " " << fo.element_count << std::endl;
                            // std::cout << "fr: " << fr.pattern->getVerticesCount() << " " << fr.fp_rate << " " << fr.element_count << std::endl;
                            continue;
                        }
                        bool satisfy_fp = true;
                        for (ui k = 0; k < fr.dominated_n.size(); k++) {
                            upper = fo.fp_rate + (fo.element_count - fr.dominated_n[k]) * (1 - fo.fp_rate) / alpha / fr.dominated_n[k];
                            if (upper > FILTER_FPR) {
                                satisfy_fp = false;
                                break;
                            }
                        }
                        if (!satisfy_fp) {
                            continue;
                        }
                        // check if the match condition is satisfied
                        auto matches = VFilterGenerator::Match(fr.pattern, fo.pattern);
                        bool dominate = false;
                        for (auto &m : matches) {
                            bool _dominate = true;
                            for (auto v: fo.vids) {
                                if (std::find(fr.vids.begin(), fr.vids.end(), m.at(v)) == fr.vids.end()) {
                                    _dominate = false;
                                    break;
                                }
                            }
                            if (_dominate) {
                                dominate = true;
                                break;
                            }
                        }
                        if (dominate) {
                            // debug
                            // std::cout << "fo: " << fo.pattern->getVerticesCount() << " " << fo.fp_rate << " " << fo.element_count << std::endl;
                            // std::cout << "fr: " << fr.pattern->getVerticesCount() << " " << fr.fp_rate << " " << fr.element_count << std::endl;
                            removed.emplace(fr.bf_path);
                            fo.dominated_fp.emplace_back(fr.fp_rate);
                            fo.dominated_n.emplace_back(fr.element_count);
                            for (ui k = 0; k < fr.dominated_n.size(); k++) {
                                fo.dominated_fp.emplace_back(fr.dominated_fp[k]);
                                fo.dominated_n.emplace_back(fr.dominated_n[k]);
                            }
                            break;
                        }
                    }
                }
                // write removed filters into cache file
                std::ofstream cache_file(cache_file_path, std::ios::out);
                cache_file << removed.size() << std::endl;
                for (auto &bf_path : removed) {
                    cache_file << bf_path << std::endl;
                }
                cache_file.close();
            }
            // remove filters
            ui rest = 0;
            for (auto &[k, v] : loaded_filters) {
                std::vector<suff::Filter> temp;
                for (auto &f: v) {
                    if (!removed.contains(f.bf_path)) {
                        temp.emplace_back(f);
                    }
                }
                v.clear();
                v.assign(temp.begin(), temp.end());
                rest += v.size();
            }
            std::cout << "removed " << removed.size() << " filters; " << rest << " remain" << std::endl;
        }
    };

}