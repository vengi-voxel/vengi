/**
 * @file
 */

#include "VolumeSculpt.h"
#include "core/GLM.h"
#include "core/Trace.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "core/collection/DynamicSet.h"
#include "math/Axis.h"
#include "voxel/BitVolume.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/SparseVolume.h"
#include "voxel/Voxel.h"
#include <cfloat>
#include <climits>

namespace voxelutil {

static bool isSolid(const voxel::BitVolume &solid, const voxel::BitVolume &anchors, const glm::ivec3 &pos) {
	return solid.hasValue(pos.x, pos.y, pos.z) || anchors.hasValue(pos.x, pos.y, pos.z);
}

static int countFaceNeighbors(const voxel::BitVolume &solid, const voxel::BitVolume &anchors,
							  const glm::ivec3 &pos) {
	int count = 0;
	for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
		if (isSolid(solid, anchors, pos + offset)) {
			count++;
		}
	}
	return count;
}

void sculptErode(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
				 float strength, int iterations) {
	core_trace_scoped(SculptErode);
	const int removeThreshold = (int)glm::mix(0.0f, 5.0f, strength);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	core::DynamicArray<glm::ivec3> toRemove;
	for (int iter = 0; iter < iterations; ++iter) {
		toRemove.clear();

		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					const int neighborCount = countFaceNeighbors(solid, anchors, pos);
					if (neighborCount == 6) {
						continue;
					}
					if (neighborCount < removeThreshold) {
						toRemove.push_back(pos);
					}
				}
			}
		}

		if (toRemove.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : toRemove) {
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
		}
	}
}

void sculptGrow(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
				float strength, int iterations, const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptGrow);
	const int addThreshold = (int)glm::mix(7.0f, 1.0f, strength);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	using AirSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;

	core::DynamicArray<glm::ivec3> toAdd;
	AirSet airCandidates;
	for (int iter = 0; iter < iterations; ++iter) {
		toAdd.clear();
		airCandidates.clear();

		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
						const glm::ivec3 neighbor = pos + offset;
						if (!isSolid(solid, anchors, neighbor) && region.containsPoint(neighbor)) {
							airCandidates.insert(neighbor);
						}
					}
				}
			}
		}

		for (auto it = airCandidates.begin(); it != airCandidates.end(); ++it) {
			const glm::ivec3 &pos = it->key;
			const int myCount = countFaceNeighbors(solid, anchors, pos);
			if (myCount >= addThreshold) {
				toAdd.push_back(pos);
			}
		}

		if (toAdd.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : toAdd) {
			solid.setVoxel(pos, true);
			// Pick color from nearest solid face-neighbor
			voxel::Voxel newVoxel = fillVoxel;
			for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
				const glm::ivec3 neighbor = pos + offset;
				if (voxelMap.hasVoxel(neighbor)) {
					newVoxel = voxelMap.voxel(neighbor);
					break;
				}
			}
			voxelMap.setVoxel(pos, newVoxel);
		}
	}
}

void sculptFlatten(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, voxel::FaceNames face, int iterations) {
	if (face == voxel::FaceNames::Max) {
		return;
	}

	core_trace_scoped(SculptFlatten);
	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool fromPositive = voxel::isPositiveFace(face);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	core::DynamicArray<glm::ivec3> toRemove;
	for (int iter = 0; iter < iterations; ++iter) {
		int extremeVal = fromPositive ? INT_MIN : INT_MAX;
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (fromPositive) {
						extremeVal = glm::max(extremeVal, pos[axisIdx]);
					} else {
						extremeVal = glm::min(extremeVal, pos[axisIdx]);
					}
				}
			}
		}

		toRemove.clear();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (pos[axisIdx] == extremeVal) {
						toRemove.push_back(pos);
					}
				}
			}
		}

		if (toRemove.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : toRemove) {
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
		}
	}
}

// Compute column height along an axis from a position set.
// Returns the number of consecutive solid voxels from bottom towards top at the given 2D column.
static int columnHeight(const voxel::BitVolume &solid, const voxel::BitVolume &anchors, const glm::ivec3 &basePos,
						int axisIdx, int bottomVal, int topVal) {
	int height = 0;
	glm::ivec3 pos = basePos;
	for (int v = bottomVal; v <= topVal; ++v) {
		pos[axisIdx] = v;
		if (isSolid(solid, anchors, pos)) {
			height = v - bottomVal + 1;
		}
	}
	return height;
}

// Get the 4 planar neighbor offsets perpendicular to a given axis
static void getPlanarOffsets(int axisIdx, glm::ivec3 offsets[4]) {
	static constexpr glm::ivec3 planarOffsetsForAxis[3][4] = {
		// axis X: neighbors in Y and Z
		{glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)},
		// axis Y: neighbors in X and Z
		{glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 0, 1), glm::ivec3(0, 0, -1)},
		// axis Z: neighbors in X and Y
		{glm::ivec3(1, 0, 0), glm::ivec3(-1, 0, 0), glm::ivec3(0, 1, 0), glm::ivec3(0, -1, 0)}};
	for (int i = 0; i < 4; ++i) {
		offsets[i] = planarOffsetsForAxis[axisIdx][i];
	}
}

void sculptSmoothAdditive(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
						  voxel::FaceNames face, int heightThreshold, int iterations,
						  const voxel::Voxel &fillVoxel) {
	if (face == voxel::FaceNames::Max || heightThreshold < 1) {
		return;
	}
	core_trace_scoped(SculptSmoothAdditive);

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool positiveUp = voxel::isPositiveFace(face);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	glm::ivec3 planarOffsets[4];
	getPlanarOffsets(axisIdx, planarOffsets);

	using ColumnSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;

	for (int iter = 0; iter < iterations; ++iter) {
		// Recompute bounds each iteration since solid set grows
		int minVal = INT_MAX;
		int maxVal = INT_MIN;
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					const int v = pos[axisIdx];
					minVal = glm::min(minVal, v);
					maxVal = glm::max(maxVal, v);
				}
			}
		}
		if (minVal > maxVal) {
			return;
		}

		const int absBottom = glm::min(minVal, maxVal);
		const int absTop = glm::max(minVal, maxVal);
		const int bottomVal = positiveUp ? minVal : maxVal;
		const int topVal = positiveUp ? maxVal : minVal;
		const int step = positiveUp ? 1 : -1;

		// Collect all positions to add for this iteration (at most one per column).
		core::DynamicArray<glm::ivec3> toAdd;
		// Track which 2D columns already have a pending add this iteration
		ColumnSet addedColumns;

		for (int layer = bottomVal; layer != topVal + step; layer += step) {
			for (int z = lo.z; z <= hi.z; ++z) {
				for (int y = lo.y; y <= hi.y; ++y) {
					for (int x = lo.x; x <= hi.x; ++x) {
						if (!solid.hasValue(x, y, z)) {
							continue;
						}
						const glm::ivec3 pos(x, y, z);
						if (pos[axisIdx] != layer) {
							continue;
						}

						// Check the air voxel one step above this solid position
						glm::ivec3 above = pos;
						above[axisIdx] += step;
						if (isSolid(solid, anchors, above)) {
							continue;
						}

						// Use the 2D column key (zero out the up-axis component)
						glm::ivec3 colKey = pos;
						colKey[axisIdx] = 0;
						if (addedColumns.has(colKey)) {
							continue;
						}

						// Compute current column height at this 2D position
						const int curHeight = columnHeight(solid, anchors, pos, axisIdx, absBottom, absTop);

						// Check 4 planar neighbors
						for (int i = 0; i < 4; ++i) {
							const glm::ivec3 neighborBase = pos + planarOffsets[i];
							const int neighborHeight =
								columnHeight(solid, anchors, neighborBase, axisIdx, absBottom, absTop);
							if (neighborHeight - curHeight >= heightThreshold) {
								toAdd.push_back(above);
								addedColumns.insert(colKey);
								break;
							}
						}
					}
				}
			}
		}

		if (toAdd.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : toAdd) {
			solid.setVoxel(pos, true);
			// Pick color from nearest solid face-neighbor
			voxel::Voxel newVoxel = fillVoxel;
			for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
				const glm::ivec3 neighbor = pos + offset;
				if (voxelMap.hasVoxel(neighbor)) {
					newVoxel = voxelMap.voxel(neighbor);
					break;
				}
			}
			voxelMap.setVoxel(pos, newVoxel);
		}
	}
}

