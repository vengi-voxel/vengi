/**
 * @file
 */

#include "FaceClassify.h"
#include "Progress.h"

#include "app/Async.h"
#include "color/RGBA.h"
#include "core/GLM.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/TimeProvider.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicSet.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/Connectivity.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <algorithm>
#include <atomic>
#include <cstdio>
#include <cstring>
#include <glm/matrix.hpp>
#if defined(__linux__)
#include <unistd.h>
#endif
#if defined(_MSC_VER)
#include <intrin.h>
#endif
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace voxedit {
namespace printing {

namespace {

// Process resident-set size in GB. Reads /proc/self/statm (Linux). Cheap (~1 us).
// Used to log ground-truth memory at every fillholes lifecycle point so we stop
// guessing when the user asks "is X GB ok?".
static double rssGB() {
#if defined(__linux__)
	FILE *f = std::fopen("/proc/self/statm", "r");
	if (!f) return 0.0;
	long sz = 0, resident = 0;
	if (std::fscanf(f, "%ld %ld", &sz, &resident) != 2) {
		std::fclose(f);
		return 0.0;
	}
	std::fclose(f);
	const long pageSize = sysconf(_SC_PAGESIZE);
	return (double)resident * (double)pageSize / (1024.0 * 1024.0 * 1024.0);
#else
	// RSS sampling reads /proc/self/statm and sysconf, which are Linux-only.
	// Other platforms skip the soft memory cap (0 never trips checkRSSCap).
	return 0.0;
#endif
}

// Hard cap on RSS at heartbeat sites. Set high enough that legitimate cs=2 work
// fits, low enough to abort on runaway frontier growth before OOM-killer hits.
// 32 GB is the agreed algorithm budget: cs=2 grid (~20 GB) plus chunked cs=1
// per-thread state (few MB per chunk) plus headroom.
static constexpr double kHolefillRSSCapGB = 32.0;
static std::atomic<bool> g_rssCapTripped{false};

// Master toggle for chatty progress / heartbeat / phase-timing logs across
// the 3dprint commands that share these primitives (fillholes, holemap,
// faceclassify, debugfrontier).
//
//   false (default): only errors, warnings, and the per-command result summary.
//   true           : full per-phase timings, BFS / init / frontier heartbeats,
//                    RSS samples, chunk progress on stderr.
//
// The verbose calls stay in source so flipping this constant + rebuilding
// restores the full debug stream when something needs investigating again.
// Declared here (before logRSS) so all gating sites see the constant.
static constexpr bool k3DPrintVerbose = true;

static bool checkRSSCap(const char *where) {
	const double rss = rssGB();
	if (rss > kHolefillRSSCapGB && !g_rssCapTripped.exchange(true)) {
		Log::error("3dprint holefill: RSS=%.1f GB exceeds cap %.1f GB at %s -- aborting",
				   rss, kHolefillRSSCapGB, where);
		return true;
	}
	return g_rssCapTripped.load(std::memory_order_relaxed);
}

static void logRSS(const char *label) {
	if (!k3DPrintVerbose) {
		return;
	}
	Log::info("3dprint holefill: [RSS=%.1f GB] %s", rssGB(), label);
}

// State values stored in the flat visited array
static constexpr uint8_t kStateEmpty    = 0; // unvisited empty (unknown)
static constexpr uint8_t kStateExterior = 1;
static constexpr uint8_t kStateInterior = 2;
static constexpr uint8_t kStateSolid    = 3; // solid placeholder so BFS skips it
static constexpr uint8_t kStateBlocked  = 4; // hole: exterior tried to enter coarse-interior, quarantined

// Chunked cs=1 pass parameters. Edit + rebuild to sweep.
//   ChunkVoxels  : default core size of a chunk in cs=1 voxels.
//   ChunkOverlap : padding on each side so leak paths crossing chunk core
//                  boundaries are still captured. Should be >= half the
//                  thickest expected wall (the chunk centers on an ext-front
//                  cell and must reach an int-front cell for a leak to be
//                  detectable inside it).
//   ChunkMaxVox  : when the default chunk doesn't capture an int-front cell,
//                  the chunk is expanded once to this size before giving up.
//                  Walls thicker than this are skipped (no leak detection).
static constexpr int kHolefillChunkVoxels  = 128;
static constexpr int kHolefillChunkOverlap = 16;
static constexpr int kHolefillChunkMaxVox  = 256;

// When true, runHoleFill skips applyHoleFills at the deepest cs=N level and
// lets the chunked cs=1 pass handle ALL hole filling at voxel precision.
//   - cs=N BFS still runs, still marks kStateBlocked on hole cells. The
//     cs=1 chunked pass inherits this classification and rediscovers the
//     same holes at voxel granularity.
//   - cs=N over-fills (8 voxels per cs=2 hole) are eliminated; cs=1 fills
//     are 1 voxel each.
//   - Toggle to false for self-test: hole/fill output then matches the
//     pre-chunked-pass cs=2-only behaviour.
static constexpr bool kRunChunkedCs1Pass = true;

struct NodeInfo {
	uint64_t nodeId;
	voxel::RawVolume *rv;
	glm::mat4 worldMat;
	glm::mat4 invWorldMat;
	glm::ivec3 cellOrigin; // snapped to coarse cell grid
	// Cached fill colour palette index. Set in a single-threaded pre-pass
	// before parallel plug detection so threads can read lock-free instead of
	// modifying the palette concurrently.
	uint8_t fillColorIdx = 0;
};

using CellHash = std::unordered_map<glm::ivec3, uint64_t, glm::hash<glm::ivec3>>;
// Multimap for cs=N cells whose voxels are owned by more than one node. The
// primary owner stays in CellHash::solid (one-to-one fast-path); secondary
// owners go in extraSolid. Most cells (regridded models with cs=128-aligned
// nodes) have no entries here -- cs=2 cells fit cleanly inside a single
// regridded box. Non-aligned nodes (orphan plug nodes from fillholes,
// manually-placed nodes, edge boxes that aren't cs=128 wide) can produce
// shared cs=2 cells, in which case extraSolid carries the additional owners.
using ExtraCellHash = std::unordered_multimap<glm::ivec3, uint64_t, glm::hash<glm::ivec3>>;
using CellSet  = std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>>;

// Flat open-addressing hash set for glm::ivec3 voxel positions. Replaces
// std::unordered_set in the sparse-BFS hot path. Why:
//   std::unordered_set is chained -- per-entry node allocations scattered
//   across hundreds of MB of heap. With 9.6M+ entries, every find() chases
//   pointers through L3 misses, costing ~500-1000ns per lookup. The sparse
//   BFS does ~30 lookups per voxel; with millions of voxels per round, this
//   dominates runtime.
//   Flat hash keeps everything in one contiguous power-of-two array with
//   linear probing. Probes hit the cache line of the bucket plus 1-2 more
//   on collision, instead of jumping to a heap node.
//
// Safety: not thread-safe. Used here in the read-only-during-parallel-scatter,
// modify-only-during-sequential-merge pattern, so no concurrent mutation.
// Sentinel: x == kEmptySentinel marks a never-used slot; x == kTombSentinel
// marks an erased slot (probes pass through, inserts can reuse). Real voxel
// positions never land at INT_MIN -- grid coords are bounded.
class FlatVoxelSet {
private:
	// Sentinel values for empty / tombstone slots. Compared against glm::ivec3::x
	// (int) so the constants stay int -- mixing int and uint64_t in == triggers
	// -Wsign-compare. Real voxel positions never land at INT_MIN.
	static constexpr int kEmptySentinel = INT_MIN;
	static constexpr int kTombSentinel  = INT_MIN + 1;

	glm::ivec3 *_data = nullptr;
	size_t _capacity = 0;
	size_t _size = 0;

	static uint64_t hashKey(const glm::ivec3 &v) {
		// Murmur-style avalanche on the 3 components folded into a 64-bit value.
		uint64_t h = (uint64_t)(uint32_t)v.x;
		h = (h ^ ((uint64_t)(uint32_t)v.y << 21)) * 0x9E3779B97F4A7C15ull;
		h = (h ^ ((uint64_t)(uint32_t)v.z << 11)) * 0x9E3779B97F4A7C15ull;
		h ^= h >> 33;
		return h;
	}

	bool insertNoGrow(const glm::ivec3 &v) {
		const size_t mask = _capacity - 1;
		size_t idx = (size_t)hashKey(v) & mask;
		size_t firstTomb = SIZE_MAX;
		while (true) {
			const glm::ivec3 &slot = _data[idx];
			if (slot.x == kEmptySentinel) {
				const size_t target = (firstTomb != SIZE_MAX) ? firstTomb : idx;
				_data[target] = v;
				++_size;
				return true;
			}
			if (slot.x == kTombSentinel) {
				if (firstTomb == SIZE_MAX) firstTomb = idx;
			} else if (slot == v) {
				return false;
			}
			idx = (idx + 1) & mask;
		}
	}

	void grow(size_t newCap) {
		glm::ivec3 *newData = new glm::ivec3[newCap];
		const glm::ivec3 emptyVal(kEmptySentinel, 0, 0);
		for (size_t i = 0; i < newCap; ++i) newData[i] = emptyVal;
		glm::ivec3 *oldData = _data;
		const size_t oldCap = _capacity;
		_data = newData;
		_capacity = newCap;
		_size = 0;
		for (size_t i = 0; i < oldCap; ++i) {
			if (oldData[i].x != kEmptySentinel && oldData[i].x != kTombSentinel) {
				insertNoGrow(oldData[i]);
			}
		}
		delete[] oldData;
	}

public:
	FlatVoxelSet() = default;
	~FlatVoxelSet() { delete[] _data; }
	FlatVoxelSet(const FlatVoxelSet &) = delete;
	FlatVoxelSet &operator=(const FlatVoxelSet &) = delete;
	FlatVoxelSet(FlatVoxelSet &&other) noexcept
		: _data(other._data), _capacity(other._capacity), _size(other._size) {
		other._data = nullptr;
		other._capacity = 0;
		other._size = 0;
	}

	void reserve(size_t expected) {
		size_t cap = 16;
		// Target ~50% max load factor to keep probe chains short.
		while (cap < expected * 2) cap *= 2;
		if (cap > _capacity) grow(cap);
	}

	// Returns true if newly inserted, false if already present.
	bool insert(const glm::ivec3 &v) {
		if ((_size + 1) * 2 > _capacity) {
			grow(_capacity ? _capacity * 2 : 16);
		}
		return insertNoGrow(v);
	}

	bool contains(const glm::ivec3 &v) const {
		if (_capacity == 0) return false;
		const size_t mask = _capacity - 1;
		size_t idx = (size_t)hashKey(v) & mask;
		while (true) {
			const glm::ivec3 &slot = _data[idx];
			if (slot.x == kEmptySentinel) return false;
			if (slot.x != kTombSentinel && slot == v) return true;
			idx = (idx + 1) & mask;
		}
	}

	bool erase(const glm::ivec3 &v) {
		if (_capacity == 0) return false;
		const size_t mask = _capacity - 1;
		size_t idx = (size_t)hashKey(v) & mask;
		while (true) {
			glm::ivec3 &slot = _data[idx];
			if (slot.x == kEmptySentinel) return false;
			if (slot.x != kTombSentinel && slot == v) {
				slot = glm::ivec3(kTombSentinel, 0, 0);
				--_size;
				return true;
			}
			idx = (idx + 1) & mask;
		}
	}

	size_t size() const { return _size; }
};

// One resolution level.
// Solid lookup: sparse CellHash (cells that contain >= 1 solid voxel).
// Exterior/interior: flat uint8_t state array indexed by grid position.
//   Size = dimX * dimY * dimZ bytes. For level-16 grid this is ~12 MB vs
//   ~450 MB for an unordered_set -- 37x smaller, fits in L3 cache.
struct Level {
	glm::ivec3 gridLower{0};
	// cellSize, dim* stay int: cellSize is small (2..128), dim* per-axis is small
	// (~2000 max at cs=2). Their PRODUCT uses size_t (state.size()). int is
	// required here because glm::ivec3 holds int and `glm::ivec3 * cellSize` in
	// toCell() needs a matching scalar type, and inBounds() needs signed
	// semantics for the >= 0 check.
	int cellSize = 0;
	int dimX = 0, dimY = 0, dimZ = 0;

	CellHash solid;             // sparse solid map: snapped cell -> primary node index
	// Secondary owners for shared cells (built when buildSolidHash sees a
	// collision). chunkInitFromParent and the chunk actions iterate primary +
	// extras so voxels in non-primary nodes still get classified instead of
	// silently disappearing into "saw=false" land.
	ExtraCellHash extraSolid;
	std::vector<uint8_t> state; // flat: kStateEmpty/Exterior/Interior per cell

	void initGrid(const glm::ivec3 &lo, const glm::ivec3 &hi) {
		// Snap the grid origin DOWN to a multiple of cellSize. toIdx()/inBounds()
		// compute the cell index as (cell - gridLower) / cellSize, which is only
		// correct when gridLower is itself a multiple of cellSize -- the cell
		// origins fed in come from toCellOrigin() and are absolute multiples of
		// cellSize. A fine level inherits the coarse grid bounds, and the coarse
		// cell size from the single-node auto-fallback can be odd (e.g. 81), so a
		// fine cellSize like 40 does NOT divide the incoming origin. Without this
		// snap the lattice is offset by (lo % cellSize) and every solid (wall)
		// cell lands in the wrong slot, so the flood walks straight through the
		// wall. The modulo form handles negative origins (model spans -x/-y/-z).
		gridLower.x = lo.x - (((lo.x % cellSize) + cellSize) % cellSize);
		gridLower.y = lo.y - (((lo.y % cellSize) + cellSize) % cellSize);
		gridLower.z = lo.z - (((lo.z % cellSize) + cellSize) % cellSize);
		dimX = (hi.x - gridLower.x) / cellSize + 1;
		dimY = (hi.y - gridLower.y) / cellSize + 1;
		dimZ = (hi.z - gridLower.z) / cellSize + 1;
		state.assign((size_t)dimX * (size_t)dimY * (size_t)dimZ, kStateEmpty);
	}

	bool inBounds(const glm::ivec3 &cell) const {
		const int ix = (cell.x - gridLower.x) / cellSize;
		const int iy = (cell.y - gridLower.y) / cellSize;
		const int iz = (cell.z - gridLower.z) / cellSize;
		return ix >= 0 && ix < dimX && iy >= 0 && iy < dimY && iz >= 0 && iz < dimZ;
	}

	size_t toIdx(const glm::ivec3 &cell) const {
		return (size_t)((cell.x - gridLower.x) / cellSize) * (size_t)dimY * (size_t)dimZ
			 + (size_t)((cell.y - gridLower.y) / cellSize) * (size_t)dimZ
			 + (size_t)((cell.z - gridLower.z) / cellSize);
	}

	glm::ivec3 toCell(size_t idx) const {
		const int iz = (int)(idx % (size_t)dimZ);
		const int iy = (int)((idx / (size_t)dimZ) % (size_t)dimY);
		const int ix = (int)(idx / ((size_t)dimY * (size_t)dimZ));
		return gridLower + glm::ivec3(ix, iy, iz) * cellSize;
	}

	uint8_t cellState(const glm::ivec3 &cell) const {
		if (!inBounds(cell)) return kStateExterior; // outside grid = exterior by definition
		return state[toIdx(cell)];
	}
};

static double elapsedSince(uint64_t startMs) {
	return (double)(core::TimeProvider::systemMillis() - startMs) / 1000.0;
}

static glm::ivec3 transformPoint(const glm::mat4 &m, const glm::ivec3 &p) {
	const glm::vec4 w = m * glm::vec4((float)p.x, (float)p.y, (float)p.z, 1.0f);
	// MUST be (int), not (uint64_t): world voxel coordinates can be negative
	// after applying a transform with translation. Casting through unsigned
	// would wrap negatives to huge positives.
	return glm::ivec3((int)glm::round(w.x), (int)glm::round(w.y), (int)glm::round(w.z));
}

// Signed floor division. For negative dividends this differs from C/C++
// truncation: floorDiv(-1, 4) = -1, while -1/4 = 0. The (a^b)<0 trick relies
// on signed semantics; uint64_t breaks this.
static int floorDiv(int a, int b) {
	return a / b - (a % b != 0 && (a ^ b) < 0);
}

static glm::ivec3 toCellOrigin(const glm::ivec3 &worldPos, int cellSize) {
	return glm::ivec3(floorDiv(worldPos.x, cellSize) * cellSize,
					  floorDiv(worldPos.y, cellSize) * cellSize,
					  floorDiv(worldPos.z, cellSize) * cellSize);
}

// Visit every node that has voxels in the cs=N cell at cs2Cell. Calls f once
// for the primary owner from lvl.solid, then once per extra owner from
// lvl.extraSolid. Most cells (regridded models with cs=128-aligned nodes)
// have only the primary -- the extraSolid lookup is a multimap equal_range
// which is essentially free for empty ranges. f receives the node index.
template<class Func>
static inline void forEachCellOwner(const Level &lvl, const glm::ivec3 &cs2Cell, Func &&f) {
	auto it = lvl.solid.find(cs2Cell);
	if (it == lvl.solid.end()) {
		return;
	}
	f(it->second);
	auto range = lvl.extraSolid.equal_range(cs2Cell);
	for (auto eit = range.first; eit != range.second; ++eit) {
		f(eit->second);
	}
}

// Build solid hash (parallel per-node, then sequential merge).
// Each node collects at most (nodeSize/fineSize)^3 unique cells (512 at level 16).
static void buildSolidHash(Level &lvl, const core::DynamicArray<NodeInfo> &nodes,
							ProgressTimer *timer = nullptr) {
	// numNodes stays int: app::for_parallel takes int range. Node coordinates
	// (x/y/z below) stay int because voxel::Region returns int and node-local
	// coords can be small or negative.
	const int numNodes = (int)nodes.size();
	core::DynamicArray<CellSet> perNodeCells;
	perNodeCells.resize((size_t)numNodes);

	std::atomic<int> processed{0};
	app::for_parallel(0, numNodes, [&](int start, int end) {
		for (int i = start; i < end; ++i) {
			const NodeInfo &ni = nodes[i];
			const voxel::Region &r = ni.rv->region();
			CellSet &cs = perNodeCells[i];
			size_t solidCount = 0;
			for (int z = r.getLowerZ(); z <= r.getUpperZ(); ++z)
				for (int y = r.getLowerY(); y <= r.getUpperY(); ++y)
					for (int x = r.getLowerX(); x <= r.getUpperX(); ++x)
						if (!voxel::isAir(ni.rv->voxel(x, y, z).getMaterial()))
							++solidCount;
			cs.reserve(solidCount);
			for (int z = r.getLowerZ(); z <= r.getUpperZ(); ++z) {
				for (int y = r.getLowerY(); y <= r.getUpperY(); ++y) {
					for (int x = r.getLowerX(); x <= r.getUpperX(); ++x) {
						if (!voxel::isAir(ni.rv->voxel(x, y, z).getMaterial())) {
							const glm::ivec3 w = transformPoint(ni.worldMat, glm::ivec3(x, y, z));
							cs.insert(toCellOrigin(w, lvl.cellSize));
						}
					}
				}
			}
			if (timer) {
				timer->addVoxels((int64_t)solidCount);
				timer->tick(++processed);
			}
		}
	});

	size_t totalCells = 0;
	for (int i = 0; i < numNodes; ++i) {
		totalCells += perNodeCells[i].size();
	}
	lvl.solid.reserve(totalCells);
	lvl.extraSolid.clear();
	uint64_t collisionCount = 0;
	for (int i = 0; i < numNodes; ++i) {
		for (const glm::ivec3 &cell : perNodeCells[i]) {
			auto inserted = lvl.solid.emplace(cell, (uint64_t)i);
			if (!inserted.second) {
				// Cell already has a primary owner (lower-index node). Track
				// this node as a secondary owner so chunkInitFromParent and
				// the chunk actions can iterate every node that has voxels in
				// the cell. Without this, voxels in higher-index nodes that
				// share cs=N cells with lower-index nodes silently drop out
				// of chunked classification (saw bit never set; faceclassify
				// renders them gray; fillholes can't see them either).
				lvl.extraSolid.emplace(cell, (uint64_t)i);
				++collisionCount;
			}
		}
	}
	if (collisionCount > 0 && k3DPrintVerbose) {
		Log::info("3dprint solidHash: %lu cs=%d cell collision(s) -- %lu shared cell(s) tracked in extraSolid",
				  (unsigned long)collisionCount, lvl.cellSize, (unsigned long)lvl.extraSolid.size());
	}
}

// Parallel frontier BFS.
// Safety: concurrent writes of the same value to a flat uint8_t array are safe on x86:
//   - All writes are idempotent (always write kStateExterior=1 or kStateInterior=2)
//   - Byte writes don't tear on x86
//   - Some cells may enter the frontier twice (harmless: second pass finds no new neighbours)
// Returns the count of cells marked with the given stateValue.
static uint64_t parallelBFS(Level &lvl, core::DynamicArray<int32_t> &frontier, uint8_t stateValue) {
	uint64_t totalMarked = 0;
	while (!frontier.empty()) {
		// sz stays int because for_parallel takes int range. Frontier is always
		// well under INT_MAX (bounded by surface area in cells). totalMarked is
		// the running sum across rounds and stays uint64_t.
		const int sz = (int)frontier.size();
		totalMarked += (uint64_t)sz;

		core::DynamicArray<core::DynamicArray<int32_t>> perRangeNext;
		const int maxSlots = 64;
		perRangeNext.resize((size_t)maxSlots);

		std::atomic<int> slotCounter{0};
		app::for_parallel(0, sz, [&](int start, int end) {
			const int slot = slotCounter.fetch_add(1, std::memory_order_relaxed) % maxSlots;
			core::DynamicArray<int32_t> &local = perRangeNext[slot];
			for (int i = start; i < end; ++i) {
				const glm::ivec3 cur = lvl.toCell((size_t)frontier[i]);
				for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
					const glm::ivec3 nb = cur + off * lvl.cellSize;
					if (!lvl.inBounds(nb)) continue;
					if (lvl.solid.count(nb) > 0) continue;
					const size_t nbIdx = lvl.toIdx(nb);
					if (lvl.state[nbIdx] != kStateEmpty) continue;
					lvl.state[nbIdx] = stateValue;
					local.push_back((int32_t)nbIdx);
				}
			}
		});

		frontier.clear();
		for (int s = 0; s < maxSlots; ++s) {
			for (int32_t idx : perRangeNext[s]) {
				frontier.push_back(idx);
			}
		}
	}
	return totalMarked;
}

// Exterior flood fill (6-connectivity from all 6 grid faces) using parallel BFS.
// Detects holes: if any exterior cell's coarse parent was interior -> returns hole cell size.
static uint64_t buildExterior(Level &lvl, const Level &prevLevel) {
	core::DynamicArray<int32_t> frontier;
	frontier.reserve((size_t)lvl.dimX * (size_t)lvl.dimY * 4); // rough seed count estimate

	// int64_t for loop counters: signed (gridLower can be negative), no overflow
	// at billions, same speed as int on x86-64 (often faster -- register-native).
	// Convert to int at glm::ivec3 construction since ivec3 components are int.
	auto seedCell = [&](int64_t cx, int64_t cy, int64_t cz) {
		const glm::ivec3 cell((int)cx, (int)cy, (int)cz);
		if (lvl.solid.count(cell) > 0) return;
		const size_t idx = lvl.toIdx(cell);
		if (lvl.state[idx] != kStateEmpty) return;
		lvl.state[idx] = kStateExterior;
		frontier.push_back((int32_t)idx);
	};

	const int64_t maxX = (int64_t)lvl.gridLower.x + (int64_t)(lvl.dimX - 1) * lvl.cellSize;
	const int64_t maxY = (int64_t)lvl.gridLower.y + (int64_t)(lvl.dimY - 1) * lvl.cellSize;
	const int64_t maxZ = (int64_t)lvl.gridLower.z + (int64_t)(lvl.dimZ - 1) * lvl.cellSize;

	for (int64_t cy = lvl.gridLower.y; cy <= maxY; cy += lvl.cellSize) {
		for (int64_t cz = lvl.gridLower.z; cz <= maxZ; cz += lvl.cellSize) {
			seedCell(lvl.gridLower.x, cy, cz);
			seedCell(maxX,            cy, cz);
		}
	}
	for (int64_t cx = (int64_t)lvl.gridLower.x + lvl.cellSize; cx < maxX; cx += lvl.cellSize) {
		for (int64_t cz = lvl.gridLower.z; cz <= maxZ; cz += lvl.cellSize) {
			seedCell(cx, lvl.gridLower.y, cz);
			seedCell(cx, maxY,            cz);
		}
	}
	for (int64_t cx = (int64_t)lvl.gridLower.x + lvl.cellSize; cx < maxX; cx += lvl.cellSize) {
		for (int64_t cy = (int64_t)lvl.gridLower.y + lvl.cellSize; cy < maxY; cy += lvl.cellSize) {
			seedCell(cx, cy, lvl.gridLower.z);
			seedCell(cx, cy, maxZ           );
		}
	}

	parallelBFS(lvl, frontier, kStateExterior);

	// Hole detection: scan for exterior cells whose coarse parent was interior.
	// Only do this if we have a previous level with interior.
	if (prevLevel.cellSize > 0) {
		const size_t total = lvl.state.size();
		for (size_t i = 0; i < total; ++i) {
			if (lvl.state[i] == kStateExterior) {
				const glm::ivec3 cell = lvl.toCell((uint64_t)i);
				if (prevLevel.cellState(toCellOrigin(cell, prevLevel.cellSize)) == kStateInterior) {
					return lvl.cellSize; // hole found at this scale
				}
			}
		}
	}
	return 0;
}

// Find the kStateEmpty cell nearest to the geometric center of the grid.
// Starting from center lands in the true model interior rather than a peripheral
// corridor or cavity that happens to be first in scan order.
static bool findInteriorSeedFromCenter(size_t &seedIdx, const Level &lvl) {
	const float cx = (float)lvl.dimX * 0.5f;
	const float cy = (float)lvl.dimY * 0.5f;
	const float cz = (float)lvl.dimZ * 0.5f;
	float bestDist = FLT_MAX;
	bool found = false;
	const size_t total = lvl.state.size();
	for (size_t i = 0; i < total; ++i) {
		if (lvl.state[i] != kStateEmpty) continue;
		const uint64_t ix = (uint64_t)(i / ((size_t)lvl.dimY * lvl.dimZ));
		const uint64_t iy = (uint64_t)((i / (size_t)lvl.dimZ) % (size_t)lvl.dimY);
		const uint64_t iz = (uint64_t)(i % (size_t)lvl.dimZ);
		const float dx = (float)ix - cx;
		const float dy = (float)iy - cy;
		const float dz = (float)iz - cz;
		const float dist = dx * dx + dy * dy + dz * dz;
		if (dist < bestDist) {
			bestDist = dist;
			seedIdx = i;
			found = true;
		}
	}
	return found;
}

// Interior flood fill (26-connectivity) from a single seed cell.
// Returns the number of cells marked interior.
static uint64_t buildInteriorAllSeeds(Level &lvl) {
	size_t seedIdx = 0;
	if (!findInteriorSeedFromCenter(seedIdx, lvl)) {
		return 0;
	}
	lvl.state[seedIdx] = kStateInterior;
	core::DynamicArray<int32_t> queue;
	queue.push_back((int32_t)seedIdx);
	uint64_t head = 0;
	while (head < (uint64_t)queue.size()) {
		const glm::ivec3 cur = lvl.toCell((size_t)queue[head++]);
		for (int dx = -1; dx <= 1; ++dx) {
			for (int dy = -1; dy <= 1; ++dy) {
				for (int dz = -1; dz <= 1; ++dz) {
					if (dx == 0 && dy == 0 && dz == 0) continue;
					const glm::ivec3 nb = cur + glm::ivec3(dx, dy, dz) * lvl.cellSize;
					if (!lvl.inBounds(nb)) continue;
					if (lvl.solid.count(nb) > 0) continue;
					const int32_t nbIdx = (int32_t)lvl.toIdx(nb);
					if (lvl.state[(size_t)nbIdx] != kStateEmpty) continue;
					lvl.state[(size_t)nbIdx] = kStateInterior;
					queue.push_back(nbIdx);
				}
			}
		}
	}
	return (uint64_t)queue.size();
}

// Solid voxel check using level's solid hash.
static bool isSolidAt(const glm::ivec3 &worldPos, const Level &lvl,
					  const core::DynamicArray<NodeInfo> &nodes) {
	const glm::ivec3 cell = toCellOrigin(worldPos, lvl.cellSize);
	auto it = lvl.solid.find(cell);
	if (it == lvl.solid.end()) return false;
	const NodeInfo &ni = nodes[it->second];
	const glm::ivec3 local = transformPoint(ni.invWorldMat, worldPos);
	return ni.rv->region().containsPoint(local) && !voxel::isAir(ni.rv->voxel(local).getMaterial());
}

static uint64_t countCellsByState(const Level &lvl, uint8_t s) {
	uint64_t n = 0;
	for (uint8_t v : lvl.state) {
		if (v == s) ++n;
	}
	return n;
}

// Seed fine cell states from the previous (coarser) level classification.
// Cells whose coarse parent is exterior/interior get that state.
// Cells in coarse-solid parents that are not in fine.solid stay kStateEmpty (the probe zone).
// Fine solid cells must already be marked kStateSolid by the caller.
static void initFineFromPrev(Level &fine, const Level &prev) {
	// Each slot writes only to its own index -- safe to parallelize.
	// prev.state reads are concurrent but read-only -- safe on all platforms.
	// Chunked by 1B cells because app::for_parallel takes int range; at cs=2 the
	// total exceeds INT_MAX. Per-chunk parallel scan with heartbeat; outer loop
	// sequences chunks. NO sequential remainder -- everything goes through the
	// parallel path.
	const size_t total = fine.state.size();
	const int64_t totalS = (int64_t)total;
	const uint64_t startMs = core::TimeProvider::systemMillis();
	if (k3DPrintVerbose) {
		Log::info("3dprint holefill: initFineFromPrev scanning %lld cells (cs=%d -> cs=%d)",
				  (long long)totalS, prev.cellSize, fine.cellSize);
	}
	std::atomic<int64_t> processed{0};
	std::atomic<uint64_t> lastLogMs{startMs};
	static constexpr int64_t kChunkSize = 1ll << 30; // 1B cells per chunk -- fits in int range
	for (int64_t base = 0; base < totalS; base += kChunkSize) {
		const int64_t chunkEnd = glm::min(base + kChunkSize, totalS);
		const int chunkLen = (int)(chunkEnd - base);
		app::for_parallel(0, chunkLen, [&](int start, int end) {
			int64_t since = 0;
			for (int64_t i = base + (int64_t)start; i < base + (int64_t)end; ++i) {
				if (fine.state[(size_t)i] == kStateSolid) { ++since; continue; }
				const glm::ivec3 cell = fine.toCell((size_t)i);
				const uint8_t parentState = prev.cellState(toCellOrigin(cell, prev.cellSize));
				if (parentState == kStateExterior) {
					fine.state[(size_t)i] = kStateExterior;
				} else if (parentState == kStateInterior) {
					fine.state[(size_t)i] = kStateInterior;
				}
				if ((++since & 0x3FFFFF) == 0) {
					const int64_t globalDone = processed.fetch_add(since, std::memory_order_relaxed) + since;
					since = 0;
					const uint64_t now = core::TimeProvider::systemMillis();
					uint64_t prevMs = lastLogMs.load(std::memory_order_relaxed);
					if (now - prevMs >= 3000u && lastLogMs.compare_exchange_strong(prevMs, now)) {
						if (k3DPrintVerbose) {
							fprintf(stderr, "[INIT] initFineFromPrev %lld/%lld (%.1f%%) elapsed=%.1fs [RSS=%.1f GB]\n",
									(long long)globalDone, (long long)totalS,
									100.0 * (double)globalDone / (double)totalS,
									(double)(now - startMs) / 1000.0,
									rssGB());

						}
						checkRSSCap("initFineFromPrev heartbeat");
					}
					if (g_rssCapTripped.load(std::memory_order_relaxed)) return;
				}
			}
			processed.fetch_add(since, std::memory_order_relaxed);
		});
		if (g_rssCapTripped.load(std::memory_order_relaxed)) return;
	}
}

// Scan the grid and build exterior and interior frontiers:
// cells that are exterior/interior AND have at least one neighbour that's a "wall".
// What counts as a wall depends on wrapSolid:
//   wrapSolid=false: only kStateEmpty (probe-zone) neighbours count -- used by
//      runHoleMap/runHoleFill where the BFS is between coarse-derived classification
//      and unclassified probe zones.
//   wrapSolid=true:  kStateEmpty OR kStateSolid neighbours count -- used by
//      single-level debugfrontier where the wrap should hug the actual fine solid
//      voxels too. Without this, an ext/int cell whose only "into the wall"
//      neighbour is a solid voxel is dropped, which leaves visible holes where a
//      thin solid skin runs along the boundary.
static void buildFrontiers(const Level &lvl,
							core::DynamicArray<size_t> &extFrontier,
							core::DynamicArray<size_t> &intFrontier,
							bool wrapSolid = false) {
	const size_t total = lvl.state.size();
	const int64_t totalS = (int64_t)total;
	const int nSlots = 64;
	core::DynamicArray<core::DynamicArray<size_t>> perExt, perInt;
	perExt.resize((size_t)nSlots);
	perInt.resize((size_t)nSlots);

	// Do NOT reserve based on grid size -- frontier is proportional to surface area, not volume.

	const uint64_t startMs = core::TimeProvider::systemMillis();
	if (k3DPrintVerbose) {
		Log::info("3dprint holefill: buildFrontiers scanning %lld cells (cs=%d, wrapSolid=%d)",
				  (long long)totalS, lvl.cellSize, wrapSolid ? 1 : 0);
	}
	std::atomic<int> slotCounter{0};
	std::atomic<int64_t> processed{0};
	std::atomic<uint64_t> lastLogMs{startMs};
	// Chunked by 1B cells because app::for_parallel takes int range; at cs=2 the
	// total exceeds INT_MAX. NO sequential remainder; previously cells beyond
	// INT_MAX were silently dropped (correctness bug at cs=2).
	static constexpr int64_t kChunkSize = 1ll << 30;
	for (int64_t base = 0; base < totalS; base += kChunkSize) {
		const int64_t chunkEnd = glm::min(base + kChunkSize, totalS);
		const int chunkLen = (int)(chunkEnd - base);
		app::for_parallel(0, chunkLen, [&](int start, int end) {
			const int slot = slotCounter.fetch_add(1, std::memory_order_relaxed) % nSlots;
			core::DynamicArray<size_t> &locExt = perExt[(size_t)slot];
			core::DynamicArray<size_t> &locInt = perInt[(size_t)slot];
			int64_t since = 0;
			for (int64_t i = base + (int64_t)start; i < base + (int64_t)end; ++i) {
				const uint8_t s = lvl.state[(size_t)i];
				if (s == kStateExterior || s == kStateInterior) {
					const glm::ivec3 cur = lvl.toCell((size_t)i);
					for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
						const glm::ivec3 nb = cur + off * lvl.cellSize;
						if (!lvl.inBounds(nb)) continue;
						const uint8_t nbState = lvl.state[lvl.toIdx(nb)];
						const bool isWall = nbState == kStateEmpty
							|| (wrapSolid && (nbState == kStateSolid || nbState == kStateBlocked));
						if (isWall) {
							if (s == kStateExterior) locExt.push_back((size_t)i);
							else locInt.push_back((size_t)i);
							break;
						}
					}
				}
				if ((++since & 0x3FFFFF) == 0) {
					const int64_t globalDone = processed.fetch_add(since, std::memory_order_relaxed) + since;
					since = 0;
					const uint64_t now = core::TimeProvider::systemMillis();
					uint64_t prevMs = lastLogMs.load(std::memory_order_relaxed);
					if (now - prevMs >= 3000u && lastLogMs.compare_exchange_strong(prevMs, now)) {
						if (k3DPrintVerbose) {
							fprintf(stderr, "[FRONT] buildFrontiers %lld/%lld (%.1f%%) elapsed=%.1fs [RSS=%.1f GB]\n",
									(long long)globalDone, (long long)totalS,
									100.0 * (double)globalDone / (double)totalS,
									(double)(now - startMs) / 1000.0,
									rssGB());

						}
						checkRSSCap("buildFrontiers heartbeat");
					}
					if (g_rssCapTripped.load(std::memory_order_relaxed)) return;
				}
			}
			processed.fetch_add(since, std::memory_order_relaxed);
		});
		if (g_rssCapTripped.load(std::memory_order_relaxed)) return;
	}

	size_t extTotal = 0, intTotal = 0;
	for (uint64_t s = 0; s < nSlots; ++s) {
		extTotal += perExt[(size_t)s].size();
		intTotal += perInt[(size_t)s].size();
	}
	extFrontier.reserve(extTotal);
	intFrontier.reserve(intTotal);
	for (uint64_t s = 0; s < nSlots; ++s) {
		for (size_t idx : perExt[(size_t)s]) extFrontier.push_back(idx);
		for (size_t idx : perInt[(size_t)s]) intFrontier.push_back(idx);
	}
	// Sort for determinism. The parallel slot fill above pushes indices in
	// whatever order app::for_parallel hands work to threads, so the slot
	// contents -- and therefore the concatenated frontier order -- vary
	// run-to-run. seedChunks consumes extFrontier in order: its first chunk
	// covers a neighborhood, later cells in that neighborhood get skipped
	// via the covered-set. Different frontier order -> different chunk
	// centers -> different cs=1 BFS coverage inside chunks -> different leak
	// voxels detected -> different fillholes plug counts (~1-2% variance
	// observed on the gothic model). Sorting here makes seedChunks
	// deterministic at O(N log N) for surface-area-sized N (negligible
	// compared to the BFS itself).
	// core::DynamicArray's iterator doesn't define iterator_traits, so std::sort
	// won't accept it; sort over the raw pointer range instead.
	std::sort(extFrontier.data(), extFrontier.data() + extFrontier.size());
	std::sort(intFrontier.data(), intFrontier.data() + intFrontier.size());
}


static core::DynamicArray<glm::ivec3> runBidirectionalBFS(Level &lvl) {
	core::DynamicArray<size_t> extFrontier, intFrontier;
	buildFrontiers(lvl, extFrontier, intFrontier);
	if (k3DPrintVerbose) {
		Log::info("holefill BFS: ext frontier=%zu int frontier=%zu", extFrontier.size(), intFrontier.size());
	}

	core::DynamicArray<glm::ivec3> holeCells;

	uint64_t bfsRound = 0;
	uint64_t bfsStartMs = core::TimeProvider::systemMillis();
	uint64_t lastLogMs = bfsStartMs;
	while (!extFrontier.empty() || !intFrontier.empty()) {
		++bfsRound;
		const uint64_t nowMs = core::TimeProvider::systemMillis();
		if (nowMs - lastLogMs >= 5000u) {
			lastLogMs = nowMs;
			if (k3DPrintVerbose) {
				fprintf(stderr, "[BFS] round %d, ext=%zu int=%zu holes=%zu elapsed=%.1fs [RSS=%.1f GB]\n",
						(int)bfsRound, extFrontier.size(), intFrontier.size(),
						holeCells.size(), (double)(nowMs - bfsStartMs) / 1000.0,
						rssGB());

			}
			if (checkRSSCap("BFS round heartbeat")) {
				return holeCells;
			}
		}
		core::DynamicArray<size_t> nextExt;
		// BFS expansion can produce up to 6 neighbours per frontier cell.
		// Reserve to that ceiling so push_back never reallocs mid-loop.
		nextExt.reserve(extFrontier.size() * 6);
		for (size_t idx : extFrontier) {
			const glm::ivec3 cur = lvl.toCell(idx);
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = cur + off * lvl.cellSize;
				if (!lvl.inBounds(nb)) continue;
				const size_t nbIdx = lvl.toIdx(nb);
				const uint8_t nbState = lvl.state[nbIdx];
				if (nbState == kStateSolid || nbState == kStateExterior || nbState == kStateBlocked) continue;
				if (nbState == kStateInterior) {
					// Ext-meets-int: this is a hole. Recording must be SYMMETRIC
					// with the int-meets-ext branch below. Without recording here,
					// any hole reached by ext FIRST gets silently absorbed (int
					// neighbour becomes kStateBlocked, BFS stops on both sides,
					// no hole emitted). That was the cause of multiple 4-voxel
					// holes being undetected at cs=2 even though the frontiers
					// physically met there.
					lvl.state[nbIdx] = kStateBlocked;
					holeCells.push_back(nb);
				} else {
					lvl.state[nbIdx] = kStateExterior;
					nextExt.push_back(nbIdx);
				}
			}
		}
		extFrontier = core::move(nextExt);

		core::DynamicArray<size_t> nextInt;
		nextInt.reserve(intFrontier.size() * 6);
		for (size_t idx : intFrontier) {
			const glm::ivec3 cur = lvl.toCell(idx);
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = cur + off * lvl.cellSize;
				if (!lvl.inBounds(nb)) continue;
				const size_t nbIdx = lvl.toIdx(nb);
				const uint8_t nbState = lvl.state[nbIdx];
				if (nbState == kStateSolid || nbState == kStateInterior || nbState == kStateBlocked) continue;
				if (nbState == kStateExterior) {
					lvl.state[nbIdx] = kStateBlocked;
					holeCells.push_back(nb);
				} else {
					lvl.state[nbIdx] = kStateInterior;
					nextInt.push_back(nbIdx);
				}
			}
		}
		intFrontier = core::move(nextInt);
	}

	return holeCells;
}

} // namespace (close anonymous early -- chunked-pass helpers live in
// printing:: scope alongside FillStats/applyHoleFills/runHoleFill so they
// can call applyHoleFills (which has internal linkage in printing::, not
// the anonymous namespace).

