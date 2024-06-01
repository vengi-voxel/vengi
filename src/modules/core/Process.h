/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"

namespace io {
class WriteStream;
}

namespace core {

class Process {
public:
	/**
	 * @return 0 for success, anything else is an error
	 * @note stdout is catched in the output buffer if given
	 * @param[out] stream The output buffer for stdout/stderr of the process.
	 */
	static int exec(const core::String &command, const core::DynamicArray<core::String> &arguments,
					const char *workingDirectory = nullptr, io::WriteStream *stream = nullptr);
};

} // namespace core
