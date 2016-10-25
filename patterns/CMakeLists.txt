INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/patterns)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/engine)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/sgf)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/utils)

########### patterns library ###############

SET(patterns_STAT_SRCS
    connections.c
    helpers.c
    transform.c
    )

ADD_LIBRARY(patterns STATIC ${patterns_STAT_SRCS})
