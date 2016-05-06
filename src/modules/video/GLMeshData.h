/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include <glm/glm.hpp>
#include "voxel/Voxel.h"

namespace video {

constexpr int MAX_LODS = voxel::MAX_VOXEL_LOD;

struct GLMeshData {
	GLuint noOfIndices[MAX_LODS] = { 0u, 0u };
	GLenum indexType[MAX_LODS] = { 0, 0 };
	GLuint indexBuffer[MAX_LODS] = { 0u, 0u };
	GLuint vertexBuffer[MAX_LODS] = { 0u, 0u };
	GLuint vertexArrayObject[MAX_LODS] = { 0u, 0u };
	GLuint baseVertex[MAX_LODS] = { 0u, 0u };
	GLuint baseIndex[MAX_LODS] = { 0u, 0u };
	GLuint materialIndex[MAX_LODS] = { 0u, 0u };
	float scale[MAX_LODS] = { 1.0f, 2.0f };
	int numLods = 1;
	glm::ivec3 translation = { 0, 0, 0 };
};

}
