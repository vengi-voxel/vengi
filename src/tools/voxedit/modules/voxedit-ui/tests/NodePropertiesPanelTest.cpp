/**
 * @file
 */

 #include "../NodePropertiesPanel.h"
 #include "scenegraph/SceneGraph.h"
 #include "scenegraph/SceneGraphNode.h"
 #include "ui/IconsLucide.h"
 #include "voxedit-util/SceneManager.h"
 #include "TestUtil.h"

 namespace voxedit {

 void NodePropertiesPanel::registerUITests(ImGuiTestEngine *engine, const char *id) {
	 IM_REGISTER_TEST(engine, testCategory(), "properties")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(focusWindow(ctx, id));
		ctx->ItemInputValue("##nodeproperties/##newpropertykey", "Key");
		ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "Value");
		ctx->ItemClick("##nodeproperties/###nodepropertyadd");
	 };

	 IM_REGISTER_TEST(engine, testCategory(), "add and remove property")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));

		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode &node = sceneGraph.node(sceneGraph.activeNode());
		const int propsBefore = (int)node.properties().size();

		// add a new property
		ctx->ItemInputValue("##nodeproperties/##newpropertykey", "TestPropKey");
		ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "TestPropValue");
		ctx->ItemClick("##nodeproperties/###nodepropertyadd");
		ctx->Yield();

		const int propsAfterAdd = (int)node.properties().size();
		IM_CHECK_EQ(propsAfterAdd, propsBefore + 1);

		// click the delete button for the property we just added
		const core::String deleteId = core::String::format("##nodeproperties/TestPropKey/%s", ICON_LC_X);
		ctx->ItemClick(deleteId.c_str());
		ctx->Yield();

		IM_CHECK_EQ((int)node.properties().size(), propsBefore);
	 };

	 IM_REGISTER_TEST(engine, testCategory(), "boolean property")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));

		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode &node = sceneGraph.node(sceneGraph.activeNode());

		// add a boolean property
		ctx->ItemInputValue("##nodeproperties/##newpropertykey", "BoolProp");
		ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "false");
		ctx->ItemClick("##nodeproperties/###nodepropertyadd");
		ctx->Yield();

		IM_CHECK(node.property("BoolProp") == "false");

		// click the checkbox to toggle it
		ctx->ItemClick("##nodeproperties/BoolProp/##val");
		ctx->Yield();
		IM_CHECK(node.property("BoolProp") == "true");
	 };

	 IM_REGISTER_TEST(engine, testCategory(), "edit existing property")->TestFunc = [=](ImGuiTestContext *ctx) {
		IM_CHECK(activateViewportSceneMode(ctx, _app));
		IM_CHECK(resetScene(ctx, _sceneMgr));
		IM_CHECK(focusWindow(ctx, id));

		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		scenegraph::SceneGraphNode &node = sceneGraph.node(sceneGraph.activeNode());

		// add a text property
		ctx->ItemInputValue("##nodeproperties/##newpropertykey", "EditMe");
		ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "original");
		ctx->ItemClick("##nodeproperties/###nodepropertyadd");
		ctx->Yield();
		IM_CHECK(node.property("EditMe") == "original");

		// edit the value via the ##val input
		ctx->ItemInputValue("##nodeproperties/EditMe/##val", "modified");
		ctx->Yield();
		IM_CHECK(node.property("EditMe") == "modified");
	 };
 }

 } // namespace voxedit
