/**
 * @file
 */

#pragma once

#include "io/Filesystem.h"
#include "Types.h"
#include <string>
#include <vector>

namespace computeshadertool {

extern bool generateSrc(const io::FilesystemPtr& filesystem,
		const std::string& _name,
		const std::string& namespaceSrc,
		const std::string& shaderDirectory,
		const std::string& sourceDirectory,
		const std::string& templateShader,
		const std::vector<Kernel>& kernels,
		const std::vector<Struct>& structs);

}
