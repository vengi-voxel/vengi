#pragma once

#include "Shader.h"
#include <vector>
#include "core/Var.h"
#include "core/Log.h"

namespace video {

class ShaderManager {
private:
	typedef std::vector<Shader*> Shaders;
	Shaders _shaders;
public:
	void registerShader(Shader* shader) {
		_shaders.push_back(shader);
	}

	void unregisterShader(Shader* shader) {
		auto i = std::find(_shaders.begin(), _shaders.end(), shader);
		if (i == _shaders.end())
			return;
		_shaders.erase(i);
	}

	/**
	 * @brief Checks whether a shader var was changed, and recompile all shaders if needed.
	 */
	void update() {
		bool refreshShaders = core::Var::check([&] (const core::VarPtr& var) {
			if (!var->isDirty()) {
				return false;
			}
			if ((var->getFlags() & core::CV_SHADER) == 0) {
				return false;
			}
			var->markClean();
			return true;
		});

		if (!refreshShaders)
			return;

		Log::debug("Reload shaders");
		Shaders copy = _shaders;
		for (Shader* shader : copy) {
			shader->reload();
		}
	}
};

}
