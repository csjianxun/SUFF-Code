#include <fstream>
#include <filesystem>
#include <algorithm>

#include "configuration/types.hpp"
#include "matching/matchingcommand.h"
#include "graph/graph.h"
#include "utility/pretty_print.h"

namespace fs = std::filesystem;

void generate(const std::string &pattern_dir,std::vector<ui> &labels, ui min_label, std::vector<ui> &candidates, ui level, ui max_level, ui &count) {
    for (auto i = min_label; i < candidates.size(); i++) {
        labels[level] = candidates[i];
        if (level == max_level - 1) {
            // triangle or square
            std::string suffix = "_triangle";
            if (max_level == 4) {
                suffix = "_square";
            }
            std::ofstream file(pattern_dir + std::to_string(++count) + suffix + ".graph", std::ios::out);
            file << "t " << max_level << " " << max_level << std::endl;
            for (auto v = 0ul; v < max_level; v++) {
                file << "v " << v << " " << labels[v] << " " << 2 << std::endl;
            }
            for (auto v = 0ul; v < max_level; v++) {
                file << "e " << v << " " << (v+1)%max_level << std::endl;
            }
            file.close();
            // twigs
            // pick a center node
            // for (ui c = 0; c < max_level; c++) {
            //     std::ofstream _file(pattern_dir + std::to_string(++count) + ".graph", std::ios::out);
            //     _file << "t " << max_level << " " << max_level - 1 << std::endl;
            //     for (auto v = 0ul; v < max_level; v++) {
            //         if (v == c) {
            //             _file << "v " << v << " " << labels[v] << " " << max_level - 1 << std::endl;
            //         } else {
            //             _file << "v " << v << " " << labels[v] << " " << 1 << std::endl;
            //         }
            //     }
            //     for (auto v = 0ul; v < max_level; v++) {
            //         if (v != c) {
            //             _file << "e " << v << " " << c << std::endl;
            //         }
            //     }
            //     _file.close();
            // }
            if (max_level == 4) {
                std::ofstream file1(pattern_dir + std::to_string(++count) + "_diamond.graph", std::ios::out);
                file1 << "t 4 5" << std::endl;
                for (auto v = 0ul; v < max_level; v++) {
                    ui degree = v % 2 == 0 ? 3 : 2;
                    file1 << "v " << v << " " << labels[v] << " " << degree << std::endl;
                }
                for (auto v = 0ul; v < max_level; v++) {
                    file1 << "e " << v << " " << (v+1)%max_level << std::endl;
                }
                file1 << "e " << 0 << " " << 2 << std::endl;
                file1.close();

                std::ofstream file2(pattern_dir + std::to_string(++count) + "_diamond.graph", std::ios::out);
                file2 << "t 4 5" << std::endl;
                for (auto v = 0ul; v < max_level; v++) {
                    ui degree = v % 2 == 1 ? 3 : 2;
                    file2 << "v " << v << " " << labels[v] << " " << degree << std::endl;
                }
                for (auto v = 0ul; v < max_level; v++) {
                    file2 << "e " << v << " " << (v+1)%max_level << std::endl;
                }
                file2 << "e " << 1 << " " << 3 << std::endl;
                file2.close();

                std::ofstream file3(pattern_dir + std::to_string(++count) + "_4clique.graph", std::ios::out);
                file3 << "t 4 6" << std::endl;
                for (auto v = 0ul; v < max_level; v++) {
                    file3 << "v " << v << " " << labels[v] << " " << 3 << std::endl;
                }
                for (auto v = 0ul; v < max_level; v++) {
                    file3 << "e " << v << " " << (v+1)%max_level << std::endl;
                }
                file3 << "e 0 2" << std::endl;
                file3 << "e 1 3" << std::endl;
                file3.close();
            }
        } else {
            generate(pattern_dir, labels, i, candidates, level+1, max_level, count);
        }
    }
}