void sculptSmoothErode(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
					   voxel::FaceNames face, int iterations, bool preserveTopHeight, int trimPerStep) {
	if (face == voxel::FaceNames::Max) {
		return;
	}

	core_trace_scoped(SculptSmoothErode);

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool positiveUp = voxel::isPositiveFace(face);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	glm::ivec3 planarOffsets[4];
	getPlanarOffsets(axisIdx, planarOffsets);

	const int step = positiveUp ? 1 : -1;

	if (preserveTopHeight) {
		// Per-island slope trimming. Find connected "top islands" (columns sharing the
		// same top height). Each island gets its own center. Columns farther from their
		// island center get trimmed more: trim = min(dist * trimPerStep, iterations).
		const int axis1 = (axisIdx == 0) ? 1 : 0;
		const int axis2 = (axisIdx == 2) ? 1 : 2;

		// Step 1: Build column top height map
		using ColHeightMap = core::DynamicMap<glm::ivec3, int, 1031, glm::hash<glm::ivec3>>;
		ColHeightMap columnTop;

		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					glm::ivec3 colKey = pos;
					colKey[axisIdx] = 0;
					const int val = pos[axisIdx];

					auto foundTop = columnTop.find(colKey);
					if (foundTop == columnTop.end()) {
						columnTop.put(colKey, val);
					} else if ((positiveUp && val > foundTop->second) ||
							   (!positiveUp && val < foundTop->second)) {
						columnTop.put(colKey, val);
					}
				}
			}
		}

		// Step 2: Flood-fill islands among columns with the same top height (4-connected in 2D)
		using IslandIdMap = core::DynamicMap<glm::ivec3, int, 1031, glm::hash<glm::ivec3>>;
		IslandIdMap islandId;
		struct IslandInfo {
			float centerA;
			float centerB;
			int topHeight;
		};
		core::DynamicArray<IslandInfo> islands;
		core::DynamicArray<glm::ivec3> bfsQueue;
		bfsQueue.reserve(columnTop.size());
		int bottomHeight = positiveUp ? INT_MAX : INT_MIN;

		for (auto it = columnTop.begin(); it != columnTop.end(); ++it) {
			const glm::ivec3 &colKey = it->key;
			if (islandId.find(colKey) != islandId.end()) {
				continue;
			}

			const int myHeight = it->second;
			const int id = (int)islands.size();
			float sumA = 0.0f;
			float sumB = 0.0f;
			int count = 0;

			bfsQueue.clear();
			bfsQueue.push_back(colKey);
			islandId.put(colKey, id);

			size_t front = 0;
			while (front < bfsQueue.size()) {
				const glm::ivec3 cur = bfsQueue[front];
				front++;
				sumA += (float)cur[axis1];
				sumB += (float)cur[axis2];
				count++;

				for (int i = 0; i < 4; ++i) {
					const glm::ivec3 neighbor = cur + planarOffsets[i];
					if (islandId.find(neighbor) != islandId.end()) {
						continue;
					}
					auto neighborTop = columnTop.find(neighbor);
					if (neighborTop == columnTop.end()) {
						continue;
					}
					// Only connect columns with the same top height
					if (neighborTop->second != myHeight) {
						continue;
					}
					islandId.put(neighbor, id);
					bfsQueue.push_back(neighbor);
				}
			}

			IslandInfo info;
			info.centerA = sumA / (float)count;
			info.centerB = sumB / (float)count;
			info.topHeight = myHeight;

			// If the centroid falls between columns (e.g. 2x2 island), snap to the
			// nearest member so that at least one column stays at dist=0.
			if (count > 1) {
				float bestDist = FLT_MAX;
				float snapA = info.centerA;
				float snapB = info.centerB;
				for (size_t qi = 0; qi < bfsQueue.size(); ++qi) {
					const float da = glm::abs((float)bfsQueue[qi][axis1] - info.centerA);
					const float db = glm::abs((float)bfsQueue[qi][axis2] - info.centerB);
					const float d = glm::max(da, db);
					if (d < bestDist) {
						bestDist = d;
						snapA = (float)bfsQueue[qi][axis1];
						snapB = (float)bfsQueue[qi][axis2];
					}
				}
				info.centerA = snapA;
				info.centerB = snapB;
			}

			islands.push_back(info);

			// Track the bottom island height
			if (positiveUp) {
				bottomHeight = glm::min(bottomHeight, myHeight);
			} else {
				bottomHeight = glm::max(bottomHeight, myHeight);
			}
		}

		// Step 3: For each island column, trim = min(dist * trimPerStep, iterations).
		// Skip the bottom island. Center columns (dist=0) keep full height.
		core::DynamicArray<glm::ivec3> toRemove;
		toRemove.reserve(columnTop.size());
		for (auto it = islandId.begin(); it != islandId.end(); ++it) {
			const glm::ivec3 &colKey = it->key;
			const int id = it->second;
			const IslandInfo &info = islands[id];

			if (info.topHeight == bottomHeight) {
				continue;
			}

			const float da = glm::abs((float)colKey[axis1] - info.centerA);
			const float db = glm::abs((float)colKey[axis2] - info.centerB);
			const int dist = (int)glm::floor(glm::max(da, db));
			if (dist == 0) {
				continue;
			}

			const int topCoord = info.topHeight;
			// Cap trim so columns never sink to the bottom island height
			const int maxTrim = glm::max(0, (topCoord - bottomHeight) * step - 1);
			const int trim = glm::min(glm::min(dist * trimPerStep, iterations), maxTrim);

			for (int t = 0; t < trim; ++t) {
				glm::ivec3 pos = colKey;
				pos[axisIdx] = topCoord - t * step;
				if (solid.hasValue(pos.x, pos.y, pos.z)) {
					toRemove.push_back(pos);
				}
			}
		}

		for (const glm::ivec3 &pos : toRemove) {
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
		}
		return;
	}

	// Normal iteration-based smooth erode
	using ColumnSet = core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>>;
	for (int iter = 0; iter < iterations; ++iter) {
		int minVal = INT_MAX;
		int maxVal = INT_MIN;
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const int v = glm::ivec3(x, y, z)[axisIdx];
					minVal = glm::min(minVal, v);
					maxVal = glm::max(maxVal, v);
				}
			}
		}
		if (minVal > maxVal) {
			return;
		}

		const int topVal = positiveUp ? maxVal : minVal;
		const int bottomVal = positiveUp ? minVal : maxVal;

		core::DynamicArray<glm::ivec3> toRemove;
		ColumnSet removedColumns;

		for (int layer = topVal; layer != bottomVal - step; layer -= step) {
			for (int z = lo.z; z <= hi.z; ++z) {
				for (int y = lo.y; y <= hi.y; ++y) {
					for (int x = lo.x; x <= hi.x; ++x) {
						if (!solid.hasValue(x, y, z)) {
							continue;
						}
						const glm::ivec3 pos(x, y, z);
						if (pos[axisIdx] != layer) {
							continue;
						}

						glm::ivec3 above = pos;
						above[axisIdx] += step;
						if (isSolid(solid, anchors, above)) {
							continue;
						}

						glm::ivec3 colKey = pos;
						colKey[axisIdx] = 0;
						if (removedColumns.has(colKey)) {
							continue;
						}

						int planarCount = 0;
						for (int i = 0; i < 4; ++i) {
							const glm::ivec3 neighbor = pos + planarOffsets[i];
							if (isSolid(solid, anchors, neighbor)) {
								planarCount++;
							}
						}
						if (planarCount < 4) {
							toRemove.push_back(pos);
							removedColumns.insert(colKey);
						}
					}
				}
			}
		}

		if (toRemove.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : toRemove) {
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
		}
	}
}

