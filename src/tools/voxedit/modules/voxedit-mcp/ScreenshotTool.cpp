/**
 * @file
 */

#include "ScreenshotTool.h"
#include "core/Log.h"
#include "image/Image.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphImage.h"
#include "voxel/Face.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {

ScreenshotTool::ScreenshotTool() : Tool("voxedit_screenshot") {
	_tool.set("description",
			  "Render a 2D screenshot of a node and its children, or the full scene, without a GL context. "
			  "Uses pixel-perfect face projection (renderToImage) or isometric (renderIsometricImage). "
			  "Returns an MCP image (PNG). Prefer this over get_voxels for visual inspection.");

	json::Json faceProp = propTypeDescription(
		"string", "View direction: front, back, left, right, up, down (also north/south/east/west). Default: front");
	json::Json faceEnum = json::Json::array();
	faceEnum.push("front");
	faceEnum.push("back");
	faceEnum.push("left");
	faceEnum.push("right");
	faceEnum.push("up");
	faceEnum.push("down");
	faceProp.set("enum", faceEnum);
	faceProp.set("default", "front");

	json::Json isometricProp = propTypeDescription("boolean", "If true, render an isometric view. Default: false");
	isometricProp.set("default", false);

	json::Json widthProp = propTypeDescription("integer", "Optional output width in pixels (-1 keeps volume size)");
	widthProp.set("default", -1);
	json::Json heightProp = propTypeDescription("integer", "Optional output height in pixels (-1 keeps volume size)");
	heightProp.set("default", -1);

	json::Json depthFactorProp = propTypeDescription(
		"number", "Optional depth shading factor for non-isometric renders (0 = off). Ignored for isometric.");
	depthFactorProp.set("default", 0.0);

	json::Json rProp = propTypeDescription("integer", "Background red (0-255)");
	rProp.set("minimum", 0);
	rProp.set("maximum", 255);
	rProp.set("default", 0);
	json::Json gProp = propTypeDescription("integer", "Background green (0-255)");
	gProp.set("minimum", 0);
	gProp.set("maximum", 255);
	gProp.set("default", 0);
	json::Json bProp = propTypeDescription("integer", "Background blue (0-255)");
	bProp.set("minimum", 0);
	bProp.set("maximum", 255);
	bProp.set("default", 0);
	json::Json aProp = propTypeDescription("integer", "Background alpha (0-255)");
	aProp.set("minimum", 0);
	aProp.set("maximum", 255);
	aProp.set("default", 255);

	json::Json properties = json::Json::object();
	properties.set(
		"nodeUUID",
		propTypeDescription("string",
							"Optional UUID of a model node. If omitted, all visible model nodes are merged and rendered."));
	properties.set("face", core::move(faceProp));
	properties.set("isometric", core::move(isometricProp));
	properties.set("width", core::move(widthProp));
	properties.set("height", core::move(heightProp));
	properties.set("depthFactor", core::move(depthFactorProp));
	properties.set("bgR", core::move(rProp));
	properties.set("bgG", core::move(gProp));
	properties.set("bgB", core::move(bProp));
	properties.set("bgA", core::move(aProp));

	json::Json inputSchema = json::Json::object();
	inputSchema.set("type", "object");
	inputSchema.set("properties", core::move(properties));
	_tool.set("inputSchema", core::move(inputSchema));
}

bool ScreenshotTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::String faceStr = args.strVal("face", "front").c_str();
	voxel::FaceNames face = voxel::toFaceNames(faceStr, voxel::FaceNames::NegativeZ);
	if (face == voxel::FaceNames::Max) {
		face = voxel::FaceNames::NegativeZ;
	}

	const bool isometric = args.boolVal("isometric", false);
	const int width = args.intVal("width", -1);
	const int height = args.intVal("height", -1);
	const float depthFactor = (float)args.doubleVal("depthFactor", 0.0);
	const color::RGBA background((uint8_t)args.intVal("bgR", 0), (uint8_t)args.intVal("bgG", 0),
								 (uint8_t)args.intVal("bgB", 0), (uint8_t)args.intVal("bgA", 255));

	int nodeId = InvalidNodeId;
	core::String sourceLabel = "merged scene";
	const core::UUID nodeUUID = argsUUID(args);
	if (nodeUUID.isValid()) {
		const scenegraph::SceneGraphNode *node = ctx.sceneMgr->sceneGraph().findNodeByUUID(nodeUUID);
		if (node == nullptr) {
			return ctx.result(id, "Node not found - fetch the scene state first", true);
		}
		nodeId = node->id();
		sourceLabel = core::String::format("node %s", node->name().c_str());
	}

	const scenegraph::SceneGraph &sceneGraph = ctx.sceneMgr->sceneGraph();
	const scenegraph::FrameIndex frameIdx = ctx.sceneMgr->currentFrame();
	image::ImagePtr image;
	if (isometric) {
		voxelutil::IsoRenderConfig config = voxelutil::IsoRenderConfig::fromCvars();
		config.frontFace = face;
		config.background = background;
		config.imgW = width;
		config.imgH = height;
		image = scenegraph::renderIsometricImage(sceneGraph, config, nodeId, frameIdx);
	} else {
		voxelutil::ImageRenderConfig config;
		config.frontFace = face;
		config.background = background;
		config.imgW = width;
		config.imgH = height;
		config.depthFactor = depthFactor;
		image = scenegraph::renderToImage(sceneGraph, config, nodeId, frameIdx);
	}

	if (!image || !image->isLoaded()) {
		return ctx.result(id, "Failed to render screenshot", true);
	}

	const core::String pngBase64 = image->pngBase64();
	if (pngBase64.empty()) {
		return ctx.result(id, "Failed to encode screenshot as PNG", true);
	}

	Log::info("Screenshot encoded %ix%i (%u base64 bytes)", image->width(), image->height(),
			  (uint32_t)pngBase64.size());

	const core::String text = core::String::format(
		"Screenshot of %s (%s, face=%s, %ix%i)", sourceLabel.c_str(), isometric ? "isometric" : "orthographic",
		faceStr.c_str(), image->width(), image->height());

	if (ctx.resultImage != nullptr) {
		return ctx.resultImage(id, pngBase64, "image/png", text, false);
	}
	// Fallback for callers that only wire text results
	return ctx.result(id, core::String::format("%s\ndata:image/png;base64,%s", text.c_str(), pngBase64.c_str()), false);
}

} // namespace voxedit
