/**
 * @file
 */

#pragma once

#include "voxel/polyvox/Voxel.h"
#include "core/Common.h"
#include "core/Bezier.h"
#include "voxel/polyvox/Raycast.h"
#include "core/GLM.h"

namespace voxel {

class RawVolume;
class PagedVolume;
class RawVolumeWrapper;
class PagedVolumeWrapper;

namespace shape {

/**
 * @brief Creates a filled circle
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] radius The radius that defines the circle
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
void createCirclePlane(Volume& volume, const glm::ivec3& center, int width, int depth, double radius, const Voxel& voxel) {
	const int xRadius = width / 2;
	const int zRadius = depth / 2;
	const double minRadius = std::min(xRadius, zRadius);
	const double ratioX = xRadius / minRadius;
	const double ratioZ = zRadius / minRadius;

	for (int z = -zRadius; z <= zRadius; ++z) {
		const double distanceZ = glm::pow(z / ratioZ, 2.0);
		for (int x = -xRadius; x <= xRadius; ++x) {
			const double distance = glm::pow(x / ratioX, 2.0) + distanceZ;
			if (distance > radius) {
				continue;
			}
			const glm::ivec3 pos(center.x + x, center.y, center.z + z);
			volume.setVoxel(pos, voxel);
		}
	}
}

/**
 * @brief Creates a cube with the given position being the center of the cube
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 * @sa createCubeNoCenter()
 */
template<class Volume, class Voxel>
void createCube(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
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
				volume.setVoxel(pos, voxel);
			}
		}
	}
}

/**
 * @brief Creates a cube with the ground surface starting exactly on the given y coordinate, x and z are the lower left
 * corner here.
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] pos The position to place the object at (lower left corner)
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 * @sa createCube()
 */
template<class Volume, class Voxel>
void createCubeNoCenter(Volume& volume, const glm::ivec3& pos, int width, int height, int depth, const Voxel& voxel) {
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
				volume.setVoxel(p, voxel);
				p.z += sd;
			}
			p.y += sh;
		}
		p.x += sw;
	}
}

/**
 * @brief Creates a plane
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
void createPlane(Volume& volume, const glm::ivec3& center, int width, int depth, const Voxel& voxel) {
	createCube(volume, center, width, 1, depth, voxel);
}

template<class Volume, class Voxel>
void createPlaneNoCenter(Volume& volume, const glm::ivec3& center, int width, int depth, const Voxel& voxel) {
	createCubeNoCenter(volume, center, width, 1, depth, voxel);
}

/**
 * @brief Creates a L form
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] pos The position to place the object at (lower left corner)
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
glm::ivec3 createL(Volume& volume, const glm::ivec3& pos, int width, int depth, int height, int thickness, const Voxel& voxel) {
	glm::ivec3 p = pos;
	if (width != 0) {
		createCubeNoCenter(volume, p, width, thickness, thickness, voxel);
		p.x += width;
		createCubeNoCenter(volume, p, thickness, height, thickness, voxel);
		p.x += thickness / 2;
		p.z += thickness / 2;
	} else if (depth != 0) {
		createCubeNoCenter(volume, p, thickness, thickness, depth, voxel);
		p.z += depth;
		createCubeNoCenter(volume, p, thickness, height, thickness, voxel);
		p.x += thickness / 2;
		p.z += thickness / 2;
	} else {
		core_assert(false);
	}
	p.y += height;
	return p;
}

/**
 * @brief Creates an ellipsis
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
void createEllipse(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double adjustedMinRadius = minDimension / 2.0;
	const double heightFactor = heightLow / adjustedMinRadius;
	const int start = heightLow - 1;
	const double minRadius = glm::pow(adjustedMinRadius + 0.5, 2.0);
	for (int y = -start; y <= heightHigh; ++y) {
		const double percent = glm::abs(y / heightFactor);
		const double circleRadius = minRadius - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(center.x, center.y + y, center.z);
		createCirclePlane(volume, planePos, width, depth, circleRadius, voxel);
	}
}

/**
 * @brief Creates a cone
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
void createCone(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = minDimension / 2.0;
	const double dHeight = (double)height;
	const int start = heightLow - 1;
	for (int y = -start; y <= heightHigh; ++y) {
		const double percent = 1.0 - ((y + start) / dHeight);
		const double circleRadius = glm::pow(percent * minRadius, 2.0);
		const glm::ivec3 planePos(center.x, center.y + y, center.z);
		createCirclePlane(volume, planePos, width, depth, circleRadius, voxel);
	}
}

/**
 * @brief Creates a dome
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class Voxel>
void createDome(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = std::min(width, depth);
	const double minRadius = glm::pow(minDimension / 2.0, 2.0);
	const double heightFactor = height / (minDimension / 2.0);
	const int start = heightLow - 1;
	for (int y = -start; y <= heightHigh; ++y) {
		const double percent = glm::abs((y + start) / heightFactor);
		const double circleRadius = minRadius - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(center.x, center.y + y, center.z);
		createCirclePlane(volume, planePos, width, depth, circleRadius, voxel);
	}
}

template<class Volume, class Voxel>
void createLine(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const Voxel& voxel) {
	voxel::raycastWithEndpointsVolume(volume, start, end, [&] (auto& sampler) {
		sampler.setVoxel(voxel);
		return true;
	});
}

template<class Volume, class Voxel>
void createBezier(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3 control, const Voxel& voxel, int steps = 8) {
	const core::Bezier<int> b(start, end, control);
	const float s = 1.0f / (float) steps;
	glm::ivec3 lastPos;
	for (int i = 0; i < steps; ++i) {
		const float t = s * i;
		const glm::ivec3& pos = b.getPoint(t);
		volume.setVoxel(pos, voxel);
		if (i > 0) {
			createLine(volume, pos, lastPos, voxel);
		}
		lastPos = pos;
	}
}

}
}
