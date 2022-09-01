aux_source_directory(. MATCHING_SRCS)                                        
add_library(matching ${MATCHING_SRCS})
target_link_libraries(matching utility)