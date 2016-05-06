/**
 * @file
 */

#include "LSystemGenerator.h"
#include "voxel/WorldContext.h"
#include "core/Log.h"

namespace voxel {

void LSystemGenerator::generateVoxel(const LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx) {
	core_assert(state->lastVoxelType != '\0');
	auto i = ctx.voxels.find(state->lastVoxelType);
	core_assert_msg(i != ctx.voxels.end(), "no voxel registered for %c", state->lastVoxelType);
	if (ctx.voxels.end() == i) {
		return;
	}
	const Voxel& voxel = i->second;
	Log::trace("add voxel %c to %i:%i:%i\n", state->lastVoxelType, state->pos.x, state->pos.y, state->pos.z);
	terrainCtx.setVoxel(state->pos, voxel);
}

bool LSystemGenerator::evaluateState(LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx, char c) {
	switch (c) {
	case LSystemAlphabet::X_FORWARD:
		for (int i = 0; i < ctx.xFactor; ++i) {
			++state->pos.x;
			generateVoxel(state, terrainCtx, ctx);
		}
		return true;
	case LSystemAlphabet::X_BACK:
		for (int i = 0; i < ctx.xFactor; ++i) {
			--state->pos.x;
			generateVoxel(state, terrainCtx, ctx);
		}
		return true;
	case LSystemAlphabet::Y_UPWARDS:
		for (int i = 0; i < ctx.yFactor; ++i) {
			++state->pos.y;
			generateVoxel(state, terrainCtx, ctx);
		}
		return true;
	case LSystemAlphabet::Y_DOWN:
		for (int i = 0; i < ctx.yFactor; ++i) {
			--state->pos.y;
			generateVoxel(state, terrainCtx, ctx);
		}
		return true;
	case LSystemAlphabet::Z_FORWARD:
		for (int i = 0; i < ctx.zFactor; ++i) {
			++state->pos.z;
			generateVoxel(state, terrainCtx, ctx);
		}
		return true;
	case LSystemAlphabet::Z_BACK:
		for (int i = 0; i < ctx.zFactor; ++i) {
			--state->pos.z;
			generateVoxel(state, terrainCtx, ctx);
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
	state->lastVoxelType = c;
	return false;
}

void LSystemGenerator::expand(LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random, const std::string& axiomStr, int generations) {
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
			expand_r(currentState, terrainCtx, ctx, random, chr, generations);
		}
	}
}

void LSystemGenerator::expand_r(LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random, char c, int generations) {
	if (generations <= 0) {
		return;
	}

	if (!evaluateState(state, terrainCtx, ctx, c)) {
		Log::trace("Current pos is %i:%i:%i\n", state->pos.x, state->pos.y, state->pos.z);
	}
	auto iter = ctx.productionRules.find(c);
	if (iter == ctx.productionRules.end()) {
		return;
	}
	expand(state, terrainCtx, ctx, random, iter->second, generations - 1);
}

void LSystemGenerator::generate(TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random) {
	LSystemState initState;
	initState.pos = ctx.start;
	expand(&initState, terrainCtx, ctx, random, ctx.axiom, ctx.generations);
}

}
