/**
 * @file
 *
 * https://www.scratchapixel.com/lessons/3d-basic-rendering/introduction-acceleration-structure/grid.html
 *
 * The principle behind raycasting is to fire a 'ray' through the volume and determine
 * what (if anything) that ray hits. This simple test can be used for the purpose of
 * picking, visibility checks, lighting calculations, or numerous other applications.
 *
 * A ray is a straight line in space define by a start point and a direction vector.
 * The length of the direction vector represents the length of the ray. When you
 * execute a raycast it will iterate over each voxel which lies on the ray,
 * starting from the defined start point. It will examine each voxel and terminate
 * either when it encounters a solid voxel or when it reaches the end of the ray.
 *
 * **Important Note:** These has been confusion in the past with people not realizing
 * that the length of the direction vector is important. Most graphics API can provide
 * a camera position and view direction for picking purposes, but the view direction is
 * usually normalized (i.e. of length one). If you use this view direction directly you
 * will only iterate over a single voxel and won't find what you are looking for. Instead
 * you must scale the direction vector so that it's length represents the maximum distance
 * over which you want the ray to be cast.
 *
 * Some further notes, the Raycast uses full 26-connectivity, which basically means it
 * will examine every voxel the ray touches, even if it just passes through the corner.
 * Also, it performs a simple binary test against a voxel's threshold, rather than making
 * use of it's density. Therefore it will work best in conjunction with one of the 'cubic'
 * surface extractors. It's behaviour with the Marching Cubes surface extractor has not
 * been tested yet.
 *
 * Note that we also have a pickVoxel() function which provides a slightly higher-level interface.
 *
 *
 * This function is based on Christer Ericson's code and description of the 'Uniform Grid Intersection Test' in
 * 'Real Time Collision Detection'. The following information from the errata on the book website is also relevant:
 *
 * pages 326-327. In the function VisitCellsOverlapped() the two lines calculating tx and ty are incorrect.
 * The less-than sign in each line should be a greater-than sign. That is, the two lines should read:
 *
 * @code
 * float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) / Abs(x2 - x1);
 * float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) / Abs(y2 - y1);
 * @endcode
 *
 * Thanks to Jetro Lauha of Fathammer in Helsinki, Finland for reporting this error.
 *
 * Jetro also points out that the computations of i, j, iend, and jend are incorrectly rounded if the line
 * coordinates are allowed to go negative. While that was not really the intent of the code -- that is, I
 * assumed grids to be numbered from (0, 0) to (m, n) -- I'm at fault for not making my assumption clear.
 * Where it is important to handle negative line coordinates the computation of these variables should be
 * changed to something like this:
 *
 * @code
 * // Determine start grid cell coordinates (i, j)
 * int i = (int)floorf(x1 / CELL_SIDE);
 * int j = (int)floorf(y1 / CELL_SIDE);
 *
 * // Determine end grid cell coordinates (iend, jend)
 * int iend = (int)floorf(x2 / CELL_SIDE);
 * int jend = (int)floorf(y2 / CELL_SIDE);
 * @endcode
 *
 * page 328. The if-statement that reads "if (ty <= tx && ty <= tz)" has a superfluous condition.
 * It should simply read "if (ty <= tz)".
 *
 * This error was reported by Joey Hammer (PixelActive).
 */

#pragma once

#include "core/Common.h"
#include "core/Trace.h"
#include "voxel/Face.h"
#include "voxel/Region.h"
#include <glm/ext/scalar_constants.hpp>
#include <glm/geometric.hpp>
#include <glm/vector_relational.hpp>
#include <limits>

namespace voxelutil {

// Outdated - but left here for reference
// The raycastWithEndpoints function is assuming that it is iterating over the areas defined between
// voxels. We actually want to define the areas as being centered on voxels (as this is
// what the CubicSurfaceExtractor generates). We can add an offset here to adjust for this.
const float RaycastOffset = 0.0f;

struct RaycastHit {
	voxel::FaceNames face = voxel::FaceNames::Max;
	float fract = 0.0f; ///< The fraction [0..1] of the ray that was traveled - 0.0 means that we were starting inside a
						///< solid voxel - 1.0 means, that we hit nothing
	glm::vec3 point{0.0f};
};

RaycastHit raycastFaceDetection(const glm::vec3 &rayOrigin, const glm::vec3 &rayDirection, const glm::vec3 &hitPos,
								float offsetMins = -0.5f, float offsetMaxs = 0.5f);
RaycastHit raycastFaceDetection(const glm::vec3 &rayOrigin, const glm::vec3 &hitPos, float offsetMins = -0.5f,
								float offsetMaxs = 0.5f);

/**
 * The results of a raycast
 */
struct RaycastResult {
	float length = 0.0f; ///< The length the ray traveled before being interrupted or completing
	float fract = 0.0f; ///< The fraction [0..1] of the ray that was traveled - 0.0 means that we were starting inside a
						///< solid voxel - 1.0 means, that we hit nothing
	glm::ivec3 normal{0, 0, 0}; ///< The normal of the intersecting face (axis-aligned, volume/voxel space)

