/**
 * @file
 */

#pragma once

#include <functional>

namespace app {

void for_not_parallel(int start, int end, const std::function<void(int, int)> &f);
void for_parallel(int start, int end, const std::function<void(int, int)> &f, bool wait = true);
int for_parallel_size(int start, int end);

} // namespace app