// FillStats + applyHoleFills are also used by runHoleMap and the chunked pass
// below. The struct is defined here (instead of just before applyHoleFills'
// body) so the chunked pass's runChunkedCs1Pass() can access stats fields. The
// function is forward-declared; the body lives further down with the rest of
// the fill-pipeline code.
struct FillStats {
	uint64_t totalFilled = 0;
	uint64_t orphanFilled = 0;
	int orphanNodes = 0;
	uint64_t nodesResized = 0;
	uint64_t nodesTouched = 0;
};
static FillStats applyHoleFills(
		const core::DynamicArray<glm::ivec3> &holeCells,
		const core::DynamicArray<glm::ivec3> &holeVoxels,
		const Level &lookup,
		core::DynamicArray<NodeInfo> &nodes,
		SceneManager *sceneMgr,
		int finalCellSize,
		const color::RGBA &fillColor = color::RGBA(0, 220, 0, 255),
		const char *orphanNodeName = "holefill_orphan");

// =====================================================================
// Wall-walking chunked cs=1 pass.
//
// After the global pipeline converges at the deepest cs that fits in the
// memory budget (typically cs=2), sub-cell leaks remain: 1-voxel slits and
// narrow gaps inside cs=2 cells classified `kStateSolid`. The cube plug
// pass that previously handled this produced visible splats because BFS
// crawled along walls before finding leaks, then plugged the entire crawl.
//
// This pass runs the same sequential ext-then-int BFS as the global path,
// but on small chunk-local grids at cs=1, seeded by walking the model's
// outer (ext) and inner (int) shells. Each chunk's bbox is positioned so
// it captures both shells; otherwise no leak inside it is detectable, so
// it's skipped entirely.
// =====================================================================

struct ChunkBox {
	glm::ivec3 lower; // world voxel, inclusive
	glm::ivec3 upper; // world voxel, inclusive
};

// Sequential variant of buildFrontiers, used inside the per-chunk parallel
// loop. The dense-grid buildFrontiers is itself parallel; nesting parallel
// regions inside a parallel chunk loop has unspecified behaviour with our
// thread pool, so the chunk path uses a sequential pass over its (small)
// state array.
//
// extFrontier/intFrontier collected here are state-array indices into chunk.
// Wraps only kStateEmpty neighbours: the chunk pass searches probe-zone
// cells inside cs=2-solid territory, the same condition the global pass
// uses with wrapSolid=false.
static void chunkBuildFrontiers(const Level &chunk,
                                 core::DynamicArray<size_t> &extFrontier,
                                 core::DynamicArray<size_t> &intFrontier) {
	const size_t total = (size_t)chunk.dimX * (size_t)chunk.dimY * (size_t)chunk.dimZ;
	for (size_t cellIdx = 0; cellIdx < total; ++cellIdx) {
		const uint8_t cellState = chunk.state[cellIdx];
		if (cellState != kStateExterior && cellState != kStateInterior) {
			continue;
		}
		const glm::ivec3 cellPos = chunk.toCell(cellIdx);
		for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
			const glm::ivec3 nb = cellPos + off * chunk.cellSize;
			if (!chunk.inBounds(nb)) {
				continue;
			}
			if (chunk.state[chunk.toIdx(nb)] != kStateEmpty) {
				continue;
			}
			if (cellState == kStateExterior) {
				extFrontier.push_back(cellIdx);
			} else {
				intFrontier.push_back(cellIdx);
			}
			break;
		}
	}
}

// Per-chunk sequential ext-then-int BFS. Same algorithm as runBidirectionalBFS,
// stripped of the heartbeat/RSS-cap logging because chunks are small and
// finish fast individually. Returns hole cell positions (at cs=1 these are
// 1-voxel positions in world coordinates).
static core::DynamicArray<glm::ivec3> chunkRunBidirectionalBFS(Level &chunk) {
	core::DynamicArray<size_t> extFrontier, intFrontier;
	chunkBuildFrontiers(chunk, extFrontier, intFrontier);

	core::DynamicArray<glm::ivec3> holeCells;

	while (!extFrontier.empty() || !intFrontier.empty()) {
		core::DynamicArray<size_t> nextExt;
		nextExt.reserve(extFrontier.size() * 6);
		for (size_t idx : extFrontier) {
			const glm::ivec3 cur = chunk.toCell(idx);
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = cur + off * chunk.cellSize;
				if (!chunk.inBounds(nb)) continue;
				const size_t nbIdx = chunk.toIdx(nb);
				const uint8_t nbState = chunk.state[nbIdx];
				if (nbState == kStateSolid || nbState == kStateExterior || nbState == kStateBlocked) continue;
				if (nbState == kStateInterior) {
					chunk.state[nbIdx] = kStateBlocked;
					holeCells.push_back(nb);
				} else {
					chunk.state[nbIdx] = kStateExterior;
					nextExt.push_back(nbIdx);
				}
			}
		}
		extFrontier = core::move(nextExt);

		core::DynamicArray<size_t> nextInt;
		nextInt.reserve(intFrontier.size() * 6);
		for (size_t idx : intFrontier) {
			const glm::ivec3 cur = chunk.toCell(idx);
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = cur + off * chunk.cellSize;
				if (!chunk.inBounds(nb)) continue;
				const size_t nbIdx = chunk.toIdx(nb);
				const uint8_t nbState = chunk.state[nbIdx];
				if (nbState == kStateSolid || nbState == kStateInterior || nbState == kStateBlocked) continue;
				if (nbState == kStateExterior) {
					chunk.state[nbIdx] = kStateBlocked;
					holeCells.push_back(nb);
				} else {
					chunk.state[nbIdx] = kStateInterior;
					nextInt.push_back(nbIdx);
				}
			}
		}
		intFrontier = core::move(nextInt);
	}

	return holeCells;
}

