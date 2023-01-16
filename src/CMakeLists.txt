set(GRAPH_SRC
    graph/graph.cpp)

set(UTILITY_SRC
    utility/graphoperations.cpp
    utility/commandparser.cpp
    utility/computesetintersection.cpp
    utility/bitsetoperation.cpp
    utility/QFliter.h
    utility/han/intersection_algos.cpp
    utility/han/utils/util.cpp)

set(MATCHING_SRC
    matching/BuildTable.cpp
    matching/EvaluateQuery.cpp
    matching/FilterVertices.cpp
    matching/GenerateFilteringPlan.cpp
    matching/GenerateQueryPlan.cpp
    matching/matchingcommand.cpp)

set(SUFF_SRC
    suff/bloom_filter.hpp
    suff/Filter.hpp
    suff/FilterGenerator.hpp
    suff/VFilterGenerator.hpp
    suff/FilterSelector.hpp
    suff/FilterManager.cpp)

set(SUBGRAPH_MATCHING_FILE_LIST
    main/StudyPerformance.cpp)

set(MULTI_QUERY_MATCHING_FILE_LIST
    main/RunMultipleQueries.cpp)


add_executable(smatch ${SUBGRAPH_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(smatch PUBLIC SUFF)

add_executable(smatch_dp ${SUBGRAPH_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(smatch_dp PUBLIC SUFF PUBLIC ENABLE_FAILING_SET)

add_executable(m-smatch ${MULTI_QUERY_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(m-smatch PUBLIC SUFF)

add_executable(m-smatch_dp ${MULTI_QUERY_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(m-smatch_dp PUBLIC SUFF PUBLIC ENABLE_FAILING_SET)

