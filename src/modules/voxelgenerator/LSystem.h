/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "math/Random.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Stack.h"
#include "voxel/Voxel.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/rotate_vector.hpp>
#include <glm/vec3.hpp>
#include <glm/gtc/random.hpp>

/**
 * Voxel generators
 */
namespace voxelgenerator {
/**
 * L-System
 */
namespace lsystem {

struct TurtleStep {
	glm::vec3 pos { 0.0f };
	glm::vec3 rotation = glm::up();
	float width;
	voxel::Voxel voxel;
};

struct Rule {
	char a = 'A';
	core::String b = "B";
};

extern bool parseRules(const core::String& rulesStr, core::DynamicArray<Rule>& rules);

struct LSystemConfig {
	glm::ivec3 position { 0 };
	core::String axiom;
	core::DynamicArray<Rule> rules;
	float angle = glm::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	uint8_t iterations = 4;
	float leafRadius = 8.0f;
};

struct LSystemState {
	core::String sentence;
	glm::ivec3 position { 0 };
	float angle = glm::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	float leafRadius = 8.0f;
};

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
void generate(Volume& volume, const voxel::Voxel& voxel, const LSystemState &state) {
	if (state.sentence.empty()) {
		return;
	}
	const float leafDistance = glm::round(2.0f * state.leafRadius);
	// apply a factor to close potential holes
	const int leavesVoxelCnt = (int)(glm::pow(leafDistance, 3) * 2.0);

	core::Stack<TurtleStep, 512> stack;

	TurtleStep step;
	step.width = state.width;
	step.voxel = voxel;

	for (size_t i = 0u; i < state.sentence.size(); ++i) {
		const char c = state.sentence[i];
		switch (c) {
		case 'F': {
			// Draw line forwards
			for (int j = 0; j < (int)state.length; j++) {
				float r = step.width / 2.0f;
				for (float x = -r; x < r; x++) {
					for (float y = -r; y < r; y++) {
						for (float z = -r; z < r; z++) {
							const glm::ivec3 dest(glm::round(step.pos + glm::vec3(x, y, z)));
							volume.setVoxel(state.position + dest, step.voxel);
						}
					}
				}
				step.pos += 1.0f * step.rotation;
			}
			break;
		}
		case '(': {
			// Set voxel type
			++i;
			size_t begin = i;
			size_t slength = 0u;
			while (state.sentence[i] >= '0' && state.sentence[i] <= '9') {
				++slength;
				++i;
			}
			core::String voxelString(state.sentence.c_str() + begin, slength);
			const int colorIndex = core::string::toInt(voxelString);
			if (colorIndex == 0) {
				step.voxel = voxel::Voxel();
			} else if (colorIndex > 0 && colorIndex < 256) {
				step.voxel = voxel::createVoxel(voxel::VoxelType::Generic, colorIndex);
			}
			break;
		}
		case 'b': {
			// Move backwards (no drawing)
			for (int j = 0; j < (int)state.length; j++) {
				step.pos -= 1.0f * step.rotation;
			}
			break;
		}

		case 'L': {
			// Leaf
			for (int j = 0; j < leavesVoxelCnt; j++) {
				const glm::vec3& r = glm::ballRand(state.leafRadius);
				const glm::ivec3 p = state.position + glm::ivec3(glm::round(step.pos + r));
				volume.setVoxel(p, step.voxel);
			}
			break;
		}

		case '+':
			// Rotate right
			step.rotation = glm::rotateZ(step.rotation, state.angle);
			break;

		case '-':
			// Rotate left
			step.rotation = glm::rotateZ(step.rotation, -state.angle);
			break;

		case '>':
			// Rotate forward
			step.rotation = glm::rotateX(step.rotation, state.angle);
			break;

		case '<':
			// Rotate back
			step.rotation = glm::rotateX(step.rotation, -state.angle);
			break;

		case '#':
			// Increment width
			step.width += state.widthIncrement;
			break;

		case '!':
			// Decrement width
			step.width -= state.widthIncrement;
			step.width = glm::max(1.1f, step.width);
			break;

		case '[':
			// Push
			stack.push(step);
			break;

		case ']':
			// Pop
			step = stack.top();
			stack.pop();
			break;
		}
	}
}

}
}