// Initialise a chunk-local Level at cs=1 from voxel data + parent classification.
// Caller has set chunk.cellSize=1, chunk.gridLower, chunk.dim*.
//
// Per cs=1 cell:
//   - voxel is solid (queried directly via owning node)  -> kStateSolid
//   - else parent classifies cs=2 cell as ext            -> kStateExterior
//   - else parent classifies cs=2 cell as int            -> kStateInterior
//   - else (parent: solid/empty/blocked, voxel air)      -> kStateEmpty (probe zone)
//
// Outer loop is cs=2 cells, inner loop is the (parent.cellSize)^3 cs=1 voxels
// inside each cs=2 cell. This caches the parent classification + node lookup
// once per cs=2 cell instead of doing isSolidAt's full lookup chain per cs=1
// voxel. Big win for chunks where most cs=2 cells are ext or int (no isSolidAt
// needed -- bulk-set the cs=1 state for the whole cs=2 cell). For cs=2 cells
// classified solid, we still check each voxel, but bypass the hash lookup
// (already done) and the toCellOrigin call.
//
// chunk.solid stays empty -- runBidirectionalBFS uses state[idx]==kStateSolid
// to skip solid cells, the parallel solid.count() check is redundant for our
// purposes.
static void chunkInitFromParent(Level &chunk, const Level &parent,
                                 const core::DynamicArray<NodeInfo> &nodes) {
	const size_t total = (size_t)chunk.dimX * (size_t)chunk.dimY * (size_t)chunk.dimZ;
	chunk.state.assign(total, kStateEmpty);

	const int cs2 = parent.cellSize;
	const glm::ivec3 chunkLower = chunk.gridLower;
	const glm::ivec3 chunkUpper = chunkLower + glm::ivec3(chunk.dimX - 1, chunk.dimY - 1, chunk.dimZ - 1);
	const glm::ivec3 cs2Lower = toCellOrigin(chunkLower, cs2);
	const glm::ivec3 cs2Upper = toCellOrigin(chunkUpper, cs2);

	const size_t rowStride  = (size_t)chunk.dimZ;
	const size_t planeStride = (size_t)chunk.dimY * (size_t)chunk.dimZ;

	// cs=2 cell iteration order matches chunk.state's index layout (x outer,
	// z inner) so the inner cs=1 voxel writes touch contiguous bytes.
	for (int cx = cs2Lower.x; cx <= cs2Upper.x; cx += cs2) {
		for (int cy = cs2Lower.y; cy <= cs2Upper.y; cy += cs2) {
			for (int cz = cs2Lower.z; cz <= cs2Upper.z; cz += cs2) {
				const glm::ivec3 cs2Cell(cx, cy, cz);
				const uint8_t parentState = parent.cellState(cs2Cell);

				const int x0 = glm::max(cx, chunkLower.x);
				const int y0 = glm::max(cy, chunkLower.y);
				const int z0 = glm::max(cz, chunkLower.z);
				const int x1 = glm::min(cx + cs2 - 1, chunkUpper.x);
				const int y1 = glm::min(cy + cs2 - 1, chunkUpper.y);
				const int z1 = glm::min(cz + cs2 - 1, chunkUpper.z);

				if (parentState == kStateExterior || parentState == kStateInterior) {
					// Bulk-fill: every cs=1 voxel in this cs=2 cell takes the
					// parent classification. cs=2 ext/int cells contain no
					// voxel data by construction so no per-voxel check needed.
					const uint8_t s = parentState;
					for (int wx = x0; wx <= x1; ++wx) {
						const size_t planeBase = (size_t)(wx - chunkLower.x) * planeStride;
						for (int wy = y0; wy <= y1; ++wy) {
							const size_t rowBase = planeBase + (size_t)(wy - chunkLower.y) * rowStride;
							for (int wz = z0; wz <= z1; ++wz) {
								chunk.state[rowBase + (size_t)(wz - chunkLower.z)] = s;
							}
						}
					}
				} else if (parentState == kStateSolid) {
					// Mark cs=1 cells kStateSolid if ANY node owning this cs=2
					// cell has a voxel at the position. parent.solid points to
					// the primary owner; extraSolid carries any additional
					// owners that share the cell. Without iterating both, voxels
					// in non-primary nodes silently appear as kStateEmpty in
					// chunk.state -- the cs=1 BFS treats them as probe zone,
					// the chunk action skips them as "not solid", saw bit
					// stays unset, faceclassify renders them gray, fillholes
					// can't see them as walls.
					forEachCellOwner(parent, cs2Cell, [&](uint64_t nodeIdx) {
						const NodeInfo &ni = nodes[(size_t)nodeIdx];
						const voxel::Region &nodeRegion = ni.rv->region();
						for (int wx = x0; wx <= x1; ++wx) {
							const size_t planeBase = (size_t)(wx - chunkLower.x) * planeStride;
							for (int wy = y0; wy <= y1; ++wy) {
								const size_t rowBase = planeBase + (size_t)(wy - chunkLower.y) * rowStride;
								for (int wz = z0; wz <= z1; ++wz) {
									const size_t stateIdx = rowBase + (size_t)(wz - chunkLower.z);
									if (chunk.state[stateIdx] == kStateSolid) {
										// Already marked solid by an earlier-iterated owner.
										continue;
									}
									const glm::ivec3 worldPos(wx, wy, wz);
									const glm::ivec3 local = transformPoint(ni.invWorldMat, worldPos);
									if (!nodeRegion.containsPoint(local)) {
										continue;
									}
									if (voxel::isAir(ni.rv->voxel(local).getMaterial())) {
										continue;
									}
									chunk.state[stateIdx] = kStateSolid;
								}
							}
						}
					});
				}
				// else (kStateEmpty, kStateBlocked): nothing to do.
				// kStateBlocked is a hole the cs=2 BFS sealed; cs=1 sees it
				// as probe zone (kStateEmpty was already set by .assign).
			}
		}
	}
}

// Wall-walking chunk seeder.
//
// extFront/intFront are sparse state-array indices into parent (cs=2) produced
// by buildFrontiers(parent, ..., wrapSolid=true). Each ext-front cell is the
// model's outer surface at cs=2; int-front is the inner surface (cavity walls).
//
// For each ext-front cell not yet covered by a previous chunk:
//   1. Default chunk = (kHolefillChunkVoxels + 2*kHolefillChunkOverlap)^3
//      centered on the ext-front cell (in world voxel coords).
//   2. If no int-front cell lies inside the chunk, expand to
//      (kHolefillChunkMaxVox + 2*kHolefillChunkOverlap)^3 once. If still no
//      int-front, the wall is too thick for this design; skip.
//   3. Mark all ext-front cells inside the chunk as covered. Skip them as
//      future chunk seeds (they're already in this one).
//
// Spatial indexing: extFront/intFront are bucketed by floor(coord/chunkVoxels)
// so per-chunk box queries scan at most 8-27 buckets (worst case for an
// expanded chunk) instead of all front cells. Without this the seeder is
// O(extFront * intFront) which is 10^12+ for large models.
// Anchors chunks on BOTH ext-front AND int-front cs=2 cells. For each shell,
// every uncovered front cell becomes a chunk centered on it (shared dedup
// via the covered set so chunks don't pile up where the two shells meet).
//
// Why both shells: the chunked cs=1 BFS only sees what's inside chunks. With
// ext-only anchoring, walls between two interior cavities -- inner walls of
// gothic buildings, sealed-room dividers, anything more than ~80 voxels from
// the model's exterior -- never get a chunk. fillholes reports those walls
// as watertight even when they have visible 1-voxel holes; faceclassify
// leaves their voxels grey because the saw bit was never set; erode misses
// them entirely. Anchoring on both shells covers every wall that touches
// either ext or int air.
//
// requireOppositeFront semantics:
//   true  (fillholes): a chunk anchored on an ext cell needs an int cell in
//         its (default or expanded) bbox; an int-anchored chunk needs an
//         ext cell. Walls with no opposite-shell cell in range produce no
//         chunk -- nothing for the cs=1 BFS to find a contact with there.
//   false (erode / faceclassify): every uncovered front cell becomes a
//         chunk at the default size. They need to visit every wall the
//         model has, regardless of whether ext meets int there.
static core::DynamicArray<ChunkBox> seedChunks(
		const Level &parent,
		const core::DynamicArray<size_t> &extFront,
		const core::DynamicArray<size_t> &intFront,
		bool requireOppositeFront = true) {
	core::DynamicArray<ChunkBox> chunks;
	if (extFront.empty() && intFront.empty()) {
		return chunks;
	}
	// With requireOppositeFront=true an empty shell on either side means no
	// chunk could ever satisfy the filter -- short-circuit to avoid the
	// per-cell "skippedNoOpposite" accounting on a guaranteed-empty result.
	if (requireOppositeFront && (extFront.empty() || intFront.empty())) {
		return chunks;
	}
	// Empirical (gothic test model): ~1 chunk per ~2400 front cells per shell.
	// Reserve to a looser ratio so growth never reallocates across thousands
	// of chunks. Sized for the combined (ext+int) anchoring.
	static constexpr size_t kFrontPerChunkEstimate = 256;
	chunks.reserve((extFront.size() + intFront.size()) / kFrontPerChunkEstimate + 64);

	const int spatialCs = kHolefillChunkVoxels;
	auto spatialKey = [&](const glm::ivec3 &p) {
		return glm::ivec3(floorDiv(p.x, spatialCs),
						  floorDiv(p.y, spatialCs),
						  floorDiv(p.z, spatialCs));
	};

	using FrontGrid = std::unordered_map<glm::ivec3, core::DynamicArray<glm::ivec3>, glm::hash<glm::ivec3>>;
	// Empirical: each spatial bucket (one chunk's worth of world space) holds
	// roughly chunkVoxels^2 ext/int-front cells in surface-area-bounded models.
	// Reserve buckets = totalFrontCells / kSpatialBucketAvgEntries to avoid
	// rehashing during construction.
	static constexpr int kSpatialBucketAvgEntries = 16;
	FrontGrid extGrid, intGrid;
	extGrid.reserve(extFront.size() / kSpatialBucketAvgEntries + 1);
	intGrid.reserve(intFront.size() / kSpatialBucketAvgEntries + 1);
	for (size_t idx : extFront) {
		const glm::ivec3 cellPos = parent.toCell(idx);
		extGrid[spatialKey(cellPos)].push_back(cellPos);
	}
	for (size_t idx : intFront) {
		const glm::ivec3 cellPos = parent.toCell(idx);
		intGrid[spatialKey(cellPos)].push_back(cellPos);
	}

	auto bboxContains = [&](const FrontGrid &grid, const ChunkBox &box) -> bool {
		const glm::ivec3 lowKey = spatialKey(box.lower);
		const glm::ivec3 highKey = spatialKey(box.upper);
		for (int kz = lowKey.z; kz <= highKey.z; ++kz) {
			for (int ky = lowKey.y; ky <= highKey.y; ++ky) {
				for (int kx = lowKey.x; kx <= highKey.x; ++kx) {
					auto it = grid.find(glm::ivec3(kx, ky, kz));
					if (it == grid.end()) {
						continue;
					}
					for (const glm::ivec3 &cellPos : it->second) {
						if (cellPos.x >= box.lower.x && cellPos.x <= box.upper.x
						 && cellPos.y >= box.lower.y && cellPos.y <= box.upper.y
						 && cellPos.z >= box.lower.z && cellPos.z <= box.upper.z) {
							return true;
						}
					}
				}
			}
		}
		return false;
	};

	// Single covered set keyed by cs=2 cell origin -- shared across both shells
	// so a chunk created by the ext-anchored pass dedupes future int-anchored
	// candidates whose cells fall inside that chunk's bbox (and vice versa).
	std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>> covered;
	covered.reserve(extFront.size() + intFront.size());

	auto markCoveredFromGrid = [&](const FrontGrid &grid, const ChunkBox &box) {
		const glm::ivec3 lowKey = spatialKey(box.lower);
		const glm::ivec3 highKey = spatialKey(box.upper);
		for (int kz = lowKey.z; kz <= highKey.z; ++kz) {
			for (int ky = lowKey.y; ky <= highKey.y; ++ky) {
				for (int kx = lowKey.x; kx <= highKey.x; ++kx) {
					auto it = grid.find(glm::ivec3(kx, ky, kz));
					if (it == grid.end()) {
						continue;
					}
					for (const glm::ivec3 &cellPos : it->second) {
						if (cellPos.x >= box.lower.x && cellPos.x <= box.upper.x
						 && cellPos.y >= box.lower.y && cellPos.y <= box.upper.y
						 && cellPos.z >= box.lower.z && cellPos.z <= box.upper.z) {
							covered.insert(cellPos);
						}
					}
				}
			}
		}
	};
	auto markCovered = [&](const ChunkBox &box) {
		markCoveredFromGrid(extGrid, box);
		markCoveredFromGrid(intGrid, box);
	};

	const int defaultHalf  = kHolefillChunkVoxels  / 2 + kHolefillChunkOverlap;
	const int expandedHalf = kHolefillChunkMaxVox  / 2 + kHolefillChunkOverlap;
	// The COVERED set marks front cells that should NOT anchor a new chunk
	// (deduplication). It must use a SMALLER box than the chunk's full bbox:
	// the chunk's bbox (seed +/- defaultHalf) includes a kHolefillChunkOverlap
	// buffer on each side. If we mark front cells at the bbox edge as covered,
	// they can't anchor their own chunks even though wall voxels just past the
	// edge fall outside the chunk's coverage. The result is gray slices in
	// faceclassify wherever a wall sits one cell past chunk A's far bbox edge.
	// Marking covered only inside the CORE (seed +/- chunkVoxels/2) lets cells
	// in the overlap zone anchor their own chunks; their bboxes overlap chunk
	// A's overlap heavily but extend coverage past A's far edge -- the wall is
	// in chunk B's bbox.
	//
	// Second subtle constraint -- cover-half must be STRICTLY less than core-half
	// (chunkVoxels/2), not equal. With cover-half == core-half, the minimum
	// permitted chebyshev distance between two seeds is core-half + 1 (since the
	// cover check uses '<= cover-half' inclusive). When two seeds land at that
	// minimum distance along TWO axes simultaneously (a "diagonal corner"), the
	// voxel at chebyshev = core-half + 1 from BOTH seeds falls outside BOTH cores
	// -- it sits one voxel past each core's boundary in one of the two axes.
	// Tiling gap. With cover-half = core-half - 1, the minimum permitted seed
	// spacing drops to core-half, which equals the core's reach, so the diagonal
	// corner voxel sits exactly on each seed's core boundary and is covered.
	// Observed in the gothic model: a 17-voxel-wide strip of wall voxels at
	// Y=-1763 was skipped because no chunk core covered it -- ci=168 core ended
	// at Y=-1762 and ci=265 core started at Y=-1764, and the diagonal seeds (at
	// chebyshev 65 in both Y and Z) couldn't be filled with an intermediate seed
	// because the cover boxes had already marked the spots as covered.
	const int defaultCoverHalf  = kHolefillChunkVoxels / 2 - 1;
	const int expandedCoverHalf = kHolefillChunkMaxVox / 2 - 1;

	uint64_t skippedCovered = 0;
	uint64_t skippedNoOpposite = 0;
	uint64_t skippedExpanded = 0;

	// One pass per shell. The body is identical apart from which front list
	// drives the seeds and which grid is the "opposite" (used by the
	// requireOppositeFront filter).
	auto processShell = [&](const core::DynamicArray<size_t> &shellFront,
							 const FrontGrid &oppositeGrid) {
		for (size_t i = 0; i < shellFront.size(); ++i) {
			const glm::ivec3 seed = parent.toCell(shellFront[i]);
			if (covered.find(seed) != covered.end()) {
				++skippedCovered;
				continue;
			}

			ChunkBox box;
			box.lower = seed - glm::ivec3(defaultHalf);
			box.upper = seed + glm::ivec3(defaultHalf);
			int coverHalf = defaultCoverHalf;

			if (requireOppositeFront) {
				if (!bboxContains(oppositeGrid, box)) {
					box.lower = seed - glm::ivec3(expandedHalf);
					box.upper = seed + glm::ivec3(expandedHalf);
					coverHalf = expandedCoverHalf;
					if (!bboxContains(oppositeGrid, box)) {
						++skippedNoOpposite;
						continue;
					}
					++skippedExpanded;
				}
			}

			chunks.push_back(box);
			ChunkBox coverBox;
			coverBox.lower = seed - glm::ivec3(coverHalf);
			coverBox.upper = seed + glm::ivec3(coverHalf);
			markCovered(coverBox);
		}
	};

	processShell(extFront, intGrid);
	processShell(intFront, extGrid);

	if (k3DPrintVerbose) {
		Log::info("3dprint holefill: chunked cs=1 seedChunks: %zu chunks (covered-skip=%llu, no-opposite-skip=%llu, expanded=%llu)",
				  chunks.size(),
				  (unsigned long long)skippedCovered,
				  (unsigned long long)skippedNoOpposite,
				  (unsigned long long)skippedExpanded);
	}
	// Always-on warning: walls thicker than the expanded chunk size (kHolefillChunkMaxVox)
	// can hide leaks that the chunked pass won't reach. The user should know.
	// Suppressed when requireOppositeFront=false (erode/faceclassify): there,
	// "no opposite shell in range" isn't a skip.
	if (requireOppositeFront && skippedNoOpposite > 0) {
		Log::warn("3dprint fillholes: %llu wall region(s) skipped -- no opposite-shell cell within %d voxels of the seed. "
				  "Walls thicker than this aren't checked for sub-cs=1 leaks. To reach them, raise "
				  "kHolefillChunkMaxVox in FaceClassify.cpp (memory cost grows ~cubically).",
				  (unsigned long long)skippedNoOpposite, kHolefillChunkMaxVox);
	}
	return chunks;
}

// Generic chunked cs=1 driver. Walks the chunk list in parallel; for each
// chunk runs chunkInitFromParent + chunkRunBidirectionalBFS, then hands the
// populated chunk Level and the BFS hole-cell list to perChunkAction, which
// runs in the parallel context.
//
// The action MUST be thread-safe with respect to any state it shares across
// chunks. It MUST also act inside the chunk: do not accumulate per-voxel
// positions into a global container that grows with the model -- that
// re-introduces the multi-GB working-set problem that the chunked
// architecture exists to avoid. Allowed shared state is per-node (bounded
// by node count, e.g. atomic dirty-region trackers, per-node mark bits)
// and small per-chunk slots indexed by ci.
//
// callerTag and startMs feed the heartbeat log line so that fillholes,
// holemap, erode and faceclassify each show their own progress identity.
template<typename PerChunkAction>
static void runChunkedCs1Driver(
		const core::DynamicArray<ChunkBox> &chunks,
		const Level &parent,
		core::DynamicArray<NodeInfo> &nodes,
		const char *callerTag,
		uint64_t startMs,
		PerChunkAction perChunkAction) {
	const int totalChunks = (int)chunks.size();
	std::atomic<int64_t> chunksProcessed{0};
	std::atomic<uint64_t> lastLogMs{startMs};

	app::for_parallel(0, totalChunks, [&](int start, int end) {
		// Per-thread chunk Level. The state vector capacity is reused across
		// iterations within this thread: same-sized chunks (the default
		// 160^3 case) skip allocation, .assign() resets values in place.
		// Different-sized chunks (the 288^3 expanded case) realloc, but
		// those are <1% of chunks.
		Level chunk;
		chunk.cellSize = 1;
		for (int ci = start; ci < end; ++ci) {
			if (g_rssCapTripped.load(std::memory_order_relaxed)) {
				return;
			}

			const ChunkBox &box = chunks[(size_t)ci];
			chunk.gridLower = box.lower;
			chunk.dimX = box.upper.x - box.lower.x + 1;
			chunk.dimY = box.upper.y - box.lower.y + 1;
			chunk.dimZ = box.upper.z - box.lower.z + 1;

			chunkInitFromParent(chunk, parent, nodes);
			core::DynamicArray<glm::ivec3> holeCells = chunkRunBidirectionalBFS(chunk);

			perChunkAction(chunk, holeCells, ci);

			const int64_t done = chunksProcessed.fetch_add(1, std::memory_order_relaxed) + 1;
			const uint64_t now = core::TimeProvider::systemMillis();
			uint64_t prevMs = lastLogMs.load(std::memory_order_relaxed);
			if (now - prevMs >= 3000u && lastLogMs.compare_exchange_strong(prevMs, now)) {
				if (k3DPrintVerbose) {
					fprintf(stderr, "[CHUNK %s] %lld/%d chunks (%.1f%%) elapsed=%.1fs [RSS=%.1f GB]\n",
							callerTag, (long long)done, totalChunks,
							100.0 * (double)done / (double)totalChunks,
							(double)(now - startMs) / 1000.0, rssGB());

				}
				checkRSSCap("chunked cs=1 heartbeat");
			}
		}
	});
}

// Detection-only kernel: builds the chunk list, runs each chunk's BFS in
// parallel, returns the deduped cs=1 hole voxels. Doesn't touch the model.
// Used by both runChunkedCs1Pass (which then plugs them) and runHoleMap (which
// recolors walls adjacent to them without modifying topology).
static core::DynamicArray<glm::ivec3> detectChunkedCs1HoleVoxels(
		const Level &parent,
		core::DynamicArray<NodeInfo> &nodes,
		const core::DynamicArray<size_t> &extFront,
		const core::DynamicArray<size_t> &intFront,
		const char *callerTag) {
	core::DynamicArray<glm::ivec3> allHoleVoxels;
	const uint64_t startMs = core::TimeProvider::systemMillis();
	if (k3DPrintVerbose) {
		Log::info("3dprint %s: chunked cs=1 detection starting (parent cs=%d, ext-front=%zu, int-front=%zu)",
				  callerTag, parent.cellSize, extFront.size(), intFront.size());
	}
	logRSS("chunked cs=1 entry");

	const uint64_t seedStartMs = core::TimeProvider::systemMillis();
	core::DynamicArray<ChunkBox> chunks = seedChunks(parent, extFront, intFront);
	const double seedSecs = (double)(core::TimeProvider::systemMillis() - seedStartMs) / 1000.0;
	if (k3DPrintVerbose) {
		Log::info("3dprint %s: chunked cs=1 seeded %zu chunks in %.1fs", callerTag, chunks.size(), seedSecs);
	}
	if (chunks.empty()) {
		Log::info("3dprint %s: no thin-wall regions detected for cs=1 inspection.", callerTag);
		return allHoleVoxels;
	}

	core::DynamicArray<core::DynamicArray<glm::ivec3>> perChunkHoles;
	perChunkHoles.resize(chunks.size());
	std::atomic<int64_t> totalHoleVoxels{0};

	// Per-chunk hole cell list moves into a slot indexed by ci. Per-chunk
	// containers are small (a few voxels each on a sealed model); the dedup
	// step below merges them. No global growing buffer.
	runChunkedCs1Driver(chunks, parent, nodes, callerTag, startMs,
		[&](Level &chunk, core::DynamicArray<glm::ivec3> &chunkHoles, int ci) {
			(void)chunk;
			perChunkHoles[(size_t)ci] = core::move(chunkHoles);
			totalHoleVoxels.fetch_add((int64_t)perChunkHoles[(size_t)ci].size(),
									   std::memory_order_relaxed);
		});

	// Aggregate hole voxels with dedup (overlapping chunks may detect the
	// same hole). std::unordered_set is fine here -- this runs sequentially
	// after the parallel BFS pass, total volume is bounded by surface area.
	std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>> uniqueHoles;
	const size_t rawTotal = (size_t)totalHoleVoxels.load(std::memory_order_relaxed);
	uniqueHoles.reserve(rawTotal);
	allHoleVoxels.reserve(rawTotal);
	for (const core::DynamicArray<glm::ivec3> &chunkList : perChunkHoles) {
		for (const glm::ivec3 &v : chunkList) {
			if (uniqueHoles.insert(v).second) {
				allHoleVoxels.push_back(v);
			}
		}
	}

	const double bfsSecs = (double)(core::TimeProvider::systemMillis() - startMs) / 1000.0;
	if (k3DPrintVerbose) {
		Log::info("3dprint %s: chunked cs=1 BFS done in %.1fs -- %zu raw -> %zu unique hole voxel(s) from %zu chunks",
				  callerTag, bfsSecs, rawTotal, allHoleVoxels.size(), chunks.size());
	}
	logRSS("chunked cs=1 after BFS");
	return allHoleVoxels;
}

// Aggregating runner used by fillholes. Detection + applyHoleFills.
static FillStats runChunkedCs1Pass(const Level &parent,
                                core::DynamicArray<NodeInfo> &nodes,
                                SceneManager *sceneMgr,
                                const core::DynamicArray<size_t> &extFront,
                                const core::DynamicArray<size_t> &intFront) {
	FillStats stats;
	const core::DynamicArray<glm::ivec3> allHoleVoxels =
		detectChunkedCs1HoleVoxels(parent, nodes, extFront, intFront, "fillholes");
	if (!allHoleVoxels.empty()) {
		// Pass cs=1 hole voxels via holeVoxels (per-voxel writes). finalCellSize
		// is parent.cellSize so the owner-search lambda queries cs=2 cells; the
		// actual writes are still 1 voxel (the holeVoxel itself).
		const core::DynamicArray<glm::ivec3> emptyCells;
		stats = applyHoleFills(emptyCells, allHoleVoxels, parent, nodes,
							   sceneMgr, parent.cellSize);
	}
	return stats;
}

// Shared dense-state cap (16 GB). cs=2 on the gothic model is ~7.6 GB; cs=1
// dense would be ~60 GB, well over this cap and why the chunked cs=1 pass
// exists. Edit + rebuild to sweep on machines with different RAM budgets.
static constexpr size_t kDensePassMaxStateCells = 16ull * 1024ull * 1024ull * 1024ull;

