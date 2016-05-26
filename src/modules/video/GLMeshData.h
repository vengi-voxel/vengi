/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/GLM.h"
#include "voxel/Voxel.h"

namespace video {

struct GLMeshData {
	GLuint noOfIndices = 0u;
	GLenum indexType = 0;
	// don't change the order of these two entries here - they are created in one step
	GLuint indexBuffer = 0u;
	GLuint vertexBuffer = 0u;
	GLuint vertexArrayObject = 0u;
	GLuint baseVertex = 0u;
	GLuint baseIndex = 0u;
	GLuint materialIndex = 0u;
	glm::ivec3 translation = { 0, 0, 0 };
};

}
