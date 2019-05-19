/**
 * @file
 */

#pragma once

#include <string>
#include <vector>
#include "core/Var.h"

namespace core {

extern int complete(std::string dir, const std::string& str, std::vector<std::string>& matches, const char* pattern);

inline auto fileCompleter(const std::string& lastDirectory, const char* pattern = "*") {
	return [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		return complete(lastDirectory, str, matches, pattern);
	};
}

inline auto fileCompleter(const core::VarPtr& lastDirectory, const char* pattern = "*") {
	return [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		return complete(lastDirectory->strVal(), str, matches, pattern);
	};
}

}
