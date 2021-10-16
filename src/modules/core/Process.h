/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"

namespace core {

class Process {
public:
	/**
	 * @return 0 for success, anything else is an error
	 * @note stdout is catched in the output buffer if given
	 * @param[out] output The output buffer for stdout/stderr of the process. @c bufSize must also be @c >0 to capture the output.
	 * @param[in] bufSize The buffer size in bytes for the output buffer
	 */
	static int exec(const core::String& command, const core::DynamicArray<core::String>& arguments, const char* workdingDirectory = nullptr, char *output = nullptr, size_t bufSize = 0);
	/**
	 * @brief Find the given binary in your configured PATH variable and return the absolute path for it.
	 */
	static core::String findInPath(const core::String& command);
};

}
