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

static image::ImagePtr renderView(const scenegraph::SceneGraph &sceneGraph, int nodeId, scenegraph::FrameIndex frameIdx,
								  voxel::FaceNames face, bool isometric, int width, int height, float depthFactor,
								  const color::RGBA &background) {
	if (isometric) {
		voxelutil::IsoRenderConfig config = voxelutil::IsoRenderConfig::fromCvars();
		config.frontFace = face;
		config.background = background;
		config.imgW = width;
		config.imgH = height;
		return scenegraph::renderIsometricImage(sceneGraph, config, nodeId, frameIdx);
	}
	voxelutil::ImageRenderConfig config;
	config.frontFace = face;
	config.background = background;
	config.imgW = width;
	config.imgH = height;
	config.depthFactor = depthFactor;
	return scenegraph::renderToImage(sceneGraph, config, nodeId, frameIdx);
}

static void blitNearest(image::Image &dst, const image::Image &src, int dstX, int dstY, int dstW, int dstH) {
	const int srcW = src.width();
	const int srcH = src.height();
	if (srcW <= 0 || srcH <= 0 || dstW <= 0 || dstH <= 0) {
		return;
	}
	for (int y = 0; y < dstH; ++y) {
		const int sy = y * srcH / dstH;
		for (int x = 0; x < dstW; ++x) {
			const int sx = x * srcW / dstW;
			dst.setColor(src.colorAt(sx, sy), dstX + x, dstY + y);
		}
	}
}

