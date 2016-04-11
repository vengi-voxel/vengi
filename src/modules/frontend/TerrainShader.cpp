#include "TerrainShader.h"

namespace frontend {

bool TerrainShader::init() {
	if (!loadProgram("shaders/terrain")) {
		return false;
	}
	return true;
}

}
