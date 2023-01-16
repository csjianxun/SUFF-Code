//
// Created by ssunah on 6/22/18.
//

#include "graph.h"
#include <fstream>
#include <vector>
#include <algorithm>
#include <chrono>

void Graph::BuildReverseIndex() {
    reverse_index_.resize(vertices_count_);
    reverse_index_offsets_.resize(labels_count_ + 1);
    reverse_index_offsets_[0] = 0;

    ui total = 0;
    for (ui i = 0; i < labels_count_; ++i) {
        reverse_index_offsets_[i + 1] = total;
        total += labels_frequency_[i];
    }

    for (ui i = 0; i < vertices_count_; ++i) {
        LabelID label = labels_[i];
        reverse_index_[reverse_index_offsets_[label + 1]++] = i;
    }
}

#if OPTIMIZED_LABELED_GRAPH == 1
void Graph::BuildNLF() {
    nlf_.resize(vertices_count_);
    for (ui i = 0; i < vertices_count_; ++i) {
        ui count;
        const VertexID * neighbors = getVertexNeighbors(i, count);

        for (ui j = 0; j < count; ++j) {
            VertexID u = neighbors[j];
            LabelID label = getVertexLabel(u);
            if (nlf_[i].find(label) == nlf_[i].end()) {
                nlf_[i][label] = 0;
            }

            nlf_[i][label] += 1;
        }
    }
}

void Graph::BuildLabelOffset() {
    size_t labels_offset_size = (size_t)vertices_count_ * labels_count_ + 1;
    labels_offsets_.assign(labels_offset_size, 0);
    // std::fill(labels_offsets_, labels_offsets_ + labels_offset_size, 0);

    for (ui i = 0; i < vertices_count_; ++i) {
        std::sort(&neighbors_[offsets_[i]], &neighbors_[offsets_[i + 1]],
            [this](const VertexID u, const VertexID v) -> bool {
                return labels_[u] == labels_[v] ? u < v : labels_[u] < labels_[v];
            });
    }

    for (ui i = 0; i < vertices_count_; ++i) {
        LabelID previous_label = 0;
        LabelID current_label = 0;

        labels_offset_size = i * labels_count_;
        labels_offsets_[labels_offset_size] = offsets_[i];

        for (ui j = offsets_[i]; j < offsets_[i + 1]; ++j) {
            current_label = labels_[neighbors_[j]];

            if (current_label != previous_label) {
                for (ui k = previous_label + 1; k <= current_label; ++k) {
                    labels_offsets_[labels_offset_size + k] = j;
                }
                previous_label = current_label;
            }
        }

        for (ui l = current_label + 1; l <= labels_count_; ++l) {
            labels_offsets_[labels_offset_size + l] = offsets_[i + 1];
        }
    }
}

#endif

void Graph::loadGraphFromFile(const std::string &file_path) {
    std::ifstream infile(file_path);

    if (!infile.is_open()) {
        std::cout << "Can not open the graph file " << file_path << " ." << std::endl;
        exit(-1);
    }

    char type;
    infile >> type >> vertices_count_ >> edges_count_;
    offsets_.resize(vertices_count_ +  1);
    offsets_[0] = 0;

    neighbors_.resize(edges_count_ * 2);
    labels_.resize(vertices_count_);
    labels_count_ = 0;
    max_degree_ = 0;
    min_degree_ = -1;

    LabelID max_label_id = 0;
    std::vector<ui> neighbors_offset(vertices_count_, 0);

    while (infile >> type) {
        if (type == 'v') { // Read vertex.
            VertexID id;
            LabelID  label;
            ui degree;
            infile >> id >> label >> degree;

            labels_[id] = label;
            offsets_[id + 1] = offsets_[id] + degree;

            if (degree > max_degree_) {
                max_degree_ = degree;
            }

            if (degree < min_degree_ || min_degree_ < 0) {
                min_degree_ = degree;
            }

            if (labels_frequency_.find(label) == labels_frequency_.end()) {
                labels_frequency_[label] = 0;
                if (label > max_label_id)
                    max_label_id = label;
            }

            labels_frequency_[label] += 1;
        }
        else if (type == 'e') { // Read edge.
            VertexID begin;
            VertexID end;
            infile >> begin >> end;

            ui offset = offsets_[begin] + neighbors_offset[begin];
            neighbors_[offset] = end;

            offset = offsets_[end] + neighbors_offset[end];
            neighbors_[offset] = begin;

            neighbors_offset[begin] += 1;
            neighbors_offset[end] += 1;
        }
    }

    infile.close();
    labels_count_ = (ui)labels_frequency_.size() > (max_label_id + 1) ? (ui)labels_frequency_.size() : max_label_id + 1;

    for (auto element : labels_frequency_) {
        if (element.second > max_label_frequency_) {
            max_label_frequency_ = element.second;
        }
    }

    for (ui i = 0; i < vertices_count_; ++i) {
        std::sort(&neighbors_[offsets_[i]], &neighbors_[offsets_[i + 1]]);
    }

    BuildReverseIndex();

#if OPTIMIZED_LABELED_GRAPH == 1
    if (enable_label_offset_) {
        BuildNLF();
        // BuildLabelOffset();
    }
#endif
}

