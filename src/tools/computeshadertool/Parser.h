/**
 * @file
 */

#pragma once

#include "Types.h"
#include <vector>
#include <string>

namespace computeshadertool {

extern bool parse(const std::string& buffer,
		const std::string& computeFilename,
		std::vector<Kernel>& kernels,
		std::vector<Struct>& structs);

}
