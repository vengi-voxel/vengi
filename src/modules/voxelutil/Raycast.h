/**
 * @file
 */

#pragma once

#include "core/Trace.h"
#include "voxel/PagedVolume.h"
#include "voxel/RawVolume.h"
#include "core/Common.h"

namespace voxel {
namespace RaycastResults {

/**
 * The results of a raycast
 */
enum RaycastResult {
	Completed, ///< If the ray passed through the volume without being interrupted
	Interupted ///< If the ray was interrupted while traveling
};

}

typedef RaycastResults::RaycastResult RaycastResult;

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

/**
 * Cast a ray through a volume by specifying the start and end positions
 *
 * The ray will move from @a v3dStart to @a v3dEnd, calling @a callback for each
 * voxel it passes through until @a callback returns @a false. In this case it
 * returns a RaycastResults::Interrupted. If it passes from start to end
 * without @a callback returning @a false, it returns RaycastResults::Completed.
 *
 * @param volData The volume to pass the ray though
 * @param v3dStart The start position in the volume
 * @param v3dEnd The end position in the volume
 * @param callback The callback to call for each voxel
 *
 * @return A RaycastResults designating whether the ray hit anything or not
 */
template<typename Callback, class Volume>
RaycastResult raycastWithEndpoints(const Volume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dEnd, Callback&& callback) {
	core_trace_scoped(raycastWithEndpoints);
	typename Volume::Sampler sampler(volData);

	//The doRaycast function is assuming that it is iterating over the areas defined between
	//voxels. We actually want to define the areas as being centered on voxels (as this is
	//what the CubicSurfaceExtractor generates). We add 0.5 here to adjust for this.
	const float offset = 0.0f;
	const float x1 = v3dStart.x + offset;
	const float y1 = v3dStart.y + offset;
	const float z1 = v3dStart.z + offset;
	const float x2 = v3dEnd.x + offset;
	const float y2 = v3dEnd.y + offset;
	const float z2 = v3dEnd.z + offset;

	int i = (int) floorf(x1);
	int j = (int) floorf(y1);
	int k = (int) floorf(z1);

	const int iend = (int) floorf(x2);
	const int jend = (int) floorf(y2);
	const int kend = (int) floorf(z2);

	const int di = ((x1 < x2) ? 1 : ((x1 > x2) ? -1 : 0));
	const int dj = ((y1 < y2) ? 1 : ((y1 > y2) ? -1 : 0));
	const int dk = ((z1 < z2) ? 1 : ((z1 > z2) ? -1 : 0));

	const float deltatx = 1.0f / glm::abs(x2 - x1);
	const float deltaty = 1.0f / glm::abs(y2 - y1);
	const float deltatz = 1.0f / glm::abs(z2 - z1);

	const float minx = floorf(x1), maxx = minx + 1.0f;
	float tx = ((x1 > x2) ? (x1 - minx) : (maxx - x1)) * deltatx;
	const float miny = floorf(y1), maxy = miny + 1.0f;
	float ty = ((y1 > y2) ? (y1 - miny) : (maxy - y1)) * deltaty;
	const float minz = floorf(z1), maxz = minz + 1.0f;
	float tz = ((z1 > z2) ? (z1 - minz) : (maxz - z1)) * deltatz;

	sampler.setPosition(i, j, k);

	for (;;) {
		if (!callback(sampler)) {
			// TODO: calculate the face that was hit
			return RaycastResults::Interupted;
		}

		if (tx <= ty && tx <= tz) {
			if (i == iend) {
				break;
			}
			tx += deltatx;
			i += di;

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
		}
	}

	return RaycastResults::Completed;
}

template<typename Callback>
inline RaycastResult raycastWithEndpointsVolume(const PagedVolume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dEnd, Callback&& callback) {
	return raycastWithEndpoints(volData, v3dStart, v3dEnd, callback);
}

template<typename Callback>
inline RaycastResult raycastWithEndpointsVolume(const RawVolume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dEnd, Callback&& callback) {
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
RaycastResult raycastWithDirection(const Volume* volData, const glm::vec3& v3dStart, const glm::vec3& v3dDirectionAndLength, Callback&& callback) {
	const glm::vec3 v3dEnd = v3dStart + v3dDirectionAndLength;
	return raycastWithEndpoints<Callback, Volume>(volData, v3dStart, v3dEnd, core::forward<Callback>(callback));
}

}