void generate1(MatchingCommand& command) {
    std::string input_data_graph_file = command.getDataGraphFilePath();
    
    std::cout << "\tData Graph: " << input_data_graph_file << std::endl;
 
    std::string pattern_dir = "../dataset/" + input_data_graph_file + "/pattern_graph/";
    if (!fs::is_directory(pattern_dir) || !fs::exists(pattern_dir)) {
        fs::create_directories(pattern_dir);
    }

    std::vector<ui> all_labels;
    std::string label_count_file = "../dataset/" + input_data_graph_file + "/label_count.txt";
    std::ifstream file(label_count_file, std::ios::in);
    ui temp, label;
    while (file >> temp) {
        file >> label;
        all_labels.emplace_back(label);
    }
    file.close();

    std::cout << "Generate patterns..." << std::endl;
 
    // generate triangles
    std::vector<ui> v_labels(4, -1);
    std::vector<ui> candidate_labels;
    for (auto i = 0ul; i < 40; i++) {
        if (i < all_labels.size()) {
            candidate_labels.emplace_back(all_labels[i]);
        }
    }
    ui count = 0;
    generate(pattern_dir, v_labels, 0, candidate_labels, 0, 3, count);
    
    candidate_labels.clear();
    for (auto i = 0ul; i < 20; i++) {
        if (i < all_labels.size()) {
            candidate_labels.emplace_back(all_labels[i]);
        }
    }
    generate(pattern_dir, v_labels, 0, candidate_labels, 0, 4, count);  
}

// check if the rest graph is connected if removing u
bool can_remove(graph_ptr query_graph, std::map<long, long>& node_degree, ui u) {
    // start from node with degree > 0
    ui start = 0;
    for (auto &[v, d]: node_degree) {
        if (d >= 0 && v != u) {
            start = v;
        }
    }
    std::set<ui> visited;
    visited.emplace(start);
    std::vector<ui> queue;
    ui _count = 0;
    const ui* _neighbors = query_graph->getVertexNeighbors(start, _count);
    for (ui i = 0; i < _count; i++) {
        if (_neighbors[i] != u && node_degree.at(_neighbors[i]) > 0) {
            queue.emplace_back(_neighbors[i]);
            visited.emplace(_neighbors[i]);
        }
    }
    while (!queue.empty()) {
        ui next = queue.back();
        queue.pop_back();
        ui count = 0;
        const ui* neighbors = query_graph->getVertexNeighbors(next, count);
        for (ui i = 0; i < count; i++) {
            if (neighbors[i] != u && node_degree.at(neighbors[i]) > 0 && !visited.contains(neighbors[i])) {
                queue.emplace_back(neighbors[i]);
                visited.emplace(neighbors[i]);
            }
        }
    }
    for (auto &[v, d]: node_degree) {
        if (v!=u && d >= 0 && !visited.contains(v)) {
            return false;
        }
    }
    return true;
}

