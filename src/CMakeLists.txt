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
    suff/concurrentqueue.h
    suff/Filter.hpp
    suff/FilterGenerator.hpp
    suff/VFilterGenerator.hpp
    suff/FilterSelector.hpp
    suff/FilterManager.cpp)

set(SUBGRAPH_MATCHING_FILE_LIST
    main/StudyPerformance.cpp)


add_executable(smatch ${SUBGRAPH_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(smatch PUBLIC SUFF)
target_link_libraries(smatch PUBLIC ${GPERFTOOLS_LIBRARIES})

add_executable(smatch_dp ${SUBGRAPH_MATCHING_FILE_LIST} ${SUFF_SRC} ${MATCHING_SRC} ${UTILITY_SRC} ${GRAPH_SRC})
target_compile_definitions(smatch_dp PUBLIC SUFF PUBLIC ENABLE_FAILING_SET)
target_link_libraries(smatch_dp PUBLIC ${GPERFTOOLS_LIBRARIES})

add_executable(test main/Test.cpp ${SUFF_SRC} ${UTILITY_SRC} ${GRAPH_SRC})