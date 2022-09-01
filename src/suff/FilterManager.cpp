#include "FilterManager.hpp"

namespace suff {
    long FilterManager::NUM_THREADS = 1;
    double FilterManager::FILTER_FPR;
    fs::path FilterManager::root_folder;
    fs::path FilterManager::graph_name;
    long FilterManager::pattern_size;
    long FilterManager::max_size;
    long FilterManager::data_size;
    fs::path FilterManager::query_name;
    std::vector<Filter> FilterManager::new_filters;
    ui FilterManager::add_level;
    bool FilterManager::CREATE_FILTER;
    std::map<std::string, std::vector<Filter>> FilterManager::loaded_filters;
    std::vector<VFilter> FilterManager::vfilters;
    std::vector<bool> FilterManager::is_check_vid;
    std::vector<int> FilterManager::vid2level;
    moodycamel::ConcurrentQueue<VertexID*> FilterManager::queue;
    std::vector<std::thread> FilterManager::thread_pool;
    bool FilterManager::stop = false;
    std::vector<std::atomic_llong> FilterManager::total_check;
    std::vector<std::atomic_llong> FilterManager::total_fail;
    std::vector<std::vector<bool>> FilterManager::temp_valid_mapping;
    std::vector<std::uint64_t> FilterManager::temp_valid_mapping_count;
    std::vector<u_int64_t> FilterManager::fail_count;
    std::vector<u_int64_t> FilterManager::total_count;
    graph_ptr FilterManager::data_graph;
    graph_ptr FilterManager::query_graph;
}