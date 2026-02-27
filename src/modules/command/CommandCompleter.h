/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Tokenizer.h"
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

/**
 * @brief Completer that offers cvar names as completion matches
 */
inline auto cvarCompleter() {
	return [=] (const core::String& str, const core::Tokens &, core::DynamicArray<core::String>& matches) -> int {
		int n = 0;
		const core::String pattern = str + "*";
		core::Var::visit([&] (const core::VarPtr& var) {
			if (str.empty() || core::string::matches(var->name(), pattern)) {
				matches.push_back(var->name());
				++n;
			}
		});
		return n;
	};
}

/**
 * @brief Complete valid values for a cvar (enum valid values or boolean true/false)
 * @param cvarName The name of the cvar to look up valid values for
 * @param str The current partial input to match against
 * @param matches Output array for completion matches
 * @return Number of matches found
 */
inline int completeCvarValue(const core::String &cvarName, const core::String &str, core::DynamicArray<core::String>& matches) {
	const core::VarPtr& var = core::Var::findVar(cvarName);
	if (!var) {
		return 0;
	}
	int n = 0;
	const core::String pattern = str + "*";
	if (var->type() == core::VarType::Enum) {
		for (const core::String &validValue : var->validValues()) {
			if (str.empty() || core::string::matches(validValue, pattern)) {
				matches.push_back(validValue);
				++n;
			}
		}
	} else if (var->type() == core::VarType::Boolean) {
		if (str.empty() || core::string::matches("true", pattern)) {
			matches.push_back("true");
			++n;
		}
		if (str.empty() || core::string::matches("false", pattern)) {
			matches.push_back("false");
			++n;
		}
	}
	return n;
}

/**
 * @brief Completer that offers valid values for a cvar based on the previously entered cvar name.
 * The cvar name is expected to be at tokens[1] (the first argument after the command name).
 */
inline auto cvarValueCompleter() {
	return [=] (const core::String& str, const core::Tokens &tokens, core::DynamicArray<core::String>& matches) -> int {
		// tokens[0] = command name (e.g. "set"), tokens[1] = cvar name
		if (tokens.size() < 2) {
			return 0;
		}
		const core::String &cvarName = tokens[1];
		return completeCvarValue(cvarName, str, matches);
	};
}

}
