#include "MeshShader.h"

namespace frontend {

bool MeshShader::init() {
	if (!loadProgram("shaders/mesh")) {
		return false;
	}

	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
		return false;
	}
	if (!hasAttribute("a_texcoords")) {
		Log::error("no attribute a_texcoords found");
		return false;
	}
	if (!hasAttribute("a_norm")) {
		Log::error("no attribute a_norm found");
		return false;
	}
	if (!hasUniform("u_projection")) {
		Log::error("no uniform u_projection found");
		return false;
	}
	if (!hasUniform("u_model")) {
		Log::error("no uniform u_model found");
		return false;
	}
	if (!hasUniform("u_view")) {
		Log::error("no uniform u_view found");
		return false;
	}
	if (!hasUniform("u_texture")) {
		Log::error("no uniform u_texture found");
		return false;
	}

	return true;
}

}
