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
#include "voxel/Region.h"
#include "voxelutil/Raycast.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"

namespace voxelgenerator {
namespace shape {

constexpr int MAX_HEIGHT = 255;

template<class Pattern>
inline bool useStipplePattern(const Pattern& stipplePattern, int stippleState) {
	const int bits = stipplePattern.bits();
	if (bits <= 0) {
		return true;
	}
	return stipplePattern[stippleState % bits];
}

inline int chebyshevDistance(const math::Vec3i& a, const math::Vec3i& b) {
	const math::Vec3i delta = math::abs(a - b);
	return math::max(delta.x, math::max(delta.y, delta.z));
}

inline bool isCollinear(const math::BezierSegment& segment) {
	const math::Vec3i startToEnd = segment.end - segment.start;
	const math::Vec3i startToControl = segment.control - segment.start;
	const math::Vec3i cross = math::cross(math::Vec3f(startToEnd), math::Vec3f(startToControl));
	return math::abs(cross.x) < 0.0001f && math::abs(cross.y) < 0.0001f && math::abs(cross.z) < 0.0001f;
}

inline voxel::Region bezierRegion(const math::BezierSegment& segment, int thickness = 1) {
	math::Vec3i mins = math::min(segment.start, segment.end);
	math::Vec3i maxs = math::max(segment.start, segment.end);
	mins = math::min(mins, segment.control);
	maxs = math::max(maxs, segment.control);
	if (thickness > 1) {
		mins -= thickness;
		maxs += thickness;
	}
	return voxel::Region(mins, maxs);
}

/**
 * @brief Creates a filled circle or ellipse plane
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] width The width (x-axis) of the object
 * @param[in] depth The height (z-axis) of the object
 * @param[in] radiusX The radius along the width axis
 * @param[in] radiusZ The radius along the depth axis
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createEllipsePlane(Volume& volume, const math::Vec3i& center, math::Axis axis, int width, int depth, double radiusX, double radiusZ, const VoxelType& voxel) {
	const int halfW = width / 2;
	const int halfD = depth / 2;

	for (int iz = 0; iz < depth; ++iz) {
		const double z = iz - depth / 2.0 + 0.5;
		for (int ix = 0; ix < width; ++ix) {
			const double x = ix - width / 2.0 + 0.5;
			const double nx = (radiusX > 0.0) ? (x / radiusX) : 0.0;
			const double nz = (radiusZ > 0.0) ? (z / radiusZ) : 0.0;
			if (nx * nx + nz * nz > 1.0) {
				continue;
			}
			const int ox = ix - halfW;
			const int oz = iz - halfD;
			if (axis == math::Axis::X) {
				volume.setVoxel(center.x, center.y + ox, center.z + oz, voxel);
			} else if (axis == math::Axis::Y) {
				volume.setVoxel(center.x + ox, center.y, center.z + oz, voxel);
			} else {
				volume.setVoxel(center.x + ox, center.y + oz, center.z, voxel);
			}
		}
	}
}

/**
 * @brief Creates a filled circle plane (convenience overload with single radius)
 */
template<class Volume, class VoxelType>
void createEllipsePlane(Volume& volume, const math::Vec3i& center, math::Axis axis, int width, int depth, double radius, const VoxelType& voxel) {
	createEllipsePlane(volume, center, axis, width, depth, radius, radius, voxel);
}

/**
 * @brief Creates a circle outline (ring) - only the perimeter voxels
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] axis The axis perpendicular to the circle plane
 * @param[in] width The width of the bounding box
 * @param[in] depth The depth of the bounding box
 * @param[in] radius The outer radius that defines the circle
 * @param[in] thickness The wall thickness in voxels
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createCircleOutline(Volume& volume, const math::Vec3i& center, math::Axis axis, int width, int depth, double radius, int thickness, const VoxelType& voxel) {
	const int halfW = width / 2;
	const int halfD = depth / 2;
	const double innerRadius = core_max(0.0, radius - thickness);

	for (int iz = 0; iz < depth; ++iz) {
		const double z = iz - depth / 2.0 + 0.5;
		for (int ix = 0; ix < width; ++ix) {
			const double x = ix - width / 2.0 + 0.5;
			const double distance = math::sqrt(x * x + z * z);
			if (distance > radius || distance < innerRadius) {
				continue;
			}
			const int ox = ix - halfW;
			const int oz = iz - halfD;
			if (axis == math::Axis::X) {
				volume.setVoxel(center.x, center.y + ox, center.z + oz, voxel);
			} else if (axis == math::Axis::Y) {
				volume.setVoxel(center.x + ox, center.y, center.z + oz, voxel);
			} else {
				volume.setVoxel(center.x + ox, center.y + oz, center.z, voxel);
			}
		}
	}
}

/**
 * @brief Creates an ellipse outline (ring) - only the perimeter voxels
 * @param[in,out] volume The volume (RawVolume) to place the voxels into
 * @param[in] center The position to place the object at
 * @param[in] axis The axis perpendicular to the ellipse plane
 * @param[in] width The width of the bounding box
 * @param[in] depth The depth of the bounding box
 * @param[in] radiusX The outer radius along the width axis
 * @param[in] radiusZ The outer radius along the depth axis
 * @param[in] thickness The wall thickness in voxels
 * @param[in] voxel The Voxel to build the object with
 */
template<class Volume, class VoxelType>
void createEllipseOutline(Volume& volume, const math::Vec3i& center, math::Axis axis, int width, int depth, double radiusX, double radiusZ, int thickness, const VoxelType& voxel) {
	const int halfW = width / 2;
	const int halfD = depth / 2;
	const double innerRadiusX = core_max(0.0, radiusX - thickness);
	const double innerRadiusZ = core_max(0.0, radiusZ - thickness);

	for (int iz = 0; iz < depth; ++iz) {
		const double z = iz - depth / 2.0 + 0.5;
		for (int ix = 0; ix < width; ++ix) {
			const double x = ix - width / 2.0 + 0.5;
			const double nx = (radiusX > 0.0) ? (x / radiusX) : 0.0;
			const double nz = (radiusZ > 0.0) ? (z / radiusZ) : 0.0;
			const double outerDist = nx * nx + nz * nz;
			if (outerDist > 1.0) {
				continue;
			}
			const double inx = (innerRadiusX > 0.0) ? (x / innerRadiusX) : 0.0;
			const double inz = (innerRadiusZ > 0.0) ? (z / innerRadiusZ) : 0.0;
			const double innerDist = inx * inx + inz * inz;
			if (innerDist < 1.0) {
				continue;
			}
			const int ox = ix - halfW;
			const int oz = iz - halfD;
			if (axis == math::Axis::X) {
				volume.setVoxel(center.x, center.y + ox, center.z + oz, voxel);
			} else if (axis == math::Axis::Y) {
				volume.setVoxel(center.x + ox, center.y, center.z + oz, voxel);
			} else {
				volume.setVoxel(center.x + ox, center.y + oz, center.z, voxel);
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
void createCube(Volume& volume, const math::Vec3i& center, int width, int height, int depth, const VoxelType& voxel) {
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
void createCubeNoCenter(Volume& volume, const math::Vec3i& pos, int width, int height, int depth, const VoxelType& voxel) {
	core::Vector<voxel::Voxel, MAX_HEIGHT> voxels;
	voxels.assign(voxel, height);
	voxel::setVoxels(volume, pos.x, pos.y, pos.z, width, depth, &voxels.front(), height);
}

template<class Volume, class VoxelType>
void createCubeNoCenter(Volume& volume, const math::Vec3i& pos, const math::Vec3i& dim, const VoxelType& voxel) {
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
void createEllipse(Volume& volume, const math::Vec3i& centerBottom, const math::Axis axis, int width, int height, int depth, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}
	const double xRadius = core_max(1.0, width / 2.0);
	const double zRadius = core_max(1.0, depth / 2.0);
	const double halfHeight = core_max(1.0, height / 2.0);
	const int axisIdx = math::getIndexForAxis(axis);
	math::Vec3i circleCenter = centerBottom;
	math::Vec3i offset{0};
	offset[axisIdx] = 1;
	for (int i = 0; i < height; ++i) {
		const double normalizedY = (i - halfHeight + 0.5) / halfHeight;
		const double scale = math::sqrt(core_max(0.0, 1.0 - normalizedY * normalizedY));
		createEllipsePlane(volume, circleCenter, axis, width, depth, xRadius * scale, zRadius * scale, voxel);
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
void createCone(Volume& volume, const math::Vec3i& centerBottom, const math::Axis axis, bool negative, int width, int height, int depth, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}
	const double xRadius = width / 2.0;
	const double zRadius = depth / 2.0;
	const double dHeight = (double)height;
	const int axisIdx = math::getIndexForAxis(axis);
	math::Vec3i circleCenter = centerBottom;
	math::Vec3i offset{0};
	offset[axisIdx] = 1;
	if (negative) {
		circleCenter += offset * (height - 1);
		offset *= -1;
	}
	for (int i = 0; i < height; ++i) {
		const double percent = 1.0 - (i / dHeight);
		createEllipsePlane(volume, circleCenter, axis, width, depth, percent * xRadius, percent * zRadius, voxel);
		circleCenter += offset;
	}
}

template<class Volume, class VoxelType>
void createCylinder(Volume& volume, const math::Vec3f& centerBottom, const math::Axis axis, int radius, int height, const VoxelType& voxel) {
	if (axis == math::Axis::None) {
		return;
	}

	const int axisIdx = math::getIndexForAxis(axis);
	math::Vec3i circleCenter = centerBottom;
	math::Vec3i offset{0};
	offset[axisIdx] = 1;
	for (int i = 0; i < height; ++i) {
		createEllipsePlane(volume, circleCenter, axis, radius * 2, radius * 2, radius, voxel);
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
void createDome(Volume& volume, const math::Vec3i& centerBottom, math::Axis axis, bool negative, int width, int height, int depth, const VoxelType& voxel) {
	const double xRadius = core_max(1.0, width / 2.0);
	const double zRadius = core_max(1.0, depth / 2.0);

	const int axisIdx = math::getIndexForAxis(axis);
	math::Vec3i circleCenter = centerBottom;
	math::Vec3i offset{0};
	offset[axisIdx] = 1;
	if (negative) {
		circleCenter += offset * (height - 1);
		offset *= -1;
	}
	for (int i = 0; i < height; ++i) {
		const double normalizedY = (double)i / (double)height;
		const double scale = math::sqrt(core_max(0.0, 1.0 - normalizedY * normalizedY));
		createEllipsePlane(volume, circleCenter, axis, width, depth, xRadius * scale, zRadius * scale, voxel);
		circleCenter += offset;
	}
}

template<class Volume, class VoxelType>
void createLine(Volume& volume, const math::Vec3i& start, const math::Vec3i& end, const VoxelType& voxel, int thickness = 1) {
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

	int i = (int) math::floor(x1);
	int j = (int) math::floor(y1);
	int k = (int) math::floor(z1);

	const int iend = (int) math::floor(x2);
	const int jend = (int) math::floor(y2);
	const int kend = (int) math::floor(z2);

	const int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	const int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	const int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	const float deltatx = 1.0f / core_max(1.0f, math::abs(x2 - x1));
	const float deltaty = 1.0f / core_max(1.0f, math::abs(y2 - y1));
	const float deltatz = 1.0f / core_max(1.0f, math::abs(z2 - z1));

	const float minx = math::floor(x1), maxx = minx + 1.0f;
	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) * deltatx;
	const float miny = math::floor(y1), maxy = miny + 1.0f;
	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) * deltaty;
	const float minz = math::floor(z1), maxz = minz + 1.0f;
	float tz = ((z1 > z2) ? (z1 - minz) : (maxz - z1)) * deltatz;

	math::Vec3i pos(i, j, k);

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

template<class Volume, class VoxelType, class Pattern>
bool drawStippledLine(Volume& volume, const math::Vec3i& start, const math::Vec3i& end, const VoxelType& voxel,
						 const Pattern& stipplePattern, int& stippleState, bool skipFirst = false, int thickness = 1) {
	bool firstPoint = true;
	bool consumedPoint = false;
	bool sawVisitedPoint = false;
	math::Vec3i lastVisited(0);
	voxelutil::raycastWithEndpoints(&volume, start, end, [&] (auto& sampler) {
		if (skipFirst && firstPoint) {
			firstPoint = false;
			return true;
		}
		firstPoint = false;
		sawVisitedPoint = true;
		lastVisited = sampler.position();
		consumedPoint = true;
		if (useStipplePattern(stipplePattern, stippleState)) {
			if (thickness > 1) {
				createEllipse(volume, sampler.position(), math::Axis::Y, thickness, thickness, thickness, voxel);
			} else {
				sampler.setVoxel(voxel);
			}
		}
		++stippleState;
		return true;
	});
	if (!(skipFirst && start == end) && volume.region().containsPoint(end) && (!sawVisitedPoint || lastVisited != end)) {
		consumedPoint = true;
		if (useStipplePattern(stipplePattern, stippleState)) {
			if (thickness > 1) {
				createEllipse(volume, end, math::Axis::Y, thickness, thickness, thickness, voxel);
			} else {
				volume.setVoxel(end.x, end.y, end.z, voxel);
			}
		}
		++stippleState;
	}
	return consumedPoint;
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
void createBezier(Volume& volume, const math::Vec3i& start, const math::Vec3i& end, const math::Vec3i& control, const VoxelType& voxel, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	b.visitPoints(steps, [&] (const math::Vec3i& pos) {
		volume.setVoxel(pos, voxel);
	});
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
void createBezierFunc(Volume& volume, const math::Vec3i& start, const math::Vec3i& end, const math::Vec3i& control, const VoxelType& voxel, F&& func, int steps = 8) {
	const math::Bezier<int> b(start, end, control);
	b.visitSegments(steps, [&] (const math::Vec3i& lastPos, const math::Vec3i& pos) {
		func(volume, lastPos, pos, voxel);
	});
}

template<class Volume, class VoxelType, class Pattern>
void drawBezierSegment(Volume& volume, const math::BezierSegment& segment, const VoxelType& voxel,
						   const Pattern& stipplePattern, int thickness = 1) {
	voxel::Region clipped = bezierRegion(segment, thickness);
	if (!clipped.cropTo(volume.region())) {
		return;
	}
	if (isCollinear(segment)) {
		int stippleState = 0;
		(void)drawStippledLine(volume, segment.start, segment.end, voxel, stipplePattern, stippleState, false, thickness);
		return;
	}
	using EvalVec = typename math::Bezier<int>::EvalVec;
	auto roundPoint = [&] (const EvalVec& point) {
		return math::Vec3i(math::round(point));
	};
	auto curveRegion = [&] (const EvalVec& start, const EvalVec& control, const EvalVec& end) {
		const math::Vec3i startPos = roundPoint(start);
		const math::Vec3i controlPos = roundPoint(control);
		const math::Vec3i endPos = roundPoint(end);
		math::Vec3i mins = math::min(startPos, endPos);
		math::Vec3i maxs = math::max(startPos, endPos);
		mins = math::min(mins, controlPos);
		maxs = math::max(maxs, controlPos);
		return voxel::Region(mins, maxs);
	};

	int stippleState = 0;
	bool hasLastPoint = false;
	math::Vec3i lastPoint(0);
	constexpr int MaxSubdivisionDepth = 24;
	const EvalVec start(segment.start);
	const EvalVec control(segment.control);
	const EvalVec end(segment.end);
	auto subdivide = [&] (auto&& self, const EvalVec& subStart, const EvalVec& subControl, const EvalVec& subEnd,
						 int depth) -> void {
		voxel::Region region = curveRegion(subStart, subControl, subEnd);
		if (!region.cropTo(volume.region())) {
			return;
		}
		const math::Vec3i startPos = roundPoint(subStart);
		const math::Vec3i endPos = roundPoint(subEnd);
		if (depth >= MaxSubdivisionDepth || chebyshevDistance(startPos, endPos) <= 1) {
			const bool skipFirst = hasLastPoint && lastPoint == startPos;
			if (drawStippledLine(volume, startPos, endPos, voxel, stipplePattern, stippleState, skipFirst, thickness)) {
				hasLastPoint = true;
				lastPoint = endPos;
			}
			return;
		}

		const EvalVec leftControl = (subStart + subControl) * 0.5f;
		const EvalVec rightControl = (subControl + subEnd) * 0.5f;
		const EvalVec mid = (leftControl + rightControl) * 0.5f;
		if (roundPoint(mid) == startPos && roundPoint(mid) == endPos) {
			const bool skipFirst = hasLastPoint && lastPoint == startPos;
			if (drawStippledLine(volume, startPos, endPos, voxel, stipplePattern, stippleState, skipFirst, thickness)) {
				hasLastPoint = true;
				lastPoint = endPos;
			}
			return;
		}

		self(self, subStart, leftControl, mid, depth + 1);
		self(self, mid, rightControl, subEnd, depth + 1);
	};
	subdivide(subdivide, start, control, end, 0);
}

template<class Volume, class VoxelType>
void createTorus(Volume& volume, const math::Vec3i& center, math::Axis axis, const math::Vec3i& dimensions, int minorRadiusOverride, const VoxelType& voxel) {
	const int halfW = dimensions.x / 2;
	const int halfH = dimensions.y / 2;
	const int halfD = dimensions.z / 2;

	// determine which world dimensions form the ring plane vs the axis (tube height)
	double minorRadiusH, halfA, halfB;
	if (axis == math::Axis::X) {
		minorRadiusH = core_max(1.0, (double)halfW);
		halfA = halfH;
		halfB = halfD;
	} else if (axis == math::Axis::Y) {
		minorRadiusH = core_max(1.0, (double)halfH);
		halfA = halfW;
		halfB = halfD;
	} else {
		minorRadiusH = core_max(1.0, (double)halfD);
		halfA = halfW;
		halfB = halfH;
	}

	// in-plane tube radius: at most half the smaller plane dimension to preserve the hole
	const double maxMinorP = core_min(halfA, halfB) / 2.0;
	const double minorRadiusP = (minorRadiusOverride > 0) ? core_min((double)minorRadiusOverride, maxMinorP) : core_min(minorRadiusH, maxMinorP);
	const double majorRadiusA = core_max(1.0, halfA - minorRadiusP);
	const double majorRadiusB = core_max(1.0, halfB - minorRadiusP);

	for (int iz = -halfD; iz < dimensions.z - halfD; ++iz) {
		const double z = iz + 0.5;
		for (int iy = -halfH; iy < dimensions.y - halfH; ++iy) {
			const double y = iy + 0.5;
			for (int ix = -halfW; ix < dimensions.x - halfW; ++ix) {
				const double x = ix + 0.5;

				double pa, pb, h;
				if (axis == math::Axis::X) {
					pa = y;
					pb = z;
					h = x;
				} else if (axis == math::Axis::Y) {
					pa = x;
					pb = z;
					h = y;
				} else {
					pa = x;
					pb = y;
					h = z;
				}

				// find closest point on the major ellipse
				const double na = (majorRadiusA > 0.0) ? (pa / majorRadiusA) : 0.0;
				const double nb = (majorRadiusB > 0.0) ? (pb / majorRadiusB) : 0.0;
				const double planeDist = math::sqrt(na * na + nb * nb);
				if (planeDist < 0.0001) {
					const double nh = (minorRadiusH > 0.0) ? (h / minorRadiusH) : 0.0;
					if (nh * nh <= 1.0) {
						volume.setVoxel(center.x + ix, center.y + iy, center.z + iz, voxel);
					}
					continue;
				}

				const double angle = math::atan(pb * majorRadiusA, pa * majorRadiusB);
				const double ellipseA = majorRadiusA * math::cos(angle);
				const double ellipseB = majorRadiusB * math::sin(angle);

				const double da = pa - ellipseA;
				const double db = pb - ellipseB;
				// elliptical tube cross-section check
				const double inPlaneDist = math::sqrt(da * da + db * db);
				const double np = (minorRadiusP > 0.0) ? (inPlaneDist / minorRadiusP) : 0.0;
				const double nh = (minorRadiusH > 0.0) ? (h / minorRadiusH) : 0.0;

				if (np * np + nh * nh <= 1.0) {
					volume.setVoxel(center.x + ix, center.y + iy, center.z + iz, voxel);
				}
			}
		}
	}
}

}
}
