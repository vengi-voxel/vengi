/**
 * @file
 */

#include "Authoring.h"
#include "color/ColorUtil.h"
#include "core/Common.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxelutil/VolumeVisitor.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "math/Vec3i.h"

namespace voxedit {

static constexpr int MaxSolids = 2000000;

scenegraph::SceneGraphNode *authoringNode(ToolContext &ctx, const json::Json &args, core::String &error) {
	const bool explicitId = args.contains("nodeUUID") && args.get("nodeUUID").isString() && !args.get("nodeUUID").str().empty();
	core::UUID uuid;
	if (explicitId) {
		uuid = core::UUID(args.get("nodeUUID").cStr());
		if (!uuid.isValid()) {
			error = "Invalid node UUID - fetch the scene state first";
			return nullptr;
		}
	} else if (ctx.sceneMgr != nullptr) {
		uuid = ctx.sceneMgr->activeNodeUUID();
	}
	if (ctx.sceneMgr == nullptr) {
		error = "Scene manager is not available";
		return nullptr;
	}
	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphModelNodeByUUID(uuid);
	if (node == nullptr || node->volume() == nullptr) {
		error = explicitId ? "Node not found or not a model node - fetch the scene state first"
						   : "No active model node - pass nodeUUID or select a model";
		return nullptr;
	}
	return node;
}

bool parseCell(const json::Json &value, math::Vec3i &out, const char *what, core::String &error) {
	if (value.isArray() && value.size() >= 3) {
		out = math::Vec3i(value.get(0).intVal(), value.get(1).intVal(), value.get(2).intVal());
		return true;
	}
	if (value.isObject() && value.contains("x") && value.contains("y") && value.contains("z")) {
		out = math::Vec3i(value.intVal("x", 0), value.intVal("y", 0), value.intVal("z", 0));
		return true;
	}
	error = core::String::format("%s must be [x, y, z]", what);
	return false;
}

bool parseRegionArg(const json::Json &region, voxel::Region &out, core::String &error) {
	if (!region.isObject()) {
		error = "region must be {minX, minY, minZ, maxX, maxY, maxZ}";
		return false;
	}
	static const char *keys[] = {"minX", "minY", "minZ", "maxX", "maxY", "maxZ"};
	for (const char *key : keys) {
		if (!region.contains(key)) {
			error = core::String::format("region is missing %s", key);
			return false;
		}
	}
	out = voxel::Region(region.intVal("minX", 0), region.intVal("minY", 0), region.intVal("minZ", 0),
						region.intVal("maxX", 0), region.intVal("maxY", 0), region.intVal("maxZ", 0));
	if (!out.isValid()) {
		error = "region is empty or inverted";
		return false;
	}
	return true;
}

static_assert(palette::PaletteMaxColors == 256, "PaletteUse::slot must cover every palette index");

void markUsedPaletteSlots(const voxel::RawVolume &volume, PaletteUse &use) {
	for (int i = 0; i < 256; ++i) {
		use.slot[i] = false;
	}
	voxelutil::visitVolume(volume, [&use](int, int, int, const voxel::Voxel &voxel) {
		use.slot[voxel.getColor()] = true;
	}, voxelutil::VisitSolid());
}

bool resolveAuthoringColor(ToolContext &ctx, scenegraph::SceneGraphNode &node, const json::Json &value,
						   PaletteUse &use, AuthoringColor &out, core::String &error) {
	out = AuthoringColor{};
	if (!value.isValid()) {
		error = "Missing color";
		return false;
	}
	if (value.isNumber()) {
		const int index = value.intVal();
		if (index == -1) {
			out.erase = true;
			return true;
		}
		const int count = node.palette().colorCount();
		if (index < 0 || index >= count) {
			error = core::String::format("Palette index %i is outside 0..%i (-1 erases)", index, core_max(0, count - 1));
			return false;
		}
		out.index = (uint8_t)index;
		use.slot[out.index] = true;
		return true;
	}
	if (!value.isString()) {
		error = "Color must be #RRGGBB or a palette index (-1 erases)";
		return false;
	}
	const core::String text = value.str();
	if (text == "-1" || text == "erase") {
		out.erase = true;
		return true;
	}
	color::RGBA rgba;
	if (!color::parseColor(text, rgba)) {
		error = core::String::format("Could not parse color '%s' - use #RRGGBB or a palette index", text.c_str());
		return false;
	}
	if (rgba.a == 0) {
		out.erase = true;
		return true;
	}
	palette::Palette &palette = node.palette();
	for (int i = 0; i < palette.colorCount(); ++i) {
		if (palette.color((uint8_t)i) == rgba) {
			out.index = (uint8_t)i;
			use.slot[out.index] = true;
			return true;
		}
	}
	int freeSlot = -1;
	if (palette.colorCount() < palette::PaletteMaxColors) {
		freeSlot = palette.colorCount();
	} else {
		for (int i = 0; i < palette.colorCount(); ++i) {
			if (!use.slot[i]) {
				freeSlot = i;
				break;
			}
		}
	}
	if (freeSlot >= 0) {
		if (ctx.sceneMgr == nullptr || !ctx.sceneMgr->nodeSetColor(node.uuid(), (uint8_t)freeSlot, rgba)) {
			error = "Failed to add the color to the palette";
			return false;
		}
		out.index = (uint8_t)freeSlot;
		use.slot[out.index] = true;
		return true;
	}
	const int closest = palette.getClosestMatch(rgba);
	if (closest < 0) {
		error = "Palette is full and has no matching color";
		return false;
	}
	out.index = (uint8_t)closest;
	out.approximated = true;
	use.slot[out.index] = true;
	return true;
}

static int writeOne(voxedit::ModifierVolumeWrapper &wrapper, const voxel::RawVolume &volume, const math::Vec3i &pos,
					const voxel::Voxel &next) {
	if (!volume.region().containsPoint(pos)) {
		return -1;
	}
	if (volume.voxel(pos) == next) {
		return 0;
	}
	if (!wrapper.setVoxel(pos.x, pos.y, pos.z, next)) {
		return -1;
	}
	return 1;
}

bool paintCells(ToolContext &ctx, scenegraph::SceneGraphNode &node, const core::DynamicArray<PaintCell> &cells,
				int &changed, int &skipped) {
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr || ctx.sceneMgr == nullptr) {
		return false;
	}
	const palette::Palette &palette = node.palette();
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	changed = 0;
	skipped = 0;
	for (const PaintCell &cell : cells) {
		const voxel::Voxel next = cell.erase ? voxel::Voxel() : voxel::createVoxel(palette, cell.index);
		const int wrote = writeOne(wrapper, *volume, cell.pos, next);
		if (wrote < 0) {
			++skipped;
		} else {
			changed += wrote;
		}
	}
	const voxel::Region &dirty = wrapper.dirtyRegion();
	if (dirty.isValid()) {
		ctx.sceneMgr->modified(node.uuid(), dirty);
	}
	return true;
}