// Result of buildCoarseShell. ok=false means an error was already logged and
// the caller should return. ok=true means nodes/coarse were populated; the
// caller decides what to do when intCount==0 (fillholes / holemap / erode /
// faceclassify each phrase the warning differently).
struct CoarseShellResult {
	bool ok;
	uint64_t intCount;
	int coarseCellSize;
	glm::ivec3 gridLower;
	glm::ivec3 gridUpper;
};

// Builds the coarse classification shared by all four chunked-cs=1 commands.
// Steps mirrored from the original copies in fillholes / holemap / faceclassify:
//   1. Walk the scene graph, collect NodeInfo (id, volume ptr, world+inverse
//      matrices) for every model node with a non-null volume.
//   2. Infer coarse cell size from the modal node width (regrid is expected
//      to have produced uniform-width nodes).
//   3. Snap each node's world-space lower corner to the coarse grid, mark its
//      cell in coarse.solid IF the node has at least one solid voxel. Empty
//      nodes are skipped: they have no voxels to classify and treating them
//      as solid blocks at coarse scale would mis-classify their cells as
//      occupied.
//   4. Pad the grid by one cell on each side (boundary seeding mustn't touch
//      genuine interior air) and allocate the dense state vector.
//   5. Run buildExterior then buildInteriorAllSeeds. intCount is returned to
//      the caller for context-specific warnings.
//
// The caller still owns the post-condition handling (snap minCellSize, build
// refinement chain, handle intCount==0 with its own message). nodes and coarse
// are output parameters because Level holds a std::vector that we want to fill
// in place rather than copy out of the helper.
// coarseCellSizeOverride: 0 = auto (mode of node widths; fast path that maps one
// solid cell per node -- valid when nodes are already cellSize-aligned, e.g.
// after 3dprint regrid). When > 0, use the override AND switch to per-voxel
// bucketing via buildSolidHash so a node that spans multiple coarse cells gets
// each sub-cell tagged correctly. Required for single-node fillholes where
// node width != target coarse cellSize (mode-of-widths would pick the whole
// node width and yield a 1-cell grid with no possible interior).
static CoarseShellResult buildCoarseShell(
		SceneManager *sceneMgr,
		const char *callerTag,
		core::DynamicArray<NodeInfo> &nodes,
		Level &coarse,
		int coarseCellSizeOverride = 0) {
	CoarseShellResult res;
	res.ok = false;
	res.intCount = 0;
	res.coarseCellSize = 0;
	res.gridLower = glm::ivec3(0);
	res.gridUpper = glm::ivec3(0);

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	nodes.reserve((size_t)graph.size());
	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxel::RawVolume *rv = node.volume();
		if (rv == nullptr) {
			continue;
		}
		NodeInfo info;
		info.nodeId = node.id();
		info.rv = rv;
		info.worldMat = graph.worldMatrix(node, 0);
		info.invWorldMat = glm::inverse(info.worldMat);
		info.cellOrigin = glm::ivec3(0);
		nodes.push_back(info);
	}
	if (nodes.empty()) {
		Log::warn("3dprint %s: no model nodes -- nothing to do.", callerTag);
		return res;
	}
	const int totalNodes = (int)nodes.size();

	int coarseCellSize = coarseCellSizeOverride;
	if (coarseCellSize <= 0) {
		std::unordered_map<int, int> widthCount;
		for (const NodeInfo &ni : nodes) {
			widthCount[ni.rv->region().getWidthInVoxels()]++;
		}
		int bestCount = 0;
		for (const auto &kv : widthCount) {
			if (kv.second > bestCount) {
				bestCount = kv.second;
				coarseCellSize = kv.first;
			}
		}
	}
	if (coarseCellSize <= 0) {
		Log::error("3dprint %s: could not determine cell size -- run '3dprint regrid' first.", callerTag);
		return res;
	}

	coarse.cellSize = coarseCellSize;
	glm::ivec3 gridLower(INT_MAX, INT_MAX, INT_MAX);
	glm::ivec3 gridUpper(INT_MIN, INT_MIN, INT_MIN);

	if (coarseCellSizeOverride > 0) {
		// Per-voxel bucketing: a single node can span many coarse cells. Without
		// this, the fast path below would tag just one cell per node and the
		// coarse grid for a non-regridded model collapses to a single solid cell
		// -- no enclosed interior possible.
		buildSolidHash(coarse, nodes);
		if (coarse.solid.empty()) {
			Log::error("3dprint %s: no solid voxels in any node at cs=%d", callerTag, coarseCellSize);
			return res;
		}
		for (const auto &kv : coarse.solid) {
			gridLower = glm::min(gridLower, kv.first);
			gridUpper = glm::max(gridUpper, kv.first);
		}
		// Keep ni.cellOrigin sensible for downstream code that reads it (anchors
		// it to the node's lower corner; ambiguous when the node spans multiple
		// cells, but no caller treats it as authoritative classification).
		for (int i = 0; i < totalNodes; ++i) {
			NodeInfo &ni = nodes[i];
			ni.cellOrigin = toCellOrigin(transformPoint(ni.worldMat, ni.rv->region().getLowerCorner()), coarseCellSize);
		}
	} else {
		coarse.solid.reserve((size_t)totalNodes);
		for (int i = 0; i < totalNodes; ++i) {
			NodeInfo &ni = nodes[i];
			const voxel::Region &r = ni.rv->region();
			ni.cellOrigin = toCellOrigin(transformPoint(ni.worldMat, r.getLowerCorner()), coarseCellSize);
			gridLower = glm::min(gridLower, ni.cellOrigin);
			gridUpper = glm::max(gridUpper, ni.cellOrigin);
			bool hasSolid = false;
			for (int z = r.getLowerZ(); z <= r.getUpperZ() && !hasSolid; ++z) {
				for (int y = r.getLowerY(); y <= r.getUpperY() && !hasSolid; ++y) {
					for (int x = r.getLowerX(); x <= r.getUpperX() && !hasSolid; ++x) {
						if (!voxel::isAir(ni.rv->voxel(x, y, z).getMaterial())) {
							hasSolid = true;
						}
					}
				}
			}
			if (hasSolid) {
				coarse.solid.emplace(ni.cellOrigin, i);
			}
		}
	}

	gridLower -= glm::ivec3(coarseCellSize);
	gridUpper += glm::ivec3(coarseCellSize);
	coarse.initGrid(gridLower, gridUpper);
	coarse.state.assign((size_t)coarse.dimX * coarse.dimY * coarse.dimZ, kStateEmpty);
	for (const auto &kv : coarse.solid) {
		if (coarse.inBounds(kv.first)) {
			coarse.state[(size_t)coarse.toIdx(kv.first)] = kStateSolid;
		}
	}

	{
		Level emptyPrev;
		buildExterior(coarse, emptyPrev);
	}
	res.intCount = buildInteriorAllSeeds(coarse);
	res.coarseCellSize = coarseCellSize;
	res.gridLower = gridLower;
	res.gridUpper = gridUpper;
	res.ok = true;

	// Auto-fallback for non-regridded models (single-node case is the common one):
	// the auto-picked coarseCellSize (mode of node widths) collapses to a 1-cell
	// grid with no possible interior. Halve the cellSize and retry with per-voxel
	// bucketing; bottom out at cs=8 to keep dense-pass memory bounded. Only fires
	// when the caller didn't supply an override AND the coarse pass found no
	// enclosed interior. Applies to every command that calls buildCoarseShell:
	// fillholes, erode, thicken, faceclassify, holemap.
	if (coarseCellSizeOverride == 0 && res.intCount == 0) {
		static constexpr int kMinCoarseRetryCellSize = 8;
		int retryCellSize = coarseCellSize / 2;
		while (retryCellSize >= kMinCoarseRetryCellSize) {
			Log::info("3dprint %s: no enclosed interior at cs=%d -- retrying with cs=%d (auto-fallback)",
					  callerTag, coarseCellSize, retryCellSize);
			nodes.clear();
			coarse = Level{};
			CoarseShellResult retry = buildCoarseShell(sceneMgr, callerTag, nodes, coarse, retryCellSize);
			if (!retry.ok || retry.intCount > 0) {
				return retry;
			}
			retryCellSize /= 2;
		}
	}
	return res;
}

// No-op default for buildRefinedShell's per-level hook: callers that don't
// need to inspect intermediate levels (faceclassify) get this and the compiler
// inlines the call away.
struct BuildRefinedShellNoopHook {
	void operator()(const Level &lvl, int cellSize) const {
		(void)lvl;
		(void)cellSize;
	}
};

// Refines coarse classification down to minCellSize. The per-level hook fires
// after each level's BFS, before the level moves into prevOut. Erode uses it
// to run disjoint detection at cs=4 while that level is alive, avoiding a
// second buildSolidHash + BFS just for that.
//
// On return, prevOut holds the deepest reached level: minCellSize when the
// 16 GB dense cap was respected at every step, or a coarser cs if a level
// would have exceeded the cap or the RSS cap tripped. Either way the chunked
// cs=1 driver can use prevOut as parent.
template<class PerLevelHook = BuildRefinedShellNoopHook>
static void buildRefinedShell(
		const Level &coarse,
		int minCellSize,
		const glm::ivec3 &gridLower,
		const glm::ivec3 &gridUpper,
		const core::DynamicArray<NodeInfo> &nodes,
		const char *callerTag,
		Level &prevOut,
		PerLevelHook perLevelHook = BuildRefinedShellNoopHook{}) {
	prevOut = coarse;
	const int totalNodes = (int)nodes.size();
	const core::String solidHashLabel = core::String(callerTag) + " solidHash";
	for (int fineSize = coarse.cellSize / 2; fineSize >= minCellSize; fineSize /= 2) {
		Level fine;
		fine.cellSize = fineSize;
		fine.initGrid(gridLower, gridUpper);
		const size_t stateCells = (size_t)fine.dimX * (size_t)fine.dimY * (size_t)fine.dimZ;
		if (stateCells > kDensePassMaxStateCells) {
			Log::warn("3dprint %s: cs=%d grid %dx%dx%d = %.1f GB dense state would exceed the 16 GB budget. "
					  "Stopping refinement at the previously-reached cs=%d -- chunked cs=1 still runs "
					  "(per-chunk allocation, no dense global state) using cs=%d as parent.",
					  callerTag, fineSize, fine.dimX, fine.dimY, fine.dimZ,
					  (double)stateCells / (1024.0 * 1024.0 * 1024.0),
					  fineSize * 2, fineSize * 2);
			break;
		}

		fine.solid.clear();
		fine.state.assign(stateCells, kStateEmpty);
		{
			ProgressTimer levelTimer(solidHashLabel.c_str(), totalNodes, k3DPrintVerbose);
			buildSolidHash(fine, nodes, &levelTimer);
		}
		for (const auto &kv : fine.solid) {
			if (fine.inBounds(kv.first)) {
				fine.state[(size_t)fine.toIdx(kv.first)] = kStateSolid;
			}
		}

		initFineFromPrev(fine, prevOut);
		if (checkRSSCap("after initFineFromPrev")) {
			break;
		}
		// Hole cells are not collected here -- callers that need them (fillholes,
		// holemap) drive their own per-level loop. Erode and faceclassify use this
		// helper because they only need the final cs=2 ext/int classification.
		runBidirectionalBFS(fine);
		if (checkRSSCap("after runBidirectionalBFS")) {
			break;
		}

		// Hook runs while the level is still alive but before it shifts into
		// prevOut. Callers can read fine.state and fine.solid here.
		perLevelHook(fine, fineSize);

		prevOut = core::move(fine);
	}
}

// Cell size at which erode runs disjoint-interior detection. Hooks into
// buildRefinedShell at this level. Larger = faster, smaller = more accurate
// at picking up thin disjoint connections to the main shell. cs=4 is the
// pragmatic choice on the gothic model: ~15 MB BFS bit array, ~3-5 s, finds
// disjoint cs=4 blocks of voxels with reasonable precision. If you change
// this, ensure runErode's minCellSize plumbing still reaches it (the chain
// must descend to at least this cs for the hook to fire).
static constexpr int kErodeDisjointCellSize = 4;

// Bit-array-backed lookup for "is this world position in a disjoint cs=N cell?".
// Stored as a flat bit per cs=N cell over the level's grid bounds: O(1) lookup,
// ~15 MB on the gothic model at cs=4 -- vs a hash set with fixed-bucket-count
// DynamicSet, which would do chain walks for every one of the ~100M post-pass
// lookups. The post-pass calls isDisjoint per non-air voxel; cache locality
// of the flat bit array matters a lot more than the hash-set sparseness would.
struct DisjointInteriorMap {
	int cellSize = 0;
	int dimX = 0;
	int dimY = 0;
	int dimZ = 0;
	glm::ivec3 gridLower{0};
	core::DynamicArray<uint8_t> bits;
	bool populated = false;

	bool isDisjoint(const glm::ivec3 &worldPos) const {
		if (!populated) {
			return false;
		}
		const int ix = (worldPos.x - gridLower.x) / cellSize;
		const int iy = (worldPos.y - gridLower.y) / cellSize;
		const int iz = (worldPos.z - gridLower.z) / cellSize;
		if (ix < 0 || ix >= dimX || iy < 0 || iy >= dimY || iz < 0 || iz >= dimZ) {
			return false;
		}
		const size_t idx = (size_t)ix * (size_t)dimY * (size_t)dimZ
						 + (size_t)iy * (size_t)dimZ
						 + (size_t)iz;
		return (bits[idx >> 3] & (uint8_t)(1u << (idx & 7))) != 0;
	}
};

// Detects cs=N "disjoint" solid cells at the given level: kStateSolid cells
// not reachable from cells touching kStateExterior or kStateBlocked via
// kStateSolid face-adjacency. By construction, an unreachable kStateSolid
// cell has no kStateExterior/kStateBlocked face-neighbour (otherwise it
// would be a seed) -- so its voxels see only int cavity or other solid
// neighbours, exactly the user's "interior shell only" criterion.
//
// Memory: temporary "reached" bit array sized to lvl.state (~15 MB at cs=4
// on gothic), freed when the function returns. The output map stores its own
// bit array of the same size (also ~15 MB) which survives for use in the
// post-pass.
//
// Frontier reservation follows the growth-ceiling rule: worst case every
// kStateSolid cell becomes a seed (a fully-faceted, ext-touching shell), so
// reserve to lvl.solid.size() up front. Per-round next-frontier reserves to
// (current frontier * 6) inside the BFS loop -- the standard 6-neighbour
// expansion cap.
static void detectDisjointInteriorCells(
		const Level &lvl,
		DisjointInteriorMap &out) {
	out.cellSize = lvl.cellSize;
	out.dimX = lvl.dimX;
	out.dimY = lvl.dimY;
	out.dimZ = lvl.dimZ;
	out.gridLower = lvl.gridLower;
	const size_t totalCells = lvl.state.size();
	const size_t bitArrSize = (totalCells + 7) / 8;
	out.bits.resize(bitArrSize);
	out.populated = true;

	if (lvl.solid.empty()) {
		return;
	}

	core::DynamicArray<uint8_t> reached;
	reached.resize(bitArrSize);

	auto markReached = [&](size_t idx) {
		reached[idx >> 3] |= (uint8_t)(1u << (idx & 7));
	};
	auto isReached = [&](size_t idx) -> bool {
		return (reached[idx >> 3] & (uint8_t)(1u << (idx & 7))) != 0;
	};

	core::DynamicArray<size_t> frontier;
	// Reserve to the seed-loop growth ceiling. Every kStateSolid cell could
	// hypothetically be ext-touching (paper-thin shell); under-reserving here
	// has burned us before. Memory cost = 8 bytes * |solid|; for cs=4 gothic
	// that's a few MB even at maximum.
	frontier.reserve(lvl.solid.size());

	// Seed: kStateSolid cells with at least one face-neighbour classified
	// kStateExterior or kStateBlocked (a leak/seam cell -- the wall around
	// such a cell is part of the main shell, not disjoint).
	for (const auto &kv : lvl.solid) {
		const glm::ivec3 cell = kv.first;
		if (!lvl.inBounds(cell)) {
			continue;
		}
		const size_t idx = lvl.toIdx(cell);
		if (lvl.state[idx] != kStateSolid) {
			continue;
		}
		for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
			const uint8_t s = lvl.cellState(cell + off * lvl.cellSize);
			if (s == kStateExterior || s == kStateBlocked) {
				if (!isReached(idx)) {
					markReached(idx);
					frontier.push_back(idx);
				}
				break;
			}
		}
	}

	// BFS through kStateSolid face-adjacent neighbours.
	while (!frontier.empty()) {
		core::DynamicArray<size_t> next;
		// Per-round growth ceiling: each frontier cell has 6 face neighbours.
		next.reserve(frontier.size() * 6);
		for (size_t idx : frontier) {
			const glm::ivec3 cell = lvl.toCell(idx);
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = cell + off * lvl.cellSize;
				if (!lvl.inBounds(nb)) {
					continue;
				}
				const size_t nbIdx = lvl.toIdx(nb);
				if (lvl.state[nbIdx] != kStateSolid) {
					continue;
				}
				if (isReached(nbIdx)) {
					continue;
				}
				markReached(nbIdx);
				next.push_back(nbIdx);
			}
		}
		frontier = core::move(next);
	}

	// Mark unreached kStateSolid cells in out.bits -- these are disjoint.
	for (const auto &kv : lvl.solid) {
		if (!lvl.inBounds(kv.first)) {
			continue;
		}
		const size_t idx = lvl.toIdx(kv.first);
		if (lvl.state[idx] != kStateSolid) {
			continue;
		}
		if (!isReached(idx)) {
			out.bits[idx >> 3] |= (uint8_t)(1u << (idx & 7));
		}
	}
}

// Portable population count for a single byte. No compiler builtin so it works
// on MSVC too; this is only used by verbose-log accounting and is not hot.
static inline int popcount8(uint8_t b) {
	int c = 0;
	while (b != 0) {
		b &= (uint8_t)(b - 1);
		++c;
	}
	return c;
}

// Portable atomic byte OR. GCC/Clang (linux, mac, emscripten, msys2) provide the
// __atomic_or_fetch builtin; MSVC needs the interlocked intrinsic. Used so parallel
// chunk actions can mark the same shared mark-bit byte race-free.
static inline void printAtomicOrFetch(uint8_t *ptr, uint8_t mask) {
#if defined(_MSC_VER)
	_InterlockedOr8(reinterpret_cast<volatile char *>(ptr), (char)mask);
#else
	__atomic_or_fetch(ptr, mask, __ATOMIC_RELAXED);
#endif
}

// Counts set bits in the disjoint map -- used for the verbose-log line so we
// don't have to keep a parallel count during construction.
static uint64_t countDisjointCells(const DisjointInteriorMap &map) {
	uint64_t total = 0;
	for (uint8_t b : map.bits) {
		total += (uint64_t)popcount8(b);
	}
	return total;
}

