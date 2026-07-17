/**
 * @file
 */

#include "FlatBase.h"

#include "app/Async.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicMap.h"
#include "memento/MementoHandler.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <climits>
#include <glm/matrix.hpp>

namespace voxedit {
namespace printing {

namespace {

// Per-(node, local x, local z) lowest solid voxel sample, recorded only when its
// world-space Y falls inside the base slab.
struct ColumnSample {
	int nodeId;
	int lx;
	int lz;
	int ly;					// local Y of the lowest solid voxel in that column
	int worldY;				// world-space Y of that voxel (after node transform)
	int translationY;		// the node's world-space Y translation (cached for inverse)
	voxel::Voxel voxel;		// the voxel itself (material + colour reused for new placements)
};

// True when the node's world matrix is a pure (integer) translation. Rotated/scaled
// nodes break "voxel-perfect" so we skip them and warn.
static bool isPureTranslation(const glm::mat4 &m, glm::ivec3 &outTranslation) {
	const float eps = 1.0e-3f;
	for (int col = 0; col < 3; ++col) {
		for (int row = 0; row < 3; ++row) {
			const float expected = (col == row) ? 1.0f : 0.0f;
			if (glm::abs(m[col][row] - expected) > eps) {
				return false;
			}
		}
	}
	outTranslation = glm::ivec3(
		(int)glm::round(m[3][0]),
		(int)glm::round(m[3][1]),
		(int)glm::round(m[3][2]));
	return true;
}

// Resize a node's volume downward so its region.lower.y reaches newLowerY. Existing
// voxels are copied into the same local positions in the new volume. No-op if the
// region already extends low enough.
static void extendVolumeDownTo(scenegraph::SceneGraphNode &node, int newLowerY) {
	voxel::RawVolume *rv = node.volume();
	const voxel::Region &oldRegion = rv->region();
	if (newLowerY >= oldRegion.getLowerY()) {
		return;
	}
	glm::ivec3 newLower = oldRegion.getLowerCorner();
	newLower.y = newLowerY;
	const voxel::Region newRegion(newLower, oldRegion.getUpperCorner());
	voxel::RawVolume *newRv = new voxel::RawVolume(newRegion);
	newRv->copyInto(*rv);
	node.setVolume(newRv);
}

} // namespace

void runFlatBase(SceneManager *sceneMgr, int slabThickness, int trimPercent, int debugColor) {
	if (slabThickness < 0) {
		slabThickness = 5;
	}
	if (trimPercent < 0) {
		trimPercent = 0;
	}
	if (trimPercent > 90) {
		// Above 90 the trim consumes nearly the whole distribution. Cap it.
		trimPercent = 90;
	}

	// FlatBase rewrites column tops/bottoms across many nodes. Treat as a non-reversible
	// pipeline step like the other 3dprint commands -- suppress per-volume undo snapshots
	// (the 3dprint flow is meant to be applied as a final pre-export pass).
	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	const scenegraph::FrameIndex frameIdx = sceneMgr->currentFrame();

	struct NodeSnapshot {
		int nodeId;
		glm::ivec3 translation;	// node's world translation (integer)
		int regionMinWorldY;	// world-space Y of region().lower
	};

	core::DynamicArray<NodeSnapshot> nodeSnaps;
	nodeSnaps.reserve(64);

	int skippedRotated = 0;
	int globalRegionMinY = INT_MAX;

	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		const voxel::RawVolume *rv = node.volume();
		if (rv == nullptr) {
			continue;
		}
		const glm::mat4 worldMat = graph.worldMatrix(node, frameIdx);
		glm::ivec3 translation;
		if (!isPureTranslation(worldMat, translation)) {
			Log::warn("3dprint flatbase: skipping node %d (%s) -- non-translation transform",
					  node.id(), node.name().c_str());
			++skippedRotated;
			continue;
		}
		NodeSnapshot ns;
		ns.nodeId = node.id();
		ns.translation = translation;
		ns.regionMinWorldY = rv->region().getLowerY() + translation.y;
		nodeSnaps.push_back(ns);
		globalRegionMinY = core_min(globalRegionMinY, ns.regionMinWorldY);
	}