bool applyEdits(ToolContext &ctx, scenegraph::SceneGraphNode &node, const core::DynamicArray<VoxelEdit> &edits,
				int &changed, int &skipped) {
	voxel::RawVolume *volume = node.volume();
	if (volume == nullptr || ctx.sceneMgr == nullptr) {
		return false;
	}
	ModifierVolumeWrapper wrapper(node, ModifierType::Override);
	changed = 0;
	skipped = 0;
	for (const VoxelEdit &edit : edits) {
		const int wrote = writeOne(wrapper, *volume, edit.pos, edit.voxel);
		if (wrote < 0) {
			++skipped;
		} else {
			changed += wrote;
		}
	}
	const voxel::Region &dirty = wrapper.dirtyRegion();
	if (dirty.isValid()) {
		ctx.sceneMgr->modified(node.uuid(), dirty);
	}
	return true;
}

bool collectSolids(const voxel::RawVolume &volume, core::DynamicArray<SolidVoxel> &out, core::String &error) {
	out.clear();
	voxelutil::visitVolume(volume, [&out](int x, int y, int z, const voxel::Voxel &voxel) {
		if ((int)out.size() >= MaxSolids) {
			return;
		}
		SolidVoxel solid;
		solid.pos = math::Vec3i(x, y, z);
		solid.voxel = voxel;
		out.push_back(solid);
	}, voxelutil::VisitSolid());
	if ((int)out.size() >= MaxSolids) {
		error = "Too many voxels to edit in one call";
		return false;
	}
	return true;
}

int axisComponent(const math::Vec3i &pos, math::Axis axis) {
	if (axis == math::Axis::X) {
		return pos.x;
	}
	if (axis == math::Axis::Y) {
		return pos.y;
	}
	return pos.z;
}

void setAxisComponent(math::Vec3i &pos, math::Axis axis, int value) {
	if (axis == math::Axis::X) {
		pos.x = value;
	} else if (axis == math::Axis::Y) {
		pos.y = value;
	} else {
		pos.z = value;
	}
}

bool parseAxis(const core::String &text, math::Axis &axis) {
	const core::String lower = text.toLower();
	if (lower == "x") {
		axis = math::Axis::X;
		return true;
	}
	if (lower == "y") {
		axis = math::Axis::Y;
		return true;
	}
	if (lower == "z") {
		axis = math::Axis::Z;
		return true;
	}
	return false;
}

} // namespace voxedit
