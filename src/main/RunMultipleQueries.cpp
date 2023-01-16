#include <chrono>
#include <future>
#include <thread>
#include <fstream>
#include <filesystem>

#include "configuration/types.hpp"
#include "matching/matchingcommand.h"
#include "graph/graph.h"
#include "matching/GenerateFilteringPlan.h"
#include "matching/FilterVertices.h"
#include "matching/BuildTable.h"
#include "matching/GenerateQueryPlan.h"
#include "matching/EvaluateQuery.h"
#include "suff/FilterManager.hpp"

namespace fs = std::filesystem;

#define NANOSECTOSEC(elapsed_time) ((elapsed_time)/(double)1000000000)
#define BYTESTOMB(memory_cost) ((memory_cost)/(double)(1024 * 1024))

void execute_query(MatchingCommand &command, graph_ptr data_graph, graph_ptr query_graph, std::string input_query_graph_file) {
    std::string input_data_graph_file = command.getDataGraphFilePath();
    std::string input_filter_type = command.getFilterType();
    std::string input_order_type = command.getOrderType();
    std::string input_engine_type = command.getEngineType();
    std::string input_max_embedding_num = command.getMaximumEmbeddingNum();
    std::string input_time_limit = command.getTimeLimit();
    std::string input_order_num = command.getOrderNum();
    std::string input_distribution_file_path = command.getDistributionFilePath();
    std::string input_csr_file_path = command.getCSRFilePath();
    std::string input_suff_dir = command.getSUFFDir();
    std::string input_suff_k = command.getSUFFK();
    bool create_filter = command.getCreateFilter();
    std::string input_suff_alpha = command.getSUFFAlpha();
    bool use_cache = command.getUseCache();
    std::string input_suff_selector = command.getSUFFSelector();
    
    /**
     * Start queries.
     */

    std::cout << "Start queries..." << std::endl;
    std::cout << "-----" << std::endl;
    std::cout << "Filter candidates..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    UIntMatrix candidates;
    UIntArray candidates_count;
    UIntArray tso_order;
    TreeNodeArray tso_tree;
    UIntArray cfl_order;
    TreeNodeArray cfl_tree;
    UIntArray dpiso_order;
    TreeNodeArray dpiso_tree;
    TreeNodeArray ceci_tree;
    UIntArray ceci_order;
    std::vector<std::unordered_map<VertexID, std::vector<VertexID >>> TE_Candidates;
    std::vector<std::vector<std::unordered_map<VertexID, std::vector<VertexID>>>> NTE_Candidates;
    if (input_filter_type == "LDF") {
        FilterVertices::LDFFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "NLF") {
        FilterVertices::NLFFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "GQL") {
        FilterVertices::GQLFilter(data_graph, query_graph, candidates, candidates_count);
    } else if (input_filter_type == "TSO") {
        FilterVertices::TSOFilter(data_graph, query_graph, candidates, candidates_count, tso_order, tso_tree);
    } else if (input_filter_type == "CFL") {
        FilterVertices::CFLFilter(data_graph, query_graph, candidates, candidates_count, cfl_order, cfl_tree);
    } else if (input_filter_type == "DPiso") {
        FilterVertices::DPisoFilter(data_graph, query_graph, candidates, candidates_count, dpiso_order, dpiso_tree);
    } else if (input_filter_type == "CECI") {
        FilterVertices::CECIFilter(data_graph, query_graph, candidates, candidates_count, ceci_order, ceci_tree, TE_Candidates, NTE_Candidates);
    }  else {
        std::cout << "The specified filter type '" << input_filter_type << "' is not supported." << std::endl;
        exit(-1);
    }

    // DEBUG check validation of candidates
    for (ui i = 0; i < query_graph->getVerticesCount(); i++) {
        for (ui j = 0; j < candidates_count[i]; j++) {
            if (candidates[i][j] == 100000000) {
                std::cout << "[DEBUG] found invalid candidates" << std::endl;
                candidates_count[i] = j;
                break;
            }
        }
    }
    std::cout << "candidates count: [";
    for (ui i = 0; i < query_graph->getVerticesCount() - 1; i++) {
        std::cout << candidates_count[i] << ", ";
    }
    std::cout << candidates_count[query_graph->getVerticesCount() - 1] << "]" << std::endl;

    // Sort the candidates to support the set intersections
    if (input_filter_type != "CECI")
        FilterVertices::sortCandidates(candidates, candidates_count, query_graph->getVerticesCount());

    auto end = std::chrono::high_resolution_clock::now();
    double filter_vertices_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    // Compute the candidates false positive ratio.
#ifdef OPTIMAL_CANDIDATES
    std::vector<ui> optimal_candidates_count;
    double avg_false_positive_ratio = FilterVertices::computeCandidatesFalsePositiveRatio(data_graph, query_graph, candidates,
                                                                                          candidates_count, optimal_candidates_count);
    FilterVertices::printCandidatesInfo(query_graph, candidates_count, optimal_candidates_count);
#endif
    std::cout << "-----" << std::endl;
    std::cout << "Build indices..." << std::endl;

    start = std::chrono::high_resolution_clock::now();

    EdgesPtrMatrix edge_matrix;
    if (input_filter_type != "CECI") {
        edge_matrix.resize(query_graph->getVerticesCount());
        for (ui i = 0; i < query_graph->getVerticesCount(); ++i) {
            edge_matrix[i].resize(query_graph->getVerticesCount(), nullptr);
        }

        BuildTable::buildTables(data_graph, query_graph, candidates, candidates_count, edge_matrix);
    }

    end = std::chrono::high_resolution_clock::now();
    double build_table_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    size_t memory_cost_in_bytes = 0;
    if (input_filter_type != "CECI") {
        // memory_cost_in_bytes = BuildTable::computeMemoryCostInBytes(query_graph, candidates_count, edge_matrix);
        // BuildTable::printTableCardinality(query_graph, edge_matrix);
    }
    else {
        // memory_cost_in_bytes = BuildTable::computeMemoryCostInBytes(query_graph, candidates_count, ceci_order, ceci_tree,
        //         TE_Candidates, NTE_Candidates);
        // BuildTable::printTableCardinality(query_graph, ceci_tree, ceci_order, TE_Candidates, NTE_Candidates);
    }
    std::cout << "-----" << std::endl;
    std::cout << "Generate a matching order..." << std::endl;

    start = std::chrono::high_resolution_clock::now();

    UIntArray matching_order;
    UIntArray pivots;
    UIntMatrix weight_array;

    size_t order_num = 0;
    sscanf(input_order_num.c_str(), "%zu", &order_num);

    std::vector<std::vector<ui>> spectrum;
    if (input_order_type == "QSI") {
        GenerateQueryPlan::generateQSIQueryPlan(data_graph, query_graph, edge_matrix, matching_order, pivots);
    } else if (input_order_type == "GQL") {
        GenerateQueryPlan::generateGQLQueryPlan(data_graph, query_graph, candidates_count, matching_order, pivots);
    } else if (input_order_type == "TSO") {
        if (tso_tree.size() == 0) {
            GenerateFilteringPlan::generateTSOFilterPlan(data_graph, query_graph, tso_tree, tso_order);
        }
        GenerateQueryPlan::generateTSOQueryPlan(query_graph, edge_matrix, matching_order, pivots, tso_tree, tso_order);
    } else if (input_order_type == "CFL") {
        if (cfl_tree.size() == 0) {
            int level_count;
            UIntArray level_offset;
            GenerateFilteringPlan::generateCFLFilterPlan(data_graph, query_graph, cfl_tree, cfl_order, level_count, level_offset);
        }
        GenerateQueryPlan::generateCFLQueryPlan(data_graph, query_graph, edge_matrix, matching_order, pivots, cfl_tree, cfl_order, candidates_count);
    } else if (input_order_type == "DPiso") {
        if (dpiso_tree.size() == 0) {
            GenerateFilteringPlan::generateDPisoFilterPlan(data_graph, query_graph, dpiso_tree, dpiso_order);
        }

        GenerateQueryPlan::generateDSPisoQueryPlan(query_graph, edge_matrix, matching_order, pivots, dpiso_tree, dpiso_order,
                                                    candidates_count, weight_array);
    }
    else if (input_order_type == "CECI") {
        GenerateQueryPlan::generateCECIQueryPlan(query_graph, ceci_tree, ceci_order, matching_order, pivots);
    }
    else if (input_order_type == "RI") {
        GenerateQueryPlan::generateRIQueryPlan(data_graph, query_graph, matching_order, pivots);
    }
    else if (input_order_type == "VF2PP") {
        GenerateQueryPlan::generateVF2PPQueryPlan(data_graph, query_graph, matching_order, pivots);
    }
    else if (input_order_type == "Spectrum") {
        GenerateQueryPlan::generateOrderSpectrum(query_graph, spectrum, order_num);
    }
    else {
        std::cout << "The specified order type '" << input_order_type << "' is not supported." << std::endl;
    }

    end = std::chrono::high_resolution_clock::now();
    double generate_query_plan_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

    if (input_order_type != "Spectrum") {
        GenerateQueryPlan::checkQueryPlanCorrectness(query_graph, matching_order, pivots);
        GenerateQueryPlan::printSimplifiedQueryPlan(query_graph, matching_order);
    }
    else {
        std::cout << "Generate " << spectrum.size() << " matching orders." << std::endl;
    }

    std::cout << "-----" << std::endl;
    std::cout << "Enumerate..." << std::endl;
    size_t output_limit = 0;
    size_t embedding_count = 0;
    if (input_max_embedding_num == "MAX") {
        output_limit = std::numeric_limits<size_t>::max();
    }
    else {
        sscanf(input_max_embedding_num.c_str(), "%zu", &output_limit);
    }

#ifdef SUFF
    std::cout << "--SUFF enabled--" << std::endl;
    fs::path root_dir(input_suff_dir);
    if (input_csr_file_path.empty()) {
        fs::path graph_name(input_data_graph_file.substr(input_data_graph_file.rfind('/') + 1));
        suff::FilterManager::Init(root_dir, graph_name, create_filter, 0.01, input_suff_selector);
    } else {
        fs::path graph_name(input_csr_file_path.substr(input_csr_file_path.rfind('/') + 1));
        suff::FilterManager::Init(root_dir, graph_name, create_filter, 0.01, input_suff_selector);
    }
    // std::vector<VertexID> _matching_order(matching_order.begin(), matching_order. + query_graph->getVerticesCount());
    // std::vector<VertexID> _pivot(pivots, pivots + query_graph->getVerticesCount());
    suff::FilterManager::CreateFilters(data_graph, input_query_graph_file, query_graph, matching_order);

    if (std::stoi(input_suff_k) > 0) {
        suff::FilterManager::LoadFilters(use_cache);
    }

    start = std::chrono::high_resolution_clock::now();

    if (input_suff_alpha == "") {
        suff::FilterManager::FilterRemoval();
    } else {
        std::cout << "Filter Removal Alpha: " << std::stof(input_suff_alpha) << std::endl;
        suff::FilterManager::FilterRemoval(std::stof(input_suff_alpha), true);
    }

    end = std::chrono::high_resolution_clock::now();
    double filterremoval_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    printf("Filter Removal time (seconds): %.4lf\n", NANOSECTOSEC(filterremoval_time_in_ns));

    if (input_suff_alpha != "") {
        return;  // alpha > 0, only compute filter cache
    }
    start = std::chrono::high_resolution_clock::now();
    
    suff::FilterManager::PrepareFilters(data_graph, query_graph, matching_order, pivots, std::stoi(input_suff_k));
    
    end = std::chrono::high_resolution_clock::now();
    double filterselection_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    printf("Filter Selection time (seconds): %.4lf\n", NANOSECTOSEC(filterselection_time_in_ns));
#endif

#if ENABLE_QFLITER == 1
    EvaluateQuery::qfliter_bsr_graph_ = BuildTable::qfliter_bsr_graph_;
#endif

    size_t call_count = 0;
    size_t time_limit = 0;
    sscanf(input_time_limit.c_str(), "%zu", &time_limit);

    start = std::chrono::high_resolution_clock::now();

    if (input_engine_type == "EXPLORE") {
        embedding_count = EvaluateQuery::exploreGraph(data_graph, query_graph, edge_matrix, candidates,
                                                      candidates_count, matching_order, pivots, output_limit, call_count);
    } else if (input_engine_type == "LFTJ") {
        embedding_count = EvaluateQuery::LFTJ(data_graph, query_graph, edge_matrix, candidates, candidates_count,
                                              matching_order, output_limit, call_count);
    } else if (input_engine_type == "GQL") {
        embedding_count = EvaluateQuery::exploreGraphQLStyle(data_graph, query_graph, candidates, candidates_count,
                                                             matching_order, output_limit, call_count);
    } else if (input_engine_type == "QSI") {
        embedding_count = EvaluateQuery::exploreQuickSIStyle(data_graph, query_graph, candidates, candidates_count,
                                                             matching_order, pivots, output_limit, call_count);
    }
    else if (input_engine_type == "DPiso") {
        embedding_count = EvaluateQuery::exploreDPisoStyle(data_graph, query_graph, dpiso_tree,
                                                           edge_matrix, candidates, candidates_count,
                                                           weight_array, dpiso_order, output_limit,
                                                           call_count);
//        embedding_count = EvaluateQuery::exploreDPisoRecursiveStyle(data_graph, query_graph, dpiso_tree,
//                                                           edge_matrix, candidates, candidates_count,
//                                                           weight_array, dpiso_order, output_limit,
//                                                           call_count);
    }
    else if (input_engine_type == "CECI") {
        embedding_count = EvaluateQuery::exploreCECIStyle(data_graph, query_graph, ceci_tree, candidates, candidates_count, TE_Candidates,
                NTE_Candidates, ceci_order, output_limit, call_count);
    }
    else {
        std::cout << "The specified engine type '" << input_engine_type << "' is not supported." << std::endl;
        exit(-1);
    }

    end = std::chrono::high_resolution_clock::now();
    double enumeration_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();

#ifdef DISTRIBUTION
    std::ofstream outfile (input_distribution_file_path , std::ofstream::binary);
    outfile.write((char*)EvaluateQuery::distribution_count_, sizeof(size_t) * data_graph->getVerticesCount());
    delete[] EvaluateQuery::distribution_count_;
#endif

#ifdef SUFF
    std::cout << "Build filters..." << std::endl;
    // suff::FilterManager::stop = true;
    // suff::FilterManager::Consume();
    // suff::FilterManager::WaitForStop();
    start = std::chrono::high_resolution_clock::now();
    suff::FilterManager::Dump();
    end = std::chrono::high_resolution_clock::now();
    double filter_construct_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    printf("Filter Construction time (seconds): %.4lf\n", NANOSECTOSEC(filter_construct_time_in_ns));
    std::cout << "filter checks:\t" << std::endl;
    for (auto i = 0; i < suff::FilterManager::pattern_size; i++) {
        std::cout << suff::FilterManager::total_check[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "filter fails:\t" << std::endl;
    for (auto i = 0; i < suff::FilterManager::pattern_size; i++) {
        std::cout << suff::FilterManager::total_fail[i] << " ";
    }
    std::cout << std::endl;

    // release memory
    suff::FilterManager::Init("", "");
#endif
    std::cout << "--------------------------------------------------------------------" << std::endl;
    std::cout << "Release memories..." << std::endl;
 
    /**
     * End.
     */
    std::cout << "--------------------------------------------------------------------" << std::endl;
    double preprocessing_time_in_ns = filter_vertices_time_in_ns + build_table_time_in_ns + generate_query_plan_time_in_ns;
    double total_time_in_ns = preprocessing_time_in_ns + enumeration_time_in_ns;

    printf("Filter vertices time (seconds): %.4lf\n", NANOSECTOSEC(filter_vertices_time_in_ns));
    printf("Build table time (seconds): %.4lf\n", NANOSECTOSEC(build_table_time_in_ns));
    printf("Generate query plan time (seconds): %.4lf\n", NANOSECTOSEC(generate_query_plan_time_in_ns));
    printf("Enumerate time (seconds): %.4lf\n", NANOSECTOSEC(enumeration_time_in_ns));
    printf("Preprocessing time (seconds): %.4lf\n", NANOSECTOSEC(preprocessing_time_in_ns));
    printf("Total time (seconds): %.4lf\n", NANOSECTOSEC(total_time_in_ns));
    // printf("Memory cost (MB): %.4lf\n", BYTESTOMB(memory_cost_in_bytes));
    printf("#Embeddings: %zu\n", embedding_count);
    printf("Call Count: %zu\n", call_count);
    printf("Per Call Count Time (nanoseconds): %.4lf\n", enumeration_time_in_ns / (call_count == 0 ? 1 : call_count));
    std::cout << "End." << std::endl;
}

int main(int argc, char** argv) {
    MatchingCommand command(argc, argv);
    std::string input_query_graph_path = command.getQueryGraphFilePath();
    std::string input_data_graph_file = command.getDataGraphFilePath();
    std::string input_filter_type = command.getFilterType();
    std::string input_order_type = command.getOrderType();
    std::string input_engine_type = command.getEngineType();
    std::string input_max_embedding_num = command.getMaximumEmbeddingNum();
    std::string input_time_limit = command.getTimeLimit();
    std::string input_order_num = command.getOrderNum();
    std::string input_distribution_file_path = command.getDistributionFilePath();
    std::string input_csr_file_path = command.getCSRFilePath();
    std::string input_suff_dir = command.getSUFFDir();
    std::string input_suff_k = command.getSUFFK();
    bool create_filter = command.getCreateFilter();
    std::string input_suff_alpha = command.getSUFFAlpha();
    bool use_cache = command.getUseCache();
    std::string input_suff_selector = command.getSUFFSelector();
    /**
     * Output the command line information.
     */
    std::cout << "Command Line:" << std::endl;
    std::cout << "\tData Graph CSR: " << input_csr_file_path << std::endl;
    std::cout << "\tData Graph: " << input_data_graph_file << std::endl;
    std::cout << "\tQuery Path: " << input_query_graph_path << std::endl;
    std::cout << "\tFilter Type: " << input_filter_type << std::endl;
    std::cout << "\tOrder Type: " << input_order_type << std::endl;
    std::cout << "\tEngine Type: " << input_engine_type << std::endl;
    std::cout << "\tOutput Limit: " << input_max_embedding_num << std::endl;
    std::cout << "\tTime Limit (seconds): " << input_time_limit << std::endl;
    std::cout << "\tOrder Num: " << input_order_num << std::endl;
    std::cout << "\tDistribution File Path: " << input_distribution_file_path << std::endl;
    std::cout << "\tSUFF Directory: " << input_suff_dir << std::endl;
    std::cout << "\tSUFF K: " << input_suff_k << std::endl;
    std::cout << "\tCreate Filter: " << create_filter << std::endl;
    std::cout << "--------------------------------------------------------------------" << std::endl;

    /**
     * Load input graphs.
     */
    std::cout << "Load graphs..." << std::endl;

    auto start = std::chrono::high_resolution_clock::now();

    graph_ptr data_graph = std::make_shared<Graph>(true);

    if (input_csr_file_path.empty()) {
        data_graph->loadGraphFromFile(input_data_graph_file);
    }
    else {
        std::string degree_file_path = input_csr_file_path + "_deg.bin";
        std::string edge_file_path = input_csr_file_path + "_adj.bin";
        std::string label_file_path = input_csr_file_path + "_label.bin";
        data_graph->loadGraphFromFileCompressed(degree_file_path, edge_file_path, label_file_path);
    }

    auto end = std::chrono::high_resolution_clock::now();

    double load_graphs_time_in_ns = std::chrono::duration_cast<std::chrono::nanoseconds>(end - start).count();
    printf("Load graphs time (seconds): %.4lf\n", NANOSECTOSEC(load_graphs_time_in_ns));

    /**
     * Enumerate and execute queries.
     */

    std::vector<std::string> queries;
    for (auto &entry : fs::directory_iterator(input_query_graph_path)) {
        queries.emplace_back(entry.path().string());
    }
    std::sort(queries.begin(), queries.end());

    for (auto &path : queries) {
        if (path.rfind(".graph") == path.length() - 6) {
            std::cout << "-----Query: " << path << "-----" << std::endl;
            graph_ptr query_graph = std::make_shared<Graph>(true);
            query_graph->loadGraphFromFile(path);
            query_graph->buildCoreTable();
            std::cout << "Query Graph Meta Information" << std::endl;
            query_graph->printGraphMetaData();
            if (query_graph->getVerticesCount() == 0 || query_graph->getEdgesCount() == 0 || query_graph->getGraphMinDegree() == 0
                || query_graph->getEdgesCount() < query_graph->getVerticesCount()) {
                std::cout << "invalid query graph, skip" << std::endl;
                std::cout << "-----" << std::endl;
            } else {
                std::cout << "-----" << std::endl;
                execute_query(command, data_graph, query_graph, path);
            }
        }
    }
    return 0;
}