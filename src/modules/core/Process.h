/**
 * @file
 */

#pragma once

#include <vector>
#include "core/String.h"

namespace core {

class Process {
public:
	/**
	 * @return 0 for success, anything else is an error
	 * @note stdout is catched in the output buffer if given
	 * @param[out] output The output buffer for stdout/stderr of the process. @c bufSize must also be @c >0 to capture the output.
	 * @param[in] bufSize The buffer size in bytes for the output buffer
	 */
	static int exec(const std::string& command, const std::vector<std::string>& arguments, const char* workdingDirectory = nullptr, size_t bufSize = 0u, char *output = nullptr);
};

}
