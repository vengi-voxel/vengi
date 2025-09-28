/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Log.h"
#include "core/Trace.h"
#include "voxel/RawVolume.h"
#include <glm/common.hpp>
#include <glm/ext/scalar_constants.hpp>

namespace voxelutil {
/**
 * The results of a raycast
 */
struct RaycastResult {
	float length = 0.0f;		///< The length the ray traveled before being interrupted or completing
	float fract = 0.0f;			///< The fraction [0..1] of the ray that was traveled - 0.0 means that we were starting inside a solid voxel - 1.0 means, that we hit nothing
	glm::ivec3 normal{0, 0, 0}; ///< The normal of the intersecting face (axis-aligned, volume/voxel space)
	enum Type : uint8_t {
		Completed,	///< If the ray passed through the volume without being interrupted
		Interrupted ///< If the ray was interrupted while traveling
	} type = Completed;

	bool isSolidStart() const {
		return fract <= 0.0f;
	}
	bool isCompleted() const {
		return type == Completed;
	}
	bool isInterrupted() const {
		return type == Interrupted;
	}
	static RaycastResult completed(float length) {
		RaycastResult r;
		r.type = Completed;
		r.length = length;
		r.fract = 1.0f;
		return r;
	}
	static RaycastResult interrupted(float length, float fract, const glm::ivec3 &normal) {
		RaycastResult r;
		r.type = Interrupted;
		r.length = length;
		r.fract = fract;
		r.normal = normal;
		return r;
	}
	// this just moves the point slightly away from the collided plane along its normal (e.g., to prevent z-fighting or
	// embedding).
	glm::vec3 adjustPoint(const glm::vec3 &point, float offset = 0.5f) const {
		return point - glm::vec3(normal) * (offset + 0.001f);
	}
	glm::vec3 projectOnPlane(const glm::vec3 &v) const {
		glm::vec3 n = glm::normalize(glm::vec3(normal));
		return v - glm::dot(v, n) * n;
	}
};

/// OUT OF DATE SINCE UNCLASSING
////////////////////////////////////////////////////////////////////////////////
/// \file Raycast.h
///
/// The principle behind raycasting is to fire a 'ray' through the volume and determine
/// what (if anything) that ray hits. This simple test can be used for the purpose of
/// picking, visibility checks, lighting calculations, or numerous other applications.
///
/// A ray is a straight line in space define by a start point and a direction vector.
/// The length of the direction vector represents the length of the ray. When you
/// execute a raycast it will iterate over each voxel which lies on the ray,
/// starting from the defined start point. It will examine each voxel and terminate
/// either when it encounters a solid voxel or when it reaches the end of the ray.
///
/// **Important Note:** These has been confusion in the past with people not realizing
/// that the length of the direction vector is important. Most graphics API can provide
/// a camera position and view direction for picking purposes, but the view direction is
/// usually normalized (i.e. of length one). If you use this view direction directly you
/// will only iterate over a single voxel and won't find what you are looking for. Instead
/// you must scale the direction vector so that it's length represents the maximum distance
/// over which you want the ray to be cast.
///
/// Some further notes, the Raycast uses full 26-connectivity, which basically means it
/// will examine every voxel the ray touches, even if it just passes through the corner.
/// Also, it performs a simple binary test against a voxel's threshold, rather than making
/// use of it's density. Therefore it will work best in conjunction with one of the 'cubic'
/// surface extractors. It's behaviour with the Marching Cubes surface extractor has not
/// been tested yet.
///
/// Note that we also have a pickVoxel() function which provides a slightly higher-level interface.
////////////////////////////////////////////////////////////////////////////////

// This function is based on Christer Ericson's code and description of the 'Uniform Grid Intersection Test' in
// 'Real Time Collision Detection'. The following information from the errata on the book website is also relevant:
//
//	pages 326-327. In the function VisitCellsOverlapped() the two lines calculating tx and ty are incorrect.
//  The less-than sign in each line should be a greater-than sign. That is, the two lines should read:
//
//	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) / Abs(x2 - x1);
//	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) / Abs(y2 - y1);
//
//	Thanks to Jetro Lauha of Fathammer in Helsinki, Finland for reporting this error.
//
//	Jetro also points out that the computations of i, j, iend, and jend are incorrectly rounded if the line
//  coordinates are allowed to go negative. While that was not really the intent of the code -- that is, I
//  assumed grids to be numbered from (0, 0) to (m, n) -- I'm at fault for not making my assumption clear.
//  Where it is important to handle negative line coordinates the computation of these variables should be
//  changed to something like this:
//
//	// Determine start grid cell coordinates (i, j)
//	int i = (int)floorf(x1 / CELL_SIDE);
//	int j = (int)floorf(y1 / CELL_SIDE);
//
//	// Determine end grid cell coordinates (iend, jend)
//	int iend = (int)floorf(x2 / CELL_SIDE);
//	int jend = (int)floorf(y2 / CELL_SIDE);
//
//	page 328. The if-statement that reads "if (ty <= tx && ty <= tz)" has a superfluous condition.
//  It should simply read "if (ty <= tz)".
//
//	This error was reported by Joey Hammer (PixelActive).

// The doRaycast function is assuming that it is iterating over the areas defined between
// voxels. We actually want to define the areas as being centered on voxels (as this is
// what the CubicSurfaceExtractor generates). We can add an offset here to adjust for this.
const float RaycastOffset = 0.0f;

/**
 * Cast a ray through a volume by specifying the start and end positions
 *
 * The ray will move from @a v3dStart to @a v3dEnd, calling @a callback for each
 * voxel it passes through until @a callback returns @a false. In this case it
 * returns a RaycastResults::Type::Interrupted. If it passes from start to end
 * without @a callback returning @a false, it returns RaycastResults::Type::Completed.
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

	const glm::vec3 v3dStart = start + RaycastOffset;
	const glm::vec3 v3dEnd = end + RaycastOffset;
	const float x1 = v3dStart.x;
	const float y1 = v3dStart.y;
	const float z1 = v3dStart.z;
	const float x2 = v3dEnd.x;
	const float y2 = v3dEnd.y;
	const float z2 = v3dEnd.z;

	const glm::ivec3 floorEnd(glm::floor(v3dEnd));
	const int iend = floorEnd.x;
	const int jend = floorEnd.y;
	const int kend = floorEnd.z;

	const int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	const int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	const int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	const glm::vec3 dist = glm::abs(v3dEnd - v3dStart);
	const float deltatx = dist.x < glm::epsilon<float>() ? 1.0f : 1.0f / dist.x;
	const float deltaty = dist.y < glm::epsilon<float>() ? 1.0f : 1.0f / dist.y;
	const float deltatz = dist.z < glm::epsilon<float>() ? 1.0f : 1.0f / dist.z;

	const glm::vec3 floorStart(glm::floor(v3dStart));
	const glm::vec3 maxs = floorStart + 1.0f;

	float tx = ((di == -1) ? (x1 - floorStart.x) : (maxs.x - x1)) * deltatx;
	float ty = ((dj == -1) ? (y1 - floorStart.y) : (maxs.y - y1)) * deltaty;
	float tz = ((dk == -1) ? (z1 - floorStart.z) : (maxs.z - z1)) * deltatz;

	int i = (int)floorStart.x;
	int j = (int)floorStart.y;
	int k = (int)floorStart.z;
	sampler.setPosition(i, j, k);

	// Track the last stepped face normal so we can report which face was hit when interrupted
	glm::ivec3 lastNormal{0, 0, 0};
	for (;;) {
		if (!callback(sampler)) {
			if (i == (int)floorStart.x && j == (int)floorStart.y && k == (int)floorStart.z) {
				return RaycastResult::interrupted(0.0f, 0.0f, lastNormal);
			}

			// hitting a voxel is returing the voxel position - but the voxel geometry at voxel 0,0,0 goes from 0,0,0
			// to 1,1,1 - we actually want to return the position of the face we hit
			glm::vec3 r = sampler.position();
			if (di == -1) {
				r.x += 1.0f;
			}
			if (dj == -1) {
				r.y += 1.0f;
			}
			if (dk == -1) {
				r.z += 1.0f;
			}

			const float length = glm::length(r - v3dStart);
			const float fract = length / glm::length(v3dEnd - v3dStart);
			return RaycastResult::interrupted(length, fract, lastNormal);
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

template<typename Callback>
inline RaycastResult raycastWithEndpointsVolume(voxel::RawVolume *volData, const glm::vec3 &v3dStart,
												const glm::vec3 &v3dEnd, Callback &&callback) {
	return raycastWithEndpoints(volData, v3dStart, v3dEnd, callback);
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