ScreenshotTool::ScreenshotTool() : Tool("voxedit_screenshot") {
	_tool.set("description",
			  "Render a 2D screenshot of a node and its children, or the full scene, without a GL context. "
			  "Uses pixel-perfect face projection (renderToImage) or isometric (renderIsometricImage). "
			  "view \"sheet\" returns one PNG with front, left, iso on the top row and back, right, top on the "
			  "bottom row - use that to inspect a model you just built. "
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

	json::Json viewProp = propTypeDescription(
		"string",
		"sheet (front, left, iso / back, right, top), or one of front, back, left, right, top, bottom, iso, "
		"iso_left, iso_back. When set, this overrides face and isometric.");
	json::Json viewEnum = json::Json::array();
	viewEnum.push("sheet");
	viewEnum.push("front");
	viewEnum.push("back");
	viewEnum.push("left");
	viewEnum.push("right");
	viewEnum.push("top");
	viewEnum.push("bottom");
	viewEnum.push("iso");
	viewEnum.push("iso_left");
	viewEnum.push("iso_back");
	viewProp.set("enum", viewEnum);

	json::Json sizeProp = propTypeDescription("integer", "Sheet width in pixels when view is sheet. Default 768, clamped to 128..1536.");
	sizeProp.set("default", 768);

	json::Json properties = json::Json::object();
	properties.set(
		"nodeUUID",
		propTypeDescription("string",
							"Optional UUID of a model node. If omitted, all visible model nodes are merged and rendered."));
	properties.set("view", core::move(viewProp));
	properties.set("size", core::move(sizeProp));
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

static bool finishImage(const json::Json &id, ToolContext &ctx, const image::ImagePtr &image, const core::String &text) {
	if (!image || !image->isLoaded()) {
		return ctx.result(id, "Failed to render screenshot", true);
	}
	const core::String pngBase64 = image->pngBase64();
	if (pngBase64.empty()) {
		return ctx.result(id, "Failed to encode screenshot as PNG", true);
	}
	Log::info("Screenshot encoded %ix%i (%u base64 bytes)", image->width(), image->height(),
			  (uint32_t)pngBase64.size());
	if (ctx.resultImage != nullptr) {
		return ctx.resultImage(id, pngBase64, "image/png", text, false);
	}
	return ctx.result(id, core::String::format("%s\ndata:image/png;base64,%s", text.c_str(), pngBase64.c_str()), false);
}

bool ScreenshotTool::execute(const json::Json &id, const json::Json &args, ToolContext &ctx) {
	const core::String view = args.strVal("view", "").toLower();
	core::String faceStr = args.strVal("face", "front").c_str();
	voxel::FaceNames face = voxel::toFaceNames(faceStr, voxel::FaceNames::NegativeZ);
	if (face == voxel::FaceNames::Max) {
		face = voxel::FaceNames::NegativeZ;
	}
	bool isometric = args.boolVal("isometric", false);
	if (view == "iso" || view == "isometric") {
		isometric = true;
		face = voxel::FaceNames::NegativeZ;
		faceStr = "front";
	} else if (view == "iso_left") {
		isometric = true;
		face = voxel::FaceNames::NegativeX;
		faceStr = "left";
	} else if (view == "iso_back") {
		isometric = true;
		face = voxel::FaceNames::PositiveZ;
		faceStr = "back";
	} else if (view == "top") {
		isometric = false;
		face = voxel::FaceNames::PositiveY;
		faceStr = "top";
	} else if (view == "bottom") {
		isometric = false;
		face = voxel::FaceNames::NegativeY;
		faceStr = "bottom";
	} else if (!view.empty() && view != "sheet") {
		face = voxel::toFaceNames(view, voxel::FaceNames::Max);
		if (face == voxel::FaceNames::Max) {
			return ctx.result(id, "Unknown view. Use sheet, front, back, left, right, top, bottom, iso, iso_left or iso_back",
							  true);
		}
		isometric = false;
		faceStr = view;
	}

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

	if (view == "sheet") {
		int size = args.intVal("size", width > 0 ? width : 768);
		if (size < 128) {
			size = 128;
		} else if (size > 1536) {
			size = 1536;
		}
		static constexpr int Gutter = 4;
		static constexpr int Cols = 3;
		static constexpr int Rows = 2;
		const int cell = (size - Gutter * (Cols + 1)) / Cols;
		if (cell < 8) {
			return ctx.result(id, "Sheet size is too small", true);
		}
		const int sheetW = cell * Cols + Gutter * (Cols + 1);
		const int sheetH = cell * Rows + Gutter * (Rows + 1);
		struct Tile {
			const char *label;
			voxel::FaceNames face;
			bool iso;
		};
		static const Tile tiles[] = {
			{"front", voxel::FaceNames::NegativeZ, false}, {"left", voxel::FaceNames::NegativeX, false},
			{"iso", voxel::FaceNames::NegativeZ, true},    {"back", voxel::FaceNames::PositiveZ, false},
			{"right", voxel::FaceNames::PositiveX, false}, {"top", voxel::FaceNames::PositiveY, false},
		};
		image::ImagePtr sheet = image::createEmptyImage("sheet");
		if (!sheet->load(sheetW, sheetH, [&background](int, int, color::RGBA &rgba) { rgba = background; })) {
			return ctx.result(id, "Failed to render screenshot", true);
		}
		for (int i = 0; i < 6; ++i) {
			image::ImagePtr tile = renderView(sceneGraph, nodeId, frameIdx, tiles[i].face, tiles[i].iso, cell, cell,
											  depthFactor, background);
			if (!tile || !tile->isLoaded()) {
				return ctx.result(id, "Failed to render screenshot", true);
			}
			const int col = i % Cols;
			const int row = i / Cols;
			blitNearest(*sheet.get(), *tile.get(), Gutter + col * (cell + Gutter), Gutter + row * (cell + Gutter), cell,
						cell);
		}
		const core::String text = core::String::format(
			"Screenshot sheet of %s (%ix%i): front, left, iso / back, right, top", sourceLabel.c_str(), sheet->width(),
			sheet->height());
		return finishImage(id, ctx, sheet, text);
	}

	image::ImagePtr image = renderView(sceneGraph, nodeId, frameIdx, face, isometric, width, height, depthFactor, background);
	const core::String text = core::String::format(
		"Screenshot of %s (%s, face=%s, %ix%i)", sourceLabel.c_str(), isometric ? "isometric" : "orthographic",
		faceStr.c_str(), image && image->isLoaded() ? image->width() : 0, image && image->isLoaded() ? image->height() : 0);
	return finishImage(id, ctx, image, text);
}

} // namespace voxedit
