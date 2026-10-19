/**
 * @file
 */

#include "Regrid.h"
#include "Progress.h"

#include "app/Async.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "memento/MementoHandler.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeCropper.h"

#include <atomic>

#include <cfloat>
#include <glm/common.hpp>
#include <glm/matrix.hpp>

namespace voxedit {
namespace printing {

namespace {

struct SourceInfo {
	int nodeId;
	voxel::RawVolume *volume;
	glm::mat4 worldMat;
	glm::mat4 invWorldMat;
	voxel::Region localRegion;
	glm::ivec3 minCell;
	glm::ivec3 maxCell;
};

struct CellResult {
	voxel::RawVolume *vol = nullptr;
	palette::Palette palette;
};

static glm::ivec3 worldToCell(const glm::vec3 &w, int cellSize) {
	return glm::ivec3(
		(int)glm::floor(w.x / (float)cellSize),
		(int)glm::floor(w.y / (float)cellSize),
		(int)glm::floor(w.z / (float)cellSize));
}

static void computeWorldAABB(const glm::mat4 &m, const voxel::Region &r, glm::vec3 &outMin,
							 glm::vec3 &outMax) {
	const glm::ivec3 lc = r.getLowerCorner();
	const glm::ivec3 uc = r.getUpperCorner();
	outMin = glm::vec3(FLT_MAX);
	outMax = glm::vec3(-FLT_MAX);
	for (int c = 0; c < 8; ++c) {
		const glm::vec4 p(
			(c & 1) ? (float)uc.x : (float)lc.x,
			(c & 2) ? (float)uc.y : (float)lc.y,
			(c & 4) ? (float)uc.z : (float)lc.z,
			1.0f);
		const glm::vec4 w = m * p;
		outMin = glm::min(outMin, glm::vec3(w));
		outMax = glm::max(outMax, glm::vec3(w));
	}
}

static void processCell(const glm::ivec3 &cellIdx, int cellSize,
						const core::DynamicArray<SourceInfo> &sources,
						const core::DynamicArray<core::Buffer<uint8_t>> &remaps,
						const palette::Palette &dedupPalette, CellResult &result,
						std::atomic<int64_t> &outputVoxelCounter) {
	const glm::ivec3 cellOrigin = cellIdx * cellSize;
	const glm::ivec3 cellMax = cellOrigin + glm::ivec3(cellSize - 1);
	// Cell volume lives in 0..cellSize-1 local space; world position comes from the
	// node's keyframe translation (set at emit time). This matches the vengi convention
	// used by addModelChild/splitObjects and avoids giving the renderer raw world-coord
	// mesh vertices for cells at large (possibly negative) cell indices.
	voxel::RawVolume *out = nullptr;

	// Per-cell inline compaction: dedupPalette index -> compact cell-palette index.
	// 0xFF sentinel means "not yet assigned". Stack-allocated, ~256 bytes.
	uint8_t compactIdx[palette::PaletteMaxColors];
	for (int i = 0; i < palette::PaletteMaxColors; ++i) {
		compactIdx[i] = 0xFF;
	}
	int compactCount = 0;
	palette::Palette cellPalette;
	cellPalette.setSize(0);
	int64_t localVoxelWrites = 0;

	for (size_t s = 0; s < sources.size(); ++s) {
		const SourceInfo &si = sources[s];
		if (si.nodeId < 0 || si.volume == nullptr) {
			continue;
		}
		if (cellIdx.x < si.minCell.x || cellIdx.x > si.maxCell.x) {
			continue;
		}
		if (cellIdx.y < si.minCell.y || cellIdx.y > si.maxCell.y) {
			continue;
		}
		if (cellIdx.z < si.minCell.z || cellIdx.z > si.maxCell.z) {
			continue;
		}

		glm::vec3 lmin(FLT_MAX);
		glm::vec3 lmax(-FLT_MAX);
		for (int c = 0; c < 8; ++c) {
			const glm::vec4 p(
				(c & 1) ? (float)cellMax.x : (float)cellOrigin.x,
				(c & 2) ? (float)cellMax.y : (float)cellOrigin.y,
				(c & 4) ? (float)cellMax.z : (float)cellOrigin.z,
				1.0f);
			const glm::vec4 l = si.invWorldMat * p;
			const glm::vec3 lp(l);
			lmin = glm::min(lmin, lp);
			lmax = glm::max(lmax, lp);
		}
		// Pad by 1 to absorb FP rounding; inner per-voxel check discards strays.
		const glm::ivec3 regionLo = si.localRegion.getLowerCorner();
		const glm::ivec3 regionHi = si.localRegion.getUpperCorner();
		glm::ivec3 srcLo(
			(int)glm::floor(lmin.x) - 1,
			(int)glm::floor(lmin.y) - 1,
			(int)glm::floor(lmin.z) - 1);
		glm::ivec3 srcHi(
			(int)glm::ceil(lmax.x) + 1,
			(int)glm::ceil(lmax.y) + 1,
			(int)glm::ceil(lmax.z) + 1);
		srcLo = glm::max(srcLo, regionLo);
		srcHi = glm::min(srcHi, regionHi);
		if (srcLo.x > srcHi.x || srcLo.y > srcHi.y || srcLo.z > srcHi.z) {
			continue;
		}

		voxel::RawVolume *srcRv = si.volume;
		const core::Buffer<uint8_t> &remap = remaps[s];
		for (int sy = srcLo.y; sy <= srcHi.y; ++sy) {
			for (int sz = srcLo.z; sz <= srcHi.z; ++sz) {
				for (int sx = srcLo.x; sx <= srcHi.x; ++sx) {
					const voxel::Voxel v = srcRv->voxel(sx, sy, sz);
					if (voxel::isAir(v.getMaterial())) {
						continue;
					}
					const glm::vec4 w = si.worldMat * glm::vec4((float)sx, (float)sy, (float)sz, 1.0f);
					const glm::ivec3 wp(
						(int)glm::round(w.x),
						(int)glm::round(w.y),
						(int)glm::round(w.z));
					if (wp.x < cellOrigin.x || wp.x > cellMax.x) {
						continue;
					}
					if (wp.y < cellOrigin.y || wp.y > cellMax.y) {
						continue;
					}
					if (wp.z < cellOrigin.z || wp.z > cellMax.z) {
						continue;
					}
					if (out == nullptr) {
						const voxel::Region cellRegion(glm::ivec3(0), glm::ivec3(cellSize - 1));
						out = new voxel::RawVolume(cellRegion);
					}
					const uint8_t dedupIdx = remap[v.getColor()];
					uint8_t ci = compactIdx[dedupIdx];
					if (ci == 0xFF) {
						ci = (uint8_t)compactCount;
						compactIdx[dedupIdx] = ci;
						cellPalette.setColor(ci, dedupPalette.color(dedupIdx));
						++compactCount;
					}
					// Drop source normal index and flags: normal indices reference the source
					// node's normal palette which we don't carry over to regrid cells, and
					// FlagOutline (selection state) shouldn't persist through a regrid.
					// Face normals come from the extracted mesh geometry, which is correct
					// regardless of what index we store here.
					const voxel::Voxel remapped = voxel::createVoxel(
						v.getMaterial(), ci, NO_NORMAL, 0u, v.getBoneIdx());
					// Always overwrite (last-wins within a cell). Store in 0-origin local space.
					out->setVoxel(wp - cellOrigin, remapped);
					++localVoxelWrites;
				}
			}
		}
	}
	cellPalette.setSize(compactCount);
	result.vol = out;
	result.palette = cellPalette;
	outputVoxelCounter.fetch_add(localVoxelWrites, std::memory_order_relaxed);
}

} // namespace

void runRegrid(SceneManager *sceneMgr, int cellSize) {
	if (cellSize < 1) {
		cellSize = 128;
	}
	// Suppress undo recording for the entire operation: storing 8 MB volumes for
	// hundreds of source nodes compresses them all and can take minutes.
	// Regrid is a non-reversible pipeline step; undo is not meaningful here.
	memento::ScopedMementoHandlerLock mementoLock(sceneMgr->mementoHandler());
	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	const scenegraph::FrameIndex frameIdx = sceneMgr->currentFrame();

	// Snapshot source nodes with composed world transforms.
	core::DynamicArray<SourceInfo> sources;
	sources.reserve(64);
	glm::ivec3 globalMinCell(INT32_MAX);
	glm::ivec3 globalMaxCell(INT32_MIN);
	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxel::RawVolume *rv = node.volume();
		if (rv == nullptr) {
			continue;
		}
		SourceInfo si;
		si.nodeId = node.id();
		si.volume = rv;
		si.worldMat = graph.worldMatrix(node, frameIdx);
		si.invWorldMat = glm::inverse(si.worldMat);
		si.localRegion = rv->region();
		glm::vec3 wmin, wmax;
		computeWorldAABB(si.worldMat, si.localRegion, wmin, wmax);
		// Pad by 1 world unit: corner AABB is computed from raw world coords but per-voxel
		// placement uses round(), which can push boundary voxels into the next cell. Without
		// this pad the source's cell range wouldn't include those cells and the voxels would
		// be silently dropped, producing node-sized holes along cell boundaries.
		wmin -= glm::vec3(1.0f);
		wmax += glm::vec3(1.0f);
		si.minCell = worldToCell(wmin, cellSize);
		si.maxCell = worldToCell(wmax, cellSize);
		globalMinCell = glm::min(globalMinCell, si.minCell);
		globalMaxCell = glm::max(globalMaxCell, si.maxCell);
		sources.push_back(si);
	}

