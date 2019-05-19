/**
 * @file
 */

#pragma once

#include <string>
#include <vector>
#include "core/Var.h"

namespace core {

extern int complete(const std::string& dir, const std::string& str, std::vector<std::string>& matches);

inline auto fileCompleter(const std::string& lastDirectory) {
	return [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		const std::string& dir = lastDirectory.empty() ? "." : lastDirectory;
		return complete(dir, str, matches);
	};
}

inline auto fileCompleter(const core::VarPtr& lastDirectory) {
	return [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		const std::string& dir = lastDirectory->strVal().empty() ? "." : lastDirectory->strVal();
		return complete(dir, str, matches);
	};
}

}
