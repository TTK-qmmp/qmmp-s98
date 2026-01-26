cmake_minimum_required(VERSION 3.0)

project(s98)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})

set(HEADERS
    s98.h
    tchar.h
)

set(SOURCES
    s98.cpp
    device/emu2413/emu2413.c
    device/fmgen/file.cpp
    device/fmgen/fmgen.cpp
    device/fmgen/fmtimer.cpp
    device/fmgen/opm.cpp
    device/fmgen/opna.cpp
    device/fmgen/psg.cpp
    device/mame/fmopl.c
    device/mame/ymf262.c
    device/s98mame.cpp
    device/s98fmgen.cpp
    device/s98opll.cpp
    device/s98sng.cpp
    device/s_logtbl.c
    device/s_sng.c
)

set(ZLIB_SOURCES
    zlib/adler32.c
    zlib/compress.c
    zlib/crc32.c
    zlib/gzio.c
    zlib/uncompr.c
    zlib/deflate.c
    zlib/trees.c
    zlib/zutil.c
    zlib/inflate.c
    zlib/infback.c
    zlib/inftrees.c
    zlib/inffast.c
)

add_library(${PROJECT_NAME} STATIC ${SOURCES} ${HEADERS} ${ZLIB_SOURCES})
set_target_properties(${PROJECT_NAME} PROPERTIES OUTPUT_NAME ${PROJECT_NAME}${STATIC_LIB_SUFFIX})

install(TARGETS ${PROJECT_NAME}
    ARCHIVE DESTINATION lib
    LIBRARY DESTINATION lib
    COMPONENT library
)

install(FILES ${HEADERS} DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lib${PROJECT_NAME})
install(FILES device/s98types.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lib${PROJECT_NAME}/device)
install(FILES device/fmgen/types.h DESTINATION ${CMAKE_INSTALL_PREFIX}/include/lib${PROJECT_NAME}/device/fmgen)