// Faceclassify: tag every solid voxel as Outer / Inner / Thin / Buried at
// cs=1 precision, then recolor for visual inspection.
//
// Tag definition (from cs=1 face-neighbor inspection of the chunked classifier):
//   extAdj && intAdj  -> Thin    (magenta) -- 1-voxel wall between ext and a cavity
//   extAdj only       -> Outer   (orange)  -- exterior shell
//   intAdj only       -> Inner   (blue)    -- inner cavity surface
//   neither           -> Buried  (yellow)  -- no air face at cs=1
//
// The previous implementation walked rays voxel-by-voxel through a hierarchical
// cs=2 ext/int level stack. That mis-classified any wall voxel near a sub-cs=2
// feature (1-voxel U-tunnel, slit, etc.): cs=2 cells along the tunnel were
// kStateSolid (they contain wall material), so rays through tunnel air walked
// past kStateSolid lookups and only terminated when they hit another solid voxel,
// giving every wall voxel near the tunnel a Buried tag.
//
// The cs=1 chunked mark approach fixes that by reusing fillholes' chunked
// architecture: per chunk, build local cs=1 state from parent cs=2 + per-voxel
// solid checks, run ext-then-int BFS so the cs=1 state encodes the truth at
// voxel granularity, then face-neighbor inspect each solid voxel inside the
// chunk and atomic-OR an extAdj / intAdj bit for it. The tunnel air gets cs=1
// kStateExterior, the wall voxel beside it sees an ext face-neighbor, gets
// the extAdj bit, ends up Outer.
void runFaceClassify(SceneManager *sceneMgr, int minCellSize) {
	g_rssCapTripped.store(false);
	logRSS("entry to runFaceClassify");
	const uint64_t totalStartMs = core::TimeProvider::systemMillis();

	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "faceclassify", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int totalNodes = (int)nodes.size();
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	if (minCellSize <= 0 || minCellSize >= coarseCellSize) {
		minCellSize = 2;
	}
	if (shell.intCount == 0) {
		Log::warn("3dprint faceclassify: no enclosed interior at coarse scale (cs=%d) -- model is open or fully solid. "
				  "Without an interior region, every Inner/Thin tag would collapse into Outer (no int seeds). "
				  "Run '3dprint fillholes' to seal the model first if it should have an interior.",
				  coarseCellSize);
	}

	Level prev;
	buildRefinedShell(coarse, minCellSize, gridLower, gridUpper, nodes, "faceclassify", prev);

	// Per-node ext / int / saw mark bit arrays. 3 bits per voxel total.
	// Atomic OR (printAtomicOrFetch) keeps the parallel chunk action race-free.
	//
	// Saw is set for any voxel the chunk inspects as kStateSolid in its
	// local cs=1 state. The post-pass uses it to distinguish "not seen, leave
	// original color" from "seen but no marks, recolor as Buried". Voxels
	// in nodes that share a cs=2 cell with another node (typically fillholes
	// orphan plug voxels) are missed by chunkInitFromParent's single-owner
	// lookup and therefore never get saw set: leaving their original colour
	// is more useful than mis-tagging them yellow Buried.
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeExtAdjBits;
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeIntAdjBits;
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeLeakAdjBits;
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeSawBits;
	nodeExtAdjBits.resize((size_t)totalNodes);
	nodeIntAdjBits.resize((size_t)totalNodes);
	nodeLeakAdjBits.resize((size_t)totalNodes);
	nodeSawBits.resize((size_t)totalNodes);
	int64_t totalMarkBytes = 0;
	for (int i = 0; i < totalNodes; ++i) {
		const voxel::Region &r = nodes[(size_t)i].rv->region();
		const int64_t voxelCount = (int64_t)r.getWidthInVoxels()
								 * (int64_t)r.getHeightInVoxels()
								 * (int64_t)r.getDepthInVoxels();
		const int64_t byteCount = (voxelCount + 7) / 8;
		nodeExtAdjBits[(size_t)i].resize((size_t)byteCount);
		nodeIntAdjBits[(size_t)i].resize((size_t)byteCount);
		nodeLeakAdjBits[(size_t)i].resize((size_t)byteCount);
		nodeSawBits[(size_t)i].resize((size_t)byteCount);
		totalMarkBytes += byteCount * 4;
	}
	if (k3DPrintVerbose) {
		Log::info("3dprint faceclassify: mark bit arrays allocated -- %.1f MB across %d node(s) (ext+int+leak+saw)",
				  (double)totalMarkBytes / (1024.0 * 1024.0), totalNodes);
	}

	core::DynamicArray<size_t> chunkExtFront;
	core::DynamicArray<size_t> chunkIntFront;
	buildFrontiers(prev, chunkExtFront, chunkIntFront, /*wrapSolid=*/true);

	// requireOppositeFront=false: faceclassify visits every front cs=2 cell on
	// either shell so all wall voxels get classified, including inner walls
	// far from any exterior surface. seedChunks anchors on both ext-front
	// AND int-front cells (deduped via shared covered set).
	core::DynamicArray<ChunkBox> chunks = seedChunks(prev, chunkExtFront, chunkIntFront,
													  /*requireOppositeFront=*/false);
	if (k3DPrintVerbose) {
		Log::info("3dprint faceclassify: chunked cs=1 mark pass entering with %zu chunk(s)", chunks.size());
	}

	const Level &parent = prev;
	runChunkedCs1Driver(chunks, parent, nodes, "faceclassify", core::TimeProvider::systemMillis(),
		[&](Level &chunk, core::DynamicArray<glm::ivec3> &chunkHoles, int ci) {
			(void)chunkHoles;
			(void)ci;

			const int cs2 = parent.cellSize;
			const glm::ivec3 chunkLower = chunk.gridLower;
			const glm::ivec3 chunkUpper = chunkLower + glm::ivec3(chunk.dimX - 1, chunk.dimY - 1, chunk.dimZ - 1);
			const glm::ivec3 cs2Lower = toCellOrigin(chunkLower, cs2);
			const glm::ivec3 cs2Upper = toCellOrigin(chunkUpper, cs2);

			for (int cx = cs2Lower.x; cx <= cs2Upper.x; cx += cs2) {
				for (int cy = cs2Lower.y; cy <= cs2Upper.y; cy += cs2) {
					for (int cz = cs2Lower.z; cz <= cs2Upper.z; cz += cs2) {
						const glm::ivec3 cs2Cell(cx, cy, cz);
						if (parent.cellState(cs2Cell) != kStateSolid) {
							continue;
						}

						const int x0 = glm::max(cx, chunkLower.x);
						const int y0 = glm::max(cy, chunkLower.y);
						const int z0 = glm::max(cz, chunkLower.z);
						const int x1 = glm::min(cx + cs2 - 1, chunkUpper.x);
						const int y1 = glm::min(cy + cs2 - 1, chunkUpper.y);
						const int z1 = glm::min(cz + cs2 - 1, chunkUpper.z);

						for (int wx = x0; wx <= x1; ++wx) {
							for (int wy = y0; wy <= y1; ++wy) {
								for (int wz = z0; wz <= z1; ++wz) {
									const glm::ivec3 worldPos(wx, wy, wz);
									const size_t cellIdx = chunk.toIdx(worldPos);
									if (chunk.state[cellIdx] != kStateSolid) {
										continue;
									}

									// Compute neighbor adjacency once per voxel position.
									// kStateBlocked is a leak/seam contact (cs=1 cell where ext
									// reached first then int was about to enter). It is its own
									// signal -- voxels face-adjacent to it get the LeakAdj tag
									// rather than being folded into Outer/Inner/Thin -- so the
									// faceclassify visualisation distinguishes "real 1-voxel
									// wall between ext and a cavity" (Thin) from "voxel sits
									// next to a hole the model still has".
									bool extAdj = false;
									bool intAdj = false;
									bool leakAdj = false;
									for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
										const glm::ivec3 nb = worldPos + off;
										uint8_t s;
										if (chunk.inBounds(nb)) {
											s = chunk.state[chunk.toIdx(nb)];
										} else {
											s = parent.cellState(toCellOrigin(nb, cs2));
										}
										if (s == kStateExterior) {
											extAdj = true;
										} else if (s == kStateInterior) {
											intAdj = true;
										} else if (s == kStateBlocked) {
											leakAdj = true;
										}
										if (extAdj && intAdj && leakAdj) {
											break;
										}
									}

									// Set bits in EACH owning node's bit arrays (multi-node cs=2
									// cells: each owner gets its saw/extAdj/intAdj/leakAdj bit
									// for the voxels it actually owns at this position). Without
									// iterating extras, voxels in non-primary nodes silently
									// stay unclassified (saw=false -> faceclassify renders gray).
									forEachCellOwner(parent, cs2Cell, [&](uint64_t nodeIdxU) {
										const int nodeIdx = (int)nodeIdxU;
										const NodeInfo &ni = nodes[(size_t)nodeIdx];
										const voxel::Region &nodeRegion = ni.rv->region();
										const glm::ivec3 local = transformPoint(ni.invWorldMat, worldPos);
										if (!nodeRegion.containsPoint(local)) {
											return;
										}
										// Per-node voxel check: this node may not have a voxel
										// at this position even though some other owner does.
										// (chunk.state was set kStateSolid by chunkInit if ANY
										// owner had a voxel; that doesn't mean THIS owner does.)
										if (voxel::isAir(ni.rv->voxel(local).getMaterial())) {
											return;
										}
										const int64_t heightVox = (int64_t)nodeRegion.getHeightInVoxels();
										const int64_t depthVox  = (int64_t)nodeRegion.getDepthInVoxels();
										const int64_t vIdx = (int64_t)(local.x - nodeRegion.getLowerX())
																* heightVox * depthVox
															+ (int64_t)(local.y - nodeRegion.getLowerY())
																* depthVox
															+ (int64_t)(local.z - nodeRegion.getLowerZ());
										const int64_t byteIdx = vIdx >> 3;
										const uint8_t bitMask = (uint8_t)(1u << (vIdx & 7));

										printAtomicOrFetch(&nodeSawBits[(size_t)nodeIdx][(size_t)byteIdx],
														  bitMask);
										if (extAdj) {
											printAtomicOrFetch(&nodeExtAdjBits[(size_t)nodeIdx][(size_t)byteIdx],
															  bitMask);
										}
										if (intAdj) {
											printAtomicOrFetch(&nodeIntAdjBits[(size_t)nodeIdx][(size_t)byteIdx],
															  bitMask);
										}
										if (leakAdj) {
											printAtomicOrFetch(&nodeLeakAdjBits[(size_t)nodeIdx][(size_t)byteIdx],
															  bitMask);
										}
									});
								}
							}
						}
					}
				}
			}
		});
	logRSS("faceclassify: after chunked cs=1 mark pass");

	// Sequential per-node recolor based on (leak, ext, int) marks. Leak takes
	// priority -- a voxel face-adjacent to a kStateBlocked cs=1 cell is sitting
	// next to a hole the model still has, regardless of what else its other
	// face-neighbors look like, and the user wants to see those distinctly.
	static constexpr color::RGBA kOuterColor (255, 140,   0, 255);
	static constexpr color::RGBA kInnerColor ( 30, 120, 255, 255);
	static constexpr color::RGBA kThinColor  (220,   0, 220, 255);
	static constexpr color::RGBA kBuriedColor(255, 230,   0, 255);
	static constexpr color::RGBA kLeakAdjColor(255,  0,    0, 255);

	int64_t legendOuter = 0;
	int64_t legendInner = 0;
	int64_t legendThin = 0;
	int64_t legendBuried = 0;
	int64_t legendLeakAdj = 0;
	uint64_t totalClassified = 0;
	uint64_t nodesTouched = 0;
	for (int i = 0; i < totalNodes; ++i) {
		if (g_rssCapTripped.load(std::memory_order_relaxed)) {
			break;
		}
		const NodeInfo &ni = nodes[(size_t)i];
		const voxel::Region &nodeRegion = ni.rv->region();
		const core::DynamicArray<uint8_t> &extBits  = nodeExtAdjBits[(size_t)i];
		const core::DynamicArray<uint8_t> &intBits  = nodeIntAdjBits[(size_t)i];
		const core::DynamicArray<uint8_t> &leakBits = nodeLeakAdjBits[(size_t)i];
		const core::DynamicArray<uint8_t> &sawBits  = nodeSawBits[(size_t)i];
		const int64_t heightVox = (int64_t)nodeRegion.getHeightInVoxels();
		const int64_t depthVox  = (int64_t)nodeRegion.getDepthInVoxels();

		scenegraph::SceneGraphNode &node = graph.node(ni.nodeId);
		palette::Palette &pal = node.palette();
		uint8_t outerIdx = 0;
		uint8_t innerIdx = 0;
		uint8_t thinIdx = 0;
		uint8_t buriedIdx = 0;
		uint8_t leakAdjIdx = 0;
		pal.tryAdd(kOuterColor,   true, &outerIdx,   true);
		pal.tryAdd(kInnerColor,   true, &innerIdx,   true);
		pal.tryAdd(kThinColor,    true, &thinIdx,    true);
		pal.tryAdd(kBuriedColor,  true, &buriedIdx,  true);
		pal.tryAdd(kLeakAdjColor, true, &leakAdjIdx, true);

		voxel::RawVolumeWrapper wrapper(ni.rv);
		uint64_t classifiedThisNode = 0;
		for (int z = nodeRegion.getLowerZ(); z <= nodeRegion.getUpperZ(); ++z) {
			for (int y = nodeRegion.getLowerY(); y <= nodeRegion.getUpperY(); ++y) {
				for (int x = nodeRegion.getLowerX(); x <= nodeRegion.getUpperX(); ++x) {
					const voxel::Voxel &orig = ni.rv->voxel(x, y, z);
					if (voxel::isAir(orig.getMaterial())) {
						continue;
					}
					const int64_t vIdx = (int64_t)(x - nodeRegion.getLowerX()) * heightVox * depthVox
									   + (int64_t)(y - nodeRegion.getLowerY()) * depthVox
									   + (int64_t)(z - nodeRegion.getLowerZ());
					const int64_t byteIdx = vIdx >> 3;
					const uint8_t bitMask = (uint8_t)(1u << (vIdx & 7));
					const bool saw    = (sawBits[(size_t)byteIdx] & bitMask) != 0;
					if (!saw) {
						// Voxel never inspected by any chunk (typically a fillholes
						// orphan plug voxel in a node sharing a cs=2 cell with another
						// node, or a voxel deeper than the chunk reach). Leave its
						// original color rather than mis-tagging it Buried.
						continue;
					}
					const bool leakAdj = (leakBits[(size_t)byteIdx] & bitMask) != 0;
					const bool extAdj  = (extBits[(size_t)byteIdx]  & bitMask) != 0;
					const bool intAdj  = (intBits[(size_t)byteIdx]  & bitMask) != 0;

					uint8_t colorIdx;
					if (leakAdj) {
						colorIdx = leakAdjIdx;
						++legendLeakAdj;
					} else if (extAdj && intAdj) {
						colorIdx = thinIdx;
						++legendThin;
					} else if (extAdj) {
						colorIdx = outerIdx;
						++legendOuter;
					} else if (intAdj) {
						colorIdx = innerIdx;
						++legendInner;
					} else {
						colorIdx = buriedIdx;
						++legendBuried;
					}
					wrapper.setVoxel(x, y, z,
									 voxel::createVoxel(orig.getMaterial(), colorIdx,
														orig.getNormal(), orig.getFlags(),
														orig.getBoneIdx()));
					++classifiedThisNode;
				}
			}
		}
		if (classifiedThisNode > 0 && wrapper.dirtyRegion().isValid()) {
			sceneMgr->modified(ni.nodeId, wrapper.dirtyRegion());
			++nodesTouched;
			totalClassified += classifiedThisNode;
		}
	}

	const double totalSecs = (double)(core::TimeProvider::systemMillis() - totalStartMs) / 1000.0;
	if (g_rssCapTripped.load(std::memory_order_relaxed)) {
		Log::warn("3dprint faceclassify: aborted at %.1fs after %lu voxel(s) classified -- RSS cap (%g GB) tripped.",
				  totalSecs, (unsigned long)totalClassified, kHolefillRSSCapGB);
	} else {
		Log::info("3dprint faceclassify: classified %lu voxel(s) across %lu node(s) in %.1fs",
				  (unsigned long)totalClassified, (unsigned long)nodesTouched, totalSecs);
	}
	Log::info("3dprint faceclassify: color legend and voxel counts:");
	Log::info("  ORANGE  (255,140,  0) = outer surface  -- cs=1 face sees exterior space        : %ld voxels", (long)legendOuter);
	Log::info("  BLUE    ( 30,120,255) = inner surface  -- cs=1 face sees interior cavity only  : %ld voxels", (long)legendInner);
	Log::info("  MAGENTA (220,  0,220) = thin wall      -- cs=1 face sees both ext and interior : %ld voxels", (long)legendThin);
	Log::info("  YELLOW  (255,230,  0) = buried solid   -- no cs=1 air face at all              : %ld voxels", (long)legendBuried);
	Log::info("  RED     (255,  0,  0) = leak-adjacent  -- cs=1 face sees a kStateBlocked seam   : %ld voxels", (long)legendLeakAdj);
}

void runHoleMap(SceneManager *sceneMgr, int minCellSize) {
	// 3dprint mutating subcommands disable undo: the rewrite scope (potentially
	// every solid voxel adjacent to a leak across hundreds of nodes) is too
	// large for memento snapshots. Same precedent as Regrid.cpp.
	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "holemap", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int totalNodes = (int)nodes.size();
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	if (minCellSize <= 0 || minCellSize >= coarseCellSize) {
		minCellSize = 2;
	}
	if (shell.intCount == 0) {
		Log::warn("3dprint holemap: no enclosed interior at coarse scale -- model may not be sealed");
		return;
	}
	if (k3DPrintVerbose) {
		Log::info("3dprint holemap: coarse=%d, refining down to minCellSize=%d, nodes=%d",
				  coarseCellSize, minCellSize, totalNodes);
		Log::info("3dprint holemap: coarse grid %dx%dx%d -- exterior=%lu interior=%lu solid=%lu",
				  coarse.dimX, coarse.dimY, coarse.dimZ,
				  (unsigned long)countCellsByState(coarse, kStateExterior),
				  (unsigned long)shell.intCount,
				  (unsigned long)coarse.solid.size());
	}

	uint64_t numLevels = 0;
	for (int s = coarseCellSize / 2; s >= minCellSize; s /= 2) ++numLevels;
	ProgressTimer timer("holemap", numLevels, k3DPrintVerbose);
	uint64_t levelsProcessed = 0;

	core::DynamicArray<glm::ivec3> allHoleCells;
	allHoleCells.reserve(totalNodes);
	Level prev = coarse;

	for (int fineSize = coarseCellSize / 2; fineSize >= minCellSize; fineSize /= 2) {
		Level fine;
		fine.cellSize = fineSize;
		fine.initGrid(gridLower, gridUpper);

		const size_t stateCells = (size_t)fine.dimX * (size_t)fine.dimY * (size_t)fine.dimZ;
		if (stateCells > kDensePassMaxStateCells) {
			Log::warn("3dprint holemap: cs=%d grid %dx%dx%d = %.1f GB state would exceed the 16 GB budget. "
					  "Stopping the dense pass at the previously-reached cs=%d -- chunked cs=1 still runs.",
					  fineSize, fine.dimX, fine.dimY, fine.dimZ,
					  (double)stateCells / (1024.0 * 1024.0 * 1024.0),
					  fineSize * 2);
			break;
		}

		{
			ProgressTimer levelTimer("holemap solidHash", totalNodes, k3DPrintVerbose);
			buildSolidHash(fine, nodes, &levelTimer);
		}
		for (const auto &kv : fine.solid)
			if (fine.inBounds(kv.first)) fine.state[(size_t)fine.toIdx(kv.first)] = kStateSolid;
		if (k3DPrintVerbose) {
			Log::info("3dprint holemap: level %d solidHash done -- grid %dx%dx%d solid=%lu",
					  fineSize, fine.dimX, fine.dimY, fine.dimZ, (unsigned long)fine.solid.size());
		}

		initFineFromPrev(fine, prev);
		if (k3DPrintVerbose) {
			Log::info("3dprint holemap: level %d initFromPrev done", fineSize);
		}

		const core::DynamicArray<glm::ivec3> holeCells = runBidirectionalBFS(fine);
		timer.addVoxels((int64_t)holeCells.size());
		timer.tick(++levelsProcessed);

		if (k3DPrintVerbose) {
			Log::info("3dprint holemap: level %d BFS done -> %zu hole cell(s)", fineSize, holeCells.size());
		}

		for (const glm::ivec3 &h : holeCells) {
			allHoleCells.push_back(h);
		}

		prev = core::move(fine);
	}

	// Chunked cs=1 detection: walls adjacent to sub-cs=N leaks. Same algorithm
	// as fillholes, detection only -- no plugging. Each leak voxel's adjacent
	// solid voxels become red below.
	core::DynamicArray<glm::ivec3> chunkLeakVoxels;
	if (!g_rssCapTripped.load(std::memory_order_relaxed)) {
		core::DynamicArray<size_t> chunkExtFront, chunkIntFront;
		buildFrontiers(prev, chunkExtFront, chunkIntFront, /*wrapSolid=*/true);
		chunkLeakVoxels = detectChunkedCs1HoleVoxels(prev, nodes, chunkExtFront, chunkIntFront, "holemap");
	}

	if (allHoleCells.empty() && chunkLeakVoxels.empty()) {
		Log::info("3dprint holemap: model appears voxel-watertight (0 cs=%d holes, 0 cs=1 leaks)", minCellSize);
		return;
	}
	Log::info("3dprint holemap: %zu cs=N hole cell(s) + %zu cs=1 leak voxel(s)",
			  allHoleCells.size(), chunkLeakVoxels.size());

	// Collect unique solid cells adjacent to each hole cell and recolor them red.
	// No new solid voxels are placed -- only existing wall voxels are recolored.
	static constexpr color::RGBA kHoleColor(255, 0, 0, 255);
	const int finalCellSize = prev.cellSize;

	CellSet adjacentSolids;
	adjacentSolids.reserve(allHoleCells.size() * 6);
	for (const glm::ivec3 &holeCell : allHoleCells) {
		for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
			const glm::ivec3 adj = holeCell + off * finalCellSize;
			if (prev.solid.count(adj) > 0)
				adjacentSolids.insert(adj);
		}
	}

	uint64_t totalColored = 0;
	uint64_t nodesTouched = 0;
	// For each adjacent cs=N cell, recolor solid voxels in EVERY owning node's
	// volume. Multi-node cs=N cells (regrid edge boxes, fillholes orphan plug
	// nodes overlapping wall nodes) need each owner visited; previously only
	// the primary owner from prev.solid.find got recolored, leaving non-primary
	// node voxels in the original color even though they're adjacent to a leak.
	for (const glm::ivec3 &solidCell : adjacentSolids) {
		forEachCellOwner(prev, solidCell, [&](uint64_t nodeIdxU) {
			const NodeInfo &ni = nodes[(size_t)nodeIdxU];
			scenegraph::SceneGraphNode &node = graph.node(ni.nodeId);
			palette::Palette &pal = node.palette();

			uint8_t skipColorIdx = palette::PaletteColorNotFound;
			for (int dz = 0; dz < finalCellSize && skipColorIdx == palette::PaletteColorNotFound; ++dz)
				for (int dy = 0; dy < finalCellSize && skipColorIdx == palette::PaletteColorNotFound; ++dy)
					for (int dx = 0; dx < finalCellSize && skipColorIdx == palette::PaletteColorNotFound; ++dx) {
						const glm::ivec3 localPos = transformPoint(ni.invWorldMat,
						                                           solidCell + glm::ivec3(dx, dy, dz));
						if (!ni.rv->region().containsPoint(localPos)) continue;
						const voxel::Voxel &v = ni.rv->voxel(localPos);
						if (!voxel::isAir(v.getMaterial()))
							skipColorIdx = v.getColor();
					}

			uint8_t holeColorIdx = 0;
			const bool paletteChanged = pal.tryAdd(kHoleColor, true, &holeColorIdx, true, skipColorIdx);

			voxel::RawVolumeWrapper wrapper(ni.rv);
			for (int dz = 0; dz < finalCellSize; ++dz)
				for (int dy = 0; dy < finalCellSize; ++dy)
					for (int dx = 0; dx < finalCellSize; ++dx) {
						const glm::ivec3 localPos = transformPoint(ni.invWorldMat,
						                                           solidCell + glm::ivec3(dx, dy, dz));
						if (!ni.rv->region().containsPoint(localPos)) continue;
						const voxel::Voxel &v = ni.rv->voxel(localPos);
						if (voxel::isAir(v.getMaterial())) continue;
						wrapper.setVoxel(localPos, voxel::createVoxel(v.getMaterial(), holeColorIdx,
						                                               v.getNormal(), v.getFlags(), v.getBoneIdx()));
						++totalColored;
					}

			if (wrapper.dirtyRegion().isValid() || paletteChanged) {
				const voxel::Region dirtyRgn = wrapper.dirtyRegion().isValid()
					? wrapper.dirtyRegion() : ni.rv->region();
				sceneMgr->modified(ni.nodeId, dirtyRgn);
				++nodesTouched;
			}
		});
	}

	// cs=1 leak voxels: recolor each face-adjacent solid voxel (1 voxel per face,
	// not finalCellSize^3). De-dup by spatial position so a voxel adjacent to
	// multiple leak voxels is recolored once.
	uint64_t cs1Colored = 0;
	uint64_t cs1NodesTouched = 0;
	if (!chunkLeakVoxels.empty()) {
		std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>> adjacentSolidVoxels;
		adjacentSolidVoxels.reserve(chunkLeakVoxels.size() * 6);
		for (const glm::ivec3 &leakVoxel : chunkLeakVoxels) {
			for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
				const glm::ivec3 nb = leakVoxel + off;
				if (isSolidAt(nb, prev, nodes)) {
					adjacentSolidVoxels.insert(nb);
				}
			}
		}

		// Group by owning node so we open RawVolumeWrapper once per node, write
		// many voxels, then call sceneMgr->modified once with the dirty region.
		// Iterate ALL owners per cs=2 cell (primary + extras) so voxels in
		// non-primary nodes get the red recolor too.
		std::unordered_map<int, core::DynamicArray<glm::ivec3>> perNodeVoxels;
		perNodeVoxels.reserve(nodes.size());
		for (const glm::ivec3 &solidVoxel : adjacentSolidVoxels) {
			const glm::ivec3 cs2Cell = toCellOrigin(solidVoxel, prev.cellSize);
			forEachCellOwner(prev, cs2Cell, [&](uint64_t nodeIdxU) {
				perNodeVoxels[(int)nodeIdxU].push_back(solidVoxel);
			});
		}

		for (auto &kv : perNodeVoxels) {
			const NodeInfo &ni = nodes[(size_t)kv.first];
			scenegraph::SceneGraphNode &node = graph.node(ni.nodeId);
			palette::Palette &pal = node.palette();
			uint8_t holeColorIdx = 0;
			const bool paletteChanged = pal.tryAdd(kHoleColor, true, &holeColorIdx, true);

			voxel::RawVolumeWrapper wrapper(ni.rv);
			for (const glm::ivec3 &solidVoxel : kv.second) {
				const glm::ivec3 localPos = transformPoint(ni.invWorldMat, solidVoxel);
				if (!ni.rv->region().containsPoint(localPos)) {
					continue;
				}
				const voxel::Voxel &v = ni.rv->voxel(localPos);
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				wrapper.setVoxel(localPos, voxel::createVoxel(v.getMaterial(), holeColorIdx,
															   v.getNormal(), v.getFlags(), v.getBoneIdx()));
				++cs1Colored;
			}
			if (wrapper.dirtyRegion().isValid() || paletteChanged) {
				const voxel::Region dirtyRgn = wrapper.dirtyRegion().isValid()
					? wrapper.dirtyRegion() : ni.rv->region();
				sceneMgr->modified(ni.nodeId, dirtyRgn);
				++cs1NodesTouched;
			}
		}
	}

	Log::info("3dprint holemap: colored %lu cs=N voxel(s) + %lu cs=1 voxel(s) across %lu node(s) red",
			  (unsigned long)totalColored,
			  (unsigned long)cs1Colored,
			  (unsigned long)(nodesTouched + cs1NodesTouched));
}

