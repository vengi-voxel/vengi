/**
 * @file
 */

#include "ShapeGenerator.h"
#include "voxel/WorldContext.h"

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
			ctx.setVoxel(pos, voxel);
		}
	}
}

void ShapeGenerator::createCube(TerrainContext& ctx, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
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

// http://members.chello.at/~easyfilter/bresenham.html
void ShapeGenerator::createLine(TerrainContext& ctx, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	const glm::ivec3 delta = end - start;

	const int xInc = (delta.x < 0) ? -1 : 1;
	const int yInc = (delta.y < 0) ? -1 : 1;
	const int zInc = (delta.z < 0) ? -1 : 1;

	const int w = glm::abs(delta.x);
	const int h = glm::abs(delta.y);
	const int d = glm::abs(delta.z);

	const int dx2 = w << 1;
	const int dy2 = h << 1;
	const int dz2 = d << 1;

	glm::ivec3 point = start;
	if (w >= h && w >= d) {
		int err1 = dy2 - w;
		int err2 = dz2 - w;
		for (int i = 0; i < w; i++) {
			ctx.setVoxel(point, voxel);
			if (err1 > 0) {
				point.y += yInc;
				err1 -= dx2;
			}
			if (err2 > 0) {
				point.z += zInc;
				err2 -= dx2;
			}
			err1 += dy2;
			err2 += dz2;
			point.x += xInc;
		}
	} else if (h >= w && h >= d) {
		int err1 = dx2 - h;
		int err2 = dz2 - h;
		for (int i = 0; i < h; i++) {
			ctx.setVoxel(point, voxel);
			if (err1 > 0) {
				point.x += xInc;
				err1 -= dy2;
			}
			if (err2 > 0) {
				point.z += zInc;
				err2 -= dy2;
			}
			err1 += dx2;
			err2 += dz2;
			point.y += yInc;
		}
	} else {
		int err1 = dy2 - d;
		int err2 = dx2 - d;
		for (int i = 0; i < d; i++) {
			ctx.setVoxel(point, voxel);
			if (err1 > 0) {
				point.y += yInc;
				err1 -= dz2;
			}
			if (err2 > 0) {
				point.x += xInc;
				err2 -= dz2;
			}
			err1 += dy2;
			err2 += dx2;
			point.z += zInc;
		}
	}
	ctx.setVoxel(point, voxel);
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
