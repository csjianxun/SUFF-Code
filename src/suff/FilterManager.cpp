#include "FilterManager.hpp"

namespace suff {
    long FilterManager::NUM_THREADS = 1;
    double FilterManager::FILTER_FPR;
    fs::path FilterManager::root_folder;
    fs::path FilterManager::graph_name;
    ui FilterManager::pattern_size;
    ui FilterManager::data_size;
    fs::path FilterManager::query_name;
    std::vector<Filter> FilterManager::new_filters;
    std::vector<ui> FilterManager::buffer;
    ui FilterManager::add_level;
    std::vector<ui> FilterManager::matching_order;
    ui FilterManager::FILTER_SIZE;
    std::string FilterManager::SELECTOR;
    std::map<std::string, std::vector<Filter>> FilterManager::loaded_filters;
    std::vector<VFilter> FilterManager::vfilters;
    std::vector<bool> FilterManager::is_check_level;
    std::vector<ui> FilterManager::vid2level;
    bool FilterManager::stop = false;
    std::vector<std::atomic_llong> FilterManager::total_check;
    std::vector<std::atomic_llong> FilterManager::total_fail;
    std::vector<u_int64_t> FilterManager::fail_count;
    std::vector<u_int64_t> FilterManager::total_count;
    graph_ptr FilterManager::data_graph;
    graph_ptr FilterManager::query_graph;
}