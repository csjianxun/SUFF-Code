set(GRAPH_SRC
        ../graph/graph.cpp)

set(SUBGRAPH_MATCHING_FILE_LIST
        StudyPerformance.cpp)

set(GRAPH_CONVERTER_FILE_LIST
        GraphConverter.cpp)

# match without SUFF
add_executable(match ${SUBGRAPH_MATCHING_FILE_LIST} ${GRAPH_SRC})
target_link_libraries(match matching suff utility)

# match with SUFF
add_executable(smatch ${SUBGRAPH_MATCHING_FILE_LIST} ${GRAPH_SRC})
target_link_libraries(smatch matching suff utility)
target_compile_definitions(smatch public SUFF=1)

# add_executable(GraphConverter ${GRAPH_CONVERTER_FILE_LIST} ${GRAPH_SRC})
# target_link_libraries(GraphConverter)

# add_executable(EdgeListConverter EdgeListToCSR.cpp)
# add_executable(ReassignLabel ReassignLabel.cpp)
# add_executable(SelectEdge SelectEdge.cpp)