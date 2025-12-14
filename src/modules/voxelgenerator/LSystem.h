/**
 * @file
 */

#pragma once

#include "core/GLM.h"
#include "core/String.h"
#include "core/StringUtil.h"
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
 *
 * https://paulbourke.net/fractals/
 */
namespace voxelgenerator {
/**
 * L-System (Lindenmayer Systems)
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

struct LSystemCommand {
	char command;
	const char* description;
};

extern const core::DynamicArray<LSystemCommand>& getLSystemCommands();

extern bool parseRules(const core::String& rulesStr, core::DynamicArray<Rule>& rules);

struct LSystemConfig {
	glm::ivec3 position { 0 };
	core::String axiom;
	core::DynamicArray<Rule> rules;
	float angle = glm::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	int iterations = 4;
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

struct LSystemExecutionState {
	core::Stack<TurtleStep, 512> stack;
	TurtleStep step;
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
bool step(Volume& volume, const voxel::Voxel& voxel, const LSystemState &state, LSystemExecutionState& execState) {
	if (state.sentence.empty()) {
		return false;
	}
	if (!execState.initialized) {
		execState.step.width = state.width;
		execState.step.voxel = voxel;
		execState.initialized = true;
		execState.index = 0;
	}

	if (execState.index >= state.sentence.size()) {
		return false;
	}

	const float leafDistance = glm::round(2.0f * state.leafRadius);
	// apply a factor to close potential holes
	const int leavesVoxelCnt = (int)(glm::pow(leafDistance, 3) * 2.0);

	const char c = state.sentence[execState.index];
	switch (c) {
	case 'F': {
		// Draw line forwards
		for (int j = 0; j < (int)state.length; j++) {
			float r = execState.step.width / 2.0f;
			for (float x = -r; x < r; x++) {
				for (float y = -r; y < r; y++) {
					for (float z = -r; z < r; z++) {
						const glm::ivec3 dest(glm::round(execState.step.pos + glm::vec3(x, y, z)));
						volume.setVoxel(state.position + dest, execState.step.voxel);
					}
				}
			}
			execState.step.pos += 1.0f * execState.step.rotation;
		}
		break;
	}
	case '(': {
		// Set voxel type
		++execState.index;
		size_t begin = execState.index;
		size_t slength = 0u;
		while (execState.index < state.sentence.size() && state.sentence[execState.index] >= '0' && state.sentence[execState.index] <= '9') {
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
		for (int j = 0; j < (int)state.length; j++) {
			execState.step.pos -= 1.0f * execState.step.rotation;
		}
		break;
	}

	case 'L': {
		// Leaf
		for (int j = 0; j < leavesVoxelCnt; j++) {
			const glm::vec3& r = glm::ballRand(state.leafRadius);
			const glm::ivec3 p = state.position + glm::ivec3(glm::round(execState.step.pos + r));
			volume.setVoxel(p, execState.step.voxel);
		}
		break;
	}

	case '+':
		// Rotate right
		execState.step.rotation = glm::rotateZ(execState.step.rotation, state.angle);
		break;

	case '-':
		// Rotate left
		execState.step.rotation = glm::rotateZ(execState.step.rotation, -state.angle);
		break;

	case '>':
		// Rotate forward
		execState.step.rotation = glm::rotateX(execState.step.rotation, state.angle);
		break;

	case '<':
		// Rotate back
		execState.step.rotation = glm::rotateX(execState.step.rotation, -state.angle);
		break;

	case '#':
		// Increment width
		execState.step.width += state.widthIncrement;
		break;

	case '!':
		// Decrement width
		execState.step.width -= state.widthIncrement;
		execState.step.width = glm::max(1.1f, execState.step.width);
		break;

	case '[':
		// Push
		execState.stack.push(execState.step);
		break;

	case ']':
		// Pop
		execState.step = execState.stack.top();
		execState.stack.pop();
		break;
	}
	++execState.index;
	return execState.index < state.sentence.size();
}

template<class Volume>
void generate(Volume& volume, const voxel::Voxel& voxel, const LSystemState &state) {
	LSystemExecutionState execState;
	while (step(volume, voxel, state, execState)) {
	}
}

}
}
