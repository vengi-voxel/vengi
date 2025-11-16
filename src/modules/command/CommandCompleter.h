/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/Var.h"
#include "io/Filesystem.h"
#include "io/FormatDescription.h"

namespace command {

int complete(const io::FilesystemPtr& filesystem, core::String dir, const core::String& match, core::DynamicArray<core::String>& matches, const char* pattern);
int complete(const core::String& match, core::DynamicArray<core::String>& matches, const char* const* values, size_t valueCount);
int completeDir(const io::FilesystemPtr& filesystem, core::String dir, const core::String& match, core::DynamicArray<core::String>& matches);

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::String& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory, str, matches, pattern);
	};
}

template<size_t N>
inline auto valueCompleter(const char *const (&values)[N]) {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(str, matches, values, N);
	};
}

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::VarPtr& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory->strVal(), str, matches, pattern);
	};
}

inline auto fileCompleter(const io::FilesystemPtr& filesystem, const core::VarPtr& lastDirectory, const io::FormatDescription* format) {
	const core::String &pattern = io::convertToAllFilePattern(format);
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(filesystem, lastDirectory->strVal(), str, matches, pattern.c_str());
	};
}

inline auto dirCompleter(const io::FilesystemPtr& filesystem, const core::VarPtr& lastDirectory) {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return completeDir(filesystem, lastDirectory->strVal(), str, matches);
	};
}

}
