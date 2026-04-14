/**
 * @file
 */

#include "VolumeSculpt.h"
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

	// Skin axes: skinDepthAxis is the outward/depth direction, the other two are tiling axes
	const int skinUpIdx = math::getIndexForAxis(config.skinDepthAxis);
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

	// Compute reference height for None/Median modes
	int referenceHeight = 0;
	if (config.follow == ReskinFollow::None || config.follow == ReskinFollow::Median) {
		core::DynamicArray<int> heights;
		heights.reserve(numCols);
		for (int i = 0; i < numCols; ++i) {
			if (surfaceHeight[i] != EMPTY) {
				heights.push_back(surfaceHeight[i]);
			}
		}
		if (heights.empty()) {
			return;
		}

		if (config.follow == ReskinFollow::None) {
			referenceHeight = heights[0];
			for (size_t i = 1; i < heights.size(); ++i) {
				if (positiveUp) {
					referenceHeight = glm::max(referenceHeight, heights[i]);
				} else {
					referenceHeight = glm::min(referenceHeight, heights[i]);
				}
			}
		} else {
			core::sort(heights.begin(), heights.end(), [](int a, int b) { return a < b; });
			referenceHeight = heights[heights.size() / 2];
		}
	}

	// Helper to compute average surface height in a small neighborhood around a selection position
	static constexpr int CornerSampleRadius = 2;
	auto cornerAvgAt = [&](int uCenter, int vCenter) -> float {
		int sum = 0;
		int count = 0;
		for (int du = -CornerSampleRadius; du <= CornerSampleRadius; ++du) {
			for (int dv = -CornerSampleRadius; dv <= CornerSampleRadius; ++dv) {
				const int u = glm::clamp(uCenter + du, 0, selW - 1);
				const int v = glm::clamp(vCenter + dv, 0, selH - 1);
				const int idx = u * selH + v;
				if (surfaceHeight[idx] != EMPTY) {
					sum += surfaceHeight[idx];
					++count;
				}
			}
		}
		if (count == 0) {
			return 0.0f;
		}
		return (float)sum / (float)count;
	};

	// Process each (u,v) column
	const int maxDepth = glm::min(config.skinDepth, skinDExtent);
	const voxel::Voxel air;

	for (int uIdx = 0; uIdx < effectiveSelW; ++uIdx) {
		for (int vIdx = 0; vIdx < effectiveSelH; ++vIdx) {
			const int colIdx = uIdx * selH + vIdx;
			if (surfaceHeight[colIdx] == EMPTY) {
				continue;
			}

			const int uCoord = uMin + uIdx;
			const int vCoord = vMin + vIdx;
			const int surface = surfaceHeight[colIdx];

			// Start height for skin application, with z offset
			const int outwardStep = -inwardStep;

			// Map (u,v) to skin coordinates (always MinMin anchor)
			int localU = uIdx + config.offsetU;
			int localV = vIdx + config.offsetV;

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

			// Compute start height based on follow mode
			int startHeight;
			if (config.follow == ReskinFollow::Voxel) {
				startHeight = surface + config.zOffset * outwardStep;
			} else if (config.follow == ReskinFollow::CornerAverage) {
				// Per-tile bilinear interpolation: sample surface height at this tile's 4 corners
				// Adjacent tiles share corners, so they connect seamlessly
				const int tileOriginU = uIdx - tU;
				const int tileOriginV = vIdx - tV;
				const float h00 = cornerAvgAt(tileOriginU, tileOriginV);
				const float h10 = cornerAvgAt(tileOriginU + tileW, tileOriginV);
				const float h01 = cornerAvgAt(tileOriginU, tileOriginV + tileH);
				const float h11 = cornerAvgAt(tileOriginU + tileW, tileOriginV + tileH);
				const float fu = (tileW > 1) ? (float)tU / (float)(tileW - 1) : 0.5f;
				const float fv = (tileH > 1) ? (float)tV / (float)(tileH - 1) : 0.5f;
				const float interpH = h00 * (1.0f - fu) * (1.0f - fv) +
									  h10 * fu * (1.0f - fv) +
									  h01 * (1.0f - fu) * fv +
									  h11 * fu * fv;
				const int planeH = (int)glm::round(interpH);
				// Clamp so texture never starts inside the wall
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
				skinPos[skinUpIdx] = skinLo[skinUpIdx] + d;
				const voxel::Voxel rawSkinVoxel = skin.voxel(skinPos.x, skinPos.y, skinPos.z);
				const bool skinIsSolid = voxel::isBlocked(rawSkinVoxel.getMaterial());
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
						voxel::Voxel v = skinVoxel;
						v.setFlags(voxel::FlagOutline);
						solid.setVoxel(worldPos, true);
						voxelMap.setVoxel(worldPos, v);
					} else {
						solid.setVoxel(worldPos, false);
						voxelMap.setVoxel(worldPos, air);
					}
					break;
				case ReskinMode::Blend:
					if (effectiveSolid) {
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
