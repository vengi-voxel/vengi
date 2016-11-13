/**
 * @file
 */

#pragma once

#include "ShapeGenerator.h"
#include "core/Random.h"
#include <unordered_map>
#include <string>
#include "voxel/WorldContext.h"
#include "core/Log.h"

namespace voxel {
namespace LSystemGenerator {

enum LSystemAlphabet {
	X_FORWARD = 'X',
	X_BACK = 'x',
	Y_UPWARDS = 'Y',
	Y_DOWN = 'y',
	Z_FORWARD = 'Z',
	Z_BACK = 'z',
	STATEPUSH = '[',
	STATEPOP = ']',
	RANDOMBEGIN = '(',
	RANDOMEND = ')'
};

struct LSystemState {
	glm::ivec3 pos;
	char lastVoxelType = '\0';
};

// https://en.wikipedia.org/wiki/L-system
struct LSystemContext {
	// the initial state (e.g. ABA)
	std::string axiom;

	// e.g. A -> AB, B -> A
	// everything that doesn't have a production rule is a terminal char
	std::unordered_map<char, std::string> productionRules;

	// each type is mapped to a voxel
	std::unordered_map<char, Voxel> voxels;

	// how often should be generate?
	int generations = 1;

	// where to put the first voxel at
	glm::ivec3 start;

	int xFactor = 1;
	int yFactor = 1;
	int zFactor = 1;
};

template<class Volume>
static void generateVoxel(const LSystemState* state, Volume& volume, const LSystemContext& ctx) {
	core_assert(state->lastVoxelType != '\0');
	auto i = ctx.voxels.find(state->lastVoxelType);
	core_assert_msg(i != ctx.voxels.end(), "no voxel registered for %c", state->lastVoxelType);
	if (ctx.voxels.end() == i) {
		return;
	}
	const Voxel& voxel = i->second;
	Log::trace("add voxel %c to %i:%i:%i\n", state->lastVoxelType, state->pos.x, state->pos.y, state->pos.z);
	volume.setVoxel(state->pos, voxel);
}

template<class Volume>
static bool evaluateState(LSystemState* state, Volume& volume, const LSystemContext& ctx, char c) {
	switch (c) {
	case LSystemAlphabet::X_FORWARD:
		for (int i = 0; i < ctx.xFactor; ++i) {
			++state->pos.x;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::X_BACK:
		for (int i = 0; i < ctx.xFactor; ++i) {
			--state->pos.x;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::Y_UPWARDS:
		for (int i = 0; i < ctx.yFactor; ++i) {
			++state->pos.y;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::Y_DOWN:
		for (int i = 0; i < ctx.yFactor; ++i) {
			--state->pos.y;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::Z_FORWARD:
		for (int i = 0; i < ctx.zFactor; ++i) {
			++state->pos.z;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::Z_BACK:
		for (int i = 0; i < ctx.zFactor; ++i) {
			--state->pos.z;
			generateVoxel(state, volume, ctx);
		}
		return true;
	case LSystemAlphabet::STATEPUSH:
	case LSystemAlphabet::STATEPOP:
	case LSystemAlphabet::RANDOMBEGIN:
	case LSystemAlphabet::RANDOMEND:
		return false;
	default:
		break;
	}
	auto i = ctx.voxels.find(c);
	if (ctx.voxels.end() == i) {
		return false;
	}
	state->lastVoxelType = c;
	return false;
}

template<class Volume>
static void expand_r(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, char c, int generations);

template<class Volume>
static void expand(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, const std::string& axiomStr, int generations) {
	std::vector<LSystemState> newStates;
	LSystemState *currentState = state;
	for (const char *axiom = axiomStr.c_str(); *axiom != '\0'; ++axiom) {
		const char chr = *axiom;
		if (chr == LSystemAlphabet::STATEPUSH) {
			newStates.emplace_back(*currentState);
			currentState = &newStates.back();
		} else if (chr == LSystemAlphabet::STATEPOP) {
			core_assert(!newStates.empty());
			newStates.pop_back();
			if (newStates.empty())
				currentState = state;
			else
				currentState = &newStates.back();
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
		} else {
			expand_r(currentState, volume, ctx, random, chr, generations);
		}
	}
}

template<class Volume>
static void expand_r(LSystemState* state, Volume& volume, const LSystemContext& ctx, core::Random& random, char c, int generations) {
	if (generations <= 0) {
		return;
	}

	if (!evaluateState(state, volume, ctx, c)) {
		Log::trace("Current pos is %i:%i:%i\n", state->pos.x, state->pos.y, state->pos.z);
	}
	auto iter = ctx.productionRules.find(c);
	if (iter == ctx.productionRules.end()) {
		return;
	}
	expand(state, volume, ctx, random, iter->second, generations - 1);
}

template<class Volume>
void generate(Volume& volume, const LSystemContext& ctx, core::Random& random) {
	LSystemState initState;
	initState.pos = ctx.start;
	expand(&initState, volume, ctx, random, ctx.axiom, ctx.generations);
}

}
}