void generate2(MatchingCommand& command) {
    std::string input_data_graph_file = command.getDataGraphFilePath();
    
    std::cout << "\tData Graph: " << input_data_graph_file << std::endl;

    std::string query_dir = "../dataset/" + input_data_graph_file + "/query_graph/";
 
    std::string pattern_dir = "../dataset/" + input_data_graph_file + "/pattern_graph/";
    if (!fs::is_directory(pattern_dir) || !fs::exists(pattern_dir)) {
        fs::create_directories(pattern_dir);
    }

    ui pattern_count = 0;
    // enumerate queries
    int instances = 40;  // # instances in each category (9 categories)
    for (auto &fname : fs::directory_iterator(query_dir)) {
        auto path = fname.path().string();
        if (path.rfind(".graph") != std::string::npos) {
            auto last_underline = path.rfind("_");
            auto last_dot = path.rfind(".");
            auto number = std::stoi(path.substr(last_underline + 1, last_dot - last_underline - 1));
            if (number > instances) {
                continue;
            }
            std::cout << "----------query graph----------" << path << std::endl;
            graph_ptr query_graph = std::make_shared<Graph>(true);
            query_graph->loadGraphFromFile(path);
            // sample a pattern
            auto to_remove = query_graph->getVerticesCount() / 5 + 1;
            std::map<long, long> node_degree;
            for (ui i = 0; i < query_graph->getVerticesCount(); i++) {
                node_degree.emplace(i, query_graph->getVertexDegree(i));
            }
            std::cout << "original node degree:" << std::endl;
            std::cout << node_degree << std::endl;
            while (to_remove--) {
                // remove the smallest-degree node
                auto smallest = -1;
                for (auto &[u, d]: node_degree) {
                    if (d >= 0 && can_remove(query_graph, node_degree, u) && (smallest == -1 || d < node_degree.at(smallest))) {
                        smallest = u;
                    }
                }
                node_degree.at(smallest) = -1;
                ui count;
                const ui* neighbors = query_graph->getVertexNeighbors(smallest, count);
                for (ui i = 0; i < count; i++) {
                    node_degree.at(neighbors[i])--;
                }
            }
            // remove all nodes with degree <= 1
            bool stop = false;
            while (!stop) {
                auto smallest = 10000;
                auto smallest_u = -1;
                for (auto &[u, d]: node_degree) {
                    if (d >= 0 && d <= 1 && d < smallest) {
                        smallest_u = u;
                        smallest = d;
                    }
                }
                if (smallest_u != -1) {
                    node_degree.at(smallest_u) = -1;
                    ui count;
                    const ui* neighbors = query_graph->getVertexNeighbors(smallest_u, count);
                    for (ui i = 0; i < count; i++) {
                        node_degree.at(neighbors[i])--;
                    }
                } else {
                    stop = true;
                }
            }
            std::cout << "after remove, node degree:" << std::endl;
            std::cout << node_degree << std::endl;
            // get the induced graph of rest nodes
            ui v = 0, e = 0;
            std::vector<ui> old_id;
            for (auto &[u1, d1]: node_degree) {
                if (d1 > 0) {
                    v++;
                    old_id.emplace_back(u1);
                    for (auto &[u2, d2]: node_degree) {
                        if (d2 > 0 && u1 < u2 && query_graph->checkEdgeExistence(u1, u2)) {
                            e++;
                        }
                    }
                }
            }
            if (v == 0 || e == 0) {
                continue;
            }
            // write graph
            auto output_path = pattern_dir + std::to_string(++pattern_count) + ".graph";
            std::cout << "output: " << output_path << std::endl;
            std::ofstream file(output_path, std::ios::out);
            file << "t " << v << " " << e << std::endl;
            for (auto i = 0ul; i < old_id.size(); i++) {
                file << "v " << i << " " << query_graph->getVertexLabel(old_id[i]) << " " << node_degree.at(old_id[i]) << std::endl;
            }
            for (auto i1 = 0ul; i1 < old_id.size(); i1++) {
                for (auto i2 = i1+1; i2 < old_id.size(); i2++) {
                    if (query_graph->checkEdgeExistence(old_id[i1], old_id[i2])) {
                        file << "e " << i1 << " " << i2 << std::endl;
                    }
                }
            }
            file.close();
            std::cout << "xxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx" << path << std::endl;
        }
    }
}

void count_label(MatchingCommand& command) {
    std::string input_data_graph_file = command.getDataGraphFilePath();
    
    std::cout << "\tData Graph: " << input_data_graph_file << std::endl;

    std::string data_graph_file = "../dataset/" + input_data_graph_file + "/data_graph/" + input_data_graph_file + ".graph";
    graph_ptr data_graph = std::make_shared<Graph>(true);
    data_graph->loadGraphFromFile(data_graph_file);

    auto labelCount = data_graph->getLabelsCount();
    std::vector<std::pair<ui, ui>> labelFrequency; 
    for (auto i=0ul; i < labelCount; i++) {
        labelFrequency.emplace_back(i, data_graph->getLabelsFrequency(i));
    }

    std::sort(labelFrequency.begin(), labelFrequency.end(), [](std::pair<ui, ui> &a, std::pair<ui, ui> &b) {
        return a.second > b.second;
    });

    std::string label_count_file = "../dataset/" + input_data_graph_file + "/label_count.txt";
    std::ofstream file(label_count_file, std::ios::out);
    for (auto &[label, count]: labelFrequency) {
        file << label << std::endl;
    }
    file.close();
}

int main(int argc, char** argv) {
    MatchingCommand command(argc, argv);
    if (command.getAlgorithm() == "1") {
        std::cout << "Generate Method 1" << std::endl;
        generate1(command);
    } else if (command.getAlgorithm() == "2") {
        std::cout << "Generate Method 2" << std::endl;
        generate2(command);
    } else {
        std::cout << "Count Labels" << std::endl;
        count_label(command);
    }
    std::cout << "End." << std::endl;
    return 0;
}