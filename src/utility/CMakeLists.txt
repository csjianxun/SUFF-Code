set(UTILITY_SRC
        graphoperations.cpp
        commandparser.cpp
        computesetintersection.cpp
        bitsetoperation.cpp
        QFliter.h
        han/intersection_algos.cpp
        han/utils/util.cpp)

add_library(utility ${UTILITY_SRC})