#pragma once

#include "Shader.h"
#include <vector>

namespace video {

class ShaderManager {
private:
	typedef std::vector<Shader*> Shaders;
	Shaders _shaders;
public:
	void registerShader(Shader* shader);
	void unregisterShader(Shader* shader);

	/**
	 * @brief Checks whether a shader var was changed, and recompile all shaders if needed.
	 */
	void update();
};

}
