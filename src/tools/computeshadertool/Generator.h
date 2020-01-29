/**
 * @file
 */

#pragma once

#include "core/io/Filesystem.h"
#include "Types.h"
#include "core/String.h"
#include <vector>
#include <map>

namespace computeshadertool {

extern bool generateSrc(const io::FilesystemPtr& filesystem,
		const core::String& _name,
		const core::String& namespaceSrc,
		const core::String& shaderDirectory,
		const core::String& sourceDirectory,
		const core::String& templateShader,
		const std::vector<Kernel>& kernels,
		const std::vector<Struct>& structs,
		const std::map<std::string, std::string>& constants,
		const core::String& postfix,
		const core::String& shaderBuffer);

}
