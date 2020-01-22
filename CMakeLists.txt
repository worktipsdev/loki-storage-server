cmake_minimum_required(VERSION 3.1)

function(WORKTIPS_ADD_SUBDIRECTORY SRC)
    get_filename_component(target ${SRC} NAME)
    if (TARGET ${target})
        return()
    endif ()
    add_subdirectory(${SRC} ${ARGN})
endfunction()

project(storage_server)

option(INTEGRATION_TEST "build for integration test" OFF)
option(DISABLE_SNODE_SIGNATURE "Generate and verify signatures for inter-snode communication"
    OFF)

if (INTEGRATION_TEST)
    add_definitions(-DDISABLE_POW)
    add_definitions(-DINTEGRATION_TEST)
endif()


if (DISABLE_SNODE_SIGNATURE)
    add_definitions(-DDISABLE_SNODE_SIGNATURE)
endif()

worktips_add_subdirectory(common)
worktips_add_subdirectory(utils)
worktips_add_subdirectory(crypto)
worktips_add_subdirectory(pow)
worktips_add_subdirectory(storage)
worktips_add_subdirectory(httpserver)
worktips_add_subdirectory(vendors/spdlog)


if (BUILD_TESTS)
    worktips_add_subdirectory(unit_test)
endif ()
