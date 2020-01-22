cmake_minimum_required(VERSION 3.1)

add_definitions(-DSPDLOG_COMPILED_LIB)

set(SRC_FILES
    src/worktips_logger.cpp
)

set(HEADER_FILES
    include/worktips_common.h
    include/worktips_logger.h
    include/dev_sink.h
)

add_library(common STATIC ${HEADER_FILES} ${SRC_FILES})

find_package(Boost
    REQUIRED
    filesystem
)

set_property(TARGET common PROPERTY CXX_STANDARD 14)

worktips_add_subdirectory(../vendors/spdlog spdlog)

target_link_libraries(common PUBLIC spdlog::spdlog ${Boost_LIBRARIES})
target_include_directories(common PUBLIC
    ${CMAKE_CURRENT_LIST_DIR}/include
    ${Boost_INCLUDE_DIRS}
)
