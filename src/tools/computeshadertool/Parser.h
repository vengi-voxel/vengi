/**
 * @file
 */

#pragma once

#include "Types.h"
#include <vector>
#include "core/String.h"
#include <map>

namespace computeshadertool {

extern bool parse(const core::String& buffer,
		const core::String& computeFilename,
		std::vector<Kernel>& kernels,
		std::vector<Struct>& structs,
		std::map<core::String, core::String>& constants);

}
