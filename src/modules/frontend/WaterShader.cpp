#include "WaterShader.h"

namespace frontend {

bool WaterShader::init() {
	if (!loadProgram("shaders/water")) {
		return false;
	}

	if (!hasAttribute("a_pos")) {
		Log::error("no attribute a_pos found");
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
	if (!hasUniform("u_wavetime")) {
		Log::error("no uniform u_wavetime found");
		return false;
	}
	if (!hasUniform("u_wavewidth")) {
		Log::error("no uniform u_wavewidth found");
		return false;
	}
	if (!hasUniform("u_waveheight")) {
		Log::error("no uniform u_waveheight found");
		return false;
	}

	return true;
}

}
