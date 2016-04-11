#include "ColoredCubesShader.h"

namespace frontend {

bool ColoredCubesShader::init() {
	if (!loadProgram("shaders/coloredcubes")) {
		return false;
	}
	return true;
}

}
