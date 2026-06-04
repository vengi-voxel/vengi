/**
 * @file
 */

#include "VolumeSculpt.h"
#include "core/Log.h"
#include "app/ForParallel.h"
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
#include "core/Algorithm.h"
#include "palette/Palette.h"
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
	// Step direction: move top voxels inward (positive face -> step -1, negative face -> step +1)
	const int step = fromPositive ? -1 : 1;

	core::DynamicArray<glm::ivec3> extremePositions;
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

		extremePositions.clear();
		for (int z = lo.z; z <= hi.z; ++z) {
			for (int y = lo.y; y <= hi.y; ++y) {
				for (int x = lo.x; x <= hi.x; ++x) {
					if (!solid.hasValue(x, y, z)) {
						continue;
					}
					const glm::ivec3 pos(x, y, z);
					if (pos[axisIdx] == extremeVal) {
						extremePositions.push_back(pos);
					}
				}
			}
		}

		if (extremePositions.empty()) {
			break;
		}

		for (const glm::ivec3 &pos : extremePositions) {
			// Move the top voxel one step inward to preserve the cap surface and its color
			glm::ivec3 dest = pos;
			dest[axisIdx] += step;
			const voxel::Voxel topVoxel = voxelMap.hasVoxel(pos) ? voxelMap.voxel(pos) : voxel::Voxel();
			// Remove the original extreme position
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
			// Place the top voxel at the destination if it is within the region
			if (region.containsPoint(dest)) {
				solid.setVoxel(dest, true);
				voxelMap.setVoxel(dest, topVoxel);
			}
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
	static const glm::ivec3 planarOffsetsForAxis[3][4] = {
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
		// Track each trimmed column's original top voxel and new top position.
		struct TrimEntry {
			glm::ivec3 colKey;
			int topCoord;
			int trim;
		};
		core::DynamicArray<glm::ivec3> toRemove;
		core::DynamicArray<TrimEntry> trimmedColumns;
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

			if (trim > 0) {
				trimmedColumns.push_back({colKey, topCoord, trim});
			}

			for (int t = 0; t < trim; ++t) {
				glm::ivec3 pos = colKey;
				pos[axisIdx] = topCoord - t * step;
				if (solid.hasValue(pos.x, pos.y, pos.z)) {
					toRemove.push_back(pos);
				}
			}
		}

		// Save original top voxel colors before erasing
		core::DynamicArray<voxel::Voxel> topVoxels;
		topVoxels.reserve(trimmedColumns.size());
		for (const TrimEntry &entry : trimmedColumns) {
			glm::ivec3 topPos = entry.colKey;
			topPos[axisIdx] = entry.topCoord;
			topVoxels.push_back(voxelMap.hasVoxel(topPos) ? voxelMap.voxel(topPos) : voxel::Voxel());
		}

		for (const glm::ivec3 &pos : toRemove) {
			solid.setVoxel(pos, false);
			voxelMap.setVoxel(pos, voxel::Voxel());
		}

		// Place original top voxel at each column's new top position
		for (size_t i = 0; i < trimmedColumns.size(); ++i) {
			const TrimEntry &entry = trimmedColumns[i];
			glm::ivec3 newTopPos = entry.colKey;
			newTopPos[axisIdx] = entry.topCoord - entry.trim * step;
			if (!region.containsPoint(newTopPos)) {
				continue;
			}
			solid.setVoxel(newTopPos, true);
			voxelMap.setVoxel(newTopPos, topVoxels[i]);
		}
		return;
	}

	// Height-map smooth erode: each iteration, columns taller than their neighbor
	// average get trimmed toward that average. Never grows, never removes the bottom layer.
	const int axis1 = (axisIdx == 0) ? 1 : 0;
	const int axis2 = (axisIdx == 2) ? 1 : 2;
	const int base1 = lo[axis1];
	const int base2 = lo[axis2];
	const int extent1 = hi[axis1] - base1 + 1;
	const int extent2 = hi[axis2] - base2 + 1;
	const int numColumns = extent1 * extent2;
	const int axisLo = lo[axisIdx];
	const int axisHi = hi[axisIdx];

	static constexpr int EMPTY = INT_MIN;
	static constexpr int SEAL_DEPTH = 2;

	core::DynamicArray<int> colTopArr(numColumns);
	core::DynamicArray<int> colBotArr(numColumns);

	for (int iter = 0; iter < iterations; ++iter) {
		// Step 1: Build column top/bottom height map
		for (int idx = 0; idx < numColumns; ++idx) {
			colTopArr[idx] = EMPTY;
			colBotArr[idx] = EMPTY;
		}

		for (int a1 = 0; a1 < extent1; ++a1) {
			for (int a2 = 0; a2 < extent2; ++a2) {
				const int flatIdx = a1 * extent2 + a2;
				const int coord1 = base1 + a1;
				const int coord2 = base2 + a2;
				for (int av = axisLo; av <= axisHi; ++av) {
					glm::ivec3 pos;
					pos[axis1] = coord1;
					pos[axis2] = coord2;
					pos[axisIdx] = av;
					if (!isSolid(solid, anchors, pos)) {
						continue;
					}
					if (colTopArr[flatIdx] == EMPTY) {
						colTopArr[flatIdx] = av;
						colBotArr[flatIdx] = av;
					} else if (positiveUp) {
						colTopArr[flatIdx] = glm::max(colTopArr[flatIdx], av);
						colBotArr[flatIdx] = glm::min(colBotArr[flatIdx], av);
					} else {
						colTopArr[flatIdx] = glm::min(colTopArr[flatIdx], av);
						colBotArr[flatIdx] = glm::max(colBotArr[flatIdx], av);
					}
				}
			}
		}

		// Global minimum bottom: used as self term so floating columns still erode
		int globalMinBot;
		if (positiveUp) {
			globalMinBot = axisHi;
			for (int idx = 0; idx < numColumns; ++idx) {
				if (colBotArr[idx] != EMPTY) {
					globalMinBot = glm::min(globalMinBot, colBotArr[idx]);
				}
			}
		} else {
			globalMinBot = axisLo;
			for (int idx = 0; idx < numColumns; ++idx) {
				if (colBotArr[idx] != EMPTY) {
					globalMinBot = glm::max(globalMinBot, colBotArr[idx]);
				}
			}
		}

		// Step 2: For each column, check if its top is above the 3x3 neighbor average.
		// If so, trim exactly 1 voxel. Limiting to 1 per iteration prevents gaps
		// between adjacent columns that erode at different rates.
		bool anyChange = false;

		for (int a1 = 0; a1 < extent1; ++a1) {
			for (int a2 = 0; a2 < extent2; ++a2) {
				const int flatIdx = a1 * extent2 + a2;
				const int myTop = colTopArr[flatIdx];
				if (myTop == EMPTY) {
					continue;
				}

				// Average top height of populated neighbors in the 3x3 kernel
				int heightSum = 0;
				int count = 0;
				for (int da1 = -1; da1 <= 1; ++da1) {
					for (int da2 = -1; da2 <= 1; ++da2) {
						const int na1 = a1 + da1;
						const int na2 = a2 + da2;
						if (na1 >= 0 && na1 < extent1 && na2 >= 0 && na2 < extent2) {
							const int nt = colTopArr[na1 * extent2 + na2];
							if (nt != EMPTY) {
								heightSum += nt;
								count++;
							}
						}
					}
				}
				if (count < 1) {
					continue;
				}

				const int avgTop = heightSum / count;
				// Only trim if column is above average. Never grow.
				if ((positiveUp && avgTop >= myTop) || (!positiveUp && avgTop <= myTop)) {
					continue;
				}
				// Never trim below global floor
				if ((positiveUp && myTop <= globalMinBot) || (!positiveUp && myTop >= globalMinBot)) {
					continue;
				}

				// Trim exactly 1 voxel from the top
				const int coord1 = base1 + a1;
				const int coord2 = base2 + a2;
				glm::ivec3 removePos;
				removePos[axis1] = coord1;
				removePos[axis2] = coord2;
				removePos[axisIdx] = myTop;

				voxel::Voxel removedVoxel;
				if (voxelMap.hasVoxel(removePos)) {
					removedVoxel = voxelMap.voxel(removePos);
				}

				if (solid.hasValue(removePos.x, removePos.y, removePos.z)) {
					solid.setVoxel(removePos, false);
					voxelMap.setVoxel(removePos, voxel::Voxel());
					anyChange = true;
				}

					// Seal: ensure voxels below the removed one are solid
				for (int s = 1; s <= SEAL_DEPTH; ++s) {
					glm::ivec3 belowPos;
					belowPos[axis1] = coord1;
					belowPos[axis2] = coord2;
					belowPos[axisIdx] = myTop - step * s;
					if (!region.containsPoint(belowPos)) {
						break;
					}
					solid.setVoxel(belowPos, true);
					voxelMap.setVoxel(belowPos, removedVoxel);
				}
			}
		}

		if (!anyChange) {
			break;
		}
	}
}

