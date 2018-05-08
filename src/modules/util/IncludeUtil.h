/**
 * @file
 */

#include <vector>
#include <string>

namespace util {

extern std::string handleIncludes(const std::string& buffer, const std::vector<std::string>& includeDirs, std::vector<std::string>* includedFiles = nullptr);

}
