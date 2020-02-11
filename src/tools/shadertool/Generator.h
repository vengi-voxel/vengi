/**
 * @file
 */

#pragma once

#include "core/io/Filesystem.h"
#include "Types.h"
#include "core/String.h"

namespace shadertool {

extern bool generateSrc(const core::String& templateHeader, const core::String& templateSource, const core::String& templateConstantsHeader,
		const core::String& templateUniformBuffer, const ShaderStruct& shaderStruct,
		const io::FilesystemPtr& filesystem, const core::String& namespaceSrc, const core::String& sourceDirectory, const core::String& shaderDirectory, const core::String& postfix,
		const core::String& vertexBuffer, const core::String& geometryBuffer, const core::String& fragmentBuffer, const core::String& computeBuffer);

}
