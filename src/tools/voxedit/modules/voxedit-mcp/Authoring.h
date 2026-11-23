/**
 * @file
 */

#pragma once

#include "Tool.h"
#include "core/collection/DynamicArray.h"
#include "math/Axis.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include <glm/vec3.hpp>

namespace voxedit {

/** @brief A color argument: palette index, or erase when the caller passed 0. */
struct AuthoringColor {
	bool erase = false;
	/** @brief True when every slot was in use and the nearest existing color was kept. */
	bool approximated = false;
	uint8_t index = 0;
};

/** @brief Which palette slots already appear on solid voxels. resolveAuthoringColor claims slots in here. */
struct PaletteUse {
	bool slot[256];
};

struct PaintCell {
	glm::ivec3 pos{0};
	bool erase = false;
	uint8_t index = 0;
};

struct VoxelEdit {
	glm::ivec3 pos{0};
	voxel::Voxel voxel;
};

/**
 * @brief Active model node, or the node named by args["nodeUUID"] when that field is set.
 */
scenegraph::SceneGraphNode *authoringNode(ToolContext &ctx, const json::Json &args, core::String &error);

/**
 * @brief Read [x, y, z] or {x, y, z}.
 */
bool parseCell(const json::Json &value, glm::ivec3 &out, const char *what, core::String &error);

/**
 * @brief Read a shape-brush style region object.
 */
bool parseRegionArg(const json::Json &region, voxel::Region &out, core::String &error);

/**
 * @brief Mark palette indexes that occur on solid voxels. Air does not count.
 */
void markUsedPaletteSlots(const voxel::RawVolume &volume, PaletteUse &use);

/**
 * @brief #RRGGBB is written into a free palette slot when missing. Numeric -1 erases.
 * 0 is palette slot 0.
 * A full palette reuses a slot that no solid voxel references before it falls back to the closest color.
 */
bool resolveAuthoringColor(ToolContext &ctx, scenegraph::SceneGraphNode &node, const json::Json &value,
						   PaletteUse &use, AuthoringColor &out, core::String &error);

/**
 * @brief Write cells through the scene manager so undo records the edit.
 * @return false only when the node has no volume.
 */
bool paintCells(ToolContext &ctx, scenegraph::SceneGraphNode &node, const core::DynamicArray<PaintCell> &cells,
				int &changed, int &skipped);

/**
 * @brief Apply absolute voxel writes (including air) in order. Later writes win.
 */
bool applyEdits(ToolContext &ctx, scenegraph::SceneGraphNode &node, const core::DynamicArray<VoxelEdit> &edits,
				int &changed, int &skipped);

struct SolidVoxel {
	glm::ivec3 pos{0};
	voxel::Voxel voxel;
};

bool collectSolids(const voxel::RawVolume &volume, core::DynamicArray<SolidVoxel> &out, core::String &error);

int axisComponent(const glm::ivec3 &pos, math::Axis axis);
void setAxisComponent(glm::ivec3 &pos, math::Axis axis, int value);
bool parseAxis(const core::String &text, math::Axis &axis);

} // namespace voxedit
