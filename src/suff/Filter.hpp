#pragma once

#include <string>
#include <fstream>
#include <filesystem>

#include "configuration/types.hpp"
#include "graph/graph.h"
#include "bloom_filter.hpp"

namespace fs = std::filesystem;
namespace suff {

    // a Filter is associated with a `pattern` graph and some of its `vertices`
    class Filter {
    public:
        bloom_filter bf;
        graph_ptr pattern;
        std::vector<ui> vids; //vids of this filer, in order
        ui v_num;
        unsigned long tuple_bytes;
        double fp_rate;
        u_int64_t element_count;
        // double filtering_ratio;  // probability of rejecting a vertex when enumerating this pattern
        std::string bf_path; // for lazy-loading bf
        std::string pattern_path;
        std::vector<u_int64_t> dominated_n;
        std::vector<double> dominated_fp;

        // for creating new filters
        Filter(const std::string &pattern_path, const std::vector<ui> &_vids):
            vids(_vids),
            fp_rate(0.0),
            element_count(0),
            pattern_path(pattern_path)
        {
            pattern = std::make_shared<Graph>(true);
            pattern->loadGraphFromFile(pattern_path);
            tuple_bytes = sizeof(ui) * vids.size();
            v_num = vids.size();
        }

        Filter(const Filter &other): pattern(other.pattern), pattern_path(other.pattern_path), vids(other.vids),
            fp_rate(other.fp_rate), tuple_bytes(other.tuple_bytes), bf_path(other.bf_path),
             element_count(other.element_count), bf(other.bf), v_num(other.v_num) {
        }

        Filter(Filter&& other) = default;

        Filter& operator=(const Filter& other) = default;

        // for loading existing filters
        explicit Filter(std::string path): fp_rate(0), element_count(0) {
            if (path.ends_with(".cfg")) {
                bf_path = path.substr(0, path.size() - 3) + "bf";  // replace suffix "cfg" with "bf"
            } else {
                bf_path = path + ".bf";
                path += ".cfg";
            }
            // only load config, do not load data
            std::ifstream cfg_file(path, std::ios::in);
            // read pattern graph
            cfg_file >> pattern_path;
            pattern = std::make_shared<Graph>(true);
            pattern->loadGraphFromFile(pattern_path);
            // read vids
            cfg_file >> v_num;
            vids.resize(v_num);
            for (ui i = 0; i < v_num; i++) {
                cfg_file >> vids[i];
            }
            tuple_bytes = sizeof(ui) * vids.size();
            // read fp rate
            cfg_file >> fp_rate;
            cfg_file >> element_count;
            // read dominated records
            ui count;
            cfg_file >> count;
            uint64_t temp_n;
            for (auto i = 0ul; i < count; i++) {
                cfg_file >> temp_n;
                dominated_n.emplace_back(temp_n); 
            }
            double temp_fp;
            for (auto i = 0ul; i < count; i++) {
                cfg_file >> temp_fp;
                dominated_fp.emplace_back(temp_fp); 
            }
            cfg_file.close();
        }

        // for loading filters from binary cache file
        explicit Filter(std::ifstream &file) {
            load_b(file);
        }

        // can only be used when the pattern graph vertices start from 0 and are continuous integers
        // tuple should have enough space
        // v should contain the embedding of nodes in `vids` in order
        void inline add(const UIntArray &embedding) {
            bf.insert((unsigned char *)&embedding[0], tuple_bytes);
            element_count++;
        }

        void dump(const std::string &path) {
            if (fs::exists(path + ".cfg") || fs::exists(path + ".lock")) {
                return;
            }
            std::ofstream lock_file(path + ".lock", std::ios::out);
            lock_file << "lock" << std::endl;
            lock_file.close();

            std::ofstream cfg_file(path + ".cfg", std::ios::out);
            cfg_file << pattern_path << std::endl;
            cfg_file << v_num << std::endl;
            for (auto t: vids) {
                cfg_file << t << std::endl;
            }
            fp_rate = bf.effective_fpp();
            cfg_file << fp_rate << std::endl;
            cfg_file << element_count << std::endl;
            cfg_file << dominated_n.size() << std::endl;
            for (auto t : dominated_n) {
                cfg_file << t << std::endl;
            }
            for (auto t : dominated_fp) {
                cfg_file << t << std::endl;
            }
            cfg_file.close();
            bf.dump(path + ".bf");
            fs::remove(path + ".lock");
        }

        void load_data() {
            // if you load filter from disk file, must call this function before querying items!
            // std::cout << "loading data from " << bf_path << std::endl;
            bf = bloom_filter(bf_path);
            fp_rate = bf.effective_fpp();
        }

