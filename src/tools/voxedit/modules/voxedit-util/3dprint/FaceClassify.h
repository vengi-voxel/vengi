/**
 * @file
 */

#pragma once

namespace voxedit {
class SceneManager;

namespace printing {

// minCellSize: finest probe box size for multi-resolution refinement.
// 0 = no refinement (coarse only, inherited from regrid cell size).
// Must be a power-of-two divisor of the regrid cell size (e.g. 32 when cellSize=128).
void runFaceClassify(SceneManager *sceneMgr, int minCellSize = 0);

// Diagnostic: run coarse exterior/interior BFS, then a blocking fine BFS at cellSize.
// Cells where exterior tries to enter the coarse-interior region are quarantined (not
// propagated further). Adjacent solid voxels are recolored bright red so holes are
// immediately visible in the scene. Does not modify classification colors.
void runHoleMap(SceneManager *sceneMgr, int minCellSize = 0);

// Fill holes: same bidirectional BFS as runHoleMap, but places solid voxels (green)
// into the gap positions instead of coloring adjacent walls.
// Positions outside all node regions are skipped and logged.
void runHoleFill(SceneManager *sceneMgr, int minCellSize = 0);

// Erode: hollow the model down to its outward-facing shell. Removes any solid
// voxel that is not face-adjacent (in cs=1 voxel space) to the global exterior.
// Inner-cavity-facing voxels and buried voxels are removed; sealed cavities
// merge into the surrounding bulk so the printed result is one big internal
// hollow bounded by the original outer skin.
//
// minCellSize controls the deepest dense refinement (default 2). The chunked
// cs=1 driver always runs after the dense pass; voxels far from any sub-cs=2
// air feature classify as buried and are removed.
//
// Pre-condition: the model SHOULD be voxel-watertight (run '3dprint fillholes'
// first). If the coarse exterior flood reaches every air cell (no enclosed
// interior), erode warns and aborts -- otherwise it would just keep the entire
// model since every solid voxel is exterior-facing at coarse scale.
//
// Undo is disabled inside this function: the rewrite scope (millions of voxels
// across hundreds of nodes) makes the memento snapshot infeasible.
void runErode(SceneManager *sceneMgr, int minCellSize = 0);

// Thicken interior walls by one voxel layer. Plugs every cs=1 air voxel that
// is BFS-classified Interior and face-adjacent to a wall voxel. Adds blue
// (kThickenColor) voxels via the same applyHoleFills owner search that
// fillholes uses, so plugs merge into the wall's owning node where possible
// and become orphan node(s) elsewhere.
//
// One invocation = one voxel of added wall thickness on the interior side.
// Run repeatedly for thicker walls.
//
// Pre-condition: model SHOULD be voxel-watertight (run '3dprint fillholes'
// first) so the cs=2 ext/int classification is correct. If '3dprint erode'
// is also wanted, run thicken BEFORE erode (erode would strip the layer).
//
// minCellSize controls the deepest dense refinement (default 2). The chunked
// cs=1 driver always runs after; the action scans chunk state and emits plug
// candidates.
//
// Undo is disabled inside this function: same precedent as fillholes/erode.
void runThicken(SceneManager *sceneMgr, int minCellSize = 0);

// Debug: paint the BFS frontier shell at a given cellSize. Single-level BFS:
// orange = exterior frontier, blue = interior frontier. cellSize controls the
// resolution of the wrap; pass 0 for "auto = modal regridded width" (typically 128).
void runDebugFrontier(SceneManager *sceneMgr, int cellSize = 0);

} // namespace printing
} // namespace voxedit
