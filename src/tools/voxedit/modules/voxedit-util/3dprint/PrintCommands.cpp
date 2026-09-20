/**
 * @file
 */

#include "PrintCommands.h"
#include "FaceClassify.h"
#include "FlatBase.h"
#include "Progress.h"
#include "Regrid.h"

#include "app/Async.h"
#include "app/I18N.h"
#include "command/Command.h"
#include "color/RGBA.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"

#include <atomic>
#include <cstdio>
#include <glm/matrix.hpp>

namespace voxedit {
namespace printing {

namespace {

struct NodeTask {
	int nodeId;
	voxel::RawVolume *rv;
};

static core::DynamicArray<NodeTask> collectNodeTasks(scenegraph::SceneGraph &graph) {
	core::DynamicArray<NodeTask> tasks;
	for (auto iter = graph.beginModel(); iter != graph.end(); ++iter) {
		scenegraph::SceneGraphNode &node = *iter;
		voxel::RawVolume *rv = node.volume();
		if (rv == nullptr) {
			continue;
		}
		tasks.push_back({node.id(), rv});
	}
	return tasks;
}

// packedRGB: 0x00RRGGBB integer. Each node gets the color added to its own palette via tryAdd.
static void runRecolor(SceneManager *sceneMgr, int packedRGB) {
	const uint8_t r = (uint8_t)((packedRGB >> 16) & 0xFF);
	const uint8_t g = (uint8_t)((packedRGB >>  8) & 0xFF);
	const uint8_t b = (uint8_t)( packedRGB        & 0xFF);
	const color::RGBA targetColor(r, g, b, 255);

	scenegraph::SceneGraph &graph = sceneMgr->sceneGraph();
	const core::DynamicArray<NodeTask> tasks = collectNodeTasks(graph);
	const int totalNodes = (int)tasks.size();
	ProgressTimer timer("recolor", totalNodes);
	std::atomic<int> processed{0};
	int totalRecolored = 0;
	int nodesTouched = 0;
	for (const NodeTask &task : tasks) {
		scenegraph::SceneGraphNode &node = graph.node(task.nodeId);
		palette::Palette &pal = node.palette();

		// Find the first solid voxel's current color to protect its palette slot from eviction.
		// Without this, tryAdd may replace the very slot all voxels use, making setVoxel a no-op.
		int skipColorIdx = palette::PaletteColorNotFound;
		{
			const voxel::Region &region = task.rv->region();
			bool found = false;
			for (int z = region.getLowerZ(); z <= region.getUpperZ() && !found; ++z) {
				for (int y = region.getLowerY(); y <= region.getUpperY() && !found; ++y) {
					for (int x = region.getLowerX(); x <= region.getUpperX() && !found; ++x) {
						const voxel::Voxel &v = task.rv->voxel(x, y, z);
						if (!voxel::isAir(v.getMaterial())) {
							skipColorIdx = (int)v.getColor();
							found = true;
						}
					}
				}
			}
		}

		uint8_t colorIdx = 0;
		const bool paletteChanged = pal.tryAdd(targetColor, true, &colorIdx, true, skipColorIdx);

		voxel::RawVolumeWrapper wrapper(task.rv);
		const voxel::Region &region = task.rv->region();
		for (int z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
			for (int y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				for (int x = region.getLowerX(); x <= region.getUpperX(); ++x) {
					const voxel::Voxel &v = task.rv->voxel(x, y, z);
					if (voxel::isAir(v.getMaterial())) continue;
					wrapper.setVoxel(x, y, z, voxel::createVoxel(v.getMaterial(), colorIdx,
																   v.getNormal(), v.getFlags(), v.getBoneIdx()));
					++totalRecolored;
				}
			}
		}
		if (wrapper.dirtyRegion().isValid() || paletteChanged) {
			const voxel::Region dirtyRgn = wrapper.dirtyRegion().isValid()
				? wrapper.dirtyRegion() : task.rv->region();
			sceneMgr->modified(task.nodeId, dirtyRgn);
			++nodesTouched;
		}
		timer.addVoxels((int64_t)totalRecolored);
		timer.tick(++processed);
	}
	Log::info("3dprint recolor: recolored %d voxel(s) across %d node(s) to #%06X",
			  totalRecolored, nodesTouched, (unsigned)packedRGB);
}


static void dispatch(SceneManager *sceneMgr, const command::CommandArgs &args) {
	if (!args.has("subcommand")) {
		Log::info("3dprint: usage: 3dprint <hello|fillholes|erode|thicken|regrid|faceclassify|holemap|debugfrontier|recolor|flatbase> [cellSize|maxHoleSize|colorIndex|slabThickness] [minSolidNeighbors|trimPercent] [debugColor]");
		Log::info("         debugColor: palette index (0-255) for newly placed voxels, or -1 for cyan marker (default -1)");
		Log::info("         all three commands mark new voxels with cyan by default so additions are visually inspectable");
		Log::info("         regrid: rebucket all model nodes into world-aligned cellSize^3 cells (default 128)");
		Log::info("         faceclassify: recolor all voxels: orange=outer surface, blue=inner cavity surface, magenta=thin wall, gray=buried");
		Log::info("         erode: hollow the model -- keep only exterior-facing voxels at cs=1 precision (run fillholes first)");
		Log::info("         thicken: add a 1-voxel-thick layer to the inside of every interior-facing wall (blue; run fillholes first; rerun for thicker walls; run before erode)");
		Log::info("         flatbase: voxel-perfect flat min-Y plane across all bottom nodes. Args: slabThickness=5, trimPercent=10, debugColor=-1");
		return;
	}
	const core::String &sub = args.str("subcommand");
	if (sub == "hello") {
		Log::info("3dprint hello: dispatch works");
		return;
	}
	if (sub == "fillholes") {
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runHoleFill(sceneMgr, minCellSize);
		return;
	}
	if (sub == "erode") {
		// maxHoleSize arg slot reused as minCellSize (deepest dense refinement,
		// default cs=2; the chunked cs=1 driver always runs after).
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runErode(sceneMgr, minCellSize);
		return;
	}
	if (sub == "thicken") {
		// maxHoleSize arg slot reused as minCellSize (deepest dense refinement,
		// default cs=2; the chunked cs=1 driver always runs after).
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runThicken(sceneMgr, minCellSize);
		return;
	}
	if (sub == "regrid") {
		// maxHoleSize arg slot is reused as cellSize (avoids adding yet another arg to the schema).
		int cellSize = args.intVal("maxHoleSize", 128);
		if (cellSize < 1) {
			cellSize = 128;
		}
		runRegrid(sceneMgr, cellSize);
		return;
	}
	if (sub == "faceclassify") {
		// maxHoleSize arg slot reused as minCellSize (0 = coarse only)
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runFaceClassify(sceneMgr, minCellSize);
		return;
	}
	if (sub == "holemap") {
		// maxHoleSize arg slot reused as minCellSize (0 = auto: coarse/2)
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runHoleMap(sceneMgr, minCellSize);
		return;
	}
	if (sub == "debugfrontier") {
		const int minCellSize = args.intVal("maxHoleSize", 0);
		runDebugFrontier(sceneMgr, minCellSize);
		return;
	}
	if (sub == "recolor") {
		// maxHoleSize arg slot reused as packed 0xRRGGBB color (default 0xB4B4B4 = neutral gray)
		const int packedRGB = args.intVal("maxHoleSize", 0xB4B4B4);
		runRecolor(sceneMgr, packedRGB);
		return;
	}
	if (sub == "flatbase") {
		// The maxHoleSize / minSolidNeighbors arg slots are shared across every
		// 3dprint subcommand and carry global registered defaults of "1000" and
		// "3" (those values were chosen for fillholes). Command::parseArgs
		// pre-populates those defaults into the args map when the user omits
		// them, so a plain `args.intVal("maxHoleSize", 5)` would always return
		// 1000 -- the registered default -- and the per-subcommand fallback
		// would never fire. Detect the registered-default leak and substitute
		// flatbase's own defaults instead. Caveat: this means an explicit
		// `flatbase 1000 3` is treated as defaults; 1000-voxel slabs and 3%
		// trim are nonsense for this command anyway.
		int slabThickness = 5;
		int trimPercent = 10;
		const core::String &slabStr = args.str("maxHoleSize");
		if (!slabStr.empty() && slabStr != "1000") {
			slabThickness = slabStr.toInt();
		}
		const core::String &trimStr = args.str("minSolidNeighbors");
		if (!trimStr.empty() && trimStr != "3") {
			trimPercent = trimStr.toInt();
		}
		const int debugColor = args.intVal("debugColor", -1);
		runFlatBase(sceneMgr, slabThickness, trimPercent, debugColor);
		return;
	}
	Log::warn("3dprint: unknown subcommand '%s'", sub.c_str());
}

} // namespace

void registerCommands(SceneManager *sceneMgr) {
	command::Command::registerCommand("3dprint")
		.addArg({"subcommand", command::ArgType::String, false, "", "hello|fillholes|erode|thicken|regrid|faceclassify|holemap|debugfrontier|recolor|flatbase"})
		.addArg({"maxHoleSize", command::ArgType::Int, true, "1000", "fillholes: optional minCellSize override for the dense pass (default cs=2; chunked cs=1 always runs after). regrid: cellSize (default 128). faceclassify/holemap: minCellSize. flatbase: slabThickness (default 5)."})
		.addArg({"minSolidNeighbors", command::ArgType::Int, true, "3",
				 "Min solid 6-neighbors for a hole seed (fillholes). For flatbase: trimPercent (default 10)."})
		.addArg({"debugColor", command::ArgType::Int, true, "-1",
				 "Palette index (0-255) to force-colorize newly placed voxels, or -1 for auto"})
		.setHandler([sceneMgr](const command::CommandArgs &args) { dispatch(sceneMgr, args); })
		.setHelp(_("3D print preparation commands. Usage: 3dprint <hello|fillholes|regrid|faceclassify|flatbase> [args...]"));
}

} // namespace printing
} // namespace voxedit
