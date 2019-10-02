/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <vector>

namespace animation {

struct Vertex {
	glm::vec3 pos;
	uint8_t colorIndex;
	uint8_t boneId;
	uint8_t ambientOcclusion;
	uint8_t padding;

	constexpr Vertex() :
			pos{}, colorIndex(0), boneId(0), ambientOcclusion(0), padding(0) {
	}

	Vertex(const glm::vec3 &_pos, uint8_t _colorIndex, uint8_t _boneId, uint8_t _ambientOcclusion) :
			pos(_pos), colorIndex(_colorIndex), boneId(_boneId), ambientOcclusion(_ambientOcclusion), padding(0) {
	}
};
static_assert(sizeof(Vertex) == 16, "Unexpected size of the vertex struct");

using IndexType = uint16_t;
using Vertices = std::vector<Vertex>;
using Indices = std::vector<IndexType>;

}
