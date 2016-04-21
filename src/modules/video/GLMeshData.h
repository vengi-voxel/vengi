#pragma once

#include "GLFunc.h"
#include <glm/glm.hpp>
#include <voxel/polyvox/Voxel.h>

namespace video {

constexpr int MAX_LODS = voxel::MAX_VOXEL_LOD;
static_assert(MAX_LODS >= 1, "MAX_LODS might not be smaller than 1");

struct GLMeshData {
	GLMeshData() {
		for (int i = 0; i < MAX_LODS; ++i) {
			noOfIndices[i] = 0u;
			indexType[i] = 0;
			indexBuffer[i] = 0u;
			vertexBuffer[i] = 0u;
			vertexArrayObject[i] = 0u;
			baseVertex[i] = 0u;
			baseIndex[i] = 0u;
			materialIndex[i] = 0u;
			scale[i] = float(1 << i);
		}
	}

	GLuint noOfIndices[MAX_LODS];
	GLenum indexType[MAX_LODS];
	GLuint indexBuffer[MAX_LODS];
	GLuint vertexBuffer[MAX_LODS];
	GLuint vertexArrayObject[MAX_LODS];
	GLuint baseVertex[MAX_LODS];
	GLuint baseIndex[MAX_LODS];
	GLuint materialIndex[MAX_LODS];
	float scale[MAX_LODS];
	int numLods = 1;
	glm::ivec3 translation = { 0, 0, 0 };
};

}