	enum Type : uint8_t {
		Completed,	///< If the ray passed through the volume without being interrupted
		Interrupted ///< If the ray was interrupted while traveling
	} type = Completed;

	bool isSolidStart() const;
	bool isCompleted() const;
	bool isInterrupted() const;
	static RaycastResult completed(float length);
	static RaycastResult interrupted(float length, float fract, const glm::ivec3 &normal);
	// this just moves the point slightly away from the collided plane along its normal (e.g., to prevent z-fighting or
	// embedding).
	glm::vec3 adjustPoint(const glm::vec3 &point, float offset = 0.5f) const;
	glm::vec3 projectOnPlane(const glm::vec3 &v) const;
};

/**
 * Clip a ray segment [start, end] against an AABB.
 *
 * @param[out] tEnter Normalized entry along [start, end] (clamped to >= 0)
 * @param[out] tExit Normalized exit along [start, end] (clamped to <= 1)
 * @param[out] entryNormal Optional face normal of the AABB face that produced tEnter when entering from outside
 * @return false if the segment misses the box
 */
bool clipRaySegmentToAABB(const glm::vec3 &start, const glm::vec3 &end, const glm::vec3 &mins, const glm::vec3 &maxs,
						  float &tEnter, float &tExit, glm::ivec3 *entryNormal = nullptr);

/**
 * Cast a ray through a volume by specifying the start and end positions
 *
 * The ray will move from @a v3dStart to @a v3dEnd, calling @a callback for each
 * voxel it passes through until @a callback returns @a false. In this case it
 * returns a RaycastResults::Type::Interrupted. If it passes from start to end
 * without @a callback returning @a false, it returns RaycastResults::Type::Completed.
 *
 * Rays that start outside the volume are clipped to the volume AABB before DDA
 * traversal so empty space between the camera and the volume is not walked cell
 * by cell. Traversal extends slightly past the exit face so callers that detect
 * leaving the volume (e.g. cursor picking) still see one exterior sample.
 *
 * @param volData The volume to pass the ray though
 * @param start The start position in the volume
 * @param end The end position in the volume
 * @param callback The callback to call for each voxel
 *
 * @return A RaycastResults designating whether the ray hit anything or not
 */
template<typename Callback, class Volume>
RaycastResult raycastWithEndpoints(Volume *volData, const glm::vec3 &start, const glm::vec3 &end, Callback &&callback) {
	core_trace_scoped(raycastWithEndpoints);
	typename Volume::Sampler sampler(volData);

	// Keep the original ray for length/fract reporting.
	const glm::vec3 v3dStart = start + RaycastOffset;
	const glm::vec3 v3dEnd = end + RaycastOffset;
	const float rayLength = glm::length(v3dEnd - v3dStart);

	// Geometric AABB of the voxel region: voxel [L,U] occupies [L, U+1].
	const voxel::Region &region = volData->region();
	const glm::vec3 boxMins = region.getLowerCornerf();
	const glm::vec3 boxMaxs = region.getUpperCornerf() + 1.0f;

	float tEnter = 0.0f;
	float tExit = 1.0f;
	glm::ivec3 entryNormal{0, 0, 0};
	const bool startInside =
		glm::all(glm::greaterThanEqual(v3dStart, boxMins)) && glm::all(glm::lessThanEqual(v3dStart, boxMaxs));
	const bool endInside =
		glm::all(glm::greaterThanEqual(v3dEnd, boxMins)) && glm::all(glm::lessThanEqual(v3dEnd, boxMaxs));

	glm::vec3 walkStart = v3dStart;
	glm::vec3 walkEnd = v3dEnd;
	if (!(startInside && endInside)) {
		if (!clipRaySegmentToAABB(v3dStart, v3dEnd, boxMins, boxMaxs, tEnter, tExit, &entryNormal)) {
			return RaycastResult::completed(rayLength);
		}

		// Walk a half-voxel past the exit so the first exterior cell is visited. That
		// preserves exit detection used by cursor picking (firstInvalidPosition).
		const float pastExit = rayLength > glm::epsilon<float>() ? (0.5f / rayLength) : 0.0f;
		const float tWalkEnd = core_min(1.0f, tExit + pastExit);
		walkStart = v3dStart + (v3dEnd - v3dStart) * tEnter;
		walkEnd = v3dStart + (v3dEnd - v3dStart) * tWalkEnd;
	}

	const float x1 = walkStart.x;
	const float y1 = walkStart.y;
	const float z1 = walkStart.z;
	const float x2 = walkEnd.x;
	const float y2 = walkEnd.y;
	const float z2 = walkEnd.z;

	const glm::ivec3 floorEnd(glm::floor(walkEnd));
	const int iend = floorEnd.x;
	const int jend = floorEnd.y;
	const int kend = floorEnd.z;

	const int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	const int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	const int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	const glm::vec3 dist = glm::abs(walkEnd - walkStart);
	const glm::vec3 floorStart(glm::floor(walkStart));
	const glm::vec3 maxs = floorStart + 1.0f;

	// Amanatides & Woo: a zero direction component must never be selected as the
	// next step. Using 1/epsilon here previously made axis-aligned rays break
	// early once t on a stationary axis became the minimum.
	const float inf = std::numeric_limits<float>::infinity();
	const float deltatx = di == 0 ? 0.0f : 1.0f / dist.x;
	const float deltaty = dj == 0 ? 0.0f : 1.0f / dist.y;
	const float deltatz = dk == 0 ? 0.0f : 1.0f / dist.z;
	float tx = di == 0 ? inf : ((di == -1) ? (x1 - floorStart.x) : (maxs.x - x1)) * deltatx;
	float ty = dj == 0 ? inf : ((dj == -1) ? (y1 - floorStart.y) : (maxs.y - y1)) * deltaty;
	float tz = dk == 0 ? inf : ((dk == -1) ? (z1 - floorStart.z) : (maxs.z - z1)) * deltatz;

	int i = (int)floorStart.x;
	int j = (int)floorStart.y;
	int k = (int)floorStart.z;
	sampler.setPosition(i, j, k);

	// Solid-start detection must use the original ray origin, not the clipped walk start.
	const glm::ivec3 floorOrigStart(glm::floor(v3dStart));

	// If we entered from outside, initialize the face normal from the AABB entry face.
	glm::ivec3 lastNormal = (tEnter > 0.0f) ? entryNormal : glm::ivec3{0, 0, 0};
	for (;;) {
		if (!callback(sampler)) {
			if (i == floorOrigStart.x && j == floorOrigStart.y && k == floorOrigStart.z) {
				return RaycastResult::interrupted(0.0f, 0.0f, lastNormal);
			}

			// The distance to a voxel corner is not the distance along the ray.
			// Intersect the original segment with the entered voxel face instead.
			// This also avoids including RaycastOffset in reported hit positions.
			float fract = 0.0f;
			for (int axis = 0; axis < 3; ++axis) {
				if (lastNormal[axis] == 0) {
					continue;
				}
				const float face = (float)sampler.position()[axis] + (lastNormal[axis] > 0 ? 1.0f : 0.0f);
				fract = glm::clamp((face - start[axis]) / (end[axis] - start[axis]), 0.0f, 1.0f);
				break;
			}
			return RaycastResult::interrupted(rayLength * fract, fract, lastNormal);
		}

		if (tx <= ty && tx <= tz) {
			if (i == iend) {
				break;
			}
			tx += deltatx;
			i += di;

			// we stepped along the x-axis; set normal accordingly
			lastNormal = glm::ivec3(-di, 0, 0);

			if (di == 1) {
				sampler.movePositiveX();
			} else if (di == -1) {
				sampler.moveNegativeX();
			}
		} else if (ty <= tz) {
			if (j == jend) {
				break;
			}
			ty += deltaty;
			j += dj;

			if (dj == 1) {
				sampler.movePositiveY();
			} else if (dj == -1) {
				sampler.moveNegativeY();
			}

			// we stepped along the y-axis; set normal accordingly
			lastNormal = glm::ivec3(0, -dj, 0);
		} else {
			if (k == kend) {
				break;
			}
			tz += deltatz;
			k += dk;

			if (dk == 1) {
				sampler.movePositiveZ();
			} else if (dk == -1) {
				sampler.moveNegativeZ();
			}

			// we stepped along the z-axis; set normal accordingly
			lastNormal = glm::ivec3(0, 0, -dk);
		}
	}

	float length = glm::distance(v3dStart, glm::vec3(i + RaycastOffset, j + RaycastOffset, k + RaycastOffset));
	return RaycastResult::completed(length);
}

/**
 * Cast a ray through a volume by specifying the start and a direction
 *
 * The ray will move from @a v3dStart along @a v3dDirectionAndLength, calling
 * @a callback for each voxel it passes through until @a callback returns
 * @a false. In this case it returns a RaycastResults::Interrupted. If it
 * passes from start to end without @a callback returning @a false, it
 * returns RaycastResults::Completed.
 *
 * @note These has been confusion in the past with people not realising
 * that the length of the direction vector is important. Most graphics API can provide
 * a camera position and view direction for picking purposes, but the view direction is
 * usually normalised (i.e. of length one). If you use this view direction directly you
 * will only iterate over a single voxel and won't find what you are looking for. Instead
 * you must scale the direction vector so that it's length represents the maximum distance
 * over which you want the ray to be cast.
 *
 * @param volData The volume to pass the ray though
 * @param v3dStart The start position in the volume
 * @param v3dDirectionAndLength The direction and length of the ray
 * @param callback The callback to call for each voxel
 *
 * @return A RaycastResults designating whether the ray hit anything or not
 */
template<typename Callback, class Volume>
RaycastResult raycastWithDirection(Volume *volData, const glm::vec3 &v3dStart, const glm::vec3 &v3dDirectionAndLength,
								   Callback &&callback) {
	const glm::vec3 v3dEnd = v3dStart + v3dDirectionAndLength;
	return raycastWithEndpoints<Callback, Volume>(volData, v3dStart, v3dEnd, core::forward<Callback>(callback));
}

} // namespace voxelutil