	if (nodeSnaps.empty()) {
		Log::info("3dprint flatbase: no eligible model nodes (skipped rotated=%d)", skippedRotated);
		return;
	}

	// Bottom-node candidates: any node whose region's lowest Y in world space is within
	// slabThickness of the global region minimum. region().lower.y is a valid lower bound
	// on the actual lowest voxel in the volume, so this filter never drops a real bottom
	// node, only tall towers that obviously can't reach the floor.
	const int regionThreshold = globalRegionMinY + slabThickness;
	core::DynamicArray<NodeSnapshot> bottomNodes;
	bottomNodes.reserve(nodeSnaps.size());
	for (const NodeSnapshot &ns : nodeSnaps) {
		if (ns.regionMinWorldY <= regionThreshold) {
			bottomNodes.push_back(ns);
		}
	}

	Log::info("3dprint flatbase: globalRegionMinY=%d, slabThickness=%d, candidate bottom nodes=%d/%d",
			  globalRegionMinY, slabThickness, (int)bottomNodes.size(), (int)nodeSnaps.size());

	// Pass 1: scan candidate volumes column-by-column to find each column's lowest solid
	// voxel. Two perf-critical bounds:
	//   (a) Per-node Y range is clamped to the conservative slab [globalRegionMinY,
	//       globalRegionMinY + slabThickness] mapped to local Y. globalRegionMinY is
	//       the worst-case lower bound on the actual global min, so a column whose
	//       lowest solid voxel sits above this clamped range cannot belong to the
	//       slab regardless. With slabThickness=5 this means scanning at most 6 Y
	//       rows per column instead of the full node height -- 100x or more speedup
	//       on tall nodes.
	//   (b) Per-node sample collection is parallelized across cores. Each worker writes
	//       only to its own slot in perNodeSamples, no shared mutable state, no locks.
	const uint64_t scanStartMs = core::TimeProvider::systemMillis();
	const int worstSlabTop = globalRegionMinY + slabThickness;
	core::DynamicArray<core::DynamicArray<ColumnSample>> perNodeSamples;
	perNodeSamples.resize(bottomNodes.size());
	app::for_parallel(0, (int)bottomNodes.size(), [&](int start, int end) {
		for (int i = start; i < end; ++i) {
			const NodeSnapshot &ns = bottomNodes[i];
			scenegraph::SceneGraphNode &node = graph.node(ns.nodeId);
			voxel::RawVolume *rv = node.volume();
			if (rv == nullptr) {
				continue;
			}
			const voxel::Region &region = rv->region();
			const int lxLo = region.getLowerX();
			const int lxHi = region.getUpperX();
			const int lzLo = region.getLowerZ();
			const int lzHi = region.getUpperZ();
			const int yLo = core_max(region.getLowerY(), globalRegionMinY - ns.translation.y);
			const int yHi = core_min(region.getUpperY(), worstSlabTop - ns.translation.y);
			if (yLo > yHi) {
				continue;
			}
			core::DynamicArray<ColumnSample> &out = perNodeSamples[i];
			// Heuristic: most surface columns inside the slab have at least one voxel.
			// Reserve a quarter of the (lx,lz) cross section to amortize push_back.
			const size_t expected = (size_t)(lxHi - lxLo + 1) * (size_t)(lzHi - lzLo + 1) / 4u + 16u;
			out.reserve(expected);
			for (int lz = lzLo; lz <= lzHi; ++lz) {
				for (int lx = lxLo; lx <= lxHi; ++lx) {
					int lyMin = INT_MAX;
					for (int ly = yLo; ly <= yHi; ++ly) {
						const voxel::Voxel &v = rv->voxel(lx, ly, lz);
						if (!voxel::isAir(v.getMaterial())) {
							lyMin = ly;
							break;
						}
					}
					if (lyMin == INT_MAX) {
						continue;
					}
					ColumnSample s;
					s.nodeId = ns.nodeId;
					s.lx = lx;
					s.lz = lz;
					s.ly = lyMin;
					s.worldY = lyMin + ns.translation.y;
					s.translationY = ns.translation.y;
					s.voxel = rv->voxel(lx, lyMin, lz);
					out.push_back(s);
				}
			}
		}
	});

