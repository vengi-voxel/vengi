#include "ShapeGenerator.h"

namespace voxel {

void ShapeGenerator::createCirclePlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
	const int xRadius = width / 2;
	const int zRadius = depth / 2;
	const double minRadius = std::min(xRadius, zRadius);
	const double ratioX = xRadius / minRadius;
	const double ratioZ = zRadius / minRadius;

	for (int z = -zRadius; z <= zRadius; ++z) {
		for (int x = -xRadius; x <= xRadius; ++x) {
			const double distance = glm::pow(x / ratioX, 2.0) + glm::pow(z / ratioZ, 2.0);
			if (distance > radius) {
				continue;
			}
			const glm::ivec3 pos(center.x + x, center.y, center.z + z);
			ctx.volume->setVoxel(pos, voxel);
		}
	}
}

void ShapeGenerator::createCube(TerrainContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int w = width / 2;
	const int h = height / 2;
	const int d = depth / 2;
	for (int x = -w; x < width - w; ++x) {
		for (int y = -h; y < height - h; ++y) {
			for (int z = -d; z < depth - d; ++z) {
				const glm::ivec3 pos(center.x + x, center.y + y, center.z + z);
				ctx.volume->setVoxel(pos, voxel);
			}
		}
	}
}

void ShapeGenerator::createPlane(TerrainContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel) {
	createCube(ctx, center, width, 1, depth, voxel);
}

void ShapeGenerator::createEllipse(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double adjustedMinRadius = minDimension / 2.0;
	const double heightFactor = heightLow / adjustedMinRadius;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs(y / heightFactor);
		const double circleRadius = glm::pow(adjustedMinRadius + 0.5, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}

void ShapeGenerator::createCone(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = 1.0 - ((y + heightLow) / double(height));
		const double circleRadius = glm::pow(percent * minRadius, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}

void ShapeGenerator::createDome(TerrainContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	const double heightFactor = height / (minDimension - 1.0) / 2.0;
	for (int y = -heightLow; y <= heightHigh; ++y) {
		const double percent = glm::abs((y + heightLow) / heightFactor);
		const double circleRadius = glm::pow(minRadius, 2.0) - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(pos.x, pos.y + y, pos.z);
		createCirclePlane(ctx, planePos, width, depth, circleRadius, voxel);
	}
}


}