        // binary dump
        void dump_b(std::ofstream &file) {
            // pattern path
            size_t size = pattern_path.size();
            file.write((char *)&size, sizeof(size));
            file.write(&pattern_path[0], size);
            // pattern
            pattern->storeBinaryFile(file);
            // bf_path
            size = bf_path.size();
            file.write((char *)&size, sizeof(size));
            file.write(&bf_path[0], size);
            // others
            file.write((char *)&v_num, sizeof(v_num));
            file.write((char *)&vids[0], sizeof(ui) * v_num);
            fp_rate = bf.effective_fpp();
            file.write((char *)&fp_rate, sizeof(fp_rate));
            file.write((char *)&element_count, sizeof(element_count));
            size_t dom_size = dominated_n.size();
            file.write((char *)&dom_size, sizeof(dom_size));
            file.write((char *)&dominated_n[0], sizeof(u_int64_t) * dom_size);
            file.write((char *)&dominated_fp[0], sizeof(double) * dom_size);
        }

        // binary load
        void load_b(std::ifstream &file) {
            // pattern path
            size_t size;
            file.read((char *)&size, sizeof(size));
            pattern_path.resize(size);
            file.read(&pattern_path[0], size);
            // pattern
            pattern = std::make_shared<Graph>(true);
            pattern->loadGraphFromBinaryFile(file);
            // bf path
            file.read((char *)&size, sizeof(size));
            bf_path.resize(size);
            file.read(&bf_path[0], size);
            // others
            file.read((char *)&v_num, sizeof(v_num));
            vids.resize(v_num);
            file.read((char *)&vids[0], sizeof(ui) * v_num);
            tuple_bytes = sizeof(ui) * vids.size();
            file.read((char *)&fp_rate, sizeof(fp_rate));
            file.read((char *)&element_count, sizeof(element_count));
            size_t dom_size = 0;
            file.read((char *)&dom_size, sizeof(dom_size));
            dominated_n.resize(dom_size);
            file.read((char *)&dominated_n[0], sizeof(u_int64_t) * dom_size);
            dominated_fp.resize(dom_size);
            file.read((char *)&dominated_fp[0], sizeof(double) * dom_size);
        }

        void init_filter(ui array_size_in_kb) {
            bloom_parameters param;
            param.projected_element_count = 1000000;
            param.maximum_size = array_size_in_kb * 8192;
            param.minimum_size = array_size_in_kb * 8192;
            param.false_positive_probability = 0.01;
            param.maximum_number_of_hashes = 3;
            param.random_seed = 10007;  // or 2147483647, a Mersenne number
            param.compute_optimal_parameters();
            bf = bloom_filter(param);
        }
    };

    // A VFilter contains a set of Filters (each associated with a mapping) at a certain level.
    // At this level, all Filters are used and their results are combined.
    class VFilter {
    public:
        std::vector<Filter> filters;  // each filter corresponds to a mapping in mappings
        std::vector<VMapping> mappings;  // each mapping maps v in filter vertices to u in query
        double score;  // estimated pruning power
        std::vector<std::vector<ui>> cache;
        ui check_level;
        std::vector<ui> buffer;

        VFilter(std::vector<Filter> filters, std::vector<VMapping> mappings, ui level): 
            filters(std::move(filters)),
            mappings(std::move(mappings)),
            check_level(level) {

            for (ui j = 0; j < this->filters.size(); j++) {
                std::vector<ui> temp(this->filters[j].vids.size());
                for (ui i = 0; i < this->filters[j].vids.size(); i++) {
                    temp[i] = this->mappings[j].at(this->filters[j].vids[i]);
                }
                cache.emplace_back(std::move(temp));
            }
            buffer.assign(level + 1, 0);
        }

        VFilter(Filter &filter, VMapping &mapping, ui level): filters(), mappings(), check_level(level) {
            filters.emplace_back(filter);
            mappings.emplace_back(mapping);
            for (ui j = 0; j < this->filters.size(); j++) {
                std::vector<ui> temp(this->filters[j].vids.size());
                for (ui i = 0; i < this->filters[j].vids.size(); i++) {
                    temp[i] = this->mappings[j].at(this->filters[j].vids[i]);
                }
                cache.emplace_back(std::move(temp));
            }
            buffer.assign(level + 1, 0);
        }

        VFilter(const VFilter &other) = default;

        VFilter(VFilter&& other) = default;

        VFilter& operator=(const VFilter& other) = default;

        VFilter(){}  // for inserting place holders in a vector

        bool inline contains(UIntArray &embedding) {
            for (ui j = 0; j < filters.size(); j++) {
                for (ui i = 0; i < filters[j].v_num; i++) {
                    buffer[i] = embedding[cache[j][i]];
                }
                if (!filters[j].bf.contains((unsigned char *) &buffer[0], filters[j].tuple_bytes)) {
                    return false;
                }
            }
            return true;
        }

        void load_data() {
            for (auto &f: filters) {
                f.load_data();
            }
            // bf = filters[0].bf;
            // for (auto j = 1; j < filters.size(); j++) {
            //     bf &= filters[j].bf;
            // }
        }
    };

}