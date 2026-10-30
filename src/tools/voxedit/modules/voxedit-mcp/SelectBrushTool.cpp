/**
 * @file
 */

#include "SelectBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/SelectBrush.h"
#include "voxel/RawVolume.h"

namespace voxedit {

static SelectMode parseSelectMode(const core::String &mode) {
	if (mode == "surface") {
		return SelectMode::Surface;
	}
	if (mode == "samecolor") {
		return SelectMode::SameColor;
	}
	if (mode == "fuzzycolor") {
		return SelectMode::FuzzyColor;
	}
	if (mode == "connected") {
		return SelectMode::Connected;
	}
	return SelectMode::All;
}

SelectBrushTool::SelectBrushTool() : BrushTool("voxedit_select_brush") {
	_tool.set("description", "Select voxels in a region with various modes (All, Surface, SameColor, FuzzyColor, Connected)");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json _requiredArr = json::Json::array();
	_requiredArr.push("nodeUUID");
	_requiredArr.push("region");
	inputSchema.set("required", _requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());
	properties.set("region", propRegion());

	// Select mode property
	json::Json selectModeProp = json::Json::object();
	selectModeProp.set("type", "string");
	selectModeProp.set("description",
		"The selection mode: 'all' (all voxels), 'surface' (visible surface only), 'samecolor' (exact color match), "
		"'fuzzycolor' (similar colors), 'connected' (flood fill same color)");
	json::Json _enumArr = json::Json::array();
	_enumArr.push("all");
	_enumArr.push("surface");
	_enumArr.push("samecolor");
	_enumArr.push("fuzzycolor");
	_enumArr.push("connected");
	selectModeProp.set("enum", _enumArr);
	selectModeProp.set("default", "all");
	properties.set("selectMode", selectModeProp);

	// Color threshold for fuzzy color mode
	json::Json thresholdProp = json::Json::object();
	thresholdProp.set("type", "number");
	thresholdProp.set("description", "Color similarity threshold for fuzzycolor mode (0.0 = exact, 1.0 = very fuzzy)");
	thresholdProp.set("default", 0.3);
	thresholdProp.set("minimum", 0.0);
	thresholdProp.set("maximum", 1.0);
	properties.set("colorThreshold", thresholdProp);

	// Clear selection option
	json::Json clearProp = json::Json::object();
	clearProp.set("type", "boolean");
	clearProp.set("description", "If true, clear the selection instead of adding to it (uses erase modifier)");
	clearProp.set("default", false);
	properties.set("clearSelection", clearProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool SelectBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	if (!args.contains("region")) {
		return ctx.result(id, "Missing region argument", true);
	}
	json::Json region = args.get("region");

	const glm::ivec3 mins(region.intVal("minX", 0), region.intVal("minY", 0), region.intVal("minZ", 0));
	const glm::ivec3 maxs(region.intVal("maxX", 0), region.intVal("maxY", 0), region.intVal("maxZ", 0));

	const core::String selectModeStr = args.strVal("selectMode", "all").c_str();
	const SelectMode selectMode = parseSelectMode(selectModeStr);
	const float colorThreshold = args.floatVal("colorThreshold", 0.3f);
	const bool clearSelection = args.boolVal("clearSelection", false);

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	voxel::RawVolume *volume = ctx.sceneMgr->volume(node->uuid());
	if (volume == nullptr) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}

	// Create the brush context
	BrushContext brushContext;
	brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	brushContext.modifierType = clearSelection ? ModifierType::Erase : ModifierType::Override;
	brushContext.cursorPosition = mins;
	brushContext.referencePos = mins;
	brushContext.cursorFace = voxel::FaceNames::PositiveY;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = glm::ivec3(1);

	// Create the modifier wrapper

	// Get and configure the select brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	SelectBrush selectBrush(ctx.sceneMgr);

	// Configure the select brush
	selectBrush.setSelectMode(selectMode);
	selectBrush.fuzzyColor().setColorThreshold(colorThreshold);
	selectBrush.setBoxMode();

	brushContext.evaluatedRegion = voxel::Region(mins, maxs);
	brushContext.cursorPosition = maxs;

	// Execute the brush
	core::Buffer<core::UUID> targets;
	targets.push_back(nodeUUID);
	const BrushApplyResult applied = modifier.executeRequest(selectBrush, brushContext, targets);
	const bool success = !applied.failed();

	// End the brush operation
	selectBrush.endBrush(brushContext);

	const voxel::Region &dirtyRegion = applied.dirtyRegion;
	if (dirtyRegion.isValid()) {
		const char *action = clearSelection ? "cleared" : "created";
		return ctx.result(
			id, core::String::format("Selection %s successfully with mode '%s'", action, selectModeStr.c_str()), false);
	}

	if (success) {
		return ctx.result(id, "Select brush executed but no voxels were selected", false);
	}

	return ctx.result(id, "Failed to execute select brush", true);
}

} // namespace voxedit
