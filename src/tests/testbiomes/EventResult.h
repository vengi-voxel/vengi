/**
 * @file
 */

#pragma once

#include <stdint.h>
#include <glm/vec2.hpp>
#include <glm/vec3.hpp>

enum class EventType {
	RecalcBiomesForPosition
};

struct Event {
	EventType type;
};

struct RecalcEvent : public Event {
	inline RecalcEvent(const glm::ivec3 &_pos) : pos(_pos) {
		type = EventType::RecalcBiomesForPosition;
	}
	glm::vec3 pos;
};

enum class ResultType {
	BiomesTexture
};

struct Result {
	ResultType type;
};

struct BiomesTextureResult : public Result {
	inline BiomesTextureResult(uint8_t *_data, const glm::ivec2 &_size) : data(_data), size(_size) {
		type = ResultType::BiomesTexture;
	}
	uint8_t *data;
	glm::ivec2 size;
};
