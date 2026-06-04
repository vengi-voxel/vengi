/**
 * @file
 */

#include "SculptBrushTool.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/Modifier.h"
#include "voxedit-util/modifier/ModifierVolumeWrapper.h"
#include "voxedit-util/modifier/brush/SculptBrush.h"
#include "voxel/ClipboardData.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"

namespace voxedit {

static SculptMode parseSculptMode(const core::String &mode) {
	if (mode == "grow") {
		return SculptMode::Grow;
	}
	if (mode == "flatten") {
		return SculptMode::Flatten;
	}
	if (mode == "smoothadditive") {
		return SculptMode::SmoothAdditive;
	}
	if (mode == "smootherode") {
		return SculptMode::SmoothErode;
	}
	if (mode == "smoothgaussian") {
		return SculptMode::SmoothGaussian;
	}
	if (mode == "bridgegap") {
		return SculptMode::BridgeGap;
	}
	if (mode == "squashtoplane") {
		return SculptMode::SquashToPlane;
	}
	if (mode == "extendplane") {
		return SculptMode::ExtendPlane;
	}
	if (mode == "reskin") {
		return SculptMode::Reskin;
	}
	if (mode == "smoothwall") {
		return SculptMode::SmoothWall;
	}
	return SculptMode::Erode;
}

SculptBrushTool::SculptBrushTool() : BrushTool("voxedit_sculpt_brush") {
	_tool.set("description",
			  "Sculpt selected voxels with various modes (Erode, Grow, Flatten, SmoothAdditive, SmoothErode, SmoothGaussian, BridgeGap, SquashToPlane, Reskin, SmoothWall). "
			  "Requires a selection first (use voxedit_select_brush to select voxels before sculpting). "
			  "Reskin mode applies a skin pattern from the clipboard onto the selected surface.");

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	json::Json requiredArr = json::Json::array();
	requiredArr.push("nodeUUID");
	inputSchema.set("required", requiredArr);

	json::Json properties = json::Json::object();
	properties.set("nodeUUID", propUUID());

	// Sculpt mode property
	json::Json sculptModeProp = json::Json::object();
	sculptModeProp.set("type", "string");
	sculptModeProp.set("description",
					   "The sculpt mode: 'erode' (remove surface voxels), 'grow' (add voxels to surface), "
					   "'flatten' (flatten surface along a face), 'smoothadditive' (smooth by adding), "
					   "'smootherode' (smooth by removing), 'smoothgaussian' (Gaussian height blur), "
				   "'bridgegap' (connect boundary voxels with lines to bridge gaps), "
				   "'squashtoplane' (project all voxels onto the clicked plane), "
				   "'reskin' (apply skin pattern from clipboard onto surface), "
				   "'smoothwall' (interpolate surface heights from edge columns to smooth a wall)");
	json::Json enumArr = json::Json::array();
	enumArr.push("erode");
	enumArr.push("grow");
	enumArr.push("flatten");
	enumArr.push("smoothadditive");
	enumArr.push("smootherode");
	enumArr.push("smoothgaussian");
	enumArr.push("bridgegap");
	enumArr.push("squashtoplane");
	enumArr.push("reskin");
	enumArr.push("smoothwall");
	sculptModeProp.set("enum", enumArr);
	sculptModeProp.set("default", "erode");
	properties.set("sculptMode", sculptModeProp);

	// Strength property
	json::Json strengthProp = json::Json::object();
	strengthProp.set("type", "number");
	strengthProp.set("description", "Sculpt strength (0.0 to 1.0)");
	strengthProp.set("default", 0.5);
	strengthProp.set("minimum", 0.0);
	strengthProp.set("maximum", 1.0);
	properties.set("strength", strengthProp);

	// Iterations property
	json::Json iterationsProp = json::Json::object();
	iterationsProp.set("type", "integer");
	iterationsProp.set("description", "Number of sculpt iterations (1-10, up to 64 for flatten)");
	iterationsProp.set("default", 1);
	iterationsProp.set("minimum", 1);
	iterationsProp.set("maximum", 64);
	properties.set("iterations", iterationsProp);

	// Face property (required for flatten/smooth modes)
	json::Json faceProp = json::Json::object();
	faceProp.set("type", "string");
	faceProp.set("description",
				 "The face direction for flatten/smooth modes: 'positivex', 'negativex', 'positivey', "
				 "'negativey', 'positivez', 'negativez' (or aliases like 'up', 'down', 'left', 'right', etc.)");
	faceProp.set("default", "positivey");
	properties.set("face", faceProp);

	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool SculptBrushTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::UUID nodeUUID = argsUUID(args);
	if (!nodeUUID.isValid()) {
		return ctx.result(id, "Invalid node UUID - fetch the scene state first", true);
	}

	const core::String sculptModeStr = args.strVal("sculptMode", "erode").c_str();
	const SculptMode sculptMode = parseSculptMode(sculptModeStr);
	const float strength = args.floatVal("strength", 0.5f);
	const int iterations = args.intVal("iterations", 1);
	const core::String faceStr = args.strVal("face", "positivey").c_str();
	const voxel::FaceNames face = voxel::toFaceNames(faceStr, voxel::FaceNames::PositiveY);

	scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraphNodeByUUID(nodeUUID);
	if (node == nullptr) {
		return ctx.result(id, "Node not found - fetch the scene state first", true);
	}

	voxel::RawVolume *volume = ctx.sceneMgr->volume(node->id());
	if (volume == nullptr) {
		return ctx.result(id, "Volume not found - this is no model node", true);
	}

	// Create the brush context
	BrushContext brushContext;
	brushContext.cursorVoxel = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	brushContext.modifierType = ModifierType::Override;
	brushContext.cursorPosition = volume->region().getCenter();
	brushContext.referencePos = brushContext.cursorPosition;
	brushContext.cursorFace = face;
	brushContext.targetVolumeRegion = volume->region();
	brushContext.gridResolution = 1;

	// Create the modifier wrapper
	ModifierVolumeWrapper wrapper(*node, ModifierType::Override);

	// Get and configure the sculpt brush
	Modifier &modifier = ctx.sceneMgr->modifier();
	SculptBrush &sculptBrush = modifier.sculptBrush();

	// Save previous state
	const BrushType prevBrushType = modifier.brushType();
	const SculptMode prevSculptMode = sculptBrush.sculptMode();
	const float prevStrength = sculptBrush.strength();
	const int prevIterations = sculptBrush.iterations();

	// Configure the sculpt brush
	modifier.setBrushType(BrushType::Sculpt);
	sculptBrush.setSculptMode(sculptMode);
	sculptBrush.setStrength(strength);
	sculptBrush.setIterations(iterations);

	// For Reskin mode, set up the skin volume from the global clipboard
	if (sculptMode == SculptMode::Reskin) {
		const voxel::ClipboardData &clip = ctx.sceneMgr->clipboardData();
		if (clip && clip.volume != nullptr) {
			sculptBrush.setSkinVolume(clip.volume);
		} else {
			return ctx.result(id, "Reskin mode requires voxels in the clipboard - copy voxels first", true);
		}
	}

	// Begin and execute the brush
	sculptBrush.beginBrush(brushContext);
	sculptBrush.preExecute(brushContext, volume);
	const bool success = sculptBrush.execute(ctx.sceneMgr->sceneGraph(), wrapper, brushContext);

	// End the brush operation
	sculptBrush.endBrush(brushContext);

	// Restore previous state
	sculptBrush.setSculptMode(prevSculptMode);
	sculptBrush.setStrength(prevStrength);
	sculptBrush.setIterations(prevIterations);
	modifier.setBrushType(prevBrushType);

	const voxel::Region &dirtyRegion = wrapper.dirtyRegion();
	if (dirtyRegion.isValid()) {
		ctx.sceneMgr->modified(node->id(), dirtyRegion);
		return ctx.result(id,
						  core::String::format("Sculpt '%s' executed successfully (strength=%.2f, iterations=%d)",
											   sculptModeStr.c_str(), strength, iterations),
						  false);
	}

	if (success) {
		return ctx.result(id, "Sculpt brush executed but no voxels were modified - ensure voxels are selected first",
						  false);
	}

	return ctx.result(id, "Failed to execute sculpt brush - ensure voxels are selected first (use voxedit_select_brush)",
					  true);
}

} // namespace voxedit