// Apply hole fills to the model. Cell-level holes are expanded to finalCellSize^3
// voxels per cell; voxel-level holes (from voxel-plug pass) are placed as single
// voxels. Each hole position is mapped to the nearest existing node via
// `lookup.solid`; positions with no adjacent existing-node owner go into orphan
// bins, which become new model nodes. Modifies the scene graph (`sceneMgr->modified`,
// `nodeResize`, `moveNodeToSceneGraph`).
// (FillStats is defined earlier so chunked-cs=1 pass can reference it.)
static FillStats applyHoleFills(
		const core::DynamicArray<glm::ivec3> &holeCells,
		const core::DynamicArray<glm::ivec3> &holeVoxels,
		const Level &lookup,
		core::DynamicArray<NodeInfo> &nodes,
		SceneManager *sceneMgr,
		int finalCellSize,
		const color::RGBA &fillColor,
		const char *orphanNodeName) {

	FillStats stats;
	if (holeCells.empty() && holeVoxels.empty()) return stats;

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();

	struct FillPos {
		int nodeIdx;
		glm::ivec3 localPos;
	};
	const size_t fillUpperBound = holeCells.size() * (size_t)(finalCellSize * finalCellSize * finalCellSize)
								  + holeVoxels.size();
	core::DynamicArray<FillPos> fills;
	fills.reserve(fillUpperBound);

	// Worst case: every fill is an orphan. Reserve to same upper bound so push_back
	// during the per-cell/per-voxel loop never reallocs.
	core::DynamicArray<glm::ivec3> orphanVoxels;
	orphanVoxels.reserve(fillUpperBound);

	// For each candidate adjacent cs=N cell, examine ALL nodes that own voxels
	// in it (primary + extras) when picking the best plug owner. Without this,
	// a leak voxel landing inside a region claimed by a non-primary node would
	// orphan unnecessarily because the search only saw the primary node, whose
	// region might not contain the leak position.
	auto considerOwnersForPlug = [&](const glm::ivec3 &lookupCell,
									 const glm::ivec3 &targetWorldPos,
									 int &bestNodeIdx, int &bestDistSq) {
		forEachCellOwner(lookup, lookupCell, [&](uint64_t nodeIdxU) {
			const int ni = (int)nodeIdxU;
			const glm::ivec3 local = transformPoint(nodes[(size_t)ni].invWorldMat, targetWorldPos);
			const glm::ivec3 lo = nodes[(size_t)ni].rv->region().getLowerCorner();
			const glm::ivec3 hi = nodes[(size_t)ni].rv->region().getUpperCorner();
			const glm::ivec3 clamped = glm::clamp(local, lo, hi);
			const glm::ivec3 d = local - clamped;
			const int distSq = d.x * d.x + d.y * d.y + d.z * d.z;
			if (distSq < bestDistSq) {
				bestDistSq = distSq;
				bestNodeIdx = ni;
			}
		});
	};

	for (const glm::ivec3 &holeCell : holeCells) {
		int bestNodeIdx = -1;
		int bestDistSq = INT_MAX;
		for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
			const glm::ivec3 adjCell = holeCell + off * finalCellSize;
			considerOwnersForPlug(adjCell, holeCell, bestNodeIdx, bestDistSq);
		}
		if (bestNodeIdx < 0) {
			for (int dz = 0; dz < finalCellSize; ++dz)
				for (int dy = 0; dy < finalCellSize; ++dy)
					for (int dx = 0; dx < finalCellSize; ++dx)
						orphanVoxels.push_back(holeCell + glm::ivec3(dx, dy, dz));
			continue;
		}
		for (int dz = 0; dz < finalCellSize; ++dz) {
			for (int dy = 0; dy < finalCellSize; ++dy) {
				for (int dx = 0; dx < finalCellSize; ++dx) {
					const glm::ivec3 worldPos = holeCell + glm::ivec3(dx, dy, dz);
					fills.push_back({bestNodeIdx,
									 transformPoint(nodes[(size_t)bestNodeIdx].invWorldMat, worldPos)});
				}
			}
		}
	}

	// Per-voxel owner search is independent across plugs, so run it in parallel
	// with a sentinel-slot scheme: each thread writes to a fixed index in
	// `fillsParallel`, using nodeIdx=-1 to mark an orphan. After the parallel
	// region we compact serially. This is a hot path for thicken (millions of
	// plugs); fillholes' typical workload (thousands) sees a small overhead.
	//
	// Heartbeat: log every ~3 s so the user sees progress instead of a freeze.
	struct ParallelFill {
		int nodeIdx;
		glm::ivec3 localPos;
	};
	const int holeVoxelCount = (int)holeVoxels.size();
	core::DynamicArray<ParallelFill> fillsParallel;
	fillsParallel.resize((size_t)holeVoxelCount);
	const uint64_t ownerPhaseStartMs = core::TimeProvider::systemMillis();
	if (holeVoxelCount > 0) {
		std::atomic<int64_t> processedAtomic{0};
		std::atomic<uint64_t> lastLogMs{ownerPhaseStartMs};
		app::for_parallel(0, holeVoxelCount, [&](int start, int end) {
			for (int i = start; i < end; ++i) {
				const glm::ivec3 &holeVoxel = holeVoxels[(size_t)i];
				const glm::ivec3 holeCell = toCellOrigin(holeVoxel, finalCellSize);
				int bestNodeIdx = -1;
				int bestDistSq = INT_MAX;
				considerOwnersForPlug(holeCell, holeVoxel, bestNodeIdx, bestDistSq);
				for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
					considerOwnersForPlug(holeCell + off * finalCellSize, holeVoxel, bestNodeIdx, bestDistSq);
				}
				if (bestNodeIdx < 0) {
					fillsParallel[(size_t)i] = {-1, glm::ivec3(0)};
				} else {
					fillsParallel[(size_t)i] = {bestNodeIdx,
												transformPoint(nodes[(size_t)bestNodeIdx].invWorldMat, holeVoxel)};
				}
			}
			const int64_t done = processedAtomic.fetch_add(end - start, std::memory_order_relaxed) + (end - start);
			const uint64_t now = core::TimeProvider::systemMillis();
			uint64_t prevMs = lastLogMs.load(std::memory_order_relaxed);
			if (now - prevMs >= 3000u && lastLogMs.compare_exchange_strong(prevMs, now)) {
				fprintf(stderr, "[applyHoleFills] owner-search %lld/%d (%.1f%%) elapsed=%.1fs\n",
						(long long)done, holeVoxelCount,
						100.0 * (double)done / (double)holeVoxelCount,
						(double)(now - ownerPhaseStartMs) / 1000.0);

			}
		});
	}
	const double ownerPhaseSecs = (double)(core::TimeProvider::systemMillis() - ownerPhaseStartMs) / 1000.0;
	if (holeVoxelCount > 1000000 || ownerPhaseSecs > 1.0) {
		Log::info("3dprint applyHoleFills: parallel owner search done in %.1fs (%d plug voxel(s))",
				  ownerPhaseSecs, holeVoxelCount);
	}

	// Compact: split into fills + orphans. The serial pass is O(N) and runs
	// at memory-bandwidth speed -- not the bottleneck.
	const uint64_t compactStartMs = core::TimeProvider::systemMillis();
	for (int i = 0; i < holeVoxelCount; ++i) {
		const ParallelFill &pf = fillsParallel[(size_t)i];
		if (pf.nodeIdx < 0) {
			orphanVoxels.push_back(holeVoxels[(size_t)i]);
		} else {
			fills.push_back({pf.nodeIdx, pf.localPos});
		}
	}
	fillsParallel.release();
	const double compactSecs = (double)(core::TimeProvider::systemMillis() - compactStartMs) / 1000.0;
	if (holeVoxelCount > 1000000 || compactSecs > 1.0) {
		Log::info("3dprint applyHoleFills: compact done in %.1fs (fills=%zu orphans=%zu)",
				  compactSecs, fills.size(), orphanVoxels.size());
	}

	const uint64_t sortStartMs = core::TimeProvider::systemMillis();
	// core::DynamicArray::sort is insertion sort (O(N^2)) -- unusable at the
	// thicken scale (45M plugs would take days). Use std::sort = introsort
	// (O(N log N)) on raw pointers (DynamicArray's iterator lacks
	// iterator_traits so std::sort can't deduce difference_type from it).
	if (!fills.empty()) {
		FillPos *fillsBegin = &fills[0];
		FillPos *fillsEnd = fillsBegin + fills.size();
		std::sort(fillsBegin, fillsEnd,
				  [](const FillPos &a, const FillPos &b) { return a.nodeIdx < b.nodeIdx; });
	}
	const double sortSecs = (double)(core::TimeProvider::systemMillis() - sortStartMs) / 1000.0;
	if (fills.size() > 1000000 || sortSecs > 1.0) {
		Log::info("3dprint applyHoleFills: sort done in %.1fs (%zu fills)", sortSecs, fills.size());
	}

	const uint64_t writeStartMs = core::TimeProvider::systemMillis();
	std::atomic<uint64_t> writeLastLogMs{writeStartMs};
	int fillIdx = 0;
	while (fillIdx < (int)fills.size()) {
		const int curNode = fills[(size_t)fillIdx].nodeIdx;
		const int nodeId = nodes[(size_t)curNode].nodeId;

		voxel::Region fillRegion = nodes[(size_t)curNode].rv->region();
		for (int fi = fillIdx; fi < (int)fills.size() && fills[(size_t)fi].nodeIdx == curNode; ++fi) {
			fillRegion.accumulate(fills[(size_t)fi].localPos);
		}
		if (fillRegion != nodes[(size_t)curNode].rv->region()) {
			sceneMgr->nodeResize(nodeId, fillRegion);
			nodes[(size_t)curNode].rv = sceneMgr->volume(nodeId);
			++stats.nodesResized;
		}

		scenegraph::SceneGraphNode &node = graph.node(nodeId);
		palette::Palette &pal = node.palette();
		uint8_t fillColorIdx = 0;
		pal.tryAdd(fillColor, true, &fillColorIdx, true);
		voxel::RawVolumeWrapper wrapper(nodes[(size_t)curNode].rv);
		while (fillIdx < (int)fills.size() && fills[(size_t)fillIdx].nodeIdx == curNode) {
			const glm::ivec3 &localPos = fills[(size_t)fillIdx].localPos;
			if (voxel::isAir(nodes[(size_t)curNode].rv->voxel(localPos).getMaterial())) {
				wrapper.setVoxel(localPos, voxel::createVoxel(voxel::VoxelType::Generic, fillColorIdx));
				++stats.totalFilled;
			}
			++fillIdx;
		}
		if (wrapper.dirtyRegion().isValid()) {
			sceneMgr->modified(nodeId, wrapper.dirtyRegion());
			++stats.nodesTouched;
		}
		// Heartbeat every ~3 s for the per-node write loop.
		const uint64_t now = core::TimeProvider::systemMillis();
		uint64_t prevMs = writeLastLogMs.load(std::memory_order_relaxed);
		if (now - prevMs >= 3000u && writeLastLogMs.compare_exchange_strong(prevMs, now)) {
			fprintf(stderr, "[applyHoleFills] writes %d/%d (%.1f%%) elapsed=%.1fs\n",
					fillIdx, (int)fills.size(),
					100.0 * (double)fillIdx / (double)fills.size(),
					(double)(now - writeStartMs) / 1000.0);

		}
	}
	const double writeSecs = (double)(core::TimeProvider::systemMillis() - writeStartMs) / 1000.0;
	if (fills.size() > 1000000 || writeSecs > 1.0) {
		Log::info("3dprint applyHoleFills: per-node writes done in %.1fs (filled=%lu touched=%lu resized=%lu)",
				  writeSecs, (unsigned long)stats.totalFilled,
				  (unsigned long)stats.nodesTouched, (unsigned long)stats.nodesResized);
	}

	if (!orphanVoxels.empty()) {
		const uint64_t orphanStartMs = core::TimeProvider::systemMillis();
		static constexpr int kBinSize = 64;
		using Bin = core::DynamicArray<glm::ivec3>;
		std::unordered_map<glm::ivec3, Bin, glm::hash<glm::ivec3>> bins;
		// Estimate: in the worst case every orphan goes to its own bin.
		// In practice orphans cluster, so this is over-reserve but bounded.
		bins.reserve(orphanVoxels.size());
		for (const glm::ivec3 &p : orphanVoxels) {
			Bin &b = bins[toCellOrigin(p, kBinSize)];
			// First-time creation: reserve 1024 entries so push_back doesn't grow
			// from default 0/1 capacity. Tight bin: ~few voxels. Worst-case bin:
			// 64^3 = 262144 voxels, but that'd require a fully-orphan cs=64 cell
			// which is extraordinary.
			if (b.capacity() == 0) b.reserve(1024);
			b.push_back(p);
		}
		for (auto &kv : bins) {
			const Bin &voxels = kv.second;
			glm::ivec3 mn = voxels[0];
			glm::ivec3 mx = voxels[0];
			for (const glm::ivec3 &p : voxels) {
				mn = glm::min(mn, p);
				mx = glm::max(mx, p);
			}
			const voxel::Region region(glm::ivec3(0), mx - mn);
			voxel::RawVolume *vol = new voxel::RawVolume(region);
			palette::Palette pal;
			uint8_t fillColorIdx = 0;
			pal.tryAdd(fillColor, true, &fillColorIdx, true);
			const voxel::Voxel fillVoxel = voxel::createVoxel(voxel::VoxelType::Generic, fillColorIdx);
			for (const glm::ivec3 &p : voxels) {
				vol->setVoxelUnsafe(p - mn, fillVoxel);
				++stats.orphanFilled;
			}
			scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
			newNode.setVolume(vol);
			newNode.setName(orphanNodeName);
			newNode.setPalette(pal);
			scenegraph::SceneGraphTransform transform;
			transform.setWorldTranslation(glm::vec3(mn));
			newNode.setTransform(0, transform);
			sceneMgr->moveNodeToSceneGraph(newNode, 0);
			++stats.orphanNodes;
		}
		const double orphanSecs = (double)(core::TimeProvider::systemMillis() - orphanStartMs) / 1000.0;
		if (orphanVoxels.size() > 100000 || orphanSecs > 1.0) {
			Log::info("3dprint applyHoleFills: orphan binning done in %.1fs (%zu voxels into %d node(s))",
					  orphanSecs, orphanVoxels.size(), stats.orphanNodes);
		}
	}

	return stats;
}

void runHoleFill(SceneManager *sceneMgr, int minCellSize) {
	g_rssCapTripped.store(false);
	logRSS("entry to runHoleFill");
	// 3dprint mutating subcommands disable undo. Plug voxels span hundreds of
	// nodes in the gothic pipeline; memento snapshots would compress and store
	// each affected node, multi-GB RAM and minutes of work for a non-reversible
	// pipeline stage. Same precedent as Regrid.cpp.
	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());
	const uint64_t totalStartMs = core::TimeProvider::systemMillis();
	// Aggregated across cs=N main loop and chunked cs=1 pass for the end-of-run
	// summary line. Currently only the chunked pass writes fills (cs=N fill is
	// skipped when kRunChunkedCs1Pass=true) but tracking both keeps the summary
	// correct if the toggle flips.
	FillStats totalStats;
	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "fillholes", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int totalNodes = (int)nodes.size();
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	// Default deepest dense level: cs=2. The main loop runs cs/2 ... cs=2 (or
	// stops earlier if the kMaxStateCells memory cap trips). The chunked cs=1
	// pass after the loop plugs sub-cs=2 leaks at voxel precision regardless.
	if (minCellSize <= 0 || minCellSize >= coarseCellSize) {
		minCellSize = 2;
	}
	if (shell.intCount == 0) {
		Log::warn("3dprint fillholes: no enclosed interior at coarse scale (cs=%d, retried down to cs=8). The model "
				  "has no sealed cavity -- exterior flood reached every air cell. Either the model is genuinely "
				  "solid/open, or it has a coarse-scale gap (>= 8 voxels wide) that lets ext flow through. Aborting.",
				  coarseCellSize);
		return;
	}
	if (k3DPrintVerbose) {
		Log::info("3dprint holefill: coarse interior: %lu cell(s)", (unsigned long)shell.intCount);
	}
	logRSS("after coarse buildInterior");

	uint64_t numLevels = 0;
	for (int s = coarseCellSize / 2; s >= minCellSize; s /= 2) ++numLevels;
	ProgressTimer timer("holefill", numLevels + 1, k3DPrintVerbose); // +1 for fill phase
	uint64_t levelsProcessed = 0;

	core::DynamicArray<glm::ivec3> allHoleCells;
	allHoleCells.reserve(totalNodes);
	Level prev = coarse;

	for (int fineSize = coarseCellSize / 2; fineSize >= minCellSize; fineSize /= 2) {
		Level fine;
		fine.cellSize = fineSize;
		fine.initGrid(gridLower, gridUpper);

		const size_t stateCells = (size_t)fine.dimX * (size_t)fine.dimY * (size_t)fine.dimZ;
		if (stateCells > kDensePassMaxStateCells) {
			Log::warn("3dprint fillholes: cs=%d grid %dx%dx%d = %.1f GB state would exceed the 16 GB dense-pass budget. "
					  "Stopping the dense refinement at the previously-reached cs=%d. The chunked cs=1 pass still runs "
					  "(it allocates per-chunk, not globally) -- expect slightly less precise results since the chunked "
					  "pass uses cs=%d as parent classification. To go finer on a higher-RAM machine, raise the cap "
					  "(kDensePassMaxStateCells in FaceClassify.cpp); to go finer on this machine, run '3dprint regrid <larger_cs>' "
					  "to produce a smaller bbox first.",
					  fineSize, fine.dimX, fine.dimY, fine.dimZ,
					  (double)stateCells / (1024.0 * 1024.0 * 1024.0),
					  fineSize * 2, fineSize * 2);
			break;
		}

		// Per-level convergence: build state from current model (which may have
		// fills from prior levels or prior sub-iterations), run BFS, fill any
		// holes, repeat until a sub-iteration produces 0 new holes. No iter cap.
		//
		// Optimization: only sub-iter 1 builds state from scratch (solidHash +
		// initFineFromPrev). Subsequent sub-iters reuse the state from the prior
		// BFS+fill: prev-level classification didn't change (so initFineFromPrev
		// would produce identical output), and the cells just filled act as
		// walls for BFS via their kStateBlocked classification (which BFS
		// skips, equivalent to kStateSolid for expansion purposes). This saves
		// ~15 s per sub-iter at cs=2 (5 s init + 10 s solidHash).
		const uint64_t levelStartMs = core::TimeProvider::systemMillis();
		int subIter = 0;
		uint64_t totalLevelFilled = 0;
		while (true) {
			++subIter;
			const uint64_t subStartMs = core::TimeProvider::systemMillis();

			// Sub-iter 2+: state was preserved from sub-iter 1 (we skip
			// solidHash + initFineFromPrev). Sub-iter 1's BFS ran to completion
			// (frontier exhausted at end). Since fills only add walls (which
			// BFS already skips via kStateBlocked) and don't create new ext/int
			// sources, sub-iter 2's BFS is guaranteed to find an empty frontier
			// and 0 holes. Skip the buildFrontiers re-scan (~10 s at cs=2).
			if (subIter > 1) {
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d skipped -- shells unchanged since previous BFS, convergence implicit",
							  fineSize, subIter);
				}
				break;
			}

			if (subIter == 1) {
				fine.solid.clear();
				fine.state.assign(stateCells, kStateEmpty);

				const uint64_t hashStartMs = core::TimeProvider::systemMillis();
				{
					ProgressTimer levelTimer("holefill solidHash", totalNodes, k3DPrintVerbose);
					buildSolidHash(fine, nodes, &levelTimer);
				}
				for (const auto &kv : fine.solid)
					if (fine.inBounds(kv.first)) fine.state[(size_t)fine.toIdx(kv.first)] = kStateSolid;
				const double hashSecs = (double)(core::TimeProvider::systemMillis() - hashStartMs) / 1000.0;
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d solidHash done in %.1fs -- solid=%zu",
							  fineSize, subIter, hashSecs, fine.solid.size());
				}

				const uint64_t initStartMs = core::TimeProvider::systemMillis();
				initFineFromPrev(fine, prev);
				const double initSecs = (double)(core::TimeProvider::systemMillis() - initStartMs) / 1000.0;
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d initFromPrev done in %.1fs",
							  fineSize, subIter, initSecs);
				}
				if (checkRSSCap("after initFineFromPrev")) {
					break;
				}
			} else if (k3DPrintVerbose) {
				Log::info("3dprint holefill: cs=%d sub-iter %d reusing state from prior BFS (skipped solidHash + initFineFromPrev)",
						  fineSize, subIter);
			}

			const uint64_t bfsStartMs = core::TimeProvider::systemMillis();
			const core::DynamicArray<glm::ivec3> holeCells = runBidirectionalBFS(fine);
			const double bfsSecs = (double)(core::TimeProvider::systemMillis() - bfsStartMs) / 1000.0;
			if (k3DPrintVerbose) {
				Log::info("3dprint holefill: cs=%d sub-iter %d BFS done in %.1fs -> %zu hole cell(s)",
						  fineSize, subIter, bfsSecs, holeCells.size());
			}
			if (checkRSSCap("after runBidirectionalBFS")) {
				break;
			}

			if (holeCells.empty()) {
				const double subSecs = (double)(core::TimeProvider::systemMillis() - subStartMs) / 1000.0;
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d converged (no holes) in %.1fs",
							  fineSize, subIter, subSecs);
				}
				break;
			}

			// Deferred fill: only fill at the deepest level (minCellSize). At
			// intermediate levels the BFS classification still propagates to
			// finer levels via initFineFromPrev (the kStateBlocked markers
			// don't propagate, but the surrounding ext/int expansion does), so
			// finer levels rediscover the holes at higher resolution. Filling
			// only at the deepest level produces the tightest plug volumes:
			// at cs=128 a fill is 128^3 = 2M voxels per hole; at cs=2 it's 8.
			if (fineSize > minCellSize) {
				const double subSecs = (double)(core::TimeProvider::systemMillis() - subStartMs) / 1000.0;
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d found %zu holes -- fill DEFERRED to cs=%d (sub-iter total %.1fs)",
							  fineSize, subIter, holeCells.size(), minCellSize, subSecs);
				}
				// No fill -> no model change -> re-iterating at this cs would
				// find the same holes again. Break to advance to cs/2.
				break;
			}

			// Deepest level: skip fill if chunked cs=1 will rediscover the same
			// holes at voxel precision. The kStateBlocked markers from the BFS
			// stay in fine.state and are inherited by chunkInitFromParent (they
			// translate to kStateEmpty -- probe zone -- so cs=1 BFS expands ext
			// and int through them and meets at the actual leak voxels).
			//
			// Saves the 8-voxel-per-cs=2-cell over-fill that produced visible
			// blocky plugs in earlier runs.
			if (kRunChunkedCs1Pass) {
				const double subSecs = (double)(core::TimeProvider::systemMillis() - subStartMs) / 1000.0;
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d found %zu holes -- fill SKIPPED (chunked cs=1 will plug at voxel precision; sub-iter total %.1fs)",
							  fineSize, subIter, holeCells.size(), subSecs);
				}
				break;
			}

			const uint64_t fillStartMs = core::TimeProvider::systemMillis();
			const core::DynamicArray<glm::ivec3> emptyVoxels;
			FillStats st = applyHoleFills(holeCells, emptyVoxels, fine, nodes, sceneMgr, fineSize);
			const double fillSecs = (double)(core::TimeProvider::systemMillis() - fillStartMs) / 1000.0;
			totalLevelFilled += st.totalFilled + st.orphanFilled;
			totalStats.totalFilled  += st.totalFilled;
			totalStats.orphanFilled += st.orphanFilled;
			totalStats.orphanNodes  += st.orphanNodes;
			totalStats.nodesResized += st.nodesResized;
			totalStats.nodesTouched += st.nodesTouched;
			const double subSecs = (double)(core::TimeProvider::systemMillis() - subStartMs) / 1000.0;
			if (k3DPrintVerbose) {
				Log::info("3dprint holefill: cs=%d sub-iter %d filled %lu voxel(s) (+%lu orphan in %d new node(s)) in %.1fs (sub-iter total %.1fs)",
						  fineSize, subIter,
						  (unsigned long)st.totalFilled,
						  (unsigned long)st.orphanFilled, st.orphanNodes,
						  fillSecs, subSecs);
			}

			// Break if BFS reported holes but applyHoleFills couldn't actually
			// add any solid voxels (every "hole" is already solid in the model).
			// Without this the loop is infinite -- BFS keeps re-detecting the
			// same already-filled cells because state is reset each sub-iter.
			if (st.totalFilled == 0 && st.orphanFilled == 0) {
				if (k3DPrintVerbose) {
					Log::info("3dprint holefill: cs=%d sub-iter %d found %zu holes but 0 actual fills -- treating as converged",
							  fineSize, subIter, holeCells.size());
				}
				break;
			}
		}
		const double levelSecs = (double)(core::TimeProvider::systemMillis() - levelStartMs) / 1000.0;
		if (k3DPrintVerbose) {
			Log::info("3dprint holefill: cs=%d CONVERGED after %d sub-iter(s) in %.1fs (%lu total voxels filled at this level)",
					  fineSize, subIter, levelSecs, (unsigned long)totalLevelFilled);
		}
		timer.addVoxels((int64_t)0);
		timer.tick(++levelsProcessed);

		prev = core::move(fine);
	}

	// All cell-level holes have been filled per-level by the loop above.
	//
	// Sub-cell leaks (1-voxel slits and narrow gaps inside cs=2 solid cells)
	// remain. The chunked cs=1 pass handles them: it walks the model's outer
	// shell (ext-front cells from the cs=2 classification), places a chunk
	// around each that captures the matching int-front, and runs the same
	// sequential ext-then-int BFS at cs=1 inside the chunk. Holes detected
	// inside chunks are 1 voxel each -- no splat from BFS wall-crawl, no
	// shell-adjacency filter, no min-cut analysis needed.
	//
	// Self-test: flip the file-level kRunChunkedCs1Pass=false to run only the
	// global cs=2 path. Hole/fill counts after this point should match the
	// pre-refactor pre-plug-pass output exactly.

	if (kRunChunkedCs1Pass && !g_rssCapTripped.load(std::memory_order_relaxed)) {
		core::DynamicArray<size_t> chunkExtFront, chunkIntFront;
		const uint64_t frontierStartMs = core::TimeProvider::systemMillis();
		buildFrontiers(prev, chunkExtFront, chunkIntFront, /*wrapSolid=*/true);
		const double frontierSecs = (double)(core::TimeProvider::systemMillis() - frontierStartMs) / 1000.0;
		if (k3DPrintVerbose) {
			Log::info("3dprint holefill: ext/int front for chunked cs=1 built in %.1fs (ext=%zu int=%zu)",
					  frontierSecs, chunkExtFront.size(), chunkIntFront.size());
		}

		const FillStats chunkStats = runChunkedCs1Pass(prev, nodes, sceneMgr, chunkExtFront, chunkIntFront);
		totalStats.totalFilled  += chunkStats.totalFilled;
		totalStats.orphanFilled += chunkStats.orphanFilled;
		totalStats.orphanNodes  += chunkStats.orphanNodes;
		totalStats.nodesResized += chunkStats.nodesResized;
		totalStats.nodesTouched += chunkStats.nodesTouched;
	} else if (!kRunChunkedCs1Pass) {
		Log::info("3dprint holefill: chunked cs=1 pass DISABLED (self-test mode)");
	}
	logRSS("after chunked cs=1 pass");

	timer.tick(++levelsProcessed);

	// Always-on summary so the user sees a single result line even with
	// k3DPrintVerbose=false. Distinguishes:
	//   - leaks were plugged: tells the user how many.
	//   - 0 plugs after a real chunked pass: model is voxel-watertight.
	//   - aborted via RSS cap: warns about that explicitly.
	const double totalSecs = (double)(core::TimeProvider::systemMillis() - totalStartMs) / 1000.0;
	const uint64_t totalPlugged = totalStats.totalFilled + totalStats.orphanFilled;
	if (g_rssCapTripped.load(std::memory_order_relaxed)) {
		Log::warn("3dprint fillholes: aborted at %.1fs after %lu voxel(s) plugged -- RSS cap (%g GB) tripped. "
				  "The model may still have unfilled leaks. Free memory or raise kHolefillRSSCapGB and rerun.",
				  totalSecs, (unsigned long)totalPlugged, kHolefillRSSCapGB);
	} else if (totalPlugged == 0) {
		Log::info("3dprint fillholes: model appears voxel-watertight -- 0 leaks found in %.1fs.",
				  totalSecs);
	} else {
		Log::info("3dprint fillholes: plugged %lu voxel(s) (+%lu orphan voxel(s) in %d new node(s)) in %.1fs",
				  (unsigned long)totalStats.totalFilled,
				  (unsigned long)totalStats.orphanFilled,
				  totalStats.orphanNodes,
				  totalSecs);
	}
}

