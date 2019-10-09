/**
 * @file
 */

#pragma once

#include "core/io/Filesystem.h"
#include "Types.h"
#include <string>

namespace shadertool {

extern bool generateSrc(const std::string& templateHeader, const std::string& templateSource, const std::string& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const std::string& namespaceSrc, const std::string& sourceDirectory, const std::string& shaderDirectory, const std::string& postfix,
		const std::string& vertexBuffer, const std::string& geometryBuffer, const std::string& fragmentBuffer, const std::string& computeBuffer);

}
