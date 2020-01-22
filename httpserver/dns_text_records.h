#pragma once

#include "worktips_logger.h"

struct pow_difficulty_t;

namespace worktips {

namespace dns {

std::vector<pow_difficulty_t> query_pow_difficulty(std::error_code& ec);

void check_latest_version();

} // namespace dns
} // namespace worktips