// Soft cap on the per-node keep-bit memory. The total budget is bounded
// (1 bit per voxel, e.g. ~460 MB on the gothic 6000^3 model) but this
// constant lets us emit a warning before allocation if a future scene
// somehow blows past the expectation -- safer than silently allocating
// many GB of bookkeeping.
static constexpr double kErodeKeepBitsSoftCapGB = 8.0;

// Erode: keep only voxels face-adjacent to the global exterior at cs=1
// precision. Removes Inner-only and Buried voxels so the model collapses
// to a 1-voxel exterior shell with all internal cavities merged.
//
// Pipeline:
//   1. Coarse setup mirrored from runHoleFill (cell-size inference, world
//      bbox, coarse buildExterior + buildInteriorAllSeeds). Aborts with a
//      warning if no enclosed interior is found at coarse scale (model is
//      open; user should run '3dprint fillholes' first).
//   2. Dense refinement chain to minCellSize: each level builds solidHash,
//      runs initFineFromPrev + runBidirectionalBFS to refine the ext/int
//      classification. No hole detection or filling. The 16 GB dense state
//      cap is honoured -- if the next level would exceed it, refinement
//      stops and the chunked pass uses the previously-reached cs as parent.
//   3. Per-node keep bit array (1 bit per voxel) is allocated. Atomic OR
//      (printAtomicOrFetch) is used so multiple chunks can mark the same
//      voxel concurrently without races.
//   4. Chunked cs=1 driver runs with seedChunks(requireOppositeFront=false) so
//      every ext-front cs=2 cell is visited (not just walls with a matching
//      int-front in range). Per chunk, after the cs=1 BFS populates ext/int
//      classification, the action walks solid voxels and ORs the keep bit
//      for any voxel with a kStateExterior cs=1 face-neighbor (ext rays
//      crossing the chunk boundary fall back to parent cs=2 classification).
//   5. Sequential post-pass iterates each node's voxels and clears any solid
//      voxel whose keep bit was not set. dirty regions accumulate per node
//      and are reported via sceneMgr->modified.
//
// Undo recording is suppressed for the duration -- the rewrite scope makes
// memento snapshots prohibitive at this model size.
void runErode(SceneManager *sceneMgr, int minCellSize) {
	g_rssCapTripped.store(false);
	logRSS("entry to runErode");
	const uint64_t totalStartMs = core::TimeProvider::systemMillis();

	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());

	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "erode", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int totalNodes = (int)nodes.size();
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	if (minCellSize <= 0 || minCellSize >= coarseCellSize) {
		minCellSize = 2;
	}
	if (shell.intCount == 0) {
		Log::warn("3dprint erode: no enclosed interior at coarse scale (cs=%d). The model is open or fully solid -- "
				  "every air cell reaches exterior at coarse scale. Erode would either remove every voxel "
				  "(no shell to keep) or do nothing meaningful. Run '3dprint fillholes' first to seal the model. "
				  "Aborting.",
				  coarseCellSize);
		return;
	}

	// Disjoint-interior bit map (cs=N grid, N = kErodeDisjointCellSize).
	// Built by the refinement-chain hook at cs=N; survives until the
	// post-pass which uses it for O(1) per-voxel lookups via isDisjoint.
	DisjointInteriorMap disjointMap;
	if (minCellSize > kErodeDisjointCellSize) {
		Log::warn("3dprint erode: minCellSize=%d skips the cs=%d level -- disjoint-interior detection disabled "
				  "(it hooks the refinement chain at cs=%d). Pass a smaller minCellSize to enable.",
				  minCellSize, kErodeDisjointCellSize, kErodeDisjointCellSize);
	}

	Level prev;
	buildRefinedShell(coarse, minCellSize, gridLower, gridUpper, nodes, "erode", prev,
		[&disjointMap](const Level &lvl, int cellSize) {
			if (cellSize != kErodeDisjointCellSize) {
				return;
			}
			const uint64_t startMs = core::TimeProvider::systemMillis();
			detectDisjointInteriorCells(lvl, disjointMap);
			const double secs = (double)(core::TimeProvider::systemMillis() - startMs) / 1000.0;
			const uint64_t cellCount = countDisjointCells(disjointMap);
			if (k3DPrintVerbose || cellCount > 0) {
				Log::info("3dprint erode: cs=%d disjoint detection found %lu cell(s) in %.1fs",
						  cellSize, (unsigned long)cellCount, secs);
			}
		});

	// Per-node keep + saw bit arrays (1 bit per voxel each). Saw is set when
	// the chunked action actually inspected a voxel as kStateSolid in the
	// chunk's local cs=1 state; keep is the subset with a kStateExterior
	// face-neighbor at cs=1.
	//
	// Why two bits: chunkInitFromParent finds the owning node via
	// parent.solid.find, which is one-to-one (CellHash::emplace keeps the
	// first node when several share a cs=2 cell). Voxels in a different node
	// that ALSO has solid voxels in the same cs=2 cell -- typically fillholes
	// orphan plug voxels living in newly-created nodes that overlap original
	// wall nodes -- are invisible to the chunk's per-voxel walk: their cs=1
	// position appears kStateEmpty (then kStateExterior/Interior/Blocked
	// after BFS), never kStateSolid. Without a saw bit, these voxels would
	// be removed by the post-pass simply because no chunk set their keep bit
	// -- reopening every leak fillholes had just plugged.
	//
	// Conservative rule in the post-pass: remove only when (saw && !keep).
	// Voxels never seen by any chunk are LEFT ALONE -- this preserves orphan
	// plug voxels and any voxels deeper than the chunk reach. Deep buried
	// voxels in walls thicker than the chunk reach also stay; that is a
	// less-aggressive trade-off than the user's "remove everything not on
	// the outer shell" goal but it's correctness-preserving (no new holes
	// created) and acceptable until/unless we move to a multi-node owner
	// hash that fixes the underlying lookup.
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeKeepBits;
	core::DynamicArray<core::DynamicArray<uint8_t>> nodeSawBits;
	nodeKeepBits.resize((size_t)totalNodes);
	nodeSawBits.resize((size_t)totalNodes);
	int64_t totalKeepBytes = 0;
	for (int i = 0; i < totalNodes; ++i) {
		const voxel::Region &r = nodes[(size_t)i].rv->region();
		const int64_t voxelCount = (int64_t)r.getWidthInVoxels()
								 * (int64_t)r.getHeightInVoxels()
								 * (int64_t)r.getDepthInVoxels();
		const int64_t byteCount = (voxelCount + 7) / 8;
		// resize(N) emplaces uint8_t{} = 0 for each element, so the bits start cleared.
		nodeKeepBits[(size_t)i].resize((size_t)byteCount);
		nodeSawBits[(size_t)i].resize((size_t)byteCount);
		totalKeepBytes += byteCount * 2;
	}
	const double keepBitsGB = (double)totalKeepBytes / (1024.0 * 1024.0 * 1024.0);
	if (keepBitsGB > kErodeKeepBitsSoftCapGB) {
		Log::warn("3dprint erode: keep+saw bit arrays use %.1f GB (soft cap %.1f GB). "
				  "Operation continues but you may run low on memory; consider running on a machine with more RAM.",
				  keepBitsGB, kErodeKeepBitsSoftCapGB);
	}
	if (k3DPrintVerbose) {
		Log::info("3dprint erode: keep+saw bit arrays allocated -- %.1f MB across %d node(s)",
				  (double)totalKeepBytes / (1024.0 * 1024.0), totalNodes);
	}

	// Build chunked-pass frontiers at the deepest dense level.
	core::DynamicArray<size_t> chunkExtFront;
	core::DynamicArray<size_t> chunkIntFront;
	buildFrontiers(prev, chunkExtFront, chunkIntFront, /*wrapSolid=*/true);

	// requireOppositeFront=false: erode visits every front cs=2 cell on either
	// shell. seedChunks anchors on both ext-front AND int-front cells, so
	// even purely interior walls (between two cavities, no exterior surface
	// within chunk reach) get classified -- their saw bits get set, and the
	// existing saw && !keep logic decides what stays.
	core::DynamicArray<ChunkBox> chunks = seedChunks(prev, chunkExtFront, chunkIntFront,
													  /*requireOppositeFront=*/false);
	if (k3DPrintVerbose) {
		Log::info("3dprint erode: chunked cs=1 driver entering with %zu chunk(s) (parent cs=%d)",
				  chunks.size(), prev.cellSize);
	}

	// Capture by reference: nodeKeepBits is the shared sink, prev/nodes are
	// read-only. The action runs in the parallel context; concurrent OR-marks
	// on the same byte are safe via printAtomicOrFetch.
	const Level &parent = prev;
	runChunkedCs1Driver(chunks, parent, nodes, "erode", core::TimeProvider::systemMillis(),
		[&](Level &chunk, core::DynamicArray<glm::ivec3> &chunkHoles, int ci) {
			(void)chunkHoles;
			(void)ci;

			const int cs2 = parent.cellSize;
			const glm::ivec3 chunkLower = chunk.gridLower;
			const glm::ivec3 chunkUpper = chunkLower + glm::ivec3(chunk.dimX - 1, chunk.dimY - 1, chunk.dimZ - 1);
			const glm::ivec3 cs2Lower = toCellOrigin(chunkLower, cs2);
			const glm::ivec3 cs2Upper = toCellOrigin(chunkUpper, cs2);

			// Outer loop is cs=2 cells. For each kStateSolid cs=2 cell we iterate
			// every node owning voxels in it (primary in parent.solid + extras in
			// parent.extraSolid) -- otherwise voxels in non-primary nodes never
			// get their saw bit set and the post-pass leaves them alone instead
			// of erode-classifying them.
			for (int cx = cs2Lower.x; cx <= cs2Upper.x; cx += cs2) {
				for (int cy = cs2Lower.y; cy <= cs2Upper.y; cy += cs2) {
					for (int cz = cs2Lower.z; cz <= cs2Upper.z; cz += cs2) {
						const glm::ivec3 cs2Cell(cx, cy, cz);
						if (parent.cellState(cs2Cell) != kStateSolid) {
							continue;
						}

						const int x0 = glm::max(cx, chunkLower.x);
						const int y0 = glm::max(cy, chunkLower.y);
						const int z0 = glm::max(cz, chunkLower.z);
						const int x1 = glm::min(cx + cs2 - 1, chunkUpper.x);
						const int y1 = glm::min(cy + cs2 - 1, chunkUpper.y);
						const int z1 = glm::min(cz + cs2 - 1, chunkUpper.z);

						for (int wx = x0; wx <= x1; ++wx) {
							for (int wy = y0; wy <= y1; ++wy) {
								for (int wz = z0; wz <= z1; ++wz) {
									const glm::ivec3 worldPos(wx, wy, wz);
									const size_t cellIdx = chunk.toIdx(worldPos);
									if (chunk.state[cellIdx] != kStateSolid) {
										continue;
									}

									// Compute extAdj once per voxel position.
									// kStateBlocked counts as ext-reachable: BFS classified
									// the cell as "ext reached me, then int tried to enter"
									// -- a leak/contact point. The wall voxel beside it must
									// stay or the leak widens.
									bool extAdj = false;
									for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
										const glm::ivec3 nb = worldPos + off;
										uint8_t s;
										if (chunk.inBounds(nb)) {
											s = chunk.state[chunk.toIdx(nb)];
										} else {
											s = parent.cellState(toCellOrigin(nb, cs2));
										}
										if (s == kStateExterior || s == kStateBlocked) {
											extAdj = true;
											break;
										}
									}

									// Set saw / keep bits in EACH owning node's bit array.
									// Multi-node cs=2 cells: each owner gets bits for the
									// voxels it actually owns at this position.
									forEachCellOwner(parent, cs2Cell, [&](uint64_t nodeIdxU) {
										const int nodeIdx = (int)nodeIdxU;
										const NodeInfo &ni = nodes[(size_t)nodeIdx];
										const voxel::Region &nodeRegion = ni.rv->region();
										const glm::ivec3 local = transformPoint(ni.invWorldMat, worldPos);
										if (!nodeRegion.containsPoint(local)) {
											return;
										}
										// Per-node check: this owner may not actually have a
										// voxel here even though some other owner does.
										if (voxel::isAir(ni.rv->voxel(local).getMaterial())) {
											return;
										}
										const int64_t heightVox = (int64_t)nodeRegion.getHeightInVoxels();
										const int64_t depthVox  = (int64_t)nodeRegion.getDepthInVoxels();
										const int64_t vIdx = (int64_t)(local.x - nodeRegion.getLowerX())
															 * heightVox * depthVox
													   + (int64_t)(local.y - nodeRegion.getLowerY())
														 * depthVox
													   + (int64_t)(local.z - nodeRegion.getLowerZ());
										const int64_t byteIdx = vIdx >> 3;
										const uint8_t bitMask = (uint8_t)(1u << (vIdx & 7));
										printAtomicOrFetch(&nodeSawBits[(size_t)nodeIdx][(size_t)byteIdx],
														  bitMask);
										if (extAdj) {
											printAtomicOrFetch(&nodeKeepBits[(size_t)nodeIdx][(size_t)byteIdx],
															  bitMask);
										}
									});
								}
							}
						}
					}
				}
			}
		});
	logRSS("erode: after chunked cs=1 mark pass");

	// Sequential per-node post-pass: clear any solid voxel whose keep bit
	// was not set. RawVolume writes in the wrapper aren't documented as
	// thread-safe, so we keep this serial. Per-node iteration cost scales
	// with node voxel count; amortised across nodes this is O(total voxels).
	uint64_t totalRemoved = 0;
	uint64_t nodesTouched = 0;
	for (int i = 0; i < totalNodes; ++i) {
		if (g_rssCapTripped.load(std::memory_order_relaxed)) {
			break;
		}
		const NodeInfo &ni = nodes[(size_t)i];
		const voxel::Region &nodeRegion = ni.rv->region();
		const core::DynamicArray<uint8_t> &keepBits = nodeKeepBits[(size_t)i];
		const core::DynamicArray<uint8_t> &sawBits  = nodeSawBits[(size_t)i];
		const int64_t heightVox = (int64_t)nodeRegion.getHeightInVoxels();
		const int64_t depthVox  = (int64_t)nodeRegion.getDepthInVoxels();
		voxel::RawVolumeWrapper wrapper(ni.rv);
		uint64_t removedThisNode = 0;
		uint64_t removedDisjointThisNode = 0;
		for (int z = nodeRegion.getLowerZ(); z <= nodeRegion.getUpperZ(); ++z) {
			for (int y = nodeRegion.getLowerY(); y <= nodeRegion.getUpperY(); ++y) {
				for (int x = nodeRegion.getLowerX(); x <= nodeRegion.getUpperX(); ++x) {
					const voxel::Voxel &v = ni.rv->voxel(x, y, z);
					if (voxel::isAir(v.getMaterial())) {
						continue;
					}

					// Disjoint-interior override: if the voxel's cs=N
					// (= kErodeDisjointCellSize) cell was flagged disjoint by the
					// refinement-chain hook, remove it regardless of saw/keep.
					// Disjoint cells by definition only see int or solid neighbours,
					// which is the user's "remove debris from inside the model"
					// criterion. Lookup is O(1) into the bit array; the worldPos
					// transform is the only per-voxel cost added.
					if (disjointMap.populated) {
						const glm::ivec3 worldPos = transformPoint(ni.worldMat, glm::ivec3(x, y, z));
						if (disjointMap.isDisjoint(worldPos)) {
							wrapper.setVoxel(x, y, z, voxel::Voxel());
							++removedDisjointThisNode;
							continue;
						}
					}

					const int64_t vIdx = (int64_t)(x - nodeRegion.getLowerX()) * heightVox * depthVox
									   + (int64_t)(y - nodeRegion.getLowerY()) * depthVox
									   + (int64_t)(z - nodeRegion.getLowerZ());
					const int64_t byteIdx = vIdx >> 3;
					const uint8_t bitMask = (uint8_t)(1u << (vIdx & 7));
					const bool saw  = (sawBits[(size_t)byteIdx]  & bitMask) != 0;
					const bool keep = (keepBits[(size_t)byteIdx] & bitMask) != 0;
					// Remove only if the chunk actually inspected this voxel as solid
					// AND found no kStateExterior face-neighbor at cs=1. Voxels never
					// seen by any chunk -- typically fillholes orphan plug voxels in
					// nodes that share a cs=2 cell with another node, or voxels deeper
					// in solid material than the chunk reach -- are left alone, which
					// preserves the watertightness fillholes established.
					if (saw && !keep) {
						wrapper.setVoxel(x, y, z, voxel::Voxel());
						++removedThisNode;
					}
				}
			}
		}
		removedThisNode += removedDisjointThisNode;
		if (removedThisNode > 0 && wrapper.dirtyRegion().isValid()) {
			sceneMgr->modified(ni.nodeId, wrapper.dirtyRegion());
			++nodesTouched;
			totalRemoved += removedThisNode;
		}
	}

	const double totalSecs = (double)(core::TimeProvider::systemMillis() - totalStartMs) / 1000.0;
	if (g_rssCapTripped.load(std::memory_order_relaxed)) {
		Log::warn("3dprint erode: aborted at %.1fs after %lu voxel(s) removed -- RSS cap (%g GB) tripped. "
				  "Result is partial: some non-shell voxels remain. Free memory and rerun.",
				  totalSecs, (unsigned long)totalRemoved, kHolefillRSSCapGB);
	} else {
		Log::info("3dprint erode: removed %lu voxel(s) across %lu node(s) in %.1fs -- kept exterior shell only.",
				  (unsigned long)totalRemoved, (unsigned long)nodesTouched, totalSecs);
	}
}

