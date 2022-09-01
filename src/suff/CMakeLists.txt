set(SUFF_SRC
        BloomFilter.hpp
        concurrentqueue.hpp
        Filter.hpp
        FilterGenerator.hpp
        VFilterGenerator.hpp
        FilterSelector.hpp
        FilterManager.cpp)

add_library(suff ${SUFF_SRC})