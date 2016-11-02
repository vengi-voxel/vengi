/**
 * @file
 */

#pragma once

#include <vector>
#include <string>

namespace core {

class Process {
public:
	/**
	 * @return 0 for success, anything else is an error
	 * @note stdout is catched in the output buffer if given
	 */
	static int exec (const std::string& command, const std::vector<std::string>& arguments, const char* workdingDirectory = nullptr, size_t bufSize = 0u, char *output = nullptr);
};

}