	if (sources.empty()) {
		Log::info("3dprint regrid: no model nodes");
		return;
	}

	Log::info("3dprint regrid: cellSize=%d, sources=%d, cell bbox=(%d,%d,%d)..(%d,%d,%d)",
			  cellSize, (int)sources.size(),
			  globalMinCell.x, globalMinCell.y, globalMinCell.z,
			  globalMaxCell.x, globalMaxCell.y, globalMaxCell.z);

	// Merge palettes once across all sources, then dedup by exact RGBA. graph.mergePalettes
	// only dedups/removes when overflowing 256 slots and uses similarity matching even then,
	// so identical RGBAs can sit at different merged indices. We strip those up-front so
	// each source's voxel -> merged mapping is one-to-one without any post-hoc scan.
	const palette::Palette rawMerged = graph.mergePalettes(true);
	palette::Palette dedupPalette;
	dedupPalette.setSize(0);
	int dedupCount = 0;
	for (int c = 0; c < rawMerged.colorCount(); ++c) {
		const color::RGBA rgba = rawMerged.color((uint8_t)c);
		int found = -1;
		for (int d = 0; d < dedupCount; ++d) {
			if (dedupPalette.color((uint8_t)d) == rgba) {
				found = d;
				break;
			}
		}
		if (found < 0) {
			dedupPalette.setColor((uint8_t)dedupCount, rgba);
			++dedupCount;
		}
	}
	dedupPalette.setSize(dedupCount);
	Log::info("3dprint regrid: palette merged=%d dedup=%d", rawMerged.colorCount(), dedupCount);

