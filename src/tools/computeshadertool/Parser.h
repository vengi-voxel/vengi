/**
 * @file
 */

#pragma once

#include "Types.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/collection/List.h"

namespace computeshadertool {

extern bool parse(const core::String& buffer,
		const core::String& computeFilename,
		core::List<Kernel>& kernels,
		core::List<Struct>& structs,
		core::StringMap<core::String>& constants);

}
