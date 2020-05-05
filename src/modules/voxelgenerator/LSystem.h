/**
 * @file
 */

#pragma once

#include "core/StandardLib.h"
#include "core/GLM.h"
#include "core/String.h"
#include "core/Log.h"
#include "math/Random.h"
#include <glm/vec3.hpp>
#include <vector>
#include "voxel/Voxel.h"
#include "voxel/MaterialColor.h"
#include "core/Tokenizer.h"
#include <glm/gtc/random.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/rotate_vector.hpp>
#include <stack>

namespace voxelgenerator {
namespace lsystem {

struct TurtleStep {
	glm::vec3 pos { 0.0f };
	glm::vec3 rotation = glm::up;
	float width;
};

struct Rule {
	char a = 'A';
	core::String b = "B";
};

extern bool parseRules(const core::String& rulesStr, std::vector<Rule>& rules);

template<class Volume>
void generate(Volume& volume, const glm::ivec3& position, const core::String &axiom, const std::vector<Rule> &rules, float angle, float length, float width,
				 float widthIncrement, int iterations, math::Random& random, float leafRadius = 8.0f) {
	const voxel::Voxel trunkVoxel = voxel::createRandomColorVoxel(voxel::VoxelType::Wood);
	const voxel::Voxel leafVoxel = voxel::createRandomColorVoxel(voxel::VoxelType::Leaf);
	const float leafDistance = glm::round(2.0f * leafRadius);
	// apply a factor to close potential holes
	const int leavesVoxelCnt = glm::pow(leafDistance, 3) * 2.0f;

	angle = glm::radians(angle);
	core::String sentence = axiom;
	for (int i = 0; i < iterations; i++) {
		core::String nextSentence = "";

		for (size_t j = 0; j < sentence.size(); ++j) {
			const char current = sentence[j];
			bool found = false;
			for (const auto &rule : rules) {
				if (rule.a == current) {
					found = true;
					nextSentence += rule.b;
					break;
				}
			}
			if (!found) {
				nextSentence += current;
			}
		}

		sentence = nextSentence;
	}

	std::stack<TurtleStep> stack;

	TurtleStep step;
	step.width = width;

	for (size_t i = 0u; i < sentence.size(); ++i) {
		const char c = sentence[i];
		switch (c) {
		case 'F': {
			// Draw line forwards
			for (int j = 0; j < length; j++) {
				float r = step.width / 2.0f;
				for (float x = -r; x < r; x++) {
					for (float y = -r; y < r; y++) {
						for (float z = -r; z < r; z++) {
							const glm::ivec3 dest(glm::round(step.pos + glm::vec3(x, y, z)));
							volume.setVoxel(position + dest, trunkVoxel);
						}
					}
				}
				step.pos += 1.0f * step.rotation;
			}
			break;
		}

		case 'b': {
			// Move backwards (no drawing)
			for (int j = 0; j < length; j++) {
				step.pos -= 1.0f * step.rotation;
			}
			break;
		}

		case 'L': {
			// Leaf
			for (int i = 0; i < leavesVoxelCnt; i++) {
				const glm::vec3& r = glm::ballRand(leafRadius);
				const glm::ivec3 p = position + glm::ivec3(glm::round(step.pos + r));
				volume.setVoxel(p, leafVoxel);
			}
			break;
		}

		case '+':
			// Rotate right
			step.rotation = glm::rotateZ(step.rotation, angle);
			break;

		case '-':
			// Rotate left
			step.rotation = glm::rotateZ(step.rotation, -angle);
			break;

		case '>':
			// Rotate forward
			step.rotation = glm::rotateX(step.rotation, angle);
			break;

		case '<':
			// Rotate back
			step.rotation = glm::rotateX(step.rotation, -angle);
			break;

		case '#':
			// Increment width
			step.width += widthIncrement;
			break;

		case '!':
			// Decrement width
			step.width -= widthIncrement;
			step.width = glm::max(1.1f, step.width);
			break;

		case '[':
			// Push
			stack.push(TurtleStep(step));
			break;

		case ']':
			// Pop
			step = TurtleStep(stack.top());
			stack.pop();
			break;
		}
	}
}

}
}