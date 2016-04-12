#pragma once

#include "GLFunc.h"
#include <glm/glm.hpp>

namespace video {

struct GLMeshData {
	GLuint noOfIndices = 0;
	GLenum indexType = 0;
	GLuint indexBuffer = 0;
	GLuint vertexBuffer = 0;
	GLuint vertexArrayObject = 0;
	GLuint baseVertex = 0u;
	GLuint baseIndex = 0u;
	GLuint materialIndex = 0u;
	glm::ivec3 translation;
	float scale = 1.0f;
};

}
