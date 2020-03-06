/**
 * @file
 */

#include "core/String.h"
#include "core/collection/List.h"
#include <utility>

namespace util {

extern std::pair<core::String, bool> handleIncludes(const core::String& buffer, const core::List<core::String>& includeDirs, core::List<core::String>* includedFiles = nullptr);

}
