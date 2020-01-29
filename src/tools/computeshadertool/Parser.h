/**
 * @file
 */

#pragma once

#include "Types.h"
#include <vector>
#include "core/String.h"
#include <map>

namespace computeshadertool {

extern bool parse(const std::string& buffer,
		const std::string& computeFilename,
		std::vector<Kernel>& kernels,
		std::vector<Struct>& structs,
		std::map<std::string, std::string>& constants);

}
