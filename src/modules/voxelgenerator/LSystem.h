/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Stack.h"
#include "math/Random.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Axes.h"
#include "math/Scalar.h"

namespace palette {
class Palette;
}

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
	math::Vec3f pos{0.0f};
	math::Vec3f rotation = math::up();
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
	math::Vec3i position{0};
	core::String axiom;
	core::DynamicArray<Rule> rules;
	float angle = math::radians(25.0f);
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
	// -1 uses the current turtle voxel, otherwise a palette index in [0, 255].
	int leafColor = -1;
};

struct LSystemState {
	core::String sentence;
	math::Vec3i position{0};
	float angle = math::radians(25.0f);
	float length = 1.0f;
	float width = 1.0f;
	float widthIncrement = 0.5f;
	float leafRadius = 8.0f;
	// Segment length is restored along with the branch state.
	float lengthScale = 1.0f;
	float minLength = 0.5f;
	float angleVariation = 0.0f;
	int seed = 1;
	// -1 uses the current turtle voxel, otherwise a palette index in [0, 255].
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

/** Closest foliage green in @p palette, or -1 if the palette has no usable color. */
int defaultLeafColor(const palette::Palette &palette);

/**
 * @return Axis-aligned bounds of every voxel the L-system would write, or InvalidRegion if none.
 */
voxel::Region evaluateRegion(const LSystemConfig &conf, const voxel::Voxel &voxel);

/**
 * Scale length, width, leaf radius and related sizes so the generated shape fits
 * @p target, and set @c conf.position so it sits on the floor, centered in XZ.
 * @return @c false if the L-system produces no voxels
 */
bool adoptToRegion(LSystemConfig &conf, const voxel::Region &target);

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
		execState.step.length = math::max(0.0f, state.length);
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
		const math::Vec3f start = execState.step.pos;
		const math::Vec3f end = start + length * execState.step.rotation;
		// Subvoxel sampling connects diagonal segments and preserves fractional endpoints.
		const int samples = (int)math::ceil(length * 2.0f);
		const int diameter = math::max(1, (int)math::ceil(execState.step.width));
		const int lower = -(diameter / 2);
		for (int j = 0; j <= samples; ++j) {
			const math::Vec3i center(math::round(math::mix(start, end, (float)j / (float)samples)));
			for (int x = 0; x < diameter; ++x) {
				for (int y = 0; y < diameter; ++y) {
					for (int z = 0; z < diameter; ++z) {
						const math::Vec3i dest = center + math::Vec3i(x + lower, y + lower, z + lower);
						volume.setVoxel(state.position + dest, execState.step.voxel);
					}
				}
			}
		}
		execState.step.pos = end;
		if (state.lengthScale != 1.0f) {
			execState.step.length = math::max(state.minLength, length * state.lengthScale);
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
		const float leafDistance = math::round(2.0f * state.leafRadius);
		const int leavesVoxelCnt = (int)(math::pow(leafDistance, 3) * 2.0f);
		const voxel::Voxel leaf = state.leafColor >= 0 && state.leafColor < 256
			? voxel::createVoxel(voxel::VoxelType::Generic, state.leafColor)
			: execState.step.voxel;
		for (int j = 0; j < leavesVoxelCnt; j++) {
			math::Vec3f r;
			do {
				const float x = execState.random.randomf(-1.0f, 1.0f);
				const float y = execState.random.randomf(-1.0f, 1.0f);
				const float z = execState.random.randomf(-1.0f, 1.0f);
				r = math::Vec3f(x, y, z);
			} while (math::dot(r, r) > 1.0f);
			r *= state.leafRadius;
			const math::Vec3i p = state.position + math::Vec3i(math::round(execState.step.pos + r));
			volume.setVoxel(p, leaf);
		}
		break;
	}

	case '+':
		// Rotate right
		execState.step.rotation = math::rotateZ(execState.step.rotation, angle);
		break;

	case '-':
		// Rotate left
		execState.step.rotation = math::rotateZ(execState.step.rotation, -angle);
		break;

	case '>':
		// Rotate forward
		execState.step.rotation = math::rotateX(execState.step.rotation, angle);
		break;

	case '<':
		// Rotate back
		execState.step.rotation = math::rotateX(execState.step.rotation, -angle);
		break;

	case '#':
		// Increment width
		execState.step.width += state.widthIncrement;
		break;

	case '!':
		// Decrement width
		execState.step.width -= state.widthIncrement;
		execState.step.width = math::max(1.0f, execState.step.width);
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
