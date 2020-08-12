/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "core/Var.h"

namespace core {

extern int complete(core::String dir, const core::String& match, core::DynamicArray<core::String>& matches, const char* pattern);

inline auto fileCompleter(const core::String& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(lastDirectory, str, matches, pattern);
	};
}

inline auto fileCompleter(const core::VarPtr& lastDirectory, const char* pattern = "*") {
	return [=] (const core::String& str, core::DynamicArray<core::String>& matches) -> int {
		return complete(lastDirectory->strVal(), str, matches, pattern);
	};
}

}
