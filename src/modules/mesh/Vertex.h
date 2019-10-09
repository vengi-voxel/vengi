/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Array.h"

namespace mesh {

#define NUM_BONES_PER_VERTEX 4

// TODO: this is a mesh vertex and doesn't belong here - but next to the mesh::Mesh class
struct Vertex {
	glm::vec3 _pos;
	glm::vec3 _norm;
	glm::vec2 _texcoords;
	glm::vec4 _color;
	uint32_t _boneIds[NUM_BONES_PER_VERTEX];
	float _boneWeights[NUM_BONES_PER_VERTEX];

	Vertex(const glm::vec3& p, const glm::vec3& n = glm::zero<glm::vec3>(), const glm::vec2& t = glm::zero<glm::vec2>(), const glm::vec4& c = glm::zero<glm::vec4>()) :
			_pos(p), _norm(n), _texcoords(t), _color(c), _boneIds { 0u, 0u, 0u, 0u }, _boneWeights { 0.0f, 0.0f, 0.0f, 0.0f } {
	}

	bool addBoneData(uint32_t boneID, float weight) {
		if (weight <= 0.0f) {
			return true;
		}

		const int size = lengthof(_boneIds);
		for (int i = 0; i < size; ++i) {
			if (glm::abs(_boneWeights[i]) <= glm::epsilon<float>()) {
				_boneIds[i] = boneID;
				_boneWeights[i] = weight;
				return true;
			}
		}

		Log::warn("more bones than we have space for - can't handle boneid %u with weight %f", boneID, weight);
		return false;
	}
};

}
