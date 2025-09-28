/**
 * @file
 */

#pragma once

#include "core/collection/Vector.h"
#include "voxel/VolumeSamplerUtil.h"
#include "voxel/Voxel.h"
#include "core/Common.h"
#include "math/Bezier.h"
#include "math/Axis.h"
#include "voxelutil/Raycast.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/norm.hpp>

namespace voxelgenerator {
namespace shape {

constexpr int MAX_HEIGHT = 255;

/**
 * @brief Creates a filled circle
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] radius The radius that defines the circle
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createCirclePlane(Volume& volume, const glm::ivec3& center, math::Axis axis, int width, int depth, double radius, const VoxelType& voxel) {
	const double xRadius = width / 2.0;
	const double zRadius = depth / 2.0;

	for (double z = -zRadius; z <= zRadius; ++z) {
		const double distanceZ = glm::pow(z, 2.0);
		for (double x = -xRadius; x <= xRadius; ++x) {
			const double distance = glm::sqrt(glm::pow(x, 2.0) + distanceZ);
			if (distance > radius) {
				continue;
			}
			if (axis == math::Axis::X) {
				volume.setVoxel(center.x, center.y + x, center.z + z, voxel);
			} else if (axis == math::Axis::Y) {
				volume.setVoxel(center.x + x, center.y, center.z + z, voxel);
			} else {
				volume.setVoxel(center.x + x, center.y + z, center.z, voxel);
			}
		}
	}
}

/**
 * @brief Creates a cube with the given position being the center of the cube
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 * @sa createCubeNoCenter()
 */
template<class Volume, class VoxelType>
void createCube(Volume& volume, const glm::ivec3& center, int width, int height, int depth, const VoxelType& voxel) {
	const int heightLow = height / 2;
	const int widthLow = width / 2;
	const int depthLow = depth / 2;
	core::Vector<voxel::Voxel, MAX_HEIGHT> voxels;
	voxels.assign(voxel, height);
	voxel::setVoxels(volume, center.x - widthLow, center.y - heightLow, center.z - depthLow,
			width, depth, &voxels.front(), height);
}

/**
 * @brief Creates a cube with the ground surface starting exactly on the given y coordinate, x and z are the lower left
 * corner here.
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] pos The position to place the object at (lower left corner)
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 * @sa createCube()
 */
template<class Volume, class VoxelType>
void createCubeNoCenter(Volume& volume, const glm::ivec3& pos, int width, int height, int depth, const VoxelType& voxel) {
	core::Vector<voxel::Voxel, MAX_HEIGHT> voxels;
	voxels.assign(voxel, height);
	voxel::setVoxels(volume, pos.x, pos.y, pos.z, width, depth, &voxels.front(), height);
}

template<class Volume, class VoxelType>
void createCubeNoCenter(Volume& volume, const glm::ivec3& pos, const glm::ivec3& dim, const VoxelType& voxel) {
	createCubeNoCenter(volume, pos, dim.x, dim.y, dim.z, voxel);
}

/**
 * @brief Creates an ellipsis
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] centerBottom The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createEllipse(Volume& volume, const glm::ivec3& centerBottom, const math::Axis axis, int width, int height, int depth, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}
	const int heightLow = core_max(1, height / 2);
	const double minDimension = core_min(width, depth);
	const double adjustedMinRadius = core_max(1.0, minDimension / 2.0);
	const double heightFactor = heightLow / adjustedMinRadius;
	const double minRadius = glm::pow(adjustedMinRadius + 0.5, 2.0);
	const int axisIdx = math::getIndexForAxis(axis);
	glm::ivec3 circleCenter = centerBottom;
	glm::ivec3 offset{0};
	offset[axisIdx] = 1;
	for (int i = 0; i < height; ++i) {
		const double percent = glm::pow(glm::abs((i - heightLow + 1) / heightFactor), 2.0);
		const double yRadiusSquared = minRadius - percent;
		if (yRadiusSquared < 0.0) {
			break;
		}
		const double circleRadius = glm::sqrt(yRadiusSquared);
		createCirclePlane(volume, circleCenter, axis, width, depth, circleRadius, voxel);
		circleCenter += offset;
	}
}

/**
 * @brief Creates a cone
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] centerBottom The position to place the object at
 * @param[in] axis Defines the direction of the cone
 * @param[in] negative If true the cone will be placed in the negative direction of the axis
 * @param[in] width The width of the object
 * @param[in] height The height of the object
 * @param[in] depth The height of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createCone(Volume& volume, const glm::ivec3& centerBottom, const math::Axis axis, bool negative, int width, int height, int depth, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}
	const double minDimension = core_min(width, depth);
	const double minRadius = minDimension / 2.0;
	const double dHeight = (double)height;
	const int axisIdx = math::getIndexForAxis(axis);
	glm::ivec3 circleCenter = centerBottom;
	glm::ivec3 offset{0};
	offset[axisIdx] = 1;
	if (negative) {
		circleCenter += offset * (height - 1);
		offset *= -1;
	}
	for (int i = 0; i < height; ++i) {
		const double percent = 1.0 - (i / dHeight);
		const double circleRadius = percent * minRadius;
		createCirclePlane(volume, circleCenter, axis, width, depth, circleRadius, voxel);
		circleCenter += offset;
	}
}

template<class Volume, class VoxelType>
void createCylinder(Volume& volume, const glm::vec3& centerBottom, const math::Axis axis, int radius, int height, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}

	const int axisIdx = math::getIndexForAxis(axis);
	glm::ivec3 circleCenter = centerBottom;
	glm::ivec3 offset{0};
	offset[axisIdx] = 1;
	for (int i = 0; i < height; ++i) {
		createCirclePlane(volume, circleCenter, axis, radius * 2, radius * 2, radius, voxel);
		circleCenter += offset;
	}
}

/**
 * @brief Creates a dome
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] centerBottom The position to place the object at
 * @param[in] axis Defines the direction of the dome
 * @param[in] negative If true the dome will be placed in the negative direction of the axis
 * @param[in] width The width (x-axis) of the object
 * @param[in] height The height (y-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createDome(Volume& volume, const glm::ivec3& centerBottom, math::Axis axis, bool negative, int width, int height, int depth, const VoxelType& voxel) {
	const double minDimension = core_min(width, depth);
	const double minRadius = glm::pow(minDimension / 2.0, 2.0);
	const double heightFactor = height / (minDimension / 2.0);

	const int axisIdx = math::getIndexForAxis(axis);
	glm::ivec3 circleCenter = centerBottom;
	glm::ivec3 offset{0};
	offset[axisIdx] = 1;
	if (negative) {
		circleCenter += offset * (height - 1);
		offset *= -1;
	}
	for (int i = 0; i < height; ++i) {
		const double percent = glm::abs((double)i / heightFactor);
		const double yRadius = glm::pow(percent, 2.0);
		const double circleRadiusSquared = minRadius - yRadius;
		if (circleRadiusSquared < 0.0) {
			break;
		}
		const double circleRadius = glm::sqrt(circleRadiusSquared);
		createCirclePlane(volume, circleCenter, axis, width, depth, circleRadius, voxel);
		circleCenter += offset;
	}
}

template<class Volume, class VoxelType>
void createLine(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const VoxelType& voxel, int thickness = 1) {
	if (thickness <= 0) {
		return;
	}

	if (thickness == 1) {
		voxelutil::raycastWithEndpoints(&volume, start, end, [&] (auto& sampler) {
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

	const float deltatx = 1.0f / core_max(1.0f, glm::abs(x2 - x1));
	const float deltaty = 1.0f / core_max(1.0f, glm::abs(y2 - y1));
	const float deltatz = 1.0f / core_max(1.0f, glm::abs(z2 - z1));

	const float minx = glm::floor(x1), maxx = minx + 1.0f;
	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) * deltatx;
	const float miny = glm::floor(y1), maxy = miny + 1.0f;
	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) * deltaty;
	const float minz = glm::floor(z1), maxz = minz + 1.0f;
	float tz = ((z1 > z2) ? (z1 - minz) : (maxz - z1)) * deltatz;

	glm::ivec3 pos(i, j, k);

	for (;;) {
		createEllipse(volume, pos, math::Axis::Y, thickness, thickness, thickness, voxel);

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
template<class Volume, class VoxelType>
void createBezier(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control, const VoxelType& voxel, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	const float s = 1.0f / (float) steps;
	for (int i = 0; i < steps; ++i) {
		const float t = s * (float)i;
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
template<class Volume, class F, class VoxelType>
void createBezierFunc(Volume& volume, const glm::ivec3& start, const glm::ivec3& end, const glm::ivec3& control, const VoxelType& voxel, F&& func, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	const float s = 1.0f / (float) steps;
	glm::ivec3 lastPos = b.getPoint(0.0f);
	for (int i = 1; i <= steps; ++i) {
		const float t = s * (float)i;
		const glm::ivec3& pos = b.getPoint(t);
		func(volume, lastPos, pos, voxel);
		lastPos = pos;
	}
}

template<class Volume, class VoxelType>
void createTorus(Volume& volume, const glm::ivec3& center, double minorRadius, double majorRadius, const VoxelType& voxel) {
	glm::dvec3 mins(-majorRadius - minorRadius, -majorRadius - minorRadius, -majorRadius - minorRadius);
	glm::dvec3 maxs(majorRadius + minorRadius, majorRadius + minorRadius, majorRadius + minorRadius);

	// shift to the voxel center
	mins += 0.5;
	maxs += 0.5;

	const double aPow = glm::pow(majorRadius, 2);
	const double bPow = glm::pow(minorRadius, 2);
	for (double x = mins.x; x <= maxs.x; ++x) {
		const double xPow = glm::pow(x, 2);
		for (double y = mins.y; y <= maxs.y; ++y) {
			const double yPow = glm::pow(y, 2);
			for (double z = mins.z; z <= maxs.z; ++z) {
				// This term is smaller than zero if the point is inside the torus
				const double zPow = glm::pow(z, 2);
				// https://stackoverflow.com/questions/13460711/given-origin-and-radii-how-to-find-out-if-px-y-z-is-inside-torus
				// (x^2+y^2+z^2+a^2-b^2)^2-4a^2(x^2+y^2)
				if (glm::pow(xPow + yPow + zPow + aPow - bPow, 2) - 4.0 * aPow * (xPow + yPow) > 0.0) {
					continue;
				}

				volume.setVoxel(center.x + (int)x, center.y + (int)y, center.z + (int)z, voxel);
			}
		}
	}
}

}
}