void sculptBridgeGap(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
					 const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptBridgeGap);

	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	// Step 1: Find boundary voxels (solid with at least one air face-neighbor)
	core::DynamicArray<glm::ivec3> boundary;
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				const glm::ivec3 pos(x, y, z);
				if (!isSolid(solid, anchors, pos)) {
					continue;
				}
				for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
					const glm::ivec3 neighbor = pos + offset;
					if (region.containsPoint(neighbor) && !isSolid(solid, anchors, neighbor)) {
						boundary.push_back(pos);
						break;
					}
				}
			}
		}
	}

	if (boundary.size() < 2) {
		return;
	}

	// Step 2: For each pair of boundary voxels, draw a 3D line and fill air along it.
	const int numBoundary = (int)boundary.size();
	for (int idxA = 0; idxA < numBoundary; ++idxA) {
		for (int idxB = idxA + 1; idxB < numBoundary; ++idxB) {
			const glm::ivec3 diff = boundary[idxB] - boundary[idxA];
			const int distSq = diff.x * diff.x + diff.y * diff.y + diff.z * diff.z;
			// Skip pairs that are already adjacent (face/edge/corner neighbors)
			if (distSq <= 3) {
				continue;
			}

			const glm::ivec3 &start = boundary[idxA];
			const glm::ivec3 &end = boundary[idxB];
			const int dx = glm::abs(end.x - start.x);
			const int dy = glm::abs(end.y - start.y);
			const int dz = glm::abs(end.z - start.z);
			const int sx = (end.x > start.x) ? 1 : (end.x < start.x) ? -1 : 0;
			const int sy = (end.y > start.y) ? 1 : (end.y < start.y) ? -1 : 0;
			const int sz = (end.z > start.z) ? 1 : (end.z < start.z) ? -1 : 0;
			const int totalSteps = dx + dy + dz;

			glm::ivec3 pos = start;
			int ex = 0;
			int ey = 0;
			int ez = 0;
			for (int step = 0; step <= totalSteps; ++step) {
				if (!solid.hasValue(pos.x, pos.y, pos.z) && region.containsPoint(pos)) {
					solid.setVoxel(pos, true);
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
				ex += dx;
				ey += dy;
				ez += dz;
				if (ex >= ey && ex >= ez) {
					pos.x += sx;
					ex -= totalSteps;
				} else if (ey >= ez) {
					pos.y += sy;
					ey -= totalSteps;
				} else {
					pos.z += sz;
					ez -= totalSteps;
				}
			}
		}
	}
}

void sculptSmoothGaussian(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
						  voxel::FaceNames face, int kernelSize, float sigma, int iterations,
						  const voxel::Voxel &fillVoxel) {
	if (face == voxel::FaceNames::Max || kernelSize < 1 || sigma <= 0.0f) {
		return;
	}

	core_trace_scoped(SculptSmoothGaussian);

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool positiveUp = voxel::isPositiveFace(face);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	const int step = positiveUp ? 1 : -1;

	const int axis1 = (axisIdx == 0) ? 1 : 0;
	const int axis2 = (axisIdx == 2) ? 1 : 2;

	// 2D grid extents on the two planar axes
	const int base1 = lo[axis1];
	const int base2 = lo[axis2];
	const int extent1 = hi[axis1] - base1 + 1;
	const int extent2 = hi[axis2] - base2 + 1;
	const int numColumns = extent1 * extent2;

	// Sentinel value for empty columns
	static constexpr int EMPTY = INT_MIN;

	// Precompute kernel entries: only non-zero weight (da, db) pairs stored with flat index offsets
	struct KernelEntry {
		int da;
		int db;
		int offset; // flat index offset: da * extent2 + db
		float weight;
	};
	const int kernelDiameter = kernelSize * 2 + 1;
	core::DynamicArray<KernelEntry> kernelEntries;
	kernelEntries.reserve(kernelDiameter * kernelDiameter);
	const float twoSigmaSq = 2.0f * sigma * sigma;
	const float radiusSq = (float)(kernelSize * kernelSize);
	for (int da = -kernelSize; da <= kernelSize; ++da) {
		for (int db = -kernelSize; db <= kernelSize; ++db) {
			const float distSq = (float)(da * da + db * db);
			if (distSq > radiusSq) {
				continue;
			}
			KernelEntry entry;
			entry.da = da;
			entry.db = db;
			entry.offset = da * extent2 + db;
			entry.weight = glm::exp(-distSq / twoSigmaSq);
			kernelEntries.push_back(entry);
		}
	}
	const int numKernelEntries = (int)kernelEntries.size();
	const KernelEntry *kernelData = kernelEntries.data();

	// Flat 2D arrays for column top/bottom coordinates (reused across iterations)
	core::DynamicArray<int> colTopArr(numColumns);
	core::DynamicArray<int> colBotArr(numColumns);
	core::DynamicArray<int> targetArr(numColumns);

	for (int iter = 0; iter < iterations; ++iter) {
		// Step 1: Build column top/bottom from the solid BitVolume
		// Initialize all columns to empty
		for (int i = 0; i < numColumns; ++i) {
			colTopArr[i] = EMPTY;
			colBotArr[i] = EMPTY;
		}

		const int axisLo = lo[axisIdx];
		const int axisHi = hi[axisIdx];
		for (int a1 = 0; a1 < extent1; ++a1) {
			for (int a2 = 0; a2 < extent2; ++a2) {
				const int flatIdx = a1 * extent2 + a2;
				const int coord1 = base1 + a1;
				const int coord2 = base2 + a2;
				int topVal = EMPTY;
				int botVal = EMPTY;
				for (int av = axisLo; av <= axisHi; ++av) {
					glm::ivec3 pos;
					pos[axis1] = coord1;
					pos[axis2] = coord2;
					pos[axisIdx] = av;
					if (!isSolid(solid, anchors, pos)) {
						continue;
					}
					if (topVal == EMPTY) {
						topVal = av;
						botVal = av;
					} else if (positiveUp) {
						topVal = glm::max(topVal, av);
						botVal = glm::min(botVal, av);
					} else {
						topVal = glm::min(topVal, av);
						botVal = glm::max(botVal, av);
					}
				}
				colTopArr[flatIdx] = topVal;
				colBotArr[flatIdx] = botVal;
			}
		}

		// Step 2: Gaussian convolution on the height map (parallel over rows)
		// Initialize targets to EMPTY (no change needed)
		for (int i = 0; i < numColumns; ++i) {
			targetArr[i] = EMPTY;
		}

		bool anyChange = false;
		const int *topData = colTopArr.data();

		app::for_parallel(0, extent1, [&](int startRow, int endRow) {
			for (int a1 = startRow; a1 < endRow; ++a1) {
				for (int a2 = 0; a2 < extent2; ++a2) {
					const int flatIdx = a1 * extent2 + a2;
					const int myTop = topData[flatIdx];
					if (myTop == EMPTY) {
						continue;
					}

					float weightSum = 0.0f;
					float heightSum = 0.0f;

					for (int ki = 0; ki < numKernelEntries; ++ki) {
						const int na1 = a1 + kernelData[ki].da;
						const int na2 = a2 + kernelData[ki].db;
						if (na1 < 0 || na1 >= extent1 || na2 < 0 || na2 >= extent2) {
							continue;
						}
						const int neighborTop = topData[flatIdx + kernelData[ki].offset];
						if (neighborTop == EMPTY) {
							continue;
						}
						heightSum += kernelData[ki].weight * (float)neighborTop;
						weightSum += kernelData[ki].weight;
					}

					if (weightSum > 0.0f) {
						const int target = (int)glm::round(heightSum / weightSum);
						if (target != myTop) {
							targetArr[flatIdx] = target;
						}
					}
				}
			}
		});

		// Step 3: Apply height changes (sequential - mutates shared BitVolume/SparseVolume)
		for (int a1 = 0; a1 < extent1; ++a1) {
			for (int a2 = 0; a2 < extent2; ++a2) {
				const int flatIdx = a1 * extent2 + a2;
				const int target = targetArr[flatIdx];
				if (target == EMPTY) {
					continue;
				}
				const int currentTop = colTopArr[flatIdx];
				const int currentBottom = colBotArr[flatIdx];
				const int coord1 = base1 + a1;
				const int coord2 = base2 + a2;

				if ((positiveUp && target < currentTop) || (!positiveUp && target > currentTop)) {
					// Trim from top down to target, but not below column bottom
					const int trimLimit = positiveUp
						? glm::max(target + 1, currentBottom)
						: glm::min(target - 1, currentBottom);
					for (int v = currentTop; v != trimLimit; v -= step) {
						glm::ivec3 pos;
						pos[axis1] = coord1;
						pos[axis2] = coord2;
						pos[axisIdx] = v;
						if (solid.hasValue(pos.x, pos.y, pos.z)) {
							solid.setVoxel(pos, false);
							voxelMap.setVoxel(pos, voxel::Voxel());
							anyChange = true;
						}
					}
				} else if ((positiveUp && target > currentTop) || (!positiveUp && target < currentTop)) {
					// Grow from current top up to target
					for (int v = currentTop + step; v != target + step; v += step) {
						glm::ivec3 pos;
						pos[axis1] = coord1;
						pos[axis2] = coord2;
						pos[axisIdx] = v;
						if (region.containsPoint(pos) && !solid.hasValue(pos.x, pos.y, pos.z)) {
							solid.setVoxel(pos, true);
							voxel::Voxel newVoxel = fillVoxel;
							for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
								const glm::ivec3 neighbor = pos + offset;
								if (voxelMap.hasVoxel(neighbor)) {
									newVoxel = voxelMap.voxel(neighbor);
									break;
								}
							}
							voxelMap.setVoxel(pos, newVoxel);
							anyChange = true;
						}
					}
				}
			}
		}

		if (!anyChange) {
			break;
		}
	}
}

