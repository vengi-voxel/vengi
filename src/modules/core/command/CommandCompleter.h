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
		std::string dir = lastDirectory;
		if (dir.empty()) {
			dir = core::string::extractPath(str.c_str());
		}
		if (dir.empty()) {
			dir = ".";
		}
		return complete(dir, str, matches);
	};
}

inline auto fileCompleter(const core::VarPtr& lastDirectory) {
	return [=] (const std::string& str, std::vector<std::string>& matches) -> int {
		std::string dir = lastDirectory->strVal();
		if (dir.empty()) {
			dir = core::string::extractPath(str.c_str());
		}
		if (dir.empty()) {
			dir = ".";
		}
		return complete(dir, str, matches);
	};
}

}
