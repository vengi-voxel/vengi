/**
 * @file
 */

#pragma once

#include "voxel/Voxel.h"
#include "core/Common.h"
#include "math/Bezier.h"
#include "math/Axis.h"
#include "voxel/Raycast.h"
#include "core/GLM.h"

namespace voxelgenerator {
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
template<class Volume>
void createCirclePlane(Volume& volume, const glm::ivec3& center, int width, int depth, double radius, const voxel::Voxel& voxel, math::Axis axis = math::Axis::Y) {
	const int xRadius = width / 2;
	const int zRadius = depth / 2;
	const double minRadius = core_min(xRadius, zRadius);
	const double ratioX = xRadius / minRadius;
	const double ratioZ = zRadius / minRadius;

	for (int z = -zRadius; z <= zRadius; ++z) {
		const double distanceZ = glm::pow(z / ratioZ, 2.0);
		for (int x = -xRadius; x <= xRadius; ++x) {
			const double distance = glm::pow(x / ratioX, 2.0) + distanceZ;
			if (distance > radius) {
				continue;
			}
			glm::ivec3 pos;
			if (axis == math::Axis::X) {
				pos = glm::ivec3(center.x, center.y + x, center.z + z);
			} else if (axis == math::Axis::Y) {
				pos = glm::ivec3(center.x + x, center.y, center.z + z);
			} else {
				pos = glm::ivec3(center.x, center.y, center.z + z);
			}
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
template<class Volume>
void createCube(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const voxel::Voxel& voxel) {
	const int heightLow = height / 2;
	const int widthLow = width / 2;
	const int depthLow = depth / 2;
	std::vector<voxel::Voxel> voxels;
	voxels.assign(height, voxel);
	volume.setVoxels(center.x - widthLow, center.y - heightLow, center.z - depthLow,
			width, depth, &voxels.front(), height);
}

template<class Volume>
void createCube(Volume& volume, const glm::ivec3& center, const glm::ivec3& dim, const voxel::Voxel& voxel) {
	createCube(volume, center, dim.x, dim.y, dim.z, voxel);
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
template<class Volume>
void createCubeNoCenter(Volume& volume, const glm::ivec3& pos, int width, int height, int depth, const voxel::Voxel& voxel) {
	std::vector<voxel::Voxel> voxels;
	voxels.assign(height, voxel);
	volume.setVoxels(pos.x, pos.y, pos.z, width, depth, &voxels.front(), height);
}

template<class Volume>
void createCubeNoCenter(Volume& volume, const glm::ivec3& pos, const glm::ivec3& dim, const voxel::Voxel& voxel) {
	createCubeNoCenter(volume, pos, dim.x, dim.y, dim.z, voxel);
}

/**
 * @brief Creates a plane
 * @param[in,out] volume The volume (RawVolume, PagedVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume>
void createPlane(Volume& volume, const glm::ivec3& center, int width, int depth, const voxel::Voxel& voxel) {
	createCube(volume, center, width, 1, depth, voxel);
}

template<class Volume>
void createPlaneNoCenter(Volume& volume, const glm::ivec3& center, int width, int depth, const voxel::Voxel& voxel) {
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
template<class Volume>
glm::ivec3 createL(Volume& volume, const glm::ivec3& pos, int width, int depth, int height, int thickness, const voxel::Voxel& voxel) {
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
template<class Volume>
void createEllipse(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const voxel::Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = core_min(width, depth);
	const double adjustedMinRadius = minDimension / 2.0;
	const double heightFactor = heightLow / adjustedMinRadius;
	const int start = heightLow - 1;
	const double minRadius = glm::pow(adjustedMinRadius + 0.5, 2.0);
	for (int y = -start; y <= heightHigh; ++y) {
		const double percent = glm::abs(y) / heightFactor;
		const double circleRadius = minRadius - glm::pow(percent, 2.0);
		const glm::ivec3 planePos(center.x, center.y + y, center.z);
		createCirclePlane(volume, planePos, width, depth, circleRadius, voxel);
	}
}

template<class Volume>
void createEllipse(Volume& volume, const glm::ivec3& center, const glm::ivec3& dim, const voxel::Voxel& voxel) {
	createEllipse(volume, center, dim.x, dim.y, dim.z, voxel);
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
template<class Volume>
void createCone(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const voxel::Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = core_min(width, depth);
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

template<class Volume>
void createCone(Volume& volume, const glm::ivec3& center, const glm::ivec3& dim, const voxel::Voxel& voxel) {
	createCone(volume, center, dim.x, dim.y, dim.z, voxel);
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
template<class Volume>
void createDome(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const voxel::Voxel& voxel) {
	const int heightLow = height / 2;
	const int heightHigh = height - heightLow;
	const double minDimension = core_min(width, depth);
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

template<class Volume>
void createDome(Volume& volume, const glm::ivec3& center, const glm::ivec3& dim, const voxel::Voxel& voxel) {
	createDome(volume, center, dim.x, dim.y, dim.z, voxel);
}

template<class Volume>
void createLine(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const voxel::Voxel& voxel, int thickness = 1) {
	if (thickness <= 0) {
		return;
	}

	if (thickness == 1) {
		voxel::raycastWithEndpointsVolume(volume, start, end, [&] (auto& sampler) {
			sampler.setVoxel(voxel);
			return true;
		});
	}

	const float offset = 0.0f;
	const float x1 = start.x + offset;
	const float y1 = start.y + offset;
	const float z1 = start.z + offset;
	const float x2 = end.x + offset;
	const float y2 = end.y + offset;
	const float z2 = end.z + offset;

	int i = (int) glm::floor(x1);
	int j = (int) glm::floor(y1);
	int k = (int) glm::floor(z1);

	const int iend = (int) glm::floor(x2);
	const int jend = (int) glm::floor(y2);
	const int kend = (int) glm::floor(z2);

	const int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	const int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	const int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	const float deltatx = 1.0f / std::abs(x2 - x1);
	const float deltaty = 1.0f / std::abs(y2 - y1);
	const float deltatz = 1.0f / std::abs(z2 - z1);

	const float minx = glm::floor(x1), maxx = minx + 1.0f;
	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) * deltatx;
	const float miny = glm::floor(y1), maxy = miny + 1.0f;
	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) * deltaty;
	const float minz = glm::floor(z1), maxz = minz + 1.0f;
	float tz = ((z1 > z2) ? (z1 - minz) : (maxz - z1)) * deltatz;

	glm::ivec3 pos(i, j, k);

	for (;;) {
		createEllipse(volume, pos, thickness, thickness, thickness, voxel);

		if (tx <= ty && tx <= tz) {
			if (i == iend) {
				break;
			}
			tx += deltatx;
			i += di;

			if (di == 1) {
				++pos.x;
			}
			if (di == -1) {
				--pos.x;
			}
		} else if (ty <= tz) {
			if (j == jend) {
				break;
			}
			ty += deltaty;
			j += dj;

			if (dj == 1) {
				++pos.y;
			}
			if (dj == -1) {
				--pos.y;
			}
		} else {
			if (k == kend) {
				break;
			}
			tz += deltatz;
			k += dk;

			if (dk == 1) {
				++pos.z;
			}
			if (dk == -1) {
				--pos.z;
			}
		}
	}
}

/**
 * @brief Places voxels along the bezier curve points - might produce holes if there are not enough steps
 * @param[in] start The start point for the bezier curve
 * @param[in] end The end point for the bezier curve
 * @param[in] control The control point for the bezier curve
 * @param[in] voxel The @c Voxel to place
 * @param[in] steps The amount of steps to do to get from @c start to @c end
 * @sa createBezierFunc()
 */
template<class Volume>
void createBezier(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control, const voxel::Voxel& voxel, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	const float s = 1.0f / (float) steps;
	for (int i = 0; i < steps; ++i) {
		const float t = s * i;
		const glm::ivec3& pos = b.getPoint(t);
		volume.setVoxel(pos, voxel);
	}
}

/**
 * @brief Execute callback for the points on the bezier curve
 * @param[in] start The start point for the bezier curve
 * @param[in] end The end point for the bezier curve
 * @param[in] control The control point for the bezier curve
 * @param[in] voxel The @c Voxel to place
 * @param[in] steps The amount of steps to do to get from @c start to @c end
 * @param[in] func The functor that accepts the given volume, last position, position and the voxel
 * @sa createBezier()
 */
template<class Volume, class F>
void createBezierFunc(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control, const voxel::Voxel& voxel, F&& func, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	const float s = 1.0f / (float) steps;
	glm::ivec3 lastPos = b.getPoint(0.0f);
	for (int i = 1; i <= steps; ++i) {
		const float t = s * i;
		const glm::ivec3& pos = b.getPoint(t);
		func(volume, lastPos, pos, voxel);
		lastPos = pos;
	}
}

template<class Volume>
void createTorus(Volume& volume, const glm::ivec3& center, int innerRadius, int outerRadius, const voxel::Voxel& voxel) {
	const int radius = outerRadius + 1;
	const int outerRadiusSquare = outerRadius * outerRadius;
	for (int x = -radius; x < radius; ++x) {
		for (int y = -radius; y < radius; ++y) {
			for (int z = -radius; z < radius; ++z) {
				const glm::vec3 pos(center.x + x, center.y + y, center.z + z);
				const glm::vec2 q(glm::length(glm::vec2(pos.x - innerRadius, pos.z - innerRadius)), pos.y);
				if (glm::length2(q) < outerRadiusSquare) {
					volume.setVoxel(pos, voxel);
				}
			}
		}
	}
}

template<class Volume>
void createCylinder(Volume& volume, const glm::vec3& center, const math::Axis axis, int radius, int height, const voxel::Voxel& voxel) {
	for (int i = 0; i < height; ++i) {
		glm::ivec3 centerH;
		if (axis == math::Axis::Y) {
			centerH = glm::ivec3(center.x, center.y + i, center.z);
		} else if (axis == math::Axis::X) {
			centerH = glm::ivec3(center.x + i, center.y, center.z);
		} else {
			centerH = glm::ivec3(center.x, center.y, center.z + i);
		}
		createCirclePlane(volume, centerH, radius * 2, radius * 2, radius, voxel, axis);
	}
}

}
}