// Helper to build solid, voxelMap, and anchor sets from a volume region
static void buildFromVolume(const voxel::RawVolume &volume, const voxel::Region &region,
							voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, voxel::BitVolume &anchors) {
	core_trace_scoped(BuildFromVolume);
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				const voxel::Voxel &v = volume.voxel(x, y, z);
				if (!voxel::isBlocked(v.getMaterial())) {
					continue;
				}
				solid.setVoxel(x, y, z, true);
				voxelMap.setVoxel(x, y, z, v);
			}
		}
	}

	// Anchors: solid voxels adjacent to but outside the region
	const voxel::Region &volRegion = volume.region();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!solid.hasValue(x, y, z)) {
					continue;
				}
				for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
					const glm::ivec3 neighbor = glm::ivec3(x, y, z) + offset;
					if (region.containsPoint(neighbor)) {
						continue;
					}
					if (!volRegion.containsPoint(neighbor)) {
						continue;
					}
					const voxel::Voxel &v = volume.voxel(neighbor);
					if (voxel::isBlocked(v.getMaterial())) {
						anchors.setVoxel(neighbor, true);
					}
				}
			}
		}
	}
}

// Helper to write position set changes back to a volume
static int writeResultToVolume(voxel::RawVolume &volume, const voxel::Region &region, const voxel::BitVolume &solid,
							   const voxel::SparseVolume &voxelMap) {
	core_trace_scoped(WriteResultToVolume);
	int changed = 0;
	const voxel::Voxel air;

	// Remove voxels that were solid in the region but are no longer
	// TODO: PERF: use visitor
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				const voxel::Voxel &v = volume.voxel(x, y, z);
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				if (!solid.hasValue(x, y, z)) {
					volume.setVoxel(x, y, z, air);
					changed++;
				}
			}
		}
	}

	// Add new voxels
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!solid.hasValue(x, y, z)) {
					continue;
				}
				const voxel::Voxel &current = volume.voxel(x, y, z);
				if (voxel::isBlocked(current.getMaterial())) {
					continue;
				}
				if (voxelMap.hasVoxel(x, y, z)) {
					volume.setVoxel(x, y, z, voxelMap.voxel(x, y, z));
					changed++;
				}
			}
		}
	}

	return changed;
}

int sculptErode(voxel::RawVolume &volume, const voxel::Region &region, float strength, int iterations) {
	core_trace_scoped(SculptErodeVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	// Grow anchor region by 1 to capture adjacent voxels outside the sculpt region
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptErode(solid, voxelMap, anchors, strength, iterations);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptGrow(voxel::RawVolume &volume, const voxel::Region &region, float strength, int iterations,
			   const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptGrowVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptGrow(solid, voxelMap, anchors, strength, iterations, fillVoxel);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptFlatten(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face, int iterations) {
	core_trace_scoped(SculptFlattenVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptFlatten(solid, voxelMap, face, iterations);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptSmoothAdditive(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
						 int heightThreshold, int iterations, const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptSmoothAdditiveVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptSmoothAdditive(solid, voxelMap, anchors, face, heightThreshold, iterations, fillVoxel);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptSmoothErode(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face, int iterations,
					  bool preserveTopHeight, int trimPerStep) {
	core_trace_scoped(SculptSmoothErodeVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptSmoothErode(solid, voxelMap, anchors, face, iterations, preserveTopHeight, trimPerStep);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

} // namespace voxelutil
