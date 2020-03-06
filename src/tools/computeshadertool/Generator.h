/**
 * @file
 */

#pragma once

#include "core/io/Filesystem.h"
#include "Types.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/collection/List.h"

namespace computeshadertool {

extern bool generateSrc(const io::FilesystemPtr& filesystem,
		const core::String& _name,
		const core::String& namespaceSrc,
		const core::String& shaderDirectory,
		const core::String& sourceDirectory,
		const core::String& templateShader,
		const core::List<Kernel>& kernels,
		const core::List<Struct>& structs,
		const core::StringMap<core::String>& constants,
		const core::String& postfix,
		const core::String& shaderBuffer);

}