	// Merge per-node samples into a single buffer. Single-threaded; counted-then-reserved
	// to avoid reallocation churn during merge.
	size_t totalSamples = 0;
	for (const auto &v : perNodeSamples) {
		totalSamples += v.size();
	}
	core::DynamicArray<ColumnSample> samples;
	samples.reserve(totalSamples);
	int actualGlobalMinY = INT_MAX;
	for (auto &v : perNodeSamples) {
		for (const ColumnSample &s : v) {
			samples.push_back(s);
			actualGlobalMinY = core_min(actualGlobalMinY, s.worldY);
		}
	}
	const uint64_t scanElapsedMs = core::TimeProvider::systemMillis() - scanStartMs;
	Log::info("3dprint flatbase: scan done in %.2fs, collected %d column samples",
			  (double)scanElapsedMs / 1000.0, (int)samples.size());

	if (samples.empty()) {
		Log::info("3dprint flatbase: no solid voxels found in candidate bottom nodes");
		return;
	}

	// Filter samples to those that fall within the slab [actualGlobalMinY, +slabThickness].
	// Columns whose lowest voxel sits above the slab are not "on the floor" -- skip them.
	const int slabTop = actualGlobalMinY + slabThickness;
	core::DynamicArray<ColumnSample> kept;
	kept.reserve(samples.size());
	for (const ColumnSample &s : samples) {
		if (s.worldY <= slabTop) {
			kept.push_back(s);
		}
	}

	if (kept.empty()) {
		Log::info("3dprint flatbase: no samples fell inside slab [%d..%d]", actualGlobalMinY, slabTop);
		return;
	}

	// Trim outliers via histogram, NOT a comparison sort. The slab is at most
	// (slabThickness+1) distinct integer Y values wide -- on a real model that's 6
	// buckets. core::DynamicArray::sort() is an O(n^2) insertion sort (see
	// DynamicArray.h:519), which on multi-million sample sets melts into hours of
	// runtime. Histogram + cumulative walk gets the trim threshold in O(n + slab).
	const int slabBuckets = slabTop - actualGlobalMinY + 1;
	core::DynamicArray<int> histogram;
	histogram.resize(slabBuckets);
	for (int b = 0; b < slabBuckets; ++b) {
		histogram[b] = 0;
	}
	for (const ColumnSample &s : kept) {
		const int b = s.worldY - actualGlobalMinY;
		// Defensive: actualGlobalMinY was computed from the same samples, so all worldY
		// must land inside the slab. The bound check is a paranoia guard.
		if (b >= 0 && b < slabBuckets) {
			++histogram[b];
		}
	}
	const int trimCount = (int)(((int64_t)kept.size() * trimPercent) / 100);
	int cumulative = 0;
	int targetWorldY = actualGlobalMinY + slabBuckets - 1;
	for (int b = 0; b < slabBuckets; ++b) {
		cumulative += histogram[b];
		// The trimCount-th sample (0-indexed) sits in the first bucket whose cumulative
		// count exceeds trimCount.
		if (cumulative > trimCount) {
			targetWorldY = actualGlobalMinY + b;
			break;
		}
	}

	Log::info("3dprint flatbase: sampled %d columns, trimmed %d outliers, target Y=%d",
			  (int)kept.size(), trimCount, targetWorldY);

	// Pass 2: apply per kept column.
	// Group samples by node so we resize each affected node's volume at most once before
	// writing voxels. extendVolumeDownTo() replaces the volume, which would otherwise
	// invalidate any cached pointer mid-loop if done per-column.
	struct NodeChanges {
		int nodeId;
		int translationY;
		int minTargetLocalY;	// most negative local-Y any sample on this node will write to
		core::DynamicArray<const ColumnSample *> samples;
	};
	core::DynamicMap<int, NodeChanges, 257> byNode;
	for (const ColumnSample &s : kept) {
		const int targetLocalY = targetWorldY - s.translationY;
		auto iter = byNode.find(s.nodeId);
		if (iter == byNode.end()) {
			NodeChanges nc;
			nc.nodeId = s.nodeId;
			nc.translationY = s.translationY;
			nc.minTargetLocalY = targetLocalY;
			nc.samples.push_back(&s);
			byNode.put(s.nodeId, nc);
		} else {
			NodeChanges &nc = iter->value;
			nc.minTargetLocalY = core_min(nc.minTargetLocalY, targetLocalY);
			nc.samples.push_back(&s);
		}
	}

