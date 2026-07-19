/**
 * @file
 */

#include "app/tests/AbstractTest.h"
#include "core/StringUtil.h"
#include "core/TimeProvider.h"
#include "core/Var.h"
#include "image/Image.h"
#include "json/JSON.h"
#include "voxel/RawVolume.h"
#include "voxedit-mcp/AnimationAddTool.h"
#include "voxedit-mcp/AnimationSetTool.h"
#include "voxedit-mcp/CommandTool.h"
#include "voxedit-mcp/ExtrudeBrushTool.h"
#include "voxedit-mcp/FindColorTool.h"
#include "voxedit-mcp/GetPaletteTool.h"
#include "voxedit-mcp/GetSceneStateTool.h"
#include "voxedit-mcp/GetVoxelsTool.h"
#include "voxedit-mcp/HistogramTool.h"
#include "voxedit-mcp/LineBrushTool.h"
#include "voxedit-mcp/MementoCanRedoTool.h"
#include "voxedit-mcp/MementoCanUndoTool.h"
#include "voxedit-mcp/MementoRedoTool.h"
#include "voxedit-mcp/MementoUndoTool.h"
#include "voxedit-mcp/NodeAddKeyframeTool.h"
#include "voxedit-mcp/NodeAddModelTool.h"
#include "voxedit-mcp/NodeMoveTool.h"
#include "voxedit-mcp/NodeRemoveTool.h"
#include "voxedit-mcp/NodeRenameTool.h"
#include "voxedit-mcp/NodeSetPropertiesTool.h"
#include "voxedit-mcp/PaintBrushTool.h"
#include "voxedit-mcp/PlaceVoxelsTool.h"
#include "voxedit-mcp/PlaneBrushTool.h"
#include "voxedit-mcp/ScriptApiTool.h"
#include "voxedit-mcp/ScriptCreateTool.h"
#include "voxedit-mcp/ScreenshotTool.h"
#include "voxedit-mcp/SculptBrushTool.h"
#include "voxedit-mcp/SelectBrushTool.h"
#include "voxedit-mcp/ShapeBrushTool.h"
#include "voxedit-mcp/ToolRegistry.h"
#include "voxedit-mcp/TransformBrushTool.h"
#include "voxedit-util/Config.h"
#include "voxedit-util/ISceneRenderer.h"
#include "voxedit-util/SceneManager.h"
#include "voxedit-util/modifier/IModifierRenderer.h"

namespace voxedit {

namespace {

struct ImageResultCapture {
	core::String pngBase64;
	core::String mimeType;
	core::String text;
	bool called = false;
	bool isError = false;

	static bool capture(const json::Json &, const core::String &pngBase64, const core::String &mimeType,
						const core::String &text, bool isError) {
		ImageResultCapture *self = _instance;
		self->called = true;
		self->pngBase64 = pngBase64;
		self->mimeType = mimeType;
		self->text = text;
		self->isError = isError;
		return !isError;
	}

	static bool textFallback(const json::Json &, const core::String &text, bool isError) {
		ImageResultCapture *self = _instance;
		self->called = true;
		self->text = text;
		self->isError = isError;
		return !isError;
	}

	static ImageResultCapture *_instance;
};

ImageResultCapture *ImageResultCapture::_instance = nullptr;

} // namespace

class ToolsTest : public app::AbstractTest {
private:
	using Super = app::AbstractTest;

protected:
	SceneManagerPtr _sceneMgr;

	void TearDown() override {
		if (_sceneMgr) {
			_sceneMgr->shutdown();
			_sceneMgr.release();
		}
		Super::TearDown();
	}