void Graph::loadGraphFromBinaryFile(std::ifstream &file) {
    file.read((char *)&vertices_count_, sizeof(vertices_count_));
    file.read((char *)&edges_count_, sizeof(edges_count_));
    offsets_.resize(vertices_count_ +  1);
    offsets_[0] = 0;

    neighbors_.resize(edges_count_ * 2);
    labels_.resize(vertices_count_);
    labels_count_ = 0;
    max_degree_ = 0;

    LabelID max_label_id = 0;
    std::vector<ui> neighbors_offset(vertices_count_, 0);

    // read vertices
    for (ui i = 0; i < vertices_count_; i++) {
        VertexID id = i;
        LabelID  label;
        ui degree;
        file.read((char *)&label, sizeof(label));
        file.read((char *)&degree, sizeof(degree));

        labels_[id] = label;
        offsets_[id + 1] = offsets_[id] + degree;

        if (degree > max_degree_) {
            max_degree_ = degree;
        }

        if (labels_frequency_.find(label) == labels_frequency_.end()) {
            labels_frequency_[label] = 0;
            if (label > max_label_id)
                max_label_id = label;
        }

        labels_frequency_[label] += 1;
    }

    // read edges
    for (ui i = 0; i < edges_count_; i++) {
        VertexID begin;
        VertexID end;
        file.read((char *)&begin, sizeof(begin));
        file.read((char *)&end, sizeof(end));

        ui offset = offsets_[begin] + neighbors_offset[begin];
        neighbors_[offset] = end;

        offset = offsets_[end] + neighbors_offset[end];
        neighbors_[offset] = begin;

        neighbors_offset[begin] += 1;
        neighbors_offset[end] += 1;
    }

    labels_count_ = (ui)labels_frequency_.size() > (max_label_id + 1) ? (ui)labels_frequency_.size() : max_label_id + 1;

    for (auto element : labels_frequency_) {
        if (element.second > max_label_frequency_) {
            max_label_frequency_ = element.second;
        }
    }

    for (ui i = 0; i < vertices_count_; ++i) {
        std::sort(&neighbors_[offsets_[i]], &neighbors_[offsets_[i + 1]]);
    }

    BuildReverseIndex();

#if OPTIMIZED_LABELED_GRAPH == 1
    if (enable_label_offset_) {
        BuildNLF();
        // BuildLabelOffset();
    }
#endif
}

void Graph::storeBinaryFile(std::ofstream& file) {
    file.write((char *)&vertices_count_, sizeof(vertices_count_));
    file.write((char *)&edges_count_, sizeof(edges_count_));
    for (ui i = 0; i < vertices_count_; i++) {
        LabelID label = getVertexLabel(i);
        ui degree = getVertexDegree(i);
        file.write((char *)&label, sizeof(label));
        file.write((char *)&degree, sizeof(degree));
    }
    for (VertexID i = 0; i < vertices_count_; i++) {
        ui nb_count;
        auto neighbors = getVertexNeighbors(i, nb_count);
        for (ui idx = 0; idx < nb_count; idx++) {
            if (neighbors[idx] > i) {
                VertexID j = neighbors[idx];
                file.write((char *)&i, sizeof(i));
                file.write((char *)&j, sizeof(j));
            }
        }
    }
}