// Fill a single voxel position: mark solid in BitVolume and copy color from nearest neighbor.
// Templated to work with SparseVolume (full path) or RawVolume (fast path).
template<typename ColorSource>
static void fillSmoothWallVoxel(voxel::BitVolume &solid, ColorSource &colorSource,
								const glm::ivec3 &pos, const voxel::Voxel &fillVoxel,
								core::DynamicArray<glm::ivec3> &addedPositions) {
	if (solid.hasValue(pos.x, pos.y, pos.z)) {
		return;
	}
	solid.setVoxel(pos, true);
	addedPositions.push_back(pos);
	voxel::Voxel newVoxel = fillVoxel;
	for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
		const glm::ivec3 neighbor = pos + offset;
		const voxel::Voxel &nv = colorSource.voxel(neighbor);
		if (voxel::isBlocked(nv.getMaterial())) {
			newVoxel = nv;
			break;
		}
	}
	colorSource.setVoxel(pos, newVoxel);
}

template<typename ColorSource>
static void sculptSmoothWallImpl(voxel::BitVolume &solid, ColorSource &colorSource, const voxel::BitVolume &anchors,
					  voxel::FaceNames face, int iterations, const voxel::Voxel &fillVoxel,
					  int removeAboveDepth, SmoothWallInterp interp, bool fillHoles,
					  core::DynamicArray<glm::ivec3> &addedPositions) {
	core_trace_scoped(SculptSmoothWall);

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const bool positiveUp = voxel::isPositiveFace(face);
	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	// U and V are the two axes perpendicular to the face normal
	const int uAxis = (axisIdx == 0) ? 1 : 0;
	const int vAxis = (axisIdx == 2) ? 1 : 2;

	const int baseU = lo[uAxis];
	const int baseV = lo[vAxis];
	const int extentU = hi[uAxis] - baseU + 1;
	const int extentV = hi[vAxis] - baseV + 1;

	// Need at least 3 columns in each direction (edges + 1 interior)
	if (extentU < 3 || extentV < 3) {
		return;
	}

	const int numColumns = extentU * extentV;
	addedPositions.reserve(numColumns);
	static constexpr int EMPTY = INT_MIN;
	const int axisLo = lo[axisIdx];
	const int axisHi = hi[axisIdx];
	// removeAboveDepth == 0 means don't clear anything above the target surface.
	// Negative values are clamped to 0.
	if (removeAboveDepth < 0) {
		removeAboveDepth = 0;
	}

	// Flat 2D arrays for column heights (preallocated, reused across iterations)
	core::DynamicArray<int> colTopArr(numColumns);
	core::DynamicArray<int> colBotArr(numColumns);
	core::DynamicArray<int> targetArr(numColumns);
	core::DynamicArray<bool> isEdgeArr(numColumns);

	// Per-column nearest-edge precomputed arrays (4 directions)
	core::DynamicArray<int> nearEdgeHeightLeft(numColumns);
	core::DynamicArray<int> nearEdgeDistLeft(numColumns);
	core::DynamicArray<int> nearEdgeHeightRight(numColumns);
	core::DynamicArray<int> nearEdgeDistRight(numColumns);
	core::DynamicArray<int> nearEdgeHeightTop(numColumns);
	core::DynamicArray<int> nearEdgeDistTop(numColumns);
	core::DynamicArray<int> nearEdgeHeightBottom(numColumns);
	core::DynamicArray<int> nearEdgeDistBottom(numColumns);

	// Exterior-empty BFS (only used when fillHoles=true)
	core::DynamicArray<bool> isExteriorEmpty;
	core::DynamicArray<int> bfsQueue;
	if (fillHoles) {
		isExteriorEmpty.resize(numColumns);
		bfsQueue.reserve(numColumns);
	}

	for (int iter = 0; iter < iterations; ++iter) {
		// Step 1: Build column top/bottom from the solid BitVolume only (parallel).
		// Anchor voxels (non-selected solid neighbors) are intentionally excluded:
		// they are not in solid and cannot be cleared, so including them in the
		// height scan causes colTopArr to reflect positions that the clear loop
		// cannot remove (solid.hasValue returns false for anchors).
		for (int idx = 0; idx < numColumns; ++idx) {
			colTopArr[idx] = EMPTY;
			colBotArr[idx] = EMPTY;
		}

		app::for_parallel(0, extentU, [&](int startRow, int endRow) {
			for (int iu = startRow; iu < endRow; ++iu) {
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					const int coordU = baseU + iu;
					const int coordV = baseV + iv;
					// Scan from both ends to find top and bottom in O(depth) worst case
					// but O(1) for thin selections (e.g. 1-voxel thick wall in 200-deep range).
					// positiveUp: top = max height (scan from axisHi down), bottom = min height (scan from axisLo up)
					// negativeUp: top = min height (scan from axisLo up), bottom = max height (scan from axisHi down)
					int topVal = EMPTY;
					int botVal = EMPTY;
					glm::ivec3 pos;
					pos[uAxis] = coordU;
					pos[vAxis] = coordV;
					if (positiveUp) {
						for (int av = axisHi; av >= axisLo; --av) {
							pos[axisIdx] = av;
							if (solid.hasValue(pos.x, pos.y, pos.z)) {
								topVal = av;
								break;
							}
						}
						if (topVal != EMPTY) {
							for (int av = axisLo; av <= topVal; ++av) {
								pos[axisIdx] = av;
								if (solid.hasValue(pos.x, pos.y, pos.z)) {
									botVal = av;
									break;
								}
							}
						}
					} else {
						for (int av = axisLo; av <= axisHi; ++av) {
							pos[axisIdx] = av;
							if (solid.hasValue(pos.x, pos.y, pos.z)) {
								topVal = av;
								break;
							}
						}
						if (topVal != EMPTY) {
							for (int av = axisHi; av >= topVal; --av) {
								pos[axisIdx] = av;
								if (solid.hasValue(pos.x, pos.y, pos.z)) {
									botVal = av;
									break;
								}
							}
						}
					}
					colTopArr[flatIdx] = topVal;
					colBotArr[flatIdx] = botVal;
				}
			}
		});

		// Step 1.5 (fillHoles only): BFS from boundary to identify exterior-empty columns.
		// Interior-empty columns (holes) are those not reachable from outside.
		if (fillHoles) {
			for (int idx = 0; idx < numColumns; ++idx) {
				isExteriorEmpty[idx] = false;
			}
			bfsQueue.clear();
			for (int iu = 0; iu < extentU; ++iu) {
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					const bool onBoundary = (iu == 0 || iu == extentU - 1 || iv == 0 || iv == extentV - 1);
					if (onBoundary && colTopArr[flatIdx] == EMPTY) {
						isExteriorEmpty[flatIdx] = true;
						bfsQueue.push_back(flatIdx);
					}
				}
			}
			static constexpr int bfsDU[4] = {-1, 1, 0, 0};
			static constexpr int bfsDV[4] = {0, 0, -1, 1};
			for (int qi = 0; qi < (int)bfsQueue.size(); ++qi) {
				const int fi = bfsQueue[qi];
				const int biu = fi / extentV;
				const int biv = fi % extentV;
				for (int di = 0; di < 4; ++di) {
					const int nu = biu + bfsDU[di];
					const int nv = biv + bfsDV[di];
					if (nu < 0 || nu >= extentU || nv < 0 || nv >= extentV) {
						continue;
					}
					const int nIdx = nu * extentV + nv;
					if (isExteriorEmpty[nIdx] || colTopArr[nIdx] != EMPTY) {
						continue;
					}
					isExteriorEmpty[nIdx] = true;
					bfsQueue.push_back(nIdx);
				}
			}
		}

		// Step 2: Identify edge columns, precompute nearest-edge in 4 directions
		// via O(N) sweeps, then compute target heights for interior columns.
		for (int idx = 0; idx < numColumns; ++idx) {
			targetArr[idx] = EMPTY;
		}

		// Mark edge columns.
		// fillHoles=false: solid column adjacent to any empty or out-of-bounds UV neighbor.
		// fillHoles=true: solid column adjacent to exterior-empty or out-of-bounds only
		//   (hole-boundary columns are NOT edges -- they get interpolated heights too).
		for (int iu = 0; iu < extentU; ++iu) {
			for (int iv = 0; iv < extentV; ++iv) {
				const int flatIdx = iu * extentV + iv;
				if (colTopArr[flatIdx] == EMPTY) {
					isEdgeArr[flatIdx] = false;
					continue;
				}
				if (fillHoles) {
					bool enclosingEdge = false;
					static constexpr int edgeDU[4] = {-1, 1, 0, 0};
					static constexpr int edgeDV[4] = {0, 0, -1, 1};
					for (int di = 0; di < 4; ++di) {
						const int nu = iu + edgeDU[di];
						const int nv = iv + edgeDV[di];
						if (nu < 0 || nu >= extentU || nv < 0 || nv >= extentV) {
							enclosingEdge = true;
							break;
						}
						if (isExteriorEmpty[nu * extentV + nv]) {
							enclosingEdge = true;
							break;
						}
					}
					isEdgeArr[flatIdx] = enclosingEdge;
				} else {
					bool edge = (iu == 0 || iu == extentU - 1 || iv == 0 || iv == extentV - 1);
					if (!edge) {
						edge = (colTopArr[(iu - 1) * extentV + iv] == EMPTY)
							|| (colTopArr[(iu + 1) * extentV + iv] == EMPTY)
							|| (colTopArr[iu * extentV + (iv - 1)] == EMPTY)
							|| (colTopArr[iu * extentV + (iv + 1)] == EMPTY);
					}
					isEdgeArr[flatIdx] = edge;
				}
			}
		}

		// Sweep precomputation: for each column, find nearest edge in 4 directions.
		// Each sweep is O(extentU * extentV), total O(N) instead of O(N * sqrt(N)).

		// Left (-U) and Right (+U) sweeps: for each row iv, sweep along iu (parallel).
		// fillHoles=true: holes (interior empty) are transparent -- only exterior-empty resets.
		// fillHoles=false: any empty column resets (current behavior).
		app::for_parallel(0, extentV, [&](int startIv, int endIv) {
			for (int iv = startIv; iv < endIv; ++iv) {
				// Left sweep: iu = 0 to extentU-1
				int lastEdgeHeight = EMPTY;
				int lastEdgeDist = 0;
				for (int iu = 0; iu < extentU; ++iu) {
					const int flatIdx = iu * extentV + iv;
					const bool shouldReset = fillHoles ? isExteriorEmpty[flatIdx] : (colTopArr[flatIdx] == EMPTY);
					if (shouldReset) {
						lastEdgeHeight = EMPTY;
					} else if (isEdgeArr[flatIdx]) {
						lastEdgeHeight = colTopArr[flatIdx];
						lastEdgeDist = 0;
					}
					if (lastEdgeHeight != EMPTY) {
						++lastEdgeDist;
					}
					nearEdgeHeightLeft[flatIdx] = lastEdgeHeight;
					nearEdgeDistLeft[flatIdx] = lastEdgeDist;
				}
				// Right sweep: iu = extentU-1 to 0
				lastEdgeHeight = EMPTY;
				lastEdgeDist = 0;
				for (int iu = extentU - 1; iu >= 0; --iu) {
					const int flatIdx = iu * extentV + iv;
					const bool shouldReset = fillHoles ? isExteriorEmpty[flatIdx] : (colTopArr[flatIdx] == EMPTY);
					if (shouldReset) {
						lastEdgeHeight = EMPTY;
					} else if (isEdgeArr[flatIdx]) {
						lastEdgeHeight = colTopArr[flatIdx];
						lastEdgeDist = 0;
					}
					if (lastEdgeHeight != EMPTY) {
						++lastEdgeDist;
					}
					nearEdgeHeightRight[flatIdx] = lastEdgeHeight;
					nearEdgeDistRight[flatIdx] = lastEdgeDist;
				}
			}
		});
		// Top (-V) and Bottom (+V) sweeps: for each column iu, sweep along iv (parallel).
		app::for_parallel(0, extentU, [&](int startIu, int endIu) {
			for (int iu = startIu; iu < endIu; ++iu) {
				// Top sweep: iv = 0 to extentV-1
				int lastEdgeHeight = EMPTY;
				int lastEdgeDist = 0;
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					const bool shouldReset = fillHoles ? isExteriorEmpty[flatIdx] : (colTopArr[flatIdx] == EMPTY);
					if (shouldReset) {
						lastEdgeHeight = EMPTY;
					} else if (isEdgeArr[flatIdx]) {
						lastEdgeHeight = colTopArr[flatIdx];
						lastEdgeDist = 0;
					}
					if (lastEdgeHeight != EMPTY) {
						++lastEdgeDist;
					}
					nearEdgeHeightTop[flatIdx] = lastEdgeHeight;
					nearEdgeDistTop[flatIdx] = lastEdgeDist;
				}
				// Bottom sweep: iv = extentV-1 to 0
				lastEdgeHeight = EMPTY;
				lastEdgeDist = 0;
				for (int iv = extentV - 1; iv >= 0; --iv) {
					const int flatIdx = iu * extentV + iv;
					const bool shouldReset = fillHoles ? isExteriorEmpty[flatIdx] : (colTopArr[flatIdx] == EMPTY);
					if (shouldReset) {
						lastEdgeHeight = EMPTY;
					} else if (isEdgeArr[flatIdx]) {
						lastEdgeHeight = colTopArr[flatIdx];
						lastEdgeDist = 0;
					}
					if (lastEdgeHeight != EMPTY) {
						++lastEdgeDist;
					}
					nearEdgeHeightBottom[flatIdx] = lastEdgeHeight;
					nearEdgeDistBottom[flatIdx] = lastEdgeDist;
				}
			}
		});

		// Compute target for each interior column using precomputed nearest edges (parallel).
		// fillHoles=true: also compute targets for interior-hole columns (EMPTY, not exterior-empty).
		// fillHoles=false: only process solid interior columns.
		app::for_parallel(0, extentU, [&](int startIu, int endIu) {
			for (int iu = startIu; iu < endIu; ++iu) {
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					if (fillHoles) {
						if (isExteriorEmpty[flatIdx] || isEdgeArr[flatIdx]) {
							continue;
						}
					} else {
						if (colTopArr[flatIdx] == EMPTY || isEdgeArr[flatIdx]) {
							continue;
						}
					}

					const int edgeLeft = nearEdgeHeightLeft[flatIdx];
					const int distLeft = nearEdgeDistLeft[flatIdx];
					const int edgeRight = nearEdgeHeightRight[flatIdx];
					const int distRight = nearEdgeDistRight[flatIdx];
					const int edgeTop = nearEdgeHeightTop[flatIdx];
					const int distTop = nearEdgeDistTop[flatIdx];
					const int edgeBottom = nearEdgeHeightBottom[flatIdx];
					const int distBottom = nearEdgeDistBottom[flatIdx];

					// Count valid edges
					int edgeCount = 0;
					int edgeHeights[4];
					int edgeDists[4];
					if (edgeLeft != EMPTY) {
						edgeHeights[edgeCount] = edgeLeft;
						edgeDists[edgeCount] = distLeft;
						++edgeCount;
					}
					if (edgeRight != EMPTY) {
						edgeHeights[edgeCount] = edgeRight;
						edgeDists[edgeCount] = distRight;
						++edgeCount;
					}
					if (edgeTop != EMPTY) {
						edgeHeights[edgeCount] = edgeTop;
						edgeDists[edgeCount] = distTop;
						++edgeCount;
					}
					if (edgeBottom != EMPTY) {
						edgeHeights[edgeCount] = edgeBottom;
						edgeDists[edgeCount] = distBottom;
						++edgeCount;
					}

					if (edgeCount < 2) {
						continue;
					}

					int target;
					if (interp == SmoothWallInterp::Linear && edgeLeft != EMPTY && edgeRight != EMPTY
						&& edgeTop != EMPTY && edgeBottom != EMPTY) {
						// Bilinear: lerp along each axis, then average
						const int totalU = distLeft + distRight;
						const int totalV = distTop + distBottom;
						const int64_t interpU = ((int64_t)edgeLeft * distRight + (int64_t)edgeRight * distLeft
												  + totalU / 2) / totalU;
						const int64_t interpV = ((int64_t)edgeTop * distBottom + (int64_t)edgeBottom * distTop
												  + totalV / 2) / totalV;
						target = (int)((interpU + interpV + 1) / 2);
					} else if (interp == SmoothWallInterp::EdgeAware && edgeCount >= 2) {
						// Edge-aware IDW: weight = 1 / (dist * (1 + gradientAlongAxis)).
						// Flat-wall axis pairs (small gradient) get stronger influence.
						const float gradientU = (edgeLeft != EMPTY && edgeRight != EMPTY)
							? (float)glm::abs(edgeLeft - edgeRight) : 0.0f;
						const float gradientV = (edgeTop != EMPTY && edgeBottom != EMPTY)
							? (float)glm::abs(edgeTop - edgeBottom) : 0.0f;
						float totalWeight = 0.0f;
						float weightedHeight = 0.0f;
						if (edgeLeft != EMPTY) {
							const float w = 1.0f / ((float)distLeft * (1.0f + gradientU));
							totalWeight += w;
							weightedHeight += (float)edgeLeft * w;
						}
						if (edgeRight != EMPTY) {
							const float w = 1.0f / ((float)distRight * (1.0f + gradientU));
							totalWeight += w;
							weightedHeight += (float)edgeRight * w;
						}
						if (edgeTop != EMPTY) {
							const float w = 1.0f / ((float)distTop * (1.0f + gradientV));
							totalWeight += w;
							weightedHeight += (float)edgeTop * w;
						}
						if (edgeBottom != EMPTY) {
							const float w = 1.0f / ((float)distBottom * (1.0f + gradientV));
							totalWeight += w;
							weightedHeight += (float)edgeBottom * w;
						}
						target = (int)glm::round(weightedHeight / totalWeight);
					} else {
						// IDW fallback (also used for Linear when not all 4 edges found)
						float totalWeight = 0.0f;
						float weightedHeight = 0.0f;
						for (int ei = 0; ei < edgeCount; ++ei) {
							const float w = 1.0f / (float)edgeDists[ei];
							totalWeight += w;
							weightedHeight += (float)edgeHeights[ei] * w;
						}
						target = (int)glm::round(weightedHeight / totalWeight);
					}

					targetArr[flatIdx] = glm::clamp(target, axisLo, axisHi);
				}
			}
		});

		// Step 3: Enforce the smooth surface for each interior column (parallel collect, sequential write).
		struct VoxelOp {
			glm::ivec3 pos;
			voxel::Voxel color;
			bool add;
		};
		core::DynamicArray<core::DynamicArray<VoxelOp>> perRowOps(extentU);

		app::for_parallel(0, extentU, [&](int startIu, int endIu) {
			for (int iu = startIu; iu < endIu; ++iu) {
				core::DynamicArray<VoxelOp> &ops = perRowOps[iu];
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					const int target = targetArr[flatIdx];
					if (target == EMPTY) {
						continue;
					}
					const bool isInteriorHole = fillHoles && (colTopArr[flatIdx] == EMPTY) && !isExteriorEmpty[flatIdx];
					const int currentBottom = isInteriorHole ? (positiveUp ? axisLo : axisHi) : colBotArr[flatIdx];
					if (currentBottom == EMPTY) {
						continue;
					}
					const int coordU = baseU + iu;
					const int coordV = baseV + iv;
					glm::ivec3 pos;
					pos[uAxis] = coordU;
					pos[vAxis] = coordV;

					if (isInteriorHole) {
						pos[axisIdx] = target;
						ops.push_back({pos, fillVoxel, true});
					} else {
						const int colTop = colTopArr[flatIdx];
						const int fillLo = positiveUp ? colTop + 1 : target;
						const int fillHi = positiveUp ? target : colTop - 1;
						if (fillLo <= fillHi) {
							pos[axisIdx] = colTop;
							voxel::Voxel lastColor = fillVoxel;
							const voxel::Voxel &edgeVoxel = colorSource.voxel(pos);
							if (voxel::isBlocked(edgeVoxel.getMaterial())) {
								lastColor = edgeVoxel;
							}
							for (int av = fillLo; av <= fillHi; ++av) {
								pos[axisIdx] = av;
								ops.push_back({pos, lastColor, true});
							}
						}
					}

					if (removeAboveDepth > 0 && !isInteriorHole) {
						const int colTop = colTopArr[flatIdx];
						const int clearStart = positiveUp ? target + 1 : target - 1;
						const int clearEnd = positiveUp
							? glm::min(glm::min(target + removeAboveDepth, axisHi), colTop)
							: glm::max(glm::max(target - removeAboveDepth, axisLo), colTop);
						const int clearStep = positiveUp ? 1 : -1;
						for (int av = clearStart; positiveUp ? (av <= clearEnd) : (av >= clearEnd); av += clearStep) {
							pos[axisIdx] = av;
							ops.push_back({pos, voxel::Voxel(), false});
						}
					}
				}
			}
		});
		int totalOps = 0;
		for (int iu = 0; iu < extentU; ++iu) {
			totalOps += (int)perRowOps[iu].size();
		}

		// Reserve addedPositions to avoid catastrophic reallocation
		// (DynamicArray grows by 32 elements, not doubling — without reserve,
		// 676K pushes cause ~17K reallocations each copying the full array)
		addedPositions.reserve(addedPositions.size() + totalOps);

		for (int iu = 0; iu < extentU; ++iu) {
			for (const VoxelOp &op : perRowOps[iu]) {
				if (op.add) {
					solid.setVoxel(op.pos, true);
					addedPositions.push_back(op.pos);
					colorSource.setVoxel(op.pos, op.color);
				} else {
					solid.setVoxel(op.pos, false);
					colorSource.setVoxel(op.pos, voxel::Voxel());
				}
			}
		}

		// Step 4: Gap-fill pass (parallel collect, sequential write).
		// For each column, extend +/-3 toward neighbor heights to close seams.
		core::DynamicArray<core::DynamicArray<glm::ivec3>> s4PerRowAdds(extentU);

		app::for_parallel(0, extentU, [&](int startIu, int endIu) {
			for (int iu = startIu; iu < endIu; ++iu) {
				core::DynamicArray<glm::ivec3> &adds = s4PerRowAdds[iu];
				for (int iv = 0; iv < extentV; ++iv) {
					const int flatIdx = iu * extentV + iv;
					const int myTarget = targetArr[flatIdx];
					if (myTarget == EMPTY) {
						continue;
					}
					const bool isInteriorHole = fillHoles && (colTopArr[flatIdx] == EMPTY) && !isExteriorEmpty[flatIdx];
					const int currentBottom = isInteriorHole ? (positiveUp ? axisLo : axisHi) : colBotArr[flatIdx];
					if (currentBottom == EMPTY) {
						continue;
					}

					int neighborMin = myTarget;
					int neighborMax = myTarget;
					static constexpr int neighborOffsets[8][2] = {
						{-1, -1}, {-1, 0}, {-1, 1}, {0, -1}, {0, 1}, {1, -1}, {1, 0}, {1, 1}
					};
					for (const auto &off : neighborOffsets) {
						const int nu = iu + off[0];
						const int nv = iv + off[1];
						if (nu < 0 || nu >= extentU || nv < 0 || nv >= extentV) {
							continue;
						}
						const int nIdx = nu * extentV + nv;
						const int nHeight = (targetArr[nIdx] != EMPTY) ? targetArr[nIdx] : colTopArr[nIdx];
						if (nHeight == EMPTY) {
							continue;
						}
						neighborMin = glm::min(neighborMin, nHeight);
						neighborMax = glm::max(neighborMax, nHeight);
					}

					const int fillLo = glm::max(neighborMin, myTarget - 3);
					const int fillHi = glm::min(neighborMax, myTarget + 3);
					const int coordU = baseU + iu;
					const int coordV = baseV + iv;
					glm::ivec3 pos;
					pos[uAxis] = coordU;
					pos[vAxis] = coordV;
					for (int av = fillLo; av <= fillHi; ++av) {
						pos[axisIdx] = av;
						if (!solid.hasValue(pos.x, pos.y, pos.z)) {
							adds.push_back(pos);
						}
					}
				}
			}
		});

		// Count and reserve before writing
		int s4Total = 0;
		for (int iu = 0; iu < extentU; ++iu) {
			s4Total += (int)s4PerRowAdds[iu].size();
		}
		addedPositions.reserve(addedPositions.size() + s4Total);
		for (int iu = 0; iu < extentU; ++iu) {
			for (const glm::ivec3 &pos : s4PerRowAdds[iu]) {
				fillSmoothWallVoxel(solid, colorSource, pos, fillVoxel, addedPositions);
			}
		}
	}
}