	void SetUp() override {
		Super::SetUp();
		const auto timeProvider = core::make_shared<core::TimeProvider>();
		const auto sceneRenderer = core::make_shared<ISceneRenderer>();
		const auto modifierRenderer = core::make_shared<IModifierRenderer>();
		_sceneMgr =
			core::make_shared<SceneManager>(timeProvider, _testApp->filesystem(), sceneRenderer, modifierRenderer);
		const core::VarDef uILastDirectory(cfg::UILastDirectory, "", "Last Directory",
										   "The last directory used in the UI", core::CV_NOPERSIST);
		core::Var::registerVar(uILastDirectory);
		const core::VarDef clientMouseRotationSpeed(cfg::ClientMouseRotationSpeed, 0.01f, "Mouse Rotation Speed",
													"The speed at which the camera rotates with the mouse",
													core::CV_NONE);
		core::Var::registerVar(clientMouseRotationSpeed);
		const core::VarDef clientCameraZoomSpeed(cfg::ClientCameraZoomSpeed, 0.1f, "Camera Zoom Speed",
												 "The speed at which the camera zooms", core::CV_NONE);
		core::Var::registerVar(clientCameraZoomSpeed);
		_sceneMgr->construct();
		ASSERT_TRUE(_sceneMgr->init());
		ASSERT_TRUE(_sceneMgr->newScene(true, "screenshot-test", voxel::Region{0, 7}));
	}
};

TEST_F(ToolsTest, screenshotToolOrthographicAndIsometric) {
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
	ASSERT_NE(nullptr, node);
	voxel::RawVolume *volume = node->volume();
	ASSERT_NE(nullptr, volume);
	ASSERT_TRUE(volume->setVoxel(2, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 1)));
	ASSERT_TRUE(volume->setVoxel(3, 2, 2, voxel::createVoxel(voxel::VoxelType::Generic, 2)));
	ASSERT_TRUE(volume->setVoxel(2, 3, 2, voxel::createVoxel(voxel::VoxelType::Generic, 3)));

	ScreenshotTool tool;
	ImageResultCapture capture;
	ImageResultCapture::_instance = &capture;

	ToolContext ctx;
	ctx.sceneMgr = _sceneMgr.get();
	ctx.result = ImageResultCapture::textFallback;
	ctx.resultImage = ImageResultCapture::capture;

	json::Json args = json::Json::object();
	args.set("nodeUUID", node->uuid().str().c_str());
	args.set("face", "front");
	ASSERT_TRUE(tool.execute(json::Json::parse("1"), args, ctx));
	ASSERT_TRUE(capture.called);
	ASSERT_FALSE(capture.isError);
	EXPECT_EQ("image/png", capture.mimeType);
	EXPECT_FALSE(capture.pngBase64.empty());
	EXPECT_TRUE(core::string::startsWith(capture.pngBase64, "iVBORw0KGgo"));
	EXPECT_TRUE(capture.text.contains("orthographic"));

	capture = ImageResultCapture{};
	ImageResultCapture::_instance = &capture;
	args.set("isometric", true);
	ASSERT_TRUE(tool.execute(json::Json::parse("2"), args, ctx));
	ASSERT_TRUE(capture.called);
	ASSERT_FALSE(capture.isError);
	EXPECT_EQ("image/png", capture.mimeType);
	EXPECT_FALSE(capture.pngBase64.empty());
	EXPECT_TRUE(core::string::startsWith(capture.pngBase64, "iVBORw0KGgo"));
	EXPECT_TRUE(capture.text.contains("isometric"));

	ImageResultCapture::_instance = nullptr;
}

TEST_F(ToolsTest, screenshotToolMergedScene) {
	scenegraph::SceneGraphNode *node = _sceneMgr->sceneGraphModelNode(_sceneMgr->sceneGraph().activeNode());
	ASSERT_NE(nullptr, node);
	ASSERT_TRUE(node->volume()->setVoxel(1, 1, 1, voxel::createVoxel(voxel::VoxelType::Generic, 1)));

	ScreenshotTool tool;
	ImageResultCapture capture;
	ImageResultCapture::_instance = &capture;

	ToolContext ctx;
	ctx.sceneMgr = _sceneMgr.get();
	ctx.result = ImageResultCapture::textFallback;
	ctx.resultImage = ImageResultCapture::capture;

	json::Json args = json::Json::object();
	args.set("face", "left");
	args.set("width", 32);
	args.set("height", 32);
	ASSERT_TRUE(tool.execute(json::Json::parse("1"), args, ctx));
	ASSERT_TRUE(capture.called);
	ASSERT_FALSE(capture.isError);
	EXPECT_TRUE(capture.text.contains("merged scene"));
	EXPECT_TRUE(core::string::startsWith(capture.pngBase64, "iVBORw0KGgo"));

	ImageResultCapture::_instance = nullptr;
}