void Graph::printGraphMetaData() {
    std::cout << "|V|: " << vertices_count_ << ", |E|: " << edges_count_ << ", |\u03A3|: " << labels_count_ << std::endl;
    std::cout << "Max Degree: " << max_degree_ << ", Max Label Frequency: " << max_label_frequency_ << std::endl;
}

IntArray Graph::getKCore() {
    IntArray core_table(vertices_count_);

    int vertices_count = getVerticesCount();
    int max_degree = getGraphMaxDegree();

    IntArray vertices(vertices_count);          // Vertices sorted by degree.
    IntArray position(vertices_count);          // The position of vertices in vertices array.
    IntArray degree_bin(max_degree + 1, 0);      // Degree from 0 to max_degree.
    IntArray offset(max_degree + 1);          // The offset in vertices array according to degree.

    // std::fill(degree_bin, degree_bin + (max_degree + 1), 0);

    for (int i = 0; i < vertices_count; ++i) {
        int degree = getVertexDegree(i);
        core_table[i] = degree;
        degree_bin[degree] += 1;
    }

    int start = 0;
    for (int i = 0; i < max_degree + 1; ++i) {
        offset[i] = start;
        start += degree_bin[i];
    }

    for (int i = 0; i < vertices_count; ++i) {
        int degree = getVertexDegree(i);
        position[i] = offset[degree];
        vertices[position[i]] = i;
        offset[degree] += 1;
    }

    for (int i = max_degree; i > 0; --i) {
        offset[i] = offset[i - 1];
    }
    offset[0] = 0;

    for (int i = 0; i < vertices_count; ++i) {
        int v = vertices[i];

        ui count;
        const VertexID * neighbors = getVertexNeighbors(v, count);

        for(int j = 0; j < count; ++j) {
            int u = neighbors[j];

            if (core_table[u] > core_table[v]) {

                // Get the position and vertex which is with the same degree
                // and at the start position of vertices array.
                int cur_degree_u = core_table[u];
                int position_u = position[u];
                int position_w = offset[cur_degree_u];
                int w = vertices[position_w];

                if (u != w) {
                    // Swap u and w.
                    position[u] = position_w;
                    position[w] = position_u;
                    vertices[position_u] = w;
                    vertices[position_w] = u;
                }

                offset[cur_degree_u] += 1;
                core_table[u] -= 1;
            }
        }
    }

    return core_table;
}

void Graph::buildCoreTable() {
    core_table_ = getKCore();

    for (ui i = 0; i < vertices_count_; ++i) {
        if (core_table_[i] > 1) {
            core_length_ += 1;
        }
    }
}

