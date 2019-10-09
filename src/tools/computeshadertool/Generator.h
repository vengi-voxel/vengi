/**
 * @file
 */

#pragma once

#include "core/io/Filesystem.h"
#include "Types.h"
#include <string>
#include <vector>
#include <map>

namespace computeshadertool {

extern bool generateSrc(const io::FilesystemPtr& filesystem,
		const std::string& _name,
		const std::string& namespaceSrc,
		const std::string& shaderDirectory,
		const std::string& sourceDirectory,
		const std::string& templateShader,
		const std::vector<Kernel>& kernels,
		const std::vector<Struct>& structs,
		const std::map<std::string, std::string>& constants,
		const std::string& postfix,
		const std::string& shaderBuffer);

}
