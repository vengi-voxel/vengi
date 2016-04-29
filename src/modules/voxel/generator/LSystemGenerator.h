#pragma once

#include "ShapeGenerator.h"
#include <unordered_map>
#include <string>

namespace voxel {

enum LSystemAlphabet {
	X_FORWARD = 'X',
	X_BACK = 'x',
	Y_UPWARDS = 'Y',
	Y_DOWN = 'y',
	Z_FORWARD = 'Z',
	Z_BACK = 'z',
	STATEPUSH = '[',
	STATEPOP = ']'
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
	int generations;

	// where to put the first voxel at
	glm::ivec3 start;
};

class LSystemGenerator {
private:
	static void expand_r(LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random, char c, int generations);
public:
	static void expand(LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random, const std::string& axiom, int generations);
	static bool evaluateState(LSystemState* state, char c);
	static void generateVoxel(const LSystemState* state, TerrainContext& terrainCtx, const LSystemContext& ctx);
	static void generate(TerrainContext& terrainCtx, const LSystemContext& ctx, core::Random& random);
};

}
