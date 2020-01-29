/**
 * @file
 */

#include <vector>
#include "core/String.h"
#include <utility>

namespace util {

extern std::pair<std::string, bool> handleIncludes(const std::string& buffer, const std::vector<std::string>& includeDirs, std::vector<std::string>* includedFiles = nullptr);

}
