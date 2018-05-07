/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "voxel/RandomVoxel.h"
#include "voxel/generator/ShapeGenerator.h"
#include "BuildingGeneratorContext.h"
#include "math/Random.h"
#include <queue>

namespace voxel {
namespace building {

template<class Volume, class Voxel>
void createRoof(Volume& volume, glm::ivec3 pos, const BuildingContext& ctx, Voxel& roofVoxel, math::Random& random) {
	int d = ctx.floorDepth;
	do {
		voxel::shape::createPlaneNoCenter(volume, pos, ctx.floorWidth, d, roofVoxel);
		pos.z += 1;
		pos.y += 1;
		d -= 2;
	} while (d > 0);
}

template<class Volume, class Voxel>
int createFloor(Volume& volume, const glm::ivec3& pos, const BuildingContext& ctx, Voxel& wallVoxel, math::Random& random) {
	const int width = ctx.floorWidth - 2 * ctx.wallOffset;
	const int depth = ctx.floorDepth - 2 * ctx.wallOffset;
	const int height = ctx.floorHeight;

	const int widthLeftRight = ctx.wallStrength;
	const int depthLeftRight = depth - 2 * ctx.wallStrength;
	const int d = width - ctx.wallStrength;

	const int widthFrontBack = width;
	const int depthFrontBack = ctx.wallStrength;
	glm::ivec3 p = pos;

	std::vector<glm::ivec3> posQ;

	// ground
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), ctx.floorWidth, ctx.wallStrength, ctx.floorDepth, wallVoxel);

	// front
	p.x += ctx.wallOffset;
	p.y += ctx.wallStrength;
	p.z += ctx.wallOffset;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthFrontBack, height, depthFrontBack, wallVoxel);

	// cut out the front door
	p.x += ctx.floorWidth / 2 + ctx.doorWidth / 2;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), ctx.doorWidth, ctx.doorHeight, depthFrontBack, voxel::Voxel());
	posQ.pop_back();
	p = posQ.back();

	// back
	p.z += depth;
	p.z -= ctx.wallStrength;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthFrontBack, height, depthFrontBack, wallVoxel);
	posQ.pop_back();
	p = posQ.back();

	// right
	p.z += ctx.wallStrength;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthLeftRight, height, depthLeftRight, wallVoxel);

	// left
	p.x += d;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthLeftRight, height, depthLeftRight, wallVoxel);

	// cut out the side window
	p.z = pos.z + ctx.floorDepth / 2 - ctx.windowWidth / 2;
	p.y += ctx.floorHeight / 2 - ctx.windowHeight / 2;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthLeftRight, ctx.windowHeight, ctx.windowWidth, voxel::Voxel());
	posQ.pop_back();
	p = posQ.back();

	return pos.y + ctx.floorHeight;
}

template<class Volume>
void createTower(Volume& volume, const BuildingContext& ctx, math::Random& random) {
	const RandomVoxel wallVoxel(VoxelType::Wall, random);
	glm::ivec3 pos = ctx.pos;
	for (int i = 0; i < ctx.floors; ++i) {
		pos.y = createFloor(volume, pos, ctx, wallVoxel, random);
	}

	const int width = ctx.floorWidth - 2 * ctx.wallOffset;
	const int depth = ctx.floorDepth - 2 * ctx.wallOffset;
	const int height = ctx.wallStrength;

	pos.x += ctx.wallOffset;
	pos.z += ctx.wallOffset;
	pos.y += 1;
	voxel::shape::createCubeNoCenter(volume, pos, width, ctx.wallStrength, depth, wallVoxel);

	const int widthLeftRight = ctx.wallStrength;
	const int depthLeftRight = depth - 2 * ctx.wallStrength;
	const int d = width - ctx.wallStrength;

	const int widthFrontBack = width;
	const int depthFrontBack = ctx.wallStrength;
	glm::ivec3 p = pos;
	std::vector<glm::ivec3> posQ;

	// front
	p.y += ctx.wallStrength;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthFrontBack, height, depthFrontBack, wallVoxel);

	// back
	p.z += depth;
	p.z -= ctx.wallStrength;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthFrontBack, height, depthFrontBack, wallVoxel);
	posQ.pop_back();
	p = posQ.back();

	// right
	p.z += ctx.wallStrength;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthLeftRight, height, depthLeftRight, wallVoxel);
	p = posQ.back();

	// left
	p.x += d;
	posQ.push_back(p);
	voxel::shape::createCubeNoCenter(volume, posQ.back(), widthLeftRight, height, depthLeftRight, wallVoxel);
}

template<class Volume>
void createHouse(Volume& volume, const BuildingContext& ctx, math::Random& random) {
	const RandomVoxel wallVoxel(VoxelType::Wall, random);
	glm::ivec3 pos = ctx.pos;
	for (int i = 0; i < ctx.floors; ++i) {
		pos.y = createFloor(volume, pos, ctx, wallVoxel, random);
	}

	const RandomVoxel roofVoxel(VoxelType::Roof, random);
	createRoof(volume, pos, ctx, roofVoxel, random);
}

template<class Volume>
void createBuilding(Volume& volume, const glm::ivec3& pos, BuildingType type) {
	math::Random random(pos.x + pos.y + pos.z);
	BuildingContext ctx;
	ctx.pos = pos;
	switch (type) {
	case BuildingType::Tower:
		ctx.wallOffset = 1;
		ctx.wallStrength = 2;
		ctx.floorWidth = ctx.floorDepth = random.random(20, 30);
		ctx.floors = random.random(2, 4);
		createTower(volume, ctx, random);
		break;
	case BuildingType::House:
		ctx.wallOffset = 1;
		ctx.floorWidth = ctx.floorDepth = random.random(20, 30);
		createHouse(volume, ctx, random);
		break;
	case BuildingType::Max:
		break;
	}
}

}
}
