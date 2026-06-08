/**
 * @file
 */

#pragma once

#ifdef _WIN32
#include "Windows.h"
#include <io.h>
#else
#include <sys/select.h>
#include <unistd.h>
#endif

namespace app {

int systemTotalMemoryMiB();
double systemProcessMemoryGB();

} // namespace app