// Wall-thickening pass. For each cs=1 voxel V with state==kStateInterior that
// has at least one face-neighbor with state==kStateSolid, plug V with a solid
// voxel (kThickenColor). One invocation adds a single 1-voxel layer to the
// inside of every interior-facing wall. The user can rerun for thicker walls.
//
// Two geometric cases this single rule covers:
//   1. "Air gap inside a Solid cs=2 cell, on the interior side" -- when a wall
//      has < 2 voxel-thick coverage on the interior face of its containing
//      cs=2 cell, the empty cs=1 voxel is BFS-classified Interior and lies
//      face-flush against the wall voxel. We plug it.
//   2. "Wall is flush with the cs=2 boundary, no air inside the Solid cell" --
//      the adjacent Interior cs=2 cell's nearest cs=1 voxel sits face-adjacent
//      to a Solid wall voxel across the cs=2 boundary. We plug there instead.
//
// Pre-conditions, in order of importance:
//   - Run after '3dprint fillholes' so the model is voxel-watertight and the
//     cs=2 ext/int classification doesn't leak (otherwise thicken would lay
//     voxels along leak paths instead of just on real interior walls).
//   - Run before '3dprint erode' if both are wanted -- erode would strip the
//     newly added interior layer.
//
// Undo is disabled inside this function: same precedent as fillholes/erode --
// the rewrite scope is too large for memento snapshots.
void runThicken(SceneManager *sceneMgr, int minCellSize) {
	g_rssCapTripped.store(false);
	logRSS("entry to runThicken");
	const uint64_t totalStartMs = core::TimeProvider::systemMillis();

	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());

	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "thicken", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	if (minCellSize <= 0 || minCellSize >= coarseCellSize) {
		minCellSize = 2;
	}
	if (shell.intCount == 0) {
		Log::warn("3dprint thicken: no enclosed interior at coarse scale (cs=%d). The model has no sealed cavity, "
				  "so there is no interior wall surface to thicken. Run '3dprint fillholes' first to seal the "
				  "model. Aborting.",
				  coarseCellSize);
		return;
	}

	Level prev;
	buildRefinedShell(coarse, minCellSize, gridLower, gridUpper, nodes, "thicken", prev);

	// Build cs=2 ext/int frontiers for the chunked cs=1 driver. wrapSolid=true
	// so the frontiers are solid-cell-adjacent and ext/int both contribute.
	core::DynamicArray<size_t> chunkExtFront;
	core::DynamicArray<size_t> chunkIntFront;
	const uint64_t frontierStartMs = core::TimeProvider::systemMillis();
	buildFrontiers(prev, chunkExtFront, chunkIntFront, /*wrapSolid=*/true);
	const double frontierSecs = (double)(core::TimeProvider::systemMillis() - frontierStartMs) / 1000.0;
	if (k3DPrintVerbose) {
		Log::info("3dprint thicken: ext/int front built in %.1fs (ext=%zu int=%zu)",
				  frontierSecs, chunkExtFront.size(), chunkIntFront.size());
	}

	// requireOppositeFront=false: thicken visits every interior wall surface,
	// not just leak-contact points. A chunk anchored on an int-front cell deep
	// inside a thick cavity does NOT need an ext-front in its bbox -- the cs=1
	// BFS still classifies Interior cs=1 voxels correctly from the intFrontier
	// alone, and the plug rule (Interior cs=1 with face-neighbor Solid) doesn't
	// need ext-side disambiguation. Same rationale as erode and faceclassify;
	// only fillholes requires both shells (it's specifically detecting ext<->int
	// contact). Skipping chunks with requireOppositeFront=true left isolated
	// gaps wherever the wall was thicker than the expanded chunk reach.
	core::DynamicArray<ChunkBox> chunks = seedChunks(prev, chunkExtFront, chunkIntFront,
													  /*requireOppositeFront=*/false);
	if (chunks.empty()) {
		Log::info("3dprint thicken: no thickenable interior wall surface found (no chunks seeded).");
		return;
	}

	const Level &parent = prev;
	core::DynamicArray<core::DynamicArray<glm::ivec3>> perChunkPlugs;
	perChunkPlugs.resize(chunks.size());
	std::atomic<int64_t> totalRawPlugs{0};
	std::atomic<int64_t> totalBlockedPlugs{0};

	runChunkedCs1Driver(chunks, parent, nodes, "thicken", core::TimeProvider::systemMillis(),
		[&](Level &chunk, core::DynamicArray<glm::ivec3> &chunkHoles, int ci) {
			(void)chunkHoles;
			// Plug rule: cs=1 cell V with state in {kStateInterior, kStateBlocked}
			// that has at least one face-neighbor with state==kStateSolid.
			//
			// Why Blocked: the cs=1 BFS marks a cell Blocked when ext-bfs and
			// int-bfs meet inside it. Globally (post-fillholes) the model is
			// voxel-watertight so there are no real leaks -- but the chunk's
			// LOCAL view can still produce a phantom leak where its bbox cuts
			// through a wall corner. The wall continues outside the bbox; the
			// chunk sees only one side of it; ext-bfs walks around the missing
			// piece into int territory. The int-bulk cell it touches first
			// gets marked Blocked instead of staying Interior, so the strict
			// "state == Interior" rule used to skip it. Including Blocked
			// recovers those cells. The cs=2 cell filter (only Interior or
			// Solid-with-Interior-neighbor cells are scanned) keeps this from
			// over-plugging genuinely exterior cells.
			//
			// Skip Empty (probe-zone-unreachable; sealed bubbles) and Exterior.
			//
			// Three perf rules govern the loop layout below:
			//   1. Confine the scan to the chunk's CORE (bbox shrunk by
			//      kHolefillChunkOverlap each side). The overlap zone is shared
			//      with adjacent chunks' cores, so emitting plugs there
			//      duplicates work 5-8x. seedChunks already guarantees every
			//      front cs=2 cell is inside some chunk's core, so plug
			//      candidates near those cells are too.
			//   2. Bound the cs=1 scan by parent cs=2 cells of interest. Plug
			//      candidates only live in cs=2 cells classified Interior or
			//      Solid-with-an-Interior-face-neighbor (after fillholes the
			//      model is voxel-watertight, so other Solid cells provably
			//      have no Interior cs=1 voxels inside them).
			//   3. State array is X-major (toIdx = ix*dimY*dimZ + iy*dimZ + iz),
			//      so the cache-friendly inner axis is z. Per cs=2 cell we
			//      iterate at most 2x2x2 cs=1 voxels in (x,y,z) order; each
			//      cs=1 cell uses chunk.toIdx() so the strides are correct.
			core::DynamicArray<glm::ivec3> &out = perChunkPlugs[(size_t)ci];
			// Upper-bound reserve: surface area of the chunk (cs=1 voxels).
			const size_t surfaceUpperBound = (size_t)(chunk.dimX * chunk.dimY
													  + chunk.dimY * chunk.dimZ
													  + chunk.dimX * chunk.dimZ) * 2;
			out.reserve(surfaceUpperBound);

			const int cs2 = parent.cellSize;
			const glm::ivec3 chunkLower = chunk.gridLower;
			const glm::ivec3 chunkUpper = chunkLower + glm::ivec3(chunk.dimX - 1, chunk.dimY - 1, chunk.dimZ - 1);
			// Core = bbox shrunk by kHolefillChunkOverlap each side. The chunk
			// state array is still populated over the FULL bbox by chunkInitFromParent
			// (so face-neighbour reads near the core boundary remain in-bbox); we
			// only restrict the EMIT range.
			const glm::ivec3 coreLower = chunkLower + glm::ivec3(kHolefillChunkOverlap);
			const glm::ivec3 coreUpper = chunkUpper - glm::ivec3(kHolefillChunkOverlap);
			const glm::ivec3 cs2Lower = toCellOrigin(coreLower, cs2);
			const glm::ivec3 cs2Upper = toCellOrigin(coreUpper, cs2);
			int64_t localCount = 0;
			int64_t localBlocked = 0;
			for (int cx = cs2Lower.x; cx <= cs2Upper.x; cx += cs2) {
				for (int cy = cs2Lower.y; cy <= cs2Upper.y; cy += cs2) {
					for (int cz = cs2Lower.z; cz <= cs2Upper.z; cz += cs2) {
						const glm::ivec3 cs2Cell(cx, cy, cz);
						const uint8_t parentState = parent.cellState(cs2Cell);

						// Skip only cells that provably contain no INT cs=1 voxels:
						// EXT (bulk-filled Exterior) and EMPTY/BLOCKED (probe-zone
						// unreachable at cs=2). INT cells are bulk-filled Interior
						// so they always have plug candidates at the boundary.
						// SOLID cells contain a mix of cs=1 SOLID voxels and probe-
						// zone empty voxels; the probe zone can be reached by the
						// chunk's cs=1 BFS as Interior via *chains* of face-adjacent
						// Solid cells (probe-zone connects across cs=2 face
						// boundaries). So the cs=2 face-neighbor check we used to
						// do here was too strict: a Solid cell whose face-neighbors
						// at cs=2 are all SOLID/EXT can still have INT cs=1 voxels
						// inside it, propagated via the chain. Drop that filter and
						// let the per-cs=1-voxel rule handle correctness. Buried
						// Solid cells cost a little extra iteration but emit
						// nothing (their cs=1 voxels are SOLID or EXT, never INT),
						// so it's a constant-factor slowdown only.
						if (parentState != kStateInterior && parentState != kStateSolid) {
							continue;
						}

						const int x0 = glm::max(cx, coreLower.x);
						const int y0 = glm::max(cy, coreLower.y);
						const int z0 = glm::max(cz, coreLower.z);
						const int x1 = glm::min(cx + cs2 - 1, coreUpper.x);
						const int y1 = glm::min(cy + cs2 - 1, coreUpper.y);
						const int z1 = glm::min(cz + cs2 - 1, coreUpper.z);
						for (int wx = x0; wx <= x1; ++wx) {
							for (int wy = y0; wy <= y1; ++wy) {
								for (int wz = z0; wz <= z1; ++wz) {
									const glm::ivec3 worldPos(wx, wy, wz);
									const uint8_t voxState = chunk.state[chunk.toIdx(worldPos)];
									if (voxState != kStateInterior && voxState != kStateBlocked) {
										continue;
									}
									bool hasSolidNeighbor = false;
									for (const glm::ivec3 &off : voxel::arrayPathfinderFaces) {
										const glm::ivec3 nb = worldPos + off;
										if (!chunk.inBounds(nb)) {
											continue;
										}
										if (chunk.state[chunk.toIdx(nb)] == kStateSolid) {
											hasSolidNeighbor = true;
											break;
										}
									}
									if (hasSolidNeighbor) {
										out.push_back(worldPos);
										++localCount;
										if (voxState == kStateBlocked) {
											++localBlocked;
										}
									}
								}
							}
						}
					}
				}
			}
			totalRawPlugs.fetch_add(localCount, std::memory_order_relaxed);
			totalBlockedPlugs.fetch_add(localBlocked, std::memory_order_relaxed);
		});
	logRSS("thicken: after chunked cs=1 scan");
	if (totalBlockedPlugs.load(std::memory_order_relaxed) > 0) {
		Log::info("3dprint thicken: raw plug breakdown -- %lld total, %lld from Blocked cells (recovered chunk-local phantom-leak voxels)",
				  (long long)totalRawPlugs.load(std::memory_order_relaxed),
				  (long long)totalBlockedPlugs.load(std::memory_order_relaxed));
	}

	// Aggregate + dedup (overlapping chunks may detect the same plug position).
	// Even with chunk-core confinement, cells on a core boundary land in
	// adjacent chunks too, so dedup is still needed -- but the ratio drops
	// from ~5x to ~2x.
	const uint64_t dedupStartMs = core::TimeProvider::systemMillis();
	std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>> uniquePlugs;
	const size_t rawTotal = (size_t)totalRawPlugs.load(std::memory_order_relaxed);
	uniquePlugs.reserve(rawTotal);
	core::DynamicArray<glm::ivec3> allPlugs;
	allPlugs.reserve(rawTotal);
	for (const core::DynamicArray<glm::ivec3> &chunkList : perChunkPlugs) {
		for (const glm::ivec3 &v : chunkList) {
			if (uniquePlugs.insert(v).second) {
				allPlugs.push_back(v);
			}
		}
	}
	const double dedupSecs = (double)(core::TimeProvider::systemMillis() - dedupStartMs) / 1000.0;
	Log::info("3dprint thicken: %zu raw -> %zu unique plug voxel(s) from %zu chunk(s) in %.1fs",
			  rawTotal, allPlugs.size(), chunks.size(), dedupSecs);
	// Free the per-chunk lists and the dedup hash before applyHoleFills runs --
	// each holds tens of millions of entries and we don't need them anymore.
	perChunkPlugs.release();
	{
		std::unordered_set<glm::ivec3, glm::hash<glm::ivec3>> empty;
		empty.swap(uniquePlugs);
	}
	logRSS("thicken: after dedup");

	FillStats stats;
	if (!allPlugs.empty()) {
		// Distinct from fillholes' green so a model run through both passes is
		// visually decomposable. Picked to read as "interior thickening".
		static constexpr color::RGBA kThickenColor(40, 120, 255, 255);
		const core::DynamicArray<glm::ivec3> emptyCells;
		const uint64_t applyStartMs = core::TimeProvider::systemMillis();
		stats = applyHoleFills(emptyCells, allPlugs, parent, nodes, sceneMgr,
							   /*finalCellSize=*/parent.cellSize,
							   /*fillColor=*/kThickenColor,
							   /*orphanNodeName=*/"thicken_orphan");
		const double applySecs = (double)(core::TimeProvider::systemMillis() - applyStartMs) / 1000.0;
		Log::info("3dprint thicken: applyHoleFills done in %.1fs (filled=%lu orphan=%lu nodes=%d)",
				  applySecs, (unsigned long)stats.totalFilled,
				  (unsigned long)stats.orphanFilled, stats.orphanNodes);
	}

	const double totalSecs = (double)(core::TimeProvider::systemMillis() - totalStartMs) / 1000.0;
	const uint64_t totalAdded = stats.totalFilled + stats.orphanFilled;
	if (g_rssCapTripped.load(std::memory_order_relaxed)) {
		Log::warn("3dprint thicken: aborted at %.1fs after %lu voxel(s) added -- RSS cap (%g GB) tripped. "
				  "Result is partial. Free memory and rerun.",
				  totalSecs, (unsigned long)totalAdded, kHolefillRSSCapGB);
	} else if (totalAdded == 0) {
		Log::info("3dprint thicken: no thickenable interior wall surface found in %.1fs.", totalSecs);
	} else {
		Log::info("3dprint thicken: added %lu voxel(s) (+%lu orphan voxel(s) in %d new node(s)) in %.1fs",
				  (unsigned long)stats.totalFilled,
				  (unsigned long)stats.orphanFilled,
				  stats.orphanNodes,
				  totalSecs);
	}
}

void runDebugFrontier(SceneManager *sceneMgr, int cellSize) {
	const uint64_t fnStart = core::TimeProvider::systemMillis();

	// Build the coarse exterior/interior shell through the SAME path that
	// fillholes/holemap/faceclassify/erode/thicken use, so what debugfrontier
	// paints is exactly what those commands classify -- including the auto-fallback
	// halving that a tight-cropped single node needs (no interior at the modal
	// width). buildCoarseShell fills `nodes`, picks the coarse cell size, builds
	// the grid and classifies it (solid/exterior/interior).
	core::DynamicArray<NodeInfo> nodes;
	Level coarse;
	const CoarseShellResult shell = buildCoarseShell(sceneMgr, "debugfrontier", nodes, coarse);
	if (!shell.ok) {
		return;
	}
	const int totalNodes = (int)nodes.size();
	const int coarseCellSize = shell.coarseCellSize;
	const glm::ivec3 gridLower = shell.gridLower;
	const glm::ivec3 gridUpper = shell.gridUpper;
	// User's requested cellSize is the deepest level we'll refine to. Clamp:
	//  cellSize <= 0          : default to coarse
	//  cellSize >= coarse     : just run coarse, don't refine
	//  0 < cellSize < coarse  : refine via halving until we cross targetCellSize
	int targetCellSize = cellSize;
	if (targetCellSize <= 0) targetCellSize = coarseCellSize;
	if (targetCellSize > coarseCellSize) targetCellSize = coarseCellSize;
	Log::info("3dprint debugfrontier: coarse=%d target=%d nodes=%d",
			  coarseCellSize, targetCellSize, totalNodes);

	// `coarse` is already classified (solid/exterior/interior) by buildCoarseShell.
	Log::info("3dprint debugfrontier: coarse %dx%dx%d ext+int classified (interior=%lu cell(s), elapsed=%.1fs)",
			  coarse.dimX, coarse.dimY, coarse.dimZ, (unsigned long)shell.intCount, elapsedSince(fnStart));

	// Progressive refinement chain (mirrors runHoleFill exactly so the visualised
	// frontier matches what holefill would actually expand from). State at each
	// fine level inherits from prev via initFineFromPrev, then runBidirectionalBFS
	// advances the frontier and detects holes. We iterate down to targetCellSize.
	static constexpr int64_t kStateBudget = 4ll * 1024ll * 1024ll * 1024ll;
	Level prev = core::move(coarse);
	for (int fineSize = coarseCellSize / 2; fineSize >= targetCellSize; fineSize /= 2) {
		Level fine;
		fine.cellSize = fineSize;
		fine.initGrid(gridLower, gridUpper);
		const int64_t fineCells = (int64_t)fine.dimX * (int64_t)fine.dimY * (int64_t)fine.dimZ;
		if (fineCells > kStateBudget) {
			Log::error("3dprint debugfrontier: level %d grid %dx%dx%d = %lld cells (%.1f GB) exceeds state budget",
					   fineSize, fine.dimX, fine.dimY, fine.dimZ, (long long)fineCells,
					   (double)fineCells / (1024.0 * 1024.0 * 1024.0));
			return;
		}
		{
			ProgressTimer t("debugfrontier solidHash", totalNodes);
			buildSolidHash(fine, nodes, &t);
		}
		for (const auto &kv : fine.solid)
			if (fine.inBounds(kv.first)) fine.state[(size_t)fine.toIdx(kv.first)] = kStateSolid;
		initFineFromPrev(fine, prev);
		const core::DynamicArray<glm::ivec3> holeCells = runBidirectionalBFS(fine);
		Log::info("3dprint debugfrontier: level %d done (grid %dx%dx%d, holes=%zu, elapsed=%.1fs)",
				  fineSize, fine.dimX, fine.dimY, fine.dimZ,
				  holeCells.size(), elapsedSince(fnStart));
		prev = core::move(fine);
		if (fineSize == targetCellSize) break;
	}

	// Extract frontier from the deepest level. wrapSolid=true picks up ext/uint64_t
	// cells whose neighbour is solid OR blocked (a detected hole), not just
	// kStateEmpty -- after runBidirectionalBFS most empties are gone, so we'd
	// otherwise get a near-empty frontier.
	const Level &fine = prev;
	const uint64_t minCellSize = fine.cellSize;
	core::DynamicArray<size_t> extFrontier, intFrontier;
	buildFrontiers(fine, extFrontier, intFrontier, /*wrapSolid=*/true);
	const uint64_t extTotal = (uint64_t)extFrontier.size();
	const uint64_t intTotal = (uint64_t)intFrontier.size();
	Log::info("3dprint debugfrontier: cellSize=%lu ext frontier=%lu int frontier=%lu (elapsed=%.1fs)",
			  (unsigned long)minCellSize, (unsigned long)extTotal, (unsigned long)intTotal, elapsedSince(fnStart));
	if (extTotal == 0 && intTotal == 0) {
		Log::info("3dprint debugfrontier: no frontier cells -- nothing to visualise");
		return;
	}

	// Chunked output: a single dense RawVolume covering the world bbox would be
	// terabytes for typical models. Instead bin frontier cells into world-aligned
	// chunks; each occupied chunk becomes one scene-graph node sized to a tight
	// bbox of the cells inside it. Empty chunks cost nothing.
	// Pick chunk granularity that's a multiple of cellSize so binning lines up
	// cleanly. Floor at 256 voxels per axis so we don't shatter into thousands of
	// tiny nodes; if the cellSize is larger than 256, snap up to cellSize.
	uint64_t chunkSizeVoxels = cellSize;
	while (chunkSizeVoxels < 256) chunkSizeVoxels *= 2;

	struct ChunkCell {
		glm::ivec3 localOrigin; // relative to chunkOrigin
		voxel::Voxel voxel;
	};
	struct ChunkData {
		glm::ivec3 chunkOrigin{0};
		glm::ivec3 bboxLo{INT_MAX, INT_MAX, INT_MAX};
		glm::ivec3 bboxHi{INT_MIN, INT_MIN, INT_MIN};
		core::DynamicArray<ChunkCell> cells;
		voxel::RawVolume *volume = nullptr;
	};
	std::unordered_map<glm::ivec3, uint64_t, glm::hash<glm::ivec3>> chunkLookup;
	chunkLookup.reserve((size_t)(extTotal + intTotal) / 16 + 16);
	core::DynamicArray<ChunkData> chunks;
	chunks.reserve((size_t)(extTotal + intTotal) / 16 + 16);

	palette::Palette pal;
	uint8_t extColorIdx = 0;
	uint8_t intColorIdx = 0;
	pal.tryAdd(color::RGBA(255, 128,   0, 255), true, &extColorIdx, true); // orange = exterior frontier
	pal.tryAdd(color::RGBA(  0, 128, 255, 255), true, &intColorIdx, true); // blue   = interior frontier
	const voxel::Voxel extV = voxel::createVoxel(voxel::VoxelType::Generic, extColorIdx);
	const voxel::Voxel intV = voxel::createVoxel(voxel::VoxelType::Generic, intColorIdx);

	const uint64_t binStart = core::TimeProvider::systemMillis();
	uint64_t lastBinLogMs = binStart;
	auto binCell = [&](size_t cellIdx, const voxel::Voxel &v) {
		const glm::ivec3 origin = fine.toCell(cellIdx);
		const glm::ivec3 chunkOrigin = toCellOrigin(origin, chunkSizeVoxels);
		auto it = chunkLookup.find(chunkOrigin);
		uint64_t idx;
		if (it == chunkLookup.end()) {
			idx = (uint64_t)chunks.size();
			ChunkData cd;
			cd.chunkOrigin = chunkOrigin;
			chunks.push_back(core::move(cd));
			chunkLookup.emplace(chunkOrigin, idx);
		} else {
			idx = it->second;
		}
		ChunkData &cd = chunks[(size_t)idx];
		const glm::ivec3 local = origin - chunkOrigin;
		const glm::ivec3 localHi = local + glm::ivec3(minCellSize - 1);
		cd.bboxLo = glm::min(cd.bboxLo, local);
		cd.bboxHi = glm::max(cd.bboxHi, localHi);
		ChunkCell c;
		c.localOrigin = local;
		c.voxel = v;
		cd.cells.push_back(c);
	};
	for (uint64_t i = 0; i < extTotal; ++i) {
		binCell(extFrontier[(size_t)i], extV);
		if (((i + 1) & 65535) == 0) {
			const uint64_t now = core::TimeProvider::systemMillis();
			if (now - lastBinLogMs >= 2000u) {
				lastBinLogMs = now;
				Log::info("3dprint debugfrontier: binning ext %lu/%lu (%zu chunks so far) elapsed=%.1fs",
						  (unsigned long)(i + 1), (unsigned long)extTotal, chunks.size(),
						  (double)(now - binStart) / 1000.0);
			}
		}
	}
	for (uint64_t i = 0; i < intTotal; ++i) {
		binCell(intFrontier[(size_t)i], intV);
		if (((i + 1) & 65535) == 0) {
			const uint64_t now = core::TimeProvider::systemMillis();
			if (now - lastBinLogMs >= 2000u) {
				lastBinLogMs = now;
				Log::info("3dprint debugfrontier: binning int %lu/%lu (%zu chunks so far) elapsed=%.1fs",
						  (unsigned long)(i + 1), (unsigned long)intTotal, chunks.size(),
						  (double)(now - binStart) / 1000.0);
			}
		}
	}
	const uint64_t chunkCount = (uint64_t)chunks.size();
	Log::info("3dprint debugfrontier: %lu cell(s) binned into %lu chunk(s) of %lu^3 voxels (elapsed=%.1fs)",
			  (unsigned long)(extTotal + intTotal), (unsigned long)chunkCount,
			  (unsigned long)chunkSizeVoxels, elapsedSince(fnStart));

	// Allocate + paint chunks in parallel. Each chunk's RawVolume is independent
	// memory, and within a chunk setVoxelUnsafe writes to disjoint indices, so
	// the parallel range is fully race-free. RawVolume new[] is the up-front
	// reservation -- one allocation per chunk, sized to the tight bbox.
	const uint64_t paintStart = core::TimeProvider::systemMillis();
	std::atomic<uint64_t> chunksDone{0};
	std::atomic<int64_t> voxelsPainted{0};
	std::atomic<uint64_t> lastLogMs{paintStart};

	app::for_parallel(0, chunkCount, [&](uint64_t start, uint64_t end) {
		for (uint64_t i = start; i < end; ++i) {
			ChunkData &cd = chunks[(size_t)i];
			const voxel::Region region(cd.bboxLo, cd.bboxHi);
			cd.volume = new voxel::RawVolume(region);
			int64_t local = 0;
			for (const ChunkCell &c : cd.cells) {
				for (uint64_t dz = 0; dz < minCellSize; ++dz)
					for (uint64_t dy = 0; dy < minCellSize; ++dy)
						for (uint64_t dx = 0; dx < minCellSize; ++dx)
							cd.volume->setVoxelUnsafe(c.localOrigin + glm::ivec3(dx, dy, dz), c.voxel);
				local += (int64_t)minCellSize * minCellSize * minCellSize;
			}
			voxelsPainted.fetch_add(local, std::memory_order_relaxed);
			const uint64_t done = chunksDone.fetch_add(1, std::memory_order_relaxed) + 1;
			const uint64_t now = core::TimeProvider::systemMillis();
			uint64_t prevMs = lastLogMs.load(std::memory_order_relaxed);
			if (now - prevMs >= 2000u && lastLogMs.compare_exchange_strong(prevMs, now)) {
				Log::info("3dprint debugfrontier: painting chunk %lu/%lu (%lld voxels) elapsed=%.1fs",
						  (unsigned long)done, (unsigned long)chunkCount,
						  (long long)voxelsPainted.load(std::memory_order_relaxed),
						  (double)(now - paintStart) / 1000.0);
			}
		}
	});
	Log::info("3dprint debugfrontier: paint done in %.1fs (%lu chunks, %lld voxels)",
			  (double)(core::TimeProvider::systemMillis() - paintStart) / 1000.0,
			  (unsigned long)chunkCount, (long long)voxelsPainted.load());

	// Sequential scene-graph insertion. moveNodeToSceneGraph touches shared state
	// (memento history, child-id maps, modified() bookkeeping), so this stays on
	// the main thread. One node per chunk -- not per cell -- keeps node count
	// bounded by surface area / chunkSize^2 instead of total surface cell count.
	//
	// Use SceneManager::moveNodeToSceneGraph so each insertion runs onNewNodeAdded:
	// that propagates the dirty world translation to a real local matrix via
	// updateTransforms() and registers the node with the renderer. Skipping that
	// (calling scenegraph::moveNodeToSceneGraph directly) leaves every node sitting
	// at world (0,0,0) because the world-translation getters set DIRTY_WORLDVALUES
	// only -- the local matrix stays identity until update() runs.
	const uint64_t insertStart = core::TimeProvider::systemMillis();
	uint64_t lastInsertLogMs = insertStart;
	for (uint64_t i = 0; i < chunkCount; ++i) {
		ChunkData &cd = chunks[(size_t)i];
		scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
		newNode.setVolume(cd.volume);
		cd.volume = nullptr;
		newNode.setName("debugfrontier");
		newNode.setPalette(pal);
		scenegraph::SceneGraphTransform transform;
		transform.setWorldTranslation(glm::vec3(cd.chunkOrigin));
		newNode.setTransform(0, transform);
		sceneMgr->moveNodeToSceneGraph(newNode, 0);
		if (((i + 1) & 31) == 0) {
			const uint64_t now = core::TimeProvider::systemMillis();
			if (now - lastInsertLogMs >= 2000u) {
				lastInsertLogMs = now;
				Log::info("3dprint debugfrontier: inserting node %lu/%lu elapsed=%.1fs",
						  (unsigned long)(i + 1), (unsigned long)chunkCount,
						  (double)(now - insertStart) / 1000.0);
			}
		}
	}
	Log::info("3dprint debugfrontier: %lu node(s) inserted -- total elapsed=%.1fs",
			  (unsigned long)chunkCount, elapsedSince(fnStart));
}

} // namespace printing
} // namespace voxedit