void sculptSmoothWall(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::BitVolume &anchors,
					  voxel::FaceNames face, int iterations, const voxel::Voxel &fillVoxel,
					  int removeAboveDepth, SmoothWallInterp interp, bool fillHoles,
					  core::DynamicArray<glm::ivec3> &addedPositions) {
	sculptSmoothWallImpl(solid, voxelMap, anchors, face, iterations, fillVoxel, removeAboveDepth, interp, fillHoles, addedPositions);
}

void sculptSmoothWall(voxel::BitVolume &solid, voxel::RawVolume &colorVolume, const voxel::BitVolume &anchors,
					  voxel::FaceNames face, int iterations, const voxel::Voxel &fillVoxel,
					  int removeAboveDepth, SmoothWallInterp interp, bool fillHoles,
					  core::DynamicArray<glm::ivec3> &addedPositions) {
	sculptSmoothWallImpl(solid, colorVolume, anchors, face, iterations, fillVoxel, removeAboveDepth, interp, fillHoles, addedPositions);
}


void sculptSquashToPlane(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, voxel::FaceNames face,
						 int planeCoord) {
	if (face == voxel::FaceNames::Max) {
		return;
	}

	core_trace_scoped(SculptSquashToPlane);

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const int perp1 = (axisIdx + 1) % 3;
	const int perp2 = (axisIdx + 2) % 3;

	const voxel::Region &region = solid.region();
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();

	// For each (perp1, perp2) column, find if any solid voxel exists and pick
	// the color from the voxel nearest to the plane coordinate.
	for (int a1 = lo[perp1]; a1 <= hi[perp1]; ++a1) {
		for (int a2 = lo[perp2]; a2 <= hi[perp2]; ++a2) {
			bool hasAnySolid = false;
			int bestDist = INT_MAX;
			voxel::Voxel bestVoxel;

			// Scan the column to find the nearest solid voxel to the plane
			for (int av = lo[axisIdx]; av <= hi[axisIdx]; ++av) {
				glm::ivec3 pos;
				pos[axisIdx] = av;
				pos[perp1] = a1;
				pos[perp2] = a2;
				if (!solid.hasValue(pos.x, pos.y, pos.z)) {
					continue;
				}
				hasAnySolid = true;
				const int dist = glm::abs(av - planeCoord);
				if (dist < bestDist && voxelMap.hasVoxel(pos)) {
					bestDist = dist;
					bestVoxel = voxelMap.voxel(pos);
				}
			}

			if (!hasAnySolid) {
				continue;
			}

			// Clear all voxels in the column
			for (int av = lo[axisIdx]; av <= hi[axisIdx]; ++av) {
				glm::ivec3 pos;
				pos[axisIdx] = av;
				pos[perp1] = a1;
				pos[perp2] = a2;
				if (solid.hasValue(pos.x, pos.y, pos.z)) {
					solid.setVoxel(pos, false);
					voxelMap.setVoxel(pos, voxel::Voxel());
				}
			}

			// Place the single voxel at the plane coordinate
			glm::ivec3 planePos;
			planePos[axisIdx] = planeCoord;
			planePos[perp1] = a1;
			planePos[perp2] = a2;
			if (region.containsPoint(planePos)) {
				bestVoxel.setFlags(voxel::FlagOutline);
				solid.setVoxel(planePos, true);
				voxelMap.setVoxel(planePos, bestVoxel);
			}
		}
	}
}