	int raisedColumns = 0;
	int loweredColumns = 0;
	int unchangedColumns = 0;
	int64_t voxelsCleared = 0;
	int64_t voxelsAdded = 0;

	const bool useDebugColor = (debugColor >= 0 && debugColor <= 255);

	for (auto it = byNode.begin(); it != byNode.end(); ++it) {
		NodeChanges &nc = it->value;
		scenegraph::SceneGraphNode &node = graph.node(nc.nodeId);

		// One-shot resize-down per node if any sample on it needs to lower the floor below
		// the existing region. Region.lower acts as the local coord clamp; without this,
		// setVoxel calls outside the region get silently dropped.
		extendVolumeDownTo(node, nc.minTargetLocalY);
		voxel::RawVolume *rv = node.volume();
		voxel::Region dirtyRgn = voxel::Region::InvalidRegion;

		for (const ColumnSample *sp : nc.samples) {
			const ColumnSample &s = *sp;
			const int targetLocalY = targetWorldY - s.translationY;
			if (targetLocalY == s.ly) {
				++unchangedColumns;
				continue;
			}
			voxel::Voxel placeVoxel = s.voxel;
			if (useDebugColor) {
				placeVoxel = voxel::createVoxel(
					s.voxel.getMaterial(), (uint8_t)debugColor,
					s.voxel.getNormal(), s.voxel.getFlags(), s.voxel.getBoneIdx());
			}
			if (targetLocalY > s.ly) {
				// Raise: clear everything in column [s.ly..targetLocalY-1], then place at target.
				for (int ly = s.ly; ly < targetLocalY; ++ly) {
					if (!voxel::isAir(rv->voxel(s.lx, ly, s.lz).getMaterial())) {
						rv->setVoxel(s.lx, ly, s.lz, voxel::Voxel());
						++voxelsCleared;
					}
				}
				rv->setVoxel(s.lx, targetLocalY, s.lz, placeVoxel);
				const glm::ivec3 lo(s.lx, s.ly, s.lz);
				const glm::ivec3 hi(s.lx, targetLocalY, s.lz);
				const voxel::Region colRgn(lo, hi);
				if (dirtyRgn.isValid()) {
					dirtyRgn.accumulate(colRgn);
				} else {
					dirtyRgn = colRgn;
				}
				++raisedColumns;
			} else {
				// Lower: fill column [targetLocalY..s.ly-1] solid (s.ly already set).
				for (int ly = targetLocalY; ly < s.ly; ++ly) {
					rv->setVoxel(s.lx, ly, s.lz, placeVoxel);
					++voxelsAdded;
				}
				const glm::ivec3 lo(s.lx, targetLocalY, s.lz);
				const glm::ivec3 hi(s.lx, s.ly, s.lz);
				const voxel::Region colRgn(lo, hi);
				if (dirtyRgn.isValid()) {
					dirtyRgn.accumulate(colRgn);
				} else {
					dirtyRgn = colRgn;
				}
				++loweredColumns;
			}
		}

		if (dirtyRgn.isValid()) {
			sceneMgr->modified(nc.nodeId, dirtyRgn);
		}
	}

	Log::info("3dprint flatbase: raised %d columns (cleared %lld voxels), lowered %d columns (added %lld voxels), unchanged %d, skipped rotated nodes %d",
			  raisedColumns, (long long)voxelsCleared,
			  loweredColumns, (long long)voxelsAdded,
			  unchangedColumns, skippedRotated);
}

} // namespace printing
} // namespace voxedit