static void assertValidMcpToolSchema(const json::Json &tool) {
	ASSERT_TRUE(tool.isObject()) << tool.dump();
	ASSERT_TRUE(tool.contains("name")) << tool.dump();
	ASSERT_TRUE(tool.get("name").isString()) << tool.dump();
	ASSERT_TRUE(tool.contains("description")) << tool.dump();
	ASSERT_TRUE(tool.contains("inputSchema")) << tool.get("name").str().c_str() << " missing inputSchema";
	const json::Json schema = tool.get("inputSchema");
	ASSERT_TRUE(schema.isObject()) << tool.get("name").str().c_str();
	ASSERT_EQ("object", schema.strVal("type", "")) << tool.get("name").str().c_str();
	ASSERT_TRUE(schema.contains("properties")) << tool.get("name").str().c_str() << " missing properties";
	ASSERT_TRUE(schema.get("properties").isObject()) << tool.get("name").str().c_str();
	if (schema.contains("required")) {
		ASSERT_TRUE(schema.get("required").isArray()) << tool.get("name").str().c_str();
		for (const auto &req : schema.get("required")) {
			ASSERT_TRUE(req.isString()) << tool.get("name").str().c_str();
			ASSERT_TRUE(schema.get("properties").contains(req.cStr()))
				<< tool.get("name").str().c_str() << " required '" << req.cStr() << "' missing from properties";
		}
	}
}

TEST_F(ToolsTest, registeredToolsHaveValidMcpInputSchemas) {
	ToolRegistry registry;
	registry.registerTool(new AnimationAddTool());
	registry.registerTool(new AnimationSetTool());
	registry.registerTool(new FindColorTool());
	registry.registerTool(new GetPaletteTool());
	registry.registerTool(new GetSceneStateTool());
	registry.registerTool(new GetVoxelsTool());
	registry.registerTool(new HistogramTool());
	registry.registerTool(new ScreenshotTool());
	registry.registerTool(new MementoCanRedoTool());
	registry.registerTool(new MementoCanUndoTool());
	registry.registerTool(new MementoRedoTool());
	registry.registerTool(new MementoUndoTool());
	registry.registerTool(new NodeAddKeyframeTool());
	registry.registerTool(new NodeAddModelTool());
	registry.registerTool(new NodeMoveTool());
	registry.registerTool(new NodeRemoveTool());
	registry.registerTool(new NodeRenameTool());
	registry.registerTool(new NodeSetPropertiesTool());
	registry.registerTool(new PlaceVoxelsTool());
	registry.registerTool(new ScriptApiTool());
	registry.registerTool(new ScriptCreateTool());
	registry.registerTool(new ShapeBrushTool());
	registry.registerTool(new PaintBrushTool());
	registry.registerTool(new LineBrushTool());
	registry.registerTool(new SelectBrushTool());
	registry.registerTool(new PlaneBrushTool());
	registry.registerTool(new SculptBrushTool());
	registry.registerTool(new TransformBrushTool());
	registry.registerTool(new ExtrudeBrushTool());
	registry.registerTool(new CommandTool());
	registry.registerTool(new CommandListTool());

	json::Json tools = json::Json::array();
	registry.addRegisteredTools(tools);
	ASSERT_GT(tools.size(), 0);
	for (const auto &tool : tools) {
		assertValidMcpToolSchema(tool);
	}
}

} // namespace voxedit