void Graph::loadGraphFromFileCompressed(const std::string &degree_path, const std::string &edge_path,
                                        const std::string &label_path) {
    std::ifstream deg_file(degree_path, std::ios::binary);

    if (deg_file.is_open()) {
        std::cout << "Open degree file " << degree_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot open degree file " << degree_path << " ." << std::endl;
        exit(-1);
    }

    auto start = std::chrono::high_resolution_clock::now();
    int int_size;
    deg_file.read(reinterpret_cast<char *>(&int_size), 4);
    deg_file.read(reinterpret_cast<char *>(&vertices_count_), 4);
    deg_file.read(reinterpret_cast<char *>(&edges_count_), 4);

    offsets_.resize(vertices_count_ + 1);
    UIntArray degrees(vertices_count_);

    deg_file.read(reinterpret_cast<char *>(&degrees[0]), sizeof(int) * vertices_count_);


    deg_file.close();
    deg_file.clear();

    auto end = std::chrono::high_resolution_clock::now();
    std::cout << "Load degree file time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;

    std::ifstream adj_file(edge_path, std::ios::binary);

    if (adj_file.is_open()) {
        std::cout << "Open edge file " << edge_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot open edge file " << edge_path << " ." << std::endl;
        exit(-1);
    }

    start = std::chrono::high_resolution_clock::now();
    size_t neighbors_count = (size_t)edges_count_ * 2;
    neighbors_.resize(neighbors_count);

    offsets_[0] = 0;
    for (ui i = 1; i <= vertices_count_; ++i) {
        offsets_[i] = offsets_[i - 1] + degrees[i - 1];
    }

    max_degree_ = 0;

    for (ui i = 0; i < vertices_count_; ++i) {
        if (degrees[i] > 0) {
            if (degrees[i] > max_degree_)
                max_degree_ = degrees[i];
            adj_file.read(reinterpret_cast<char *>(&neighbors_[offsets_[i]]), degrees[i] * sizeof(int));
            std::sort(&neighbors_[offsets_[i]], &neighbors_[offsets_[i + 1]]);
        }
    }

    adj_file.close();
    adj_file.clear();

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Load adj file time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;


    std::ifstream label_file(label_path, std::ios::binary);
    if (label_file.is_open())  {
        std::cout << "Open label file " << label_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot open label file " << label_path << " ." << std::endl;
        exit(-1);
    }

    start = std::chrono::high_resolution_clock::now();

    labels_.resize(vertices_count_);
    label_file.read(reinterpret_cast<char *>(&labels_[0]), sizeof(int) * vertices_count_);

    label_file.close();
    label_file.clear();

    ui max_label_id = 0;
    for (ui i = 0; i < vertices_count_; ++i) {
        ui label = labels_[i];

        if (labels_frequency_.find(label) == labels_frequency_.end()) {
            labels_frequency_[label] = 0;
            if (label > max_label_id)
                max_label_id = label;
        }

        labels_frequency_[label] += 1;
    }

    labels_count_ = (ui)labels_frequency_.size() > (max_label_id + 1) ? (ui)labels_frequency_.size() : max_label_id + 1;

    for (auto element : labels_frequency_) {
        if (element.second > max_label_frequency_) {
            max_label_frequency_ = element.second;
        }
    }

    end = std::chrono::high_resolution_clock::now();
    std::cout << "Load label file time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;

    start = std::chrono::high_resolution_clock::now();
    BuildReverseIndex();
    end = std::chrono::high_resolution_clock::now();
    std::cout << "Build reverse index file time: " << std::chrono::duration_cast<std::chrono::seconds>(end - start).count() << " seconds" << std::endl;
#if OPTIMIZED_LABELED_GRAPH == 1
    if (enable_label_offset_) {
        BuildNLF();
        // BuildLabelOffset();
    }
#endif
}

void Graph::storeComparessedGraph(const std::string& degree_path, const std::string& edge_path,
                                  const std::string& label_path) {
    UIntArray degrees(vertices_count_);
    for (ui i = 0; i < vertices_count_; ++i) {
        degrees[i] = offsets_[i + 1] - offsets_[i];
    }

    std::ofstream deg_outputfile(degree_path, std::ios::binary);

    if (deg_outputfile.is_open()) {
        std::cout << "Open degree file " << degree_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot degree edge file " << degree_path << " ." << std::endl;
        exit(-1);
    }

    int int_size = sizeof(int);
    size_t vertex_array_bytes = ((size_t)vertices_count_) * 4;
    deg_outputfile.write(reinterpret_cast<const char *>(&int_size), 4);
    deg_outputfile.write(reinterpret_cast<const char *>(&vertices_count_), 4);
    deg_outputfile.write(reinterpret_cast<const char *>(&edges_count_), 4);
    deg_outputfile.write(reinterpret_cast<const char *>(&degrees[0]), vertex_array_bytes);

    deg_outputfile.close();
    deg_outputfile.clear();

    std::ofstream edge_outputfile(edge_path, std::ios::binary);

    if (edge_outputfile.is_open()) {
        std::cout << "Open edge file " << edge_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot edge file " << edge_path << " ." << std::endl;
        exit(-1);
    }

    size_t edge_array_bytes = ((size_t)edges_count_ * 2) * 4;
    edge_outputfile.write(reinterpret_cast<const char *>(&neighbors_[0]), edge_array_bytes);

    edge_outputfile.close();
    edge_outputfile.clear();

    std::ofstream label_outputfile(label_path, std::ios::binary);

    if (label_outputfile.is_open()) {
        std::cout << "Open label file " << label_path << " successfully." << std::endl;
    }
    else {
        std::cerr << "Cannot label file " << label_path << " ." << std::endl;
        exit(-1);
    }

    size_t label_array_bytes = ((size_t)vertices_count_) * 4;
    label_outputfile.write(reinterpret_cast<const char *>(&labels_[0]), label_array_bytes);

    label_outputfile.close();
    label_outputfile.clear();
}
