/**
 * @file
 */

#pragma once

#include "GLFunc.h"
#include "core/GLM.h"
#include "core/Common.h"
#include "core/AABB.h"

namespace video {

struct GLMeshData {
	GLuint noOfIndices = 0u;
	GLuint noOfVertices = 0u;
	GLenum indexType = 0;
	// don't change the order of these three entries here - they are created and deleted in one step
	GLuint indexBuffer = 0u;
	GLuint vertexBuffer = 0u;
	// used for instanced rendering
	GLuint offsetBuffer = 0u;
	GLuint vertexArrayObject = 0u;
	GLuint baseVertex = 0u;
	GLuint baseIndex = 0u;
	GLuint materialIndex = 0u;
	glm::ivec3 translation = { 0, 0, 0 };
	glm::vec3 scale = { 1.0f, 1.0f, 1.0f };
	core::AABB<float> aabb = {glm::zero<glm::vec3>(), glm::zero<glm::vec3>()};
	int amount = 1;
	// this can only be u8vec3 because the mesh chunk size is small enough
	std::vector<glm::vec3> instancedPositions;

	inline void draw() const {
		if (amount == 1) {
			glDrawElementsBaseVertex(GL_TRIANGLES, noOfIndices, indexType, GL_OFFSET_CAST(sizeof(uint32_t) * baseIndex), baseVertex);
		} else {
			const int amount = (int)instancedPositions.size();
			glBindBuffer(GL_ARRAY_BUFFER, offsetBuffer);
			glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * amount, &instancedPositions[0], GL_DYNAMIC_DRAW);
			glDrawElementsInstancedBaseVertex(GL_TRIANGLES, noOfIndices, indexType, GL_OFFSET_CAST(sizeof(uint32_t) * baseIndex), amount, baseVertex);
		}
	}

	inline void bindVAO() const {
		core_assert(vertexArrayObject > 0);
		glBindVertexArray(vertexArrayObject);
	}

	inline void create(int buffers) {
		core_assert(vertexArrayObject == 0u);
		// Create the VAOs for the meshes
		glGenVertexArrays(1, &vertexArrayObject);

		core_assert(indexBuffer == 0u);
		core_assert(vertexBuffer == 0u);
		core_assert(offsetBuffer == 0u);

		// The GL_ARRAY_BUFFER will contain the list of vertex positions
		// and GL_ELEMENT_ARRAY_BUFFER will contain the indices
		// and GL_ARRAY_BUFFER will contain the offsets for instanced rendering
		core_assert(buffers == 2 || buffers == 3);
		glGenBuffers(buffers, &indexBuffer);
		core_assert(buffers == 2 || offsetBuffer > 0);
	}

	inline void deleteBuffers() {
		if (indexBuffer != 0u) {
			glDeleteBuffers(2, &indexBuffer);
			indexBuffer = 0u;
			vertexBuffer = 0u;
		}
		if (offsetBuffer != 0u) {
			glDeleteBuffers(1, &offsetBuffer);
			offsetBuffer = 0u;
		}
	}

	inline void deleteVAO() {
		if (vertexArrayObject != 0u) {
			glDeleteVertexArrays(1, &vertexArrayObject);
			vertexArrayObject = 0u;
		}
	}

	inline void shutdown() {
		deleteBuffers();
		deleteVAO();
	}
};

}
