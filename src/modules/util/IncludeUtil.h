/**
 * @file
 */

#include <vector>
#include "core/String.h"
#include <utility>

namespace util {

extern std::pair<core::String, bool> handleIncludes(const core::String& buffer, const std::vector<core::String>& includeDirs, std::vector<core::String>* includedFiles = nullptr);

}
