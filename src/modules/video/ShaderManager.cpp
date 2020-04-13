/**
 * @file
 */

#include "ShaderManager.h"
#include "Shader.h"
#include "core/Var.h"
#include "core/Log.h"
#include "core/Trace.h"
#include <algorithm>

namespace video {

void ShaderManager::registerShader(Shader* shader) {
	_shaders.push_back(shader);
}

void ShaderManager::unregisterShader(Shader* shader) {
	auto i = std::find(_shaders.begin(), _shaders.end(), shader);
	if (i == _shaders.end()) {
		return;
	}
	_shaders.erase(i);
}

void ShaderManager::update() {
	core_trace_scoped(ShaderManagerUpdate);
	if (!core::Var::hasDirtyShaderVars()) {
		return;
	}

	Log::debug("Reload shaders");
	Shaders copy = _shaders;
	for (Shader* shader : copy) {
		shader->reload();
	}
}

}
