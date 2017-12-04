/**
 * @file
 */

#pragma once

#include "math/Random.h"
#include "core/Log.h"
#include "voxel/polyvox/Voxel.h"
#include <unordered_map>
#include <string>

namespace voxel {
namespace lsystem {

/**
 * @brief Valid characters that can be used in an axiom
 */
enum LSystemAlphabet {
	X_FORWARD = 'X',
	X_BACK = 'x',
	Y_UPWARDS = 'Y',
	Y_DOWN = 'y',
	Z_FORWARD = 'Z',
	Z_BACK = 'z',
	// state stack modifiers
	STATEPUSH = '[',
	STATEPOP = ']',
	// begin of a section that might or might not be included in the evaluation
	RANDOMBEGIN = '(',
	RANDOMEND = ')',
	RESETVOXELTOEMPTY = '0'
};

/**
 * @brief The current state of the evaluation
 */
struct LSystemState {
	glm::vec4 pos {0.0f};
	char lastVoxelType = '\0';

	// how many voxels should be generated in each direction when the axiom evaluation
	// hits a voxel
	int xFactor = 1;
	int yFactor = 1;
	int zFactor = 1;
};

/**
 * @brief The context defines what is going to be generated in the evaluation phase.
 * https://en.wikipedia.org/wiki/L-system
 */
struct LSystemContext {
	/** the initial state (e.g. ABA) */
	std::string axiom;

	/** everything that doesn't have a production rule is a terminal char
	 * e.g. A -> AB, B -> A
	 */
	std::unordered_map<char, std::string> productionRules;

	/** each type is mapped to a voxel */
	std::unordered_map<char, Voxel> voxels;

	/** how often should we evaluate and apply production rules recursivly ? */
	int generations = 1;

	/** where to put the first voxel at */
	glm::ivec3 start;
};

template<class Volume>
static bool generateVoxel(const LSystemState* state, Volume& volume, const LSystemContext& ctx) {
	if (state->lastVoxelType == '\0') {
		Log::debug("No voxel set in generation step");
		return false;
	}
	auto i = ctx.voxels.find(state->lastVoxelType);
	if (ctx.voxels.end() == i) {
		Log::error("Could not find a voxel for %c in the lsystem", state->lastVoxelType);
		return false;
	}
	const Voxel& voxel = i->second;
	const glm::ivec3 pos(glm::round(state->pos));
	Log::trace("add voxel %c to %i:%i:%i\n", state->lastVoxelType, pos.x, pos.y, pos.z);
	volume.setVoxel(state->pos, voxel);
	return true;
}

template<class Volume>
static bool evaluateState(LSystemState* state, Volume& volume, const LSystemContext& ctx, char c) {
	switch (c) {
	case LSystemAlphabet::X_FORWARD:
		for (int i = 0; i < state->xFactor; ++i) {
			generateVoxel(state, volume, ctx);
			++state->pos.x;
		}
		return true;
	case LSystemAlphabet::X_BACK:
		for (int i = 0; i < state->xFactor; ++i) {
			generateVoxel(state, volume, ctx);
			--state->pos.x;
		}
		return true;
	case LSystemAlphabet::Y_UPWARDS:
		for (int i = 0; i < state->yFactor; ++i) {
			generateVoxel(state, volume, ctx);
			++state->pos.y;
		}
		return true;
	case LSystemAlphabet::Y_DOWN:
		for (int i = 0; i < state->yFactor; ++i) {
			generateVoxel(state, volume, ctx);
			--state->pos.y;
		}
		return true;
	case LSystemAlphabet::Z_FORWARD:
		for (int i = 0; i < state->zFactor; ++i) {
			generateVoxel(state, volume, ctx);
			++state->pos.z;
		}
		return true;
	case LSystemAlphabet::Z_BACK:
		for (int i = 0; i < state->zFactor; ++i) {
			generateVoxel(state, volume, ctx);
			--state->pos.z;
		}
		return true;
	case LSystemAlphabet::STATEPUSH:
	case LSystemAlphabet::STATEPOP:
	case LSystemAlphabet::RANDOMBEGIN:
	case LSystemAlphabet::RANDOMEND:
		Log::error("Illegal character found: %c", c);
		return false;
	case LSystemAlphabet::RESETVOXELTOEMPTY:
		state->lastVoxelType = '\0';
		break;
	default:
		break;
	}
	auto i = ctx.voxels.find(c);
	if (ctx.voxels.end() == i) {
		Log::debug("Could not find a voxel for %c - maybe only a production rule", c);
		return true;
	}
	state->lastVoxelType = c;
	return true;
}

template<class Volume>
static bool expand_r(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, char c, int generations);

template<class Volume>
static bool expand(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, const std::string& axiomStr, int generations) {
	std::vector<LSystemState> newStates;
	LSystemState *currentState = state;
	for (const char *axiom = axiomStr.c_str(); *axiom != '\0'; ++axiom) {
		const char chr = *axiom;
		if (chr == LSystemAlphabet::STATEPUSH) {
			newStates.emplace_back(*currentState);
			currentState = &newStates.back();
		} else if (chr == LSystemAlphabet::STATEPOP) {
			if (newStates.empty()) {
				Log::error("Could not pop a state - the stack is empty");
				return false;
			}
			newStates.pop_back();
			if (newStates.empty()) {
				currentState = state;
			} else {
				currentState = &newStates.back();
			}
		} else if (chr == LSystemAlphabet::RANDOMEND) {
			continue;
		} else if (chr == LSystemAlphabet::RANDOMBEGIN) {
			if (random.random(0, 100) > 50) {
				int depth = 0;
				for (++axiom; *axiom != '\0'; ++axiom) {
					if (*axiom == LSystemAlphabet::RANDOMEND) {
						if (depth == 0) {
							break;
						} else {
							--depth;
						}
					} else if (*axiom == LSystemAlphabet::RANDOMBEGIN) {
						++depth;
					}
				}
			}
		} else if (!expand_r(currentState, volume, ctx, random, chr, generations)) {
			return false;
		}
	}
	return true;
}

template<class Volume>
static bool expand_r(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, char c, int generations) {
	if (generations <= 0) {
		return true;
	}

	if (!evaluateState(state, volume, ctx, c)) {
		return false;
	}
	// check whether there are further production rules for this character.
	// if there are none, quit the evaluation here
	auto iter = ctx.productionRules.find(c);
	if (iter == ctx.productionRules.end()) {
		return true;
	}
	// evaluate the new production rule
	return expand(state, volume, ctx, random, iter->second, generations - 1);
}

template<class Volume>
bool generate(Volume& volume, const LSystemContext& ctx, core::Random& random) {
	LSystemState initState;
	initState.pos = glm::vec4(ctx.start, 1.0f);
	return expand(&initState, volume, ctx, random, ctx.axiom, ctx.generations);
}

}
}