	// Per-source: src palette idx -> dedup palette idx.
	palette::PaletteLookup palLookup(dedupPalette);
	core::DynamicArray<core::Buffer<uint8_t>> remaps;
	remaps.resize(sources.size());
	for (size_t i = 0; i < sources.size(); ++i) {
		const scenegraph::SceneGraphNode &node = graph.node(sources[i].nodeId);
		const palette::Palette &srcPal = node.palette();
		remaps[i].resize(palette::PaletteMaxColors);
		for (int c = 0; c < palette::PaletteMaxColors; ++c) {
			remaps[i][c] = palLookup.findClosestIndex(srcPal.color((uint8_t)c));
		}
	}

	// Iterate global cell bbox in tiles. Each tile: parallel cell gather (Phase A),
	// then sequential emit (Phase B). Sources are deleted in one batch at the end.
	const int tileSide = 4;
	const int cellsPerTile = tileSide * tileSide * tileSide;
	const glm::ivec3 globalSize = globalMaxCell - globalMinCell + glm::ivec3(1);
	const glm::ivec3 numTiles(
		(globalSize.x + tileSide - 1) / tileSide,
		(globalSize.y + tileSide - 1) / tileSide,
		(globalSize.z + tileSide - 1) / tileSide);
	const int totalTiles = numTiles.x * numTiles.y * numTiles.z;
	ProgressTimer timer("regrid", totalTiles);

	core::DynamicArray<CellResult> cellResults;
	cellResults.resize(cellsPerTile);

	// Collect new cell IDs so we can flush to the renderer and call updateTransforms
	// ONCE at the end instead of once per node (avoids O(N^2) cost).
	core::DynamicArray<int> newCellIds;
	newCellIds.reserve(256);
	core::DynamicArray<int> deletedSourceIds;
	deletedSourceIds.reserve((int)sources.size());

	int emittedCells = 0;
	int processedTiles = 0;
	std::atomic<int64_t> outputVoxelCount{0};

