/**
 * @file
 */

#include "core/Pair.h"
#include "core/String.h"
#include "core/collection/List.h"

namespace util {

extern core::Pair<core::String, bool> handleIncludes(const core::String& filename, const core::String& buffer, const core::List<core::String>& includeDirs, core::List<core::String>* includedFiles = nullptr);

}
