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
#include "voxel/RandomVoxel.h"
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
				 float widthIncrement, int iterations, math::Random& random) {
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

	for (size_t i = 0; i < sentence.size(); ++i) {
		char c = sentence[i];
		switch (c) {
		case 'F': {
			// Draw line forwards
			const voxel::RandomVoxel trunkVoxel(voxel::VoxelType::Wood, random);
			for (int j = 0; j < length; j++) {
				float r = step.width / 2.0f;
				for (float x = -r; x < r; x++) {
					for (float y = -r; y < r; y++) {
						for (float z = -r; z < r; z++) {
							const glm::ivec3& dest = glm::ivec3(glm::round(step.pos)) + glm::ivec3(x, y, z);
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
			const voxel::RandomVoxel leafVoxel(voxel::VoxelType::Leaf, random);
			for (int i = 0; i < 1000; i++) {
				auto r = glm::ballRand(8.0f);
				volume.setVoxel(position + glm::ivec3(glm::round(step.pos + r)), leafVoxel);
			}
			break;
		}

		case '+':
			// Rotate right
			step.rotation = glm::rotateZ(step.rotation, angle);
			break;

			// Rotate left
		case '-':
			step.rotation = glm::rotateZ(step.rotation, -angle);
			break;

			// Rotate forward
		case '>':
			step.rotation = glm::rotateX(step.rotation, angle);
			break;

			// Rotate back
		case '<':
			step.rotation = glm::rotateX(step.rotation, -angle);
			break;

			// Increment width
		case '#':
			step.width += widthIncrement;
			break;

			// Decrement width
		case '!':
			step.width -= widthIncrement;
			step.width = glm::max(1.1f, step.width);
			break;

			// Push
		case '[':
			stack.push(TurtleStep(step));
			break;

			// Pop
		case ']':
			step = TurtleStep(stack.top());
			stack.pop();
			break;
		}
	}
}

}
}