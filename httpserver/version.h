#pragma once

#include "worktips_logger.h"

#include <iostream>

#define VERSION_MAJOR 1
#define VERSION_MINOR 0
#define VERSION_PATCH 9

#define WORKTIPS_STRINGIFY2(val) #val
#define WORKTIPS_STRINGIFY(val) WORKTIPS_STRINGIFY2(val)

#define VERSION_MAJOR_STR WORKTIPS_STRINGIFY(VERSION_MAJOR)
#define VERSION_MINOR_STR WORKTIPS_STRINGIFY(VERSION_MINOR)
#define VERSION_PATCH_STR WORKTIPS_STRINGIFY(VERSION_PATCH)

#ifndef STORAGE_SERVER_VERSION_STRING
#define STORAGE_SERVER_VERSION_STRING                                          \
    VERSION_MAJOR_STR "." VERSION_MINOR_STR "." VERSION_PATCH_STR
#endif

#ifndef STORAGE_SERVER_GIT_HASH_STRING
#define STORAGE_SERVER_GIT_HASH_STRING "?"
#endif

#ifndef STORAGE_SERVER_BUILD_TIME
#define STORAGE_SERVER_BUILD_TIME "?"
#endif

static void print_version() {
    WORKTIPS_LOG(info,
             "Worktips Storage Server v{}\n git commit hash: {}\n build time: {}",
             STORAGE_SERVER_VERSION_STRING, STORAGE_SERVER_GIT_HASH_STRING,
             STORAGE_SERVER_BUILD_TIME);
}
