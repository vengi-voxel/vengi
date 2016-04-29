#include "LSystemGenerator.h"
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
	PagedVolume* volume = terrainCtx.volume;
	if (volume == nullptr) {
		return;
	}
	Log::trace("add voxel %c to %i:%i:%i\n", state->lastVoxelType, state->pos.x, state->pos.y, state->pos.z);
	volume->setVoxel(state->pos, voxel);
}

bool LSystemGenerator::evaluateState(LSystemState* state, char c) {
	switch (c) {
	case LSystemAlphabet::X_FORWARD:
		++state->pos.x;
		return true;
	case LSystemAlphabet::X_BACK:
		--state->pos.x;
		return true;
	case LSystemAlphabet::Y_UPWARDS:
		++state->pos.y;
		return true;
	case LSystemAlphabet::Y_DOWN:
		--state->pos.y;
		return true;
	case LSystemAlphabet::Z_FORWARD:
		++state->pos.z;
		return true;
	case LSystemAlphabet::Z_BACK:
		--state->pos.z;
		return true;
	case LSystemAlphabet::STATEPUSH:
	case LSystemAlphabet::STATEPOP:
		return true;
	default:
		break;
	}
	state->lastVoxelType = c;
	return false;
}

void LSystemGenerator::expand(LSystemState* state, LSystemState* prevState, TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random, char c, int generations) {
	if (generations <= 0) {
		return;
	}

	if (!evaluateState(state, c)) {
		generateVoxel(state, terrainCtx, ctx);
		Log::trace("Current pos is %i:%i:%i\n", state->pos.x, state->pos.y, state->pos.z);
	}
	auto iter = ctx.productionRules.find(c);
	if (iter == ctx.productionRules.end()) {
		return;
	}
	LSystemState newState;
	for (const char chr : iter->second) {
		if (chr == LSystemAlphabet::STATEPUSH) {
			newState = *state;
			prevState = state;
			state = &newState;
		} else if (chr == LSystemAlphabet::STATEPOP) {
			state = prevState;
		} else {
			expand(state, prevState, terrainCtx, ctx, random, chr, generations - 1);
		}
	}
}

void LSystemGenerator::generate(TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random) {
	LSystemState state;
	state.pos = ctx.start;
	for (const char c : ctx.axiom) {
		expand(&state, &state, terrainCtx, ctx, random, c, ctx.generations);
	}
}

}
