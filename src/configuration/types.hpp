//
// Created by ssunah on 6/22/18.
//

#ifndef SUBGRAPHMATCHING_TYPES_H
#define SUBGRAPHMATCHING_TYPES_H

#include <cstdint>
#include <stdlib.h>
#include <map>
#include <vector>
#include <memory>

typedef unsigned int ui;

typedef uint32_t VertexID;
typedef ui LabelID;

enum MatchingIndexType {
    VertexCentric = 0,
    EdgeCentric = 1
};

typedef std::map<VertexID, VertexID> VMapping;
typedef std::vector<int> IntArray;
typedef std::vector<ui> UIntArray;
typedef std::vector<std::vector<ui>> UIntMatrix;
typedef std::vector<std::vector<bool>> BoolMatrix;
typedef std::vector<bool> BoolArray;

class TreeNode {
public:
    VertexID id_;
    VertexID parent_;
    ui level_;
    ui under_level_count_;
    ui children_count_;
    ui bn_count_;
    ui fn_count_;
    UIntArray under_level_;
    UIntArray children_;
    UIntArray bn_;
    UIntArray fn_;
    size_t estimated_embeddings_num_;
public:
    TreeNode() {
        id_ = 0;
        parent_ = 0;
        level_ = 0;
        under_level_count_ = 0;
        children_count_ = 0;
        bn_count_ = 0;
        fn_count_ = 0;
        estimated_embeddings_num_ = 0;
    }

    void initialize(const ui size) {
        under_level_.resize(size);
        bn_.resize(size);
        fn_.resize(size);
        children_.resize(size);
    }
};

typedef std::vector<TreeNode> TreeNodeArray;

class Edges {
public:
    UIntArray offset_;
    UIntArray edge_;
    ui vertex_count_;
    ui edge_count_;
    ui max_degree_;
public:
    Edges() {
        vertex_count_ = 0;
        edge_count_ = 0;
        max_degree_ = 0;
    }
};

typedef std::shared_ptr<Edges> EdgesPtr;

typedef std::vector<std::vector<EdgesPtr>> EdgesPtrMatrix;

#endif //SUBGRAPHMATCHING_TYPES_H