	for (int tz = 0; tz < numTiles.z; ++tz) {
		for (int ty = 0; ty < numTiles.y; ++ty) {
			for (int tx = 0; tx < numTiles.x; ++tx) {
				const glm::ivec3 tileBase = globalMinCell + glm::ivec3(tx, ty, tz) * tileSide;

				for (int k = 0; k < cellsPerTile; ++k) {
					cellResults[k].vol = nullptr;
				}

				// Phase A: parallel per-cell voxel gather + inline palette compaction.
				// Thread-local writes to cellResults[k] only; no shared mutable state.
				app::for_parallel(0, cellsPerTile, [&](int start, int end) {
					for (int k = start; k < end; ++k) {
						const int lx = k % tileSide;
						const int ly = (k / tileSide) % tileSide;
						const int lz = k / (tileSide * tileSide);
						const glm::ivec3 cellIdx = tileBase + glm::ivec3(lx, ly, lz);
						if (cellIdx.x > globalMaxCell.x || cellIdx.y > globalMaxCell.y ||
							cellIdx.z > globalMaxCell.z) {
							continue;
						}
						processCell(cellIdx, cellSize, sources, remaps, dedupPalette, cellResults[k],
									outputVoxelCount);
					}
				});

				// Phase B: sequential emit.
				for (int k = 0; k < cellsPerTile; ++k) {
					const int lx = k % tileSide;
					const int ly = (k / tileSide) % tileSide;
					const int lz = k / (tileSide * tileSide);
					const glm::ivec3 cellIdx = tileBase + glm::ivec3(lx, ly, lz);
					if (cellIdx.x > globalMaxCell.x || cellIdx.y > globalMaxCell.y ||
						cellIdx.z > globalMaxCell.z) {
						continue;
					}

					CellResult &cr = cellResults[k];
					if (cr.vol != nullptr) {
						// NOTE: voxelutil::cropVolume returns nullptr in TWO cases:
						//   (a) volume is empty -> drop.
						//   (b) cropped region == volume region (no crop reduction possible)
						//       -> must still emit the original volume, not drop it.
						// Because processCell only allocates cr.vol when a voxel gets written,
						// cr.vol != nullptr here means case (b): emit original if cropVolume
						// returns null.
						voxel::RawVolume *cropped = voxelutil::cropVolume(cr.vol);
						voxel::RawVolume *toEmit;
						if (cropped != nullptr) {
							delete cr.vol;
							toEmit = cropped;
						} else {
							// Case (b): already-tight volume; cropVolume can't reduce. Use original.
							toEmit = cr.vol;
						}
						cr.vol = nullptr;
						if (toEmit != nullptr) {
							// Offset name by globalMinCell so indices are always non-negative.
							const glm::ivec3 nameIdx = cellIdx - globalMinCell;
							scenegraph::SceneGraphNode newNode(scenegraph::SceneGraphNodeType::Model);
							newNode.setVolume(toEmit);
							newNode.setName(core::String::format(
								"regrid_%d_%d_%d", nameIdx.x, nameIdx.y, nameIdx.z));
							newNode.setPalette(cr.palette);
							// World position via keyframe translation, not via region offset.
							// Cell volume is in 0..cellSize-1 local space, translated to cellOrigin.
							const glm::ivec3 cellOrigin = cellIdx * cellSize;
							scenegraph::SceneGraphTransform transform;
							transform.setWorldTranslation(glm::vec3(cellOrigin));
							newNode.setTransform(0, transform);
							// Bypass SceneManager::moveNodeToSceneGraph (which calls
							// onNewNodeAdded → updateTransforms per node → O(N^2) total).
							// We call updateTransforms once after the loop instead.
							const int newId = scenegraph::moveNodeToSceneGraph(graph, newNode, 0);
							if (newId != InvalidNodeId) {
								newCellIds.push_back(newId);
							}
							++emittedCells;
						}
					}
				}

				++processedTiles;
				timer.tick(processedTiles);
			}
		}
	}

	// Delete all sources at once: simpler and avoids mid-loop graph mutations.
	for (SourceInfo &si : sources) {
		if (si.nodeId >= 0) {
			deletedSourceIds.push_back(si.nodeId);
			graph.removeNode(si.nodeId, false);
			si.nodeId = -1;
			si.volume = nullptr;
		}
	}

	// Single updateTransforms call for all added/removed nodes (avoids O(N^2) cost).
	graph.updateTransforms();
	// Flush all node changes to the renderer and mark the scene dirty.
	sceneMgr->onRegridComplete(newCellIds, deletedSourceIds);

	const int64_t outputV = outputVoxelCount.load();
	Log::info("3dprint regrid: emitted %d cell(s), removed %d source node(s), voxels out=%ld",
			  emittedCells, (int)deletedSourceIds.size(), (long)outputV);
}

} // namespace printing
} // namespace voxedit
