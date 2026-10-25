/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Stack.h"
#include "math/Random.h"
#include "voxel/Voxel.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec3.hpp>

/**
 * Voxel generators
 *
 * https://paulbourke.net/fractals/
 */
namespace voxelgenerator {
/**
 * L-System (Lindenmayer Systems)
 */
namespace lsystem {

struct TurtleStep {
	glm::vec3 pos{0.0f};
	glm::vec3 rotation = glm::up();
	float width;
	float length = 1.0f;
	voxel::Voxel voxel;
};

struct Rule {
	char a = 'A';
	core::String b = "B";
};

struct LSystemCommand {
	char command;
	const char *description;
};

extern const core::DynamicArray<LSystemCommand> &getLSystemCommands();

extern bool parseRules(const core::String &rulesStr, core::DynamicArray<Rule> &rules);

struct LSystemConfig {
	glm::ivec3 position{0};
	core::String axiom;
	core::DynamicArray<Rule> rules;
	float angle = glm::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	int iterations = 4;
	float leafRadius = 8.0f;
	// Segment length is restored along with the branch state.
	float lengthScale = 1.0f;
	float minLength = 0.5f;
	float angleVariation = 0.0f;
	int seed = 1;
	// -1 uses the current turtle voxel, otherwise a palette index in [1, 255].
	int leafColor = -1;
};

struct LSystemState {
	core::String sentence;
	glm::ivec3 position{0};
	float angle = glm::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	float leafRadius = 8.0f;
	// Segment length is restored along with the branch state.
	float lengthScale = 1.0f;
	float minLength = 0.5f;
	float angleVariation = 0.0f;
	int seed = 1;
	// -1 uses the current turtle voxel, otherwise a palette index in [1, 255].
	int leafColor = -1;
};

struct LSystemExecutionState {
	core::Stack<TurtleStep, 512> stack;
	TurtleStep step;
	math::Random random{1};
	size_t index = 0;
	bool initialized = false;
};

struct LSystemTemplate {
	core::String name;
	core::String description;
	LSystemConfig config;
};
core::DynamicArray<LSystemTemplate> defaultTemplates();

void prepareState(const LSystemConfig &conf, LSystemState &state);

/**
 * @brief Generate voxels according to the given L-System rules
 *
 * @li @c F Draw line forwards
 * @li @c ( Set voxel type
 * @li @c b Move backwards (no drawing)
 * @li @c L Leaf
 * @li @c + Rotate right
 * @li @c - Rotate left
 * @li @c > Rotate forward
 * @li @c < Rotate back
 * @li @c # Increment width
 * @li @c ! Decrement width
 * @li @c [ Push
 * @li @c ] Pop
 */
template<class Volume>
bool step(Volume &volume, const voxel::Voxel &voxel, const LSystemState &state, LSystemExecutionState &execState) {
	if (state.sentence.empty()) {
		return false;
	}
	if (!execState.initialized) {
		execState.step.width = state.width;
		execState.step.length = glm::max(0.0f, state.length);
		execState.random.setSeed((unsigned int)state.seed);
		execState.step.voxel = voxel;
		execState.initialized = true;
		execState.index = 0;
	}

	if (execState.index >= state.sentence.size()) {
		return false;
	}

	const char c = state.sentence[execState.index];
	float angle = state.angle;
	if (state.angleVariation > 0.0f && (c == '+' || c == '-' || c == '>' || c == '<')) {
		angle += execState.random.randomf(-state.angleVariation, state.angleVariation);
	}
	switch (c) {
	case 'F': {
		// Draw line forwards
		const float length = execState.step.length;
		if (length <= 0.0f) {
			break;
		}
		const glm::vec3 start = execState.step.pos;
		const glm::vec3 end = start + length * execState.step.rotation;
		// Subvoxel sampling connects diagonal segments and preserves fractional endpoints.
		const int samples = (int)glm::ceil(length * 2.0f);
		const int diameter = glm::max(1, (int)glm::ceil(execState.step.width));
		const int lower = -(diameter / 2);
		for (int j = 0; j <= samples; ++j) {
			const glm::ivec3 center(glm::round(glm::mix(start, end, (float)j / (float)samples)));
			for (int x = 0; x < diameter; ++x) {
				for (int y = 0; y < diameter; ++y) {
					for (int z = 0; z < diameter; ++z) {
						const glm::ivec3 dest = center + glm::ivec3(x + lower, y + lower, z + lower);
						volume.setVoxel(state.position + dest, execState.step.voxel);
					}
				}
			}
		}
		execState.step.pos = end;
		if (state.lengthScale != 1.0f) {
			execState.step.length = glm::max(state.minLength, length * state.lengthScale);
		}
		break;
	}
	case '(': {
		// Set voxel type
		++execState.index;
		size_t begin = execState.index;
		size_t slength = 0u;
		while (execState.index < state.sentence.size() && state.sentence[execState.index] >= '0' &&
			   state.sentence[execState.index] <= '9') {
			++slength;
			++execState.index;
		}
		core::String voxelString(state.sentence.c_str() + begin, slength);
		const int colorIndex = core::string::toInt(voxelString);
		if (colorIndex == 0) {
			execState.step.voxel = voxel::Voxel();
		} else if (colorIndex > 0 && colorIndex < 256) {
			execState.step.voxel = voxel::createVoxel(voxel::VoxelType::Generic, colorIndex);
		}
		break;
	}
	case 'b': {
		// Move backwards (no drawing)
		execState.step.pos -= execState.step.length * execState.step.rotation;
		break;
	}

	case 'L': {
		// Oversample the sphere to close potential holes.
		const float leafDistance = glm::round(2.0f * state.leafRadius);
		const int leavesVoxelCnt = (int)(glm::pow(leafDistance, 3) * 2.0f);
		const voxel::Voxel leaf = state.leafColor > 0 && state.leafColor < 256
			? voxel::createVoxel(voxel::VoxelType::Generic, state.leafColor)
			: execState.step.voxel;
		for (int j = 0; j < leavesVoxelCnt; j++) {
			glm::vec3 r;
			do {
				const float x = execState.random.randomf(-1.0f, 1.0f);
				const float y = execState.random.randomf(-1.0f, 1.0f);
				const float z = execState.random.randomf(-1.0f, 1.0f);
				r = glm::vec3(x, y, z);
			} while (glm::dot(r, r) > 1.0f);
			r *= state.leafRadius;
			const glm::ivec3 p = state.position + glm::ivec3(glm::round(execState.step.pos + r));
			volume.setVoxel(p, leaf);
		}
		break;
	}

	case '+':
		// Rotate right
		execState.step.rotation = glm::rotateZ(execState.step.rotation, angle);
		break;

	case '-':
		// Rotate left
		execState.step.rotation = glm::rotateZ(execState.step.rotation, -angle);
		break;

	case '>':
		// Rotate forward
		execState.step.rotation = glm::rotateX(execState.step.rotation, angle);
		break;

	case '<':
		// Rotate back
		execState.step.rotation = glm::rotateX(execState.step.rotation, -angle);
		break;

	case '#':
		// Increment width
		execState.step.width += state.widthIncrement;
		break;

	case '!':
		// Decrement width
		execState.step.width -= state.widthIncrement;
		execState.step.width = glm::max(1.0f, execState.step.width);
		break;

	case '[':
		// Push
		execState.stack.push(execState.step);
		break;

	case ']':
		// Pop
		if (!execState.stack.empty()) {
			execState.step = execState.stack.top();
			execState.stack.pop();
		}
		break;
	}
	++execState.index;
	return execState.index < state.sentence.size();
}

} // namespace lsystem
} // namespace voxelgenerator
