/**
 * @file
 */

#include "ShapeGenerator.h"
#include "voxel/WorldContext.h"
#include "voxel/Spiral.h"
#include "voxel/polyvox/Raycast.h"

namespace voxel {

void ShapeGenerator::createCirclePlane(GeneratorContext& ctx, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
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
			ctx.setVoxel(pos, voxel);
		}
	}
}

void ShapeGenerator::createCube(GeneratorContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const int widthLow = width / 2;
	const int widthHigh = width - widthLow;
	const int depthLow = depth / 2;
	const int depthHigh = depth - depthLow;
	for (int x = -widthLow; x < widthHigh; ++x) {
		for (int y = -heightLow; y < heightHigh; ++y) {
			for (int z = -depthLow; z < depthHigh; ++z) {
				const glm::ivec3 pos(center.x + x, center.y + y, center.z + z);
				ctx.setVoxel(pos, voxel);
			}
		}
	}
}

void ShapeGenerator::createCubeNoCenter(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
	const int w = glm::abs(width);
	const int h = glm::abs(height);
	const int d = glm::abs(depth);

	const int sw = width / w;
	const int sh = height / h;
	const int sd = depth / d;

	glm::ivec3 p = pos;
	for (int x = 0; x < w; ++x) {
		p.y = pos.y;
		for (int y = 0; y < h; ++y) {
			p.z = pos.z;
			for (int z = 0; z < d; ++z) {
				ctx.setVoxel(p, voxel);
				p.z += sd;
			}
			p.y += sh;
		}
		p.x += sw;
	}
}

glm::ivec3 ShapeGenerator::createL(GeneratorContext& ctx, const glm::ivec3& pos, int width, int depth, int height, int thickness, const Voxel& voxel) {
	glm::ivec3 p = pos;
	if (width != 0) {
		createCubeNoCenter(ctx, p, width, thickness, thickness, voxel);
		p.x += width;
		createCubeNoCenter(ctx, p, thickness, height, thickness, voxel);
		p.x += thickness / 2;
		p.z += thickness / 2;
	} else if (depth != 0) {
		createCubeNoCenter(ctx, p, thickness, thickness, depth, voxel);
		p.z += depth;
		createCubeNoCenter(ctx, p, thickness, height, thickness, voxel);
		p.x += thickness / 2;
		p.z += thickness / 2;
	} else {
		core_assert(false);
	}
	p.y += height;
	return p;
}

void ShapeGenerator::createPlane(GeneratorContext& ctx, const glm::ivec3& center, int width, int depth, const Voxel& voxel) {
	createCube(ctx, center, width, 1, depth, voxel);
}

void ShapeGenerator::createEllipse(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
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

void ShapeGenerator::createCone(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
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

// http://members.chello.at/~easyfilter/bresenham.html
void ShapeGenerator::createLine(GeneratorContext& ctx, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	voxel::raycastWithEndpoints(ctx.getVolume(), start, end, [&] (PagedVolume::Sampler& sampler) {
		sampler.setVoxel(voxel);
		return true;
	});
}

void ShapeGenerator::createDome(GeneratorContext& ctx, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
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
