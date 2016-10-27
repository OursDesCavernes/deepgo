INCLUDE_DIRECTORIES(${CMAKE_CURRENT_SOURCE_DIR})
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/sgf)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/utils)


########### engine library ###############

SET(engine_STAT_SRCS
    board.c
    boardlib.c
    cache.c
    clock.c
    endgame.c
    filllib.c
    genmove.c
    globals.c
    handicap.c
    hash.c
    interface.c
    movelist.c
    printutils.c
    reading.c
    sgffile.c
    showbord.c
    unconditional.c
    utils.c
    )

ADD_LIBRARY(engine STATIC ${engine_STAT_SRCS})


########### board library ###############

SET(board_STAT_SRCS
    board.c
    boardlib.c
    hash.c
    printutils.c
    )

ADD_LIBRARY(board STATIC ${board_STAT_SRCS})
