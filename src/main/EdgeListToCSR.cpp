//
// Created by yche on 10/29/19.
//
#include <cassert>
#include <algorithm>
#include <vector>
#include <fstream>
#include <iostream>
#include <sstream>
#include <chrono>
#include <tuple>
#include <random>
// using namespace std;

std::vector<std::pair<int, int>> GetEdgeList(std::string &input_file_path, int &max_ele) {
    std::vector<std::pair<int, int>> lines;

    std::ifstream ifs(input_file_path);

    while (ifs.good()) {
        std::string tmp_str;
        std::stringstream ss;
        std::getline(ifs, tmp_str);
        if (!ifs.good())
            break;
        if (tmp_str[0] != '#') {
            ss.clear();
            ss << tmp_str;
            int first, second;
            ss >> first >> second;

            if (first > second) {
                std::swap(first, second);
            }
            // 1st case first == second: skip these self loop, (i,i)
            // 2nd case first > second: unique (i,j), (j,i)

            assert(first < INT32_MAX and second < INT32_MAX);
            if (second > max_ele)
                max_ele = second;
            lines.emplace_back(first, second);
        }
    }
    std::sort(lines.begin(), lines.end(), [](const std::pair<int, int> &left, const std::pair<int, int> &right) {
        if (left.first == right.first) {
            return left.second < right.second;
        }
        return left.first < right.first;
    });
    return lines;
};

bool IsAlreadyCSROrder(std::vector<std::pair<int, int>> &lines) {
    int cur_src_vertex = -1;
    int prev_dst_val = -1;
    auto line_count = 0u;
    for (const auto &line : lines) {
        int src, dst;
        std::tie(src, dst) = line;
        if (src >= dst) {
            std::cout << "src >= dst" << "\n";
            return false;
        }

        if (src == cur_src_vertex) {
            if (dst < prev_dst_val) {
                std::cout << "dst < prev_dst_val" << "\n";
                std::cout << "cur line:" << line_count << "\n";
                return false;
            }
        } else {
            cur_src_vertex = src;
        }
        prev_dst_val = dst;
        line_count++;
    }
    return true;
}

void WriteToOutputFiles(std::string &deg_output_file, std::string &adj_output_file, std::string& label_output_file,
                        int num_labels, std::vector<std::pair<int, int>> &lines, int max_ele) {
    auto vertex_num = static_cast<unsigned long>(max_ele + 1);
    auto edge_num = lines.size();
    std::vector<int> degree_arr(vertex_num, 0);
    std::vector<std::vector<int>> matrix(vertex_num);

    std::ofstream deg_ofs(deg_output_file, std::ios::binary);

    for (const auto &line : lines) {
        int src, dst;
        std::tie(src, dst) = line;
        degree_arr[src]++;
        degree_arr[dst]++;
        matrix[src].emplace_back(dst);
        matrix[dst].emplace_back(src);
    }

    std::cout << "begin write" << std::endl;
    int int_size = sizeof(int);
    deg_ofs.write(reinterpret_cast<const char *>(&int_size), 4);
    deg_ofs.write(reinterpret_cast<const char *>(&vertex_num), 4);
    deg_ofs.write(reinterpret_cast<const char *>(&edge_num), 4);
    deg_ofs.write(reinterpret_cast<const char *>(&degree_arr.front()), degree_arr.size() * 4);

    std::cout << "finish degree write..." << std::endl;
    std::ofstream adj_ofs(adj_output_file, std::ios::binary);
    for (auto &adj_arr: matrix) {
        adj_ofs.write(reinterpret_cast<const char *>(&adj_arr.front()), adj_arr.size() * 4);
    }
    std::cout << "finish edge write..." << std::endl;

    std::vector<int> labels(vertex_num);
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, num_labels - 1);

    for (int i = 0; i < vertex_num; ++i) {
        labels[i] = dis(gen);
    }
    assert(labels.size() == degree_arr.size());
    std::ofstream label_ofs(label_output_file, std::ios::binary);
    label_ofs.write(reinterpret_cast<const char *>(&labels.front()), labels.size() * 4);
    std::cout << "finish write..." << std::endl;
}

int main(int argc, char *argv[]) {
    std::string input_file_path(argv[1]);
    std::string output_file_path(argv[2]);
    std::string num_labels(argv[3]);

    std::string deg_output_file_path = output_file_path + "_deg.bin";
    std::string adj_output_file_path = output_file_path + "_adj.bin";
    std::string label_output_file_path = output_file_path + "_label.bin";
    int vertex_num = std::stoi(num_labels);

    int max_ele = -1;
    // using namespace std::chrono;
    auto io_start = std::chrono::high_resolution_clock::now();

#ifdef WITHGPERFTOOLS
    cout << "\nwith google perf start\n";
    ProfilerStart("converterProfile.log");
#endif
    auto lines = GetEdgeList(input_file_path, max_ele);

    auto io_end = std::chrono::high_resolution_clock::now();
    std::cout << "1st, read file and parse string cost:" << std::chrono::duration_cast<std::chrono::milliseconds>(io_end - io_start).count()
         << " ms\n";
#ifdef WITHGPERFTOOLS
    cout << "\nwith google perf end\n";
    ProfilerStop();
#endif

    std::cout << "max vertex id:" << max_ele << "\n";
    std::cout << "number of edges:" << lines.size() << "\n";

    auto check_start = std::chrono::high_resolution_clock::now();
    if (IsAlreadyCSROrder(lines)) {
        std::cout << "already csr" << "\n";
        auto check_end = std::chrono::high_resolution_clock::now();
        std::cout << "2nd, check csr representation cost:" << std::chrono::duration_cast<std::chrono::milliseconds>(check_end - check_start).count()
             << " ms\n";
        WriteToOutputFiles(deg_output_file_path, adj_output_file_path, label_output_file_path, vertex_num, lines, max_ele);
        auto write_end = std::chrono::high_resolution_clock::now();
        std::cout << "3rd, construct csr and write file cost:" << std::chrono::duration_cast<std::chrono::milliseconds>(write_end - check_end).count()
             << " ms\n";
    }


}
