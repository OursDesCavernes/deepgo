INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/engine)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/sgf)
INCLUDE_DIRECTORIES(${DeepGo_SOURCE_DIR}/utils)

########### gnugo executable ###############

SET(deepgo_SRCS
    main.c
    play_ascii.c
    play_gmp.c
    play_gtp.c
    play_solo.c
    play_test.c
    gmp.c
    gtp.c
    )

ADD_EXECUTABLE(deepgo ${deepgo_SRCS})

IF(WIN32)
    SET(PLATFORM_LIBRARIES wsock32)
ENDIF(WIN32)

IF(UNIX)
    SET(PLATFORM_LIBRARIES m)
ENDIF(UNIX)

TARGET_LINK_LIBRARIES(deepgo sgf engine sgf utils ${PLATFORM_LIBRARIES})

INSTALL(TARGETS deepgo DESTINATION bin)
