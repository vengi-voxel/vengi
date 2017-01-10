/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/GLM.h"
#include "core/Common.h"

namespace video {

struct GLMeshData {
	GLuint noOfIndices = 0u;
	GLuint noOfVertices = 0u;
	GLuint baseVertex = 0u;
	GLuint baseIndex = 0u;
	GLuint materialIndex = 0u;

	// TODO: remove everything below this line - use the VertexBuffer class instead
	// don't change the order of these three entries here - they are created and deleted in one step
	GLuint indexBuffer = 0u;
	GLuint vertexBuffer = 0u;
	// used for instanced rendering
	GLuint offsetBuffer = 0u;
	GLuint vertexArrayObject = 0u;
	glm::mat4 model;
	int amount = 1;
	// this can only be u8vec3 because the mesh chunk size is small enough
	std::vector<glm::vec3> instancedPositions;

	inline void shutdown() {
		if (indexBuffer != 0u) {
			glDeleteBuffers(2, &indexBuffer);
			indexBuffer = 0u;
			vertexBuffer = 0u;
		}
		if (offsetBuffer != 0u) {
			glDeleteBuffers(1, &offsetBuffer);
			offsetBuffer = 0u;
		}
		if (vertexArrayObject != 0u) {
			glDeleteVertexArrays(1, &vertexArrayObject);
			vertexArrayObject = 0u;
		}
	}
};

}