void sculptReskin(voxel::BitVolume &solid, voxel::SparseVolume &voxelMap, const voxel::RawVolume &skin,
				  voxel::FaceNames face, const ReskinConfig &config,
				  const palette::Palette *skinPalette, palette::Palette *targetPalette) {
	if (face == voxel::FaceNames::Max) {
		return;
	}

	core_trace_scoped(SculptReskin);

	const voxel::Region &skinRegion = skin.region();
	if (!skinRegion.isValid()) {
		return;
	}

	// Build color remap table: skin palette index -> target palette index.
	// Only import colors that are actually used by skin voxels into the target palette.
	static constexpr int PaletteSize = palette::PaletteMaxColors;
	bool hasRemap = false;
	uint8_t colorRemap[PaletteSize];
	if (skinPalette != nullptr && targetPalette != nullptr) {
		hasRemap = true;
		for (int i = 0; i < PaletteSize; ++i) {
			colorRemap[i] = 0;
		}
		// Scan skin volume to find which color indices are actually used
		bool usedColors[PaletteSize];
		for (int i = 0; i < PaletteSize; ++i) {
			usedColors[i] = false;
		}
		const glm::ivec3 &sLo = skinRegion.getLowerCorner();
		const glm::ivec3 &sHi = skinRegion.getUpperCorner();
		for (int z = sLo.z; z <= sHi.z; ++z) {
			for (int y = sLo.y; y <= sHi.y; ++y) {
				for (int x = sLo.x; x <= sHi.x; ++x) {
					const voxel::Voxel &v = skin.voxel(x, y, z);
					if (voxel::isBlocked(v.getMaterial())) {
						usedColors[v.getColor()] = true;
					}
				}
			}
		}
		// Only add used colors to the target palette
		for (int i = 0; i < PaletteSize; ++i) {
			if (!usedColors[i]) {
				continue;
			}
			const color::RGBA skinColor = skinPalette->color(i);
			uint8_t targetIdx = 0;
			if (!targetPalette->tryAdd(skinColor, true, &targetIdx, false)) {
				// Color already exists or is very similar - tryAdd sets targetIdx
			}
			colorRemap[i] = targetIdx;
		}
	}

	// Skin axes: skinFace encodes both the depth axis and reading direction.
	// Skin layer 0 (skinLo on the up axis) is the base placed at the surface; deeper
	// layers grow outward. The outward world direction is carried by outwardStep.
	const int skinUpIdx = math::getIndexForAxis(voxel::faceToAxis(config.skinFace));
	const int skinUIdx = (skinUpIdx + 1) % 3;
	const int skinVIdx = (skinUpIdx + 2) % 3;

	const glm::ivec3 &skinLo = skinRegion.getLowerCorner();
	const glm::ivec3 &skinHi = skinRegion.getUpperCorner();
	const int skinUExtent = skinHi[skinUIdx] - skinLo[skinUIdx] + 1;
	const int skinVExtent = skinHi[skinVIdx] - skinLo[skinVIdx] + 1;
	const int skinDExtent = skinHi[skinUpIdx] - skinLo[skinUpIdx] + 1;

	const int axisIdx = math::getIndexForAxis(voxel::faceToAxis(face));
	const int perp1 = (axisIdx + 1) % 3; // U axis
	const int perp2 = (axisIdx + 2) % 3; // V axis
	const bool positiveUp = voxel::isPositiveFace(face);
	const int inwardStep = positiveUp ? -1 : 1;

	const voxel::Region &selRegion = solid.region();
	const glm::ivec3 &lo = selRegion.getLowerCorner();
	const glm::ivec3 &hi = selRegion.getUpperCorner();

	const int uMin = lo[perp1];
	const int uMax = hi[perp1];
	const int vMin = lo[perp2];
	const int vMax = hi[perp2];
	const int selW = uMax - uMin + 1;
	const int selH = vMax - vMin + 1;

	// Effective tile dimensions based on rotation
	int tileW;
	int tileH;
	if (config.rotation == ReskinRotation::R0 || config.rotation == ReskinRotation::R180) {
		tileW = skinUExtent;
		tileH = skinVExtent;
	} else {
		tileW = skinVExtent;
		tileH = skinUExtent;
	}

	// In preview mode, limit the applied area to 2x2 tiles
	int effectiveSelW = selW;
	int effectiveSelH = selH;
	if (config.preview) {
		static constexpr int PreviewTiles = 2;
		effectiveSelW = glm::min(selW, tileW * PreviewTiles);
		effectiveSelH = glm::min(selH, tileH * PreviewTiles);
	}

	// Build surface height map per (u,v) column
	static constexpr int EMPTY = INT_MIN;
	const int numCols = selW * selH;
	core::DynamicArray<int> surfaceHeight(numCols);
	for (int i = 0; i < numCols; ++i) {
		surfaceHeight[i] = EMPTY;
	}

	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!solid.hasValue(x, y, z)) {
					continue;
				}
				const glm::ivec3 pos(x, y, z);
				const int uCoord = pos[perp1];
				const int vCoord = pos[perp2];
				const int h = pos[axisIdx];
				const int idx = (uCoord - uMin) * selH + (vCoord - vMin);
				if (surfaceHeight[idx] == EMPTY) {
					surfaceHeight[idx] = h;
				} else if (positiveUp) {
					surfaceHeight[idx] = glm::max(surfaceHeight[idx], h);
				} else {
					surfaceHeight[idx] = glm::min(surfaceHeight[idx], h);
				}
			}
		}
	}

	// Compute reference height for None mode (flat plane at max/min). Median mode samples
	// the per-tile footprint locally in the column loop below so each tile sits at its
	// own local surface median rather than one global plane.
	int referenceHeight = 0;
	if (config.follow == ReskinFollow::None) {
		bool anyValid = false;
		for (int i = 0; i < numCols; ++i) {
			if (surfaceHeight[i] == EMPTY) {
				continue;
			}
			if (!anyValid) {
				referenceHeight = surfaceHeight[i];
				anyValid = true;
			} else if (positiveUp) {
				referenceHeight = glm::max(referenceHeight, surfaceHeight[i]);
			} else {
				referenceHeight = glm::min(referenceHeight, surfaceHeight[i]);
			}
		}
		if (!anyValid) {
			return;
		}
	}

	// Corner median: sample the surface depth map in a tile-sized neighborhood centered
	// on (uCenter, vCenter) and return its median. Used at tile corners - adjacent tiles
	// share the same (clamped) corner position and thus the same median, so tile edges
	// meet at the same height. A tile-sized window gives many samples for robust smoothing,
	// far more than cornerAvgAt's 5x5 neighborhood. Returns false when every sample in the
	// neighborhood is empty so the caller can substitute a per-column fallback.
	core::DynamicArray<int> medianScratch;
	auto cornerMedianAt = [&](int uCenter, int vCenter, float &out) -> bool {
		medianScratch.clear();
		const int halfU = tileW / 2;
		const int halfV = tileH / 2;
		medianScratch.reserve((2 * halfU + 1) * (2 * halfV + 1));
		for (int du = -halfU; du <= halfU; ++du) {
			const int u = uCenter + du;
			if (u < 0 || u >= selW) {
				continue;
			}
			for (int dv = -halfV; dv <= halfV; ++dv) {
				const int v = vCenter + dv;
				if (v < 0 || v >= selH) {
					continue;
				}
				const int idx = u * selH + v;
				if (surfaceHeight[idx] != EMPTY) {
					medianScratch.push_back(surfaceHeight[idx]);
				}
			}
		}
		if (medianScratch.empty()) {
			return false;
		}
		core::sort(medianScratch.begin(), medianScratch.end(), [](int a, int b) { return a < b; });
		out = (float)medianScratch[medianScratch.size() / 2];
		return true;
	};

	// Cache corner medians so the 4 corners of N tiles are each computed once.
	// Key encodes the clamped (uCenter, vCenter) into 64 bits so that two tiles whose
	// unclamped corners sit at different out-of-bounds positions but clamp to the same
	// edge share a cache entry. Values can be negative. Only sample-derived medians are
	// cached; if the neighborhood is empty the per-column fallback is applied at the
	// caller so we don't leak one column's surface height to another.
	using CornerMedianMap = core::DynamicMap<int64_t, float, 257, std::hash<int64_t>>;
	CornerMedianMap cornerMedianCache;
	auto cornerKey = [](int uCenter, int vCenter) -> int64_t {
		return ((int64_t)(uint32_t)uCenter << 32) | (int64_t)(uint32_t)vCenter;
	};
	auto cornerMedianCached = [&](int uCenter, int vCenter, int fallbackHeight) -> float {
		const int clampedU = glm::clamp(uCenter, 0, selW - 1);
		const int clampedV = glm::clamp(vCenter, 0, selH - 1);
		const int64_t key = cornerKey(clampedU, clampedV);
		auto iter = cornerMedianCache.find(key);
		if (iter != cornerMedianCache.end()) {
			return iter->value;
		}
		float m;
		if (!cornerMedianAt(clampedU, clampedV, m)) {
			return (float)fallbackHeight;
		}
		cornerMedianCache.put(key, m);
		return m;
	};

	// Helper to compute average surface height in a small neighborhood around a selection position.
	// Corners at tile boundaries (including the tile at the far edge) can sit outside the selection.
	// Individual out-of-bounds samples are skipped rather than clamped so a single tall edge outlier
	// doesn't get over-weighted. The center itself is clamped into the selection so a corner that
	// sits fully outside still anchors to the edge the skin actually lands on; if that neighborhood
	// is still empty (sparse selection), fall back to the caller-supplied per-column surface height.
	static constexpr int CornerSampleRadius = 2;
	auto cornerAvgAt = [&](int uCenter, int vCenter, int fallbackHeight) -> float {
		uCenter = glm::clamp(uCenter, 0, selW - 1);
		vCenter = glm::clamp(vCenter, 0, selH - 1);
		int sum = 0;
		int count = 0;
		for (int du = -CornerSampleRadius; du <= CornerSampleRadius; ++du) {
			const int u = uCenter + du;
			if (u < 0 || u >= selW) {
				continue;
			}
			for (int dv = -CornerSampleRadius; dv <= CornerSampleRadius; ++dv) {
				const int v = vCenter + dv;
				if (v < 0 || v >= selH) {
					continue;
				}
				const int idx = u * selH + v;
				if (surfaceHeight[idx] != EMPTY) {
					sum += surfaceHeight[idx];
					++count;
				}
			}
		}
		if (count == 0) {
			return (float)fallbackHeight;
		}
		return (float)sum / (float)count;
	};

	// Process each (u,v) column
	const int maxDepth = glm::min(config.skinDepth, skinDExtent);
	const voxel::Voxel air;

	// Anchor re-orients the local tile coordinate so that skin(0,0) lands at the chosen
	// corner of the selection and the tile reads inward from there. distU/distV are the
	// distance from the anchored corner, based on the true selection size (selW/selH) so
	// preview and final output agree on where the skin lands.
	const bool anchorRight =
		(config.anchor == ReskinAnchor::TopRight || config.anchor == ReskinAnchor::BottomRight);
	const bool anchorBottom =
		(config.anchor == ReskinAnchor::BottomLeft || config.anchor == ReskinAnchor::BottomRight);
	// Shift the preview window toward the anchored corner so the user sees the part of
	// the selection where the skin actually lands, not a blank area far from the anchor.
	const int uStart = anchorRight ? (selW - effectiveSelW) : 0;
	const int vStart = anchorBottom ? (selH - effectiveSelH) : 0;
	const int uEnd = uStart + effectiveSelW;
	const int vEnd = vStart + effectiveSelH;

	for (int uIdx = uStart; uIdx < uEnd; ++uIdx) {
		for (int vIdx = vStart; vIdx < vEnd; ++vIdx) {
			const int colIdx = uIdx * selH + vIdx;
			if (surfaceHeight[colIdx] == EMPTY) {
				continue;
			}

			const int uCoord = uMin + uIdx;
			const int vCoord = vMin + vIdx;
			const int surface = surfaceHeight[colIdx];

			// Start height for skin application, with z offset
			const int outwardStep = -inwardStep;

			const int distU = anchorRight ? (selW - 1 - uIdx) : uIdx;
			const int distV = anchorBottom ? (selH - 1 - vIdx) : vIdx;
			int localU = distU + config.offsetU;
			int localV = distV + config.offsetV;

			// Apply offset (not for Stretch mode)
			if (config.tile == ReskinTile::Stretch) {
				localU = uIdx;
				localV = vIdx;
			}

			// Apply tiling to get position in effective skin space [0, tileW) x [0, tileH)
			int tU = 0;
			int tV = 0;
			bool skipColumn = false;
			switch (config.tile) {
			case ReskinTile::Once:
				if (localU < 0 || localU >= tileW || localV < 0 || localV >= tileH) {
					skipColumn = true;
				}
				tU = localU;
				tV = localV;
				break;
			case ReskinTile::Repeat: {
				// Check max repeat limits
				if (tileW > 0 && config.maxRepeatU > 0) {
					const int tileIdxU = localU >= 0 ? localU / tileW : -1;
					if (tileIdxU >= config.maxRepeatU || tileIdxU < 0) {
						skipColumn = true;
					}
				}
				if (tileH > 0 && config.maxRepeatV > 0) {
					const int tileIdxV = localV >= 0 ? localV / tileH : -1;
					if (tileIdxV >= config.maxRepeatV || tileIdxV < 0) {
						skipColumn = true;
					}
				}
				tU = ((localU % tileW) + tileW) % tileW;
				tV = ((localV % tileH) + tileH) % tileH;
				break;
			}
			case ReskinTile::Stretch:
				if (selW > 1) {
					tU = localU * (tileW - 1) / (selW - 1);
				}
				if (selH > 1) {
					tV = localV * (tileH - 1) / (selH - 1);
				}
				tU = glm::clamp(tU, 0, tileW - 1);
				tV = glm::clamp(tV, 0, tileH - 1);
				break;
			default:
				break;
			}

			if (skipColumn) {
				continue;
			}

			// Tile corner positions in uIdx/vIdx space. uNear/vNear are the anchor-side
			// edges (where fu=0 / fv=0), uFar/vFar the opposite edges (shared with the next
			// tile inward). Adjacent tiles share these corner positions so heights match.
			const int uNear = anchorRight ? (uIdx + tU) : (uIdx - tU);
			const int uFar = anchorRight ? (uNear - tileW) : (uNear + tileW);
			const int vNear = anchorBottom ? (vIdx + tV) : (vIdx - tV);
			const int vFar = anchorBottom ? (vNear - tileH) : (vNear + tileH);
			const float fu = (tileW > 1) ? (float)tU / (float)(tileW - 1) : 0.5f;
			const float fv = (tileH > 1) ? (float)tV / (float)(tileH - 1) : 0.5f;

			// Compute start height based on follow mode
			int startHeight;
			if (config.follow == ReskinFollow::Voxel) {
				startHeight = surface + config.zOffset * outwardStep;
			} else if (config.follow == ReskinFollow::CornerAverage) {
				// Per-tile bilinear interpolation: sample surface height at this tile's 4 corners
				// Adjacent tiles share corners, so they connect seamlessly. Pass the column's own
				// surface as the fallback so an all-empty corner neighborhood resolves to a sane
				// height instead of absolute 0.
				const float h00 = cornerAvgAt(uNear, vNear, surface);
				const float h10 = cornerAvgAt(uFar, vNear, surface);
				const float h01 = cornerAvgAt(uNear, vFar, surface);
				const float h11 = cornerAvgAt(uFar, vFar, surface);
				const float interpH = h00 * (1.0f - fu) * (1.0f - fv) +
									  h10 * fu * (1.0f - fv) +
									  h01 * (1.0f - fu) * fv +
									  h11 * fu * fv;
				const int planeH = (int)glm::round(interpH);
				// Clamp so texture never starts inside the wall
				startHeight = (positiveUp ? glm::max(planeH, surface) : glm::min(planeH, surface))
							  + config.zOffset * outwardStep;
			} else if (config.follow == ReskinFollow::Median) {
				// Hybrid: sample a tile-sized neighborhood median at each of the 4 tile
				// corners, then bilinearly interpolate between them. Adjacent tiles share
				// corners, so heights line up at tile edges; the wide median sampling
				// smooths over uneven surfaces that would confuse a 4-voxel corner average.
				// Column surface is the fallback when a corner neighborhood is empty.
				const float h00 = cornerMedianCached(uNear, vNear, surface);
				const float h10 = cornerMedianCached(uFar, vNear, surface);
				const float h01 = cornerMedianCached(uNear, vFar, surface);
				const float h11 = cornerMedianCached(uFar, vFar, surface);
				const float interpH = h00 * (1.0f - fu) * (1.0f - fv) +
									  h10 * fu * (1.0f - fv) +
									  h01 * (1.0f - fu) * fv +
									  h11 * fu * fv;
				const int planeH = (int)glm::round(interpH);
				startHeight = (positiveUp ? glm::max(planeH, surface) : glm::min(planeH, surface))
							  + config.zOffset * outwardStep;
			} else {
				startHeight = referenceHeight + config.zOffset * outwardStep;
			}

			// Apply rotation: tiled coords -> skin tiling plane offsets
			int skinSU;
			int skinSV;
			switch (config.rotation) {
			case ReskinRotation::R0:
				skinSU = tU;
				skinSV = tV;
				break;
			case ReskinRotation::R90:
				skinSU = skinUExtent - 1 - tV;
				skinSV = tU;
				break;
			case ReskinRotation::R180:
				skinSU = skinUExtent - 1 - tU;
				skinSV = skinVExtent - 1 - tV;
				break;
			case ReskinRotation::R270:
				skinSV = skinVExtent - 1 - tU;
				skinSU = tV;
				break;
			default:
				skinSU = tU;
				skinSV = tV;
				break;
			}

			// Apply skin layers. Base at startHeight, layers grow outward from surface.
			for (int d = 0; d < maxDepth; ++d) {
				glm::ivec3 worldPos;
				worldPos[perp1] = uCoord;
				worldPos[perp2] = vCoord;
				worldPos[axisIdx] = startHeight + d * outwardStep;

				glm::ivec3 skinPos;
				skinPos[skinUIdx] = skinLo[skinUIdx] + skinSU;
				skinPos[skinVIdx] = skinLo[skinVIdx] + skinSV;
				// Depth d is the distance outward from the surface; the world direction is
				// already carried by outwardStep. Sample the skin from its base (skinLo) and
				// grow outward by layer, so skin layer 0 lands on the surface for both
				// positive and negative faces.
				skinPos[skinUpIdx] = skinLo[skinUpIdx] + d;
				const voxel::Voxel rawSkinVoxel = skin.voxel(skinPos.x, skinPos.y, skinPos.z);
				const bool skinIsSolid = voxel::isBlocked(rawSkinVoxel.getMaterial());
				// invertSkin flips skin presence: a solid skin cell is treated as absent and
				// vice versa. The skin color only exists where the skin voxel is really solid,
				// so an inverted "present" cell (skinIsSolid == false) carries no color and
				// preserves the existing voxel instead of stamping one.
				const bool effectiveSolid = config.invertSkin ? !skinIsSolid : skinIsSolid;

				// Remap skin color to target palette if available
				const voxel::Voxel skinVoxel = (hasRemap && skinIsSolid)
					? voxel::createVoxel(rawSkinVoxel.getMaterial(), colorRemap[rawSkinVoxel.getColor()],
										 rawSkinVoxel.getNormal(), rawSkinVoxel.getFlags(),
										 rawSkinVoxel.getBoneIdx())
					: rawSkinVoxel;

				switch (config.mode) {
				case ReskinMode::Replace:
					if (effectiveSolid) {
						if (skinIsSolid) {
							solid.setVoxel(worldPos, true);
							voxel::Voxel v = skinVoxel;
							v.setFlags(voxel::FlagOutline);
							voxelMap.setVoxel(worldPos, v);
						}
						// inverted present cell without a skin color: keep the existing voxel
					} else {
						solid.setVoxel(worldPos, false);
						voxelMap.setVoxel(worldPos, air);
					}
					break;
				case ReskinMode::Blend:
					if (effectiveSolid && skinIsSolid) {
						voxel::Voxel v = skinVoxel;
						v.setFlags(voxel::FlagOutline);
						solid.setVoxel(worldPos, true);
						voxelMap.setVoxel(worldPos, v);
					}
					break;
				case ReskinMode::Negate:
					if (effectiveSolid) {
						solid.setVoxel(worldPos, false);
						voxelMap.setVoxel(worldPos, air);
					}
					break;
				default:
					break;
				}
			}
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

	// Add or update voxels
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				if (!solid.hasValue(x, y, z)) {
					continue;
				}
				if (!voxelMap.hasVoxel(x, y, z)) {
					continue;
				}
				const voxel::Voxel &current = volume.voxel(x, y, z);
				const voxel::Voxel &mapped = voxelMap.voxel(x, y, z);
				if (voxel::isBlocked(current.getMaterial()) && current == mapped) {
					continue;
				}
				volume.setVoxel(x, y, z, mapped);
				changed++;
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

int sculptBridgeGap(voxel::RawVolume &volume, const voxel::Region &region,
					const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptBridgeGapVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptBridgeGap(solid, voxelMap, anchors, fillVoxel);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptSmoothGaussian(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
						 int kernelSize, float sigma, int iterations, const voxel::Voxel &fillVoxel) {
	core_trace_scoped(SculptSmoothGaussianVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptSmoothGaussian(solid, voxelMap, anchors, face, kernelSize, sigma, iterations, fillVoxel);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptSmoothWall(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
					 int iterations, const voxel::Voxel &fillVoxel, int removeAboveDepth,
					 SmoothWallInterp interp, bool fillHoles) {
	core_trace_scoped(SculptSmoothWallVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	core::DynamicArray<glm::ivec3> addedPositions;
	sculptSmoothWall(solid, voxelMap, anchors, face, iterations, fillVoxel, removeAboveDepth, interp, fillHoles, addedPositions);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptSquashToPlane(voxel::RawVolume &volume, const voxel::Region &region, voxel::FaceNames face,
					   int planeCoord) {
	core_trace_scoped(SculptSquashToPlaneVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptSquashToPlane(solid, voxelMap, face, planeCoord);
	return writeResultToVolume(volume, region, solid, voxelMap);
}

int sculptReskin(voxel::RawVolume &volume, const voxel::Region &region, const voxel::RawVolume &skin,
				 voxel::FaceNames face, const ReskinConfig &config) {
	core_trace_scoped(SculptReskinVolume);
	voxel::BitVolume solid(region);
	voxel::SparseVolume voxelMap;
	voxel::Region anchorRegion = region;
	anchorRegion.grow(1);
	anchorRegion.cropTo(volume.region());
	voxel::BitVolume anchors(anchorRegion);
	buildFromVolume(volume, region, solid, voxelMap, anchors);
	sculptReskin(solid, voxelMap, skin, face, config);

	// Custom write-back that also handles color changes (not just add/remove)
	int changed = 0;
	const voxel::Voxel air;
	const glm::ivec3 &lo = region.getLowerCorner();
	const glm::ivec3 &hi = region.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				const voxel::Voxel &current = volume.voxel(x, y, z);
				const bool wasSolid = voxel::isBlocked(current.getMaterial());
				const bool nowSolid = solid.hasValue(x, y, z);
				if (wasSolid && !nowSolid) {
					volume.setVoxel(x, y, z, air);
					changed++;
				} else if (nowSolid && voxelMap.hasVoxel(x, y, z)) {
					const voxel::Voxel &newVoxel = voxelMap.voxel(x, y, z);
					if (!wasSolid || current.getColor() != newVoxel.getColor()) {
						volume.setVoxel(x, y, z, newVoxel);
						changed++;
					}
				}
			}
		}
	}
	return changed;
}

} // namespace voxelutil
