/**
 * @file
 */

 #include "../NodePropertiesPanel.h"
 #include "scenegraph/SceneGraph.h"
 #include "scenegraph/SceneGraphNode.h"
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
		 IM_CHECK(_sceneMgr->newScene(true, "nodepropsaddremove", voxel::Region(0, 31)));
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

		 // add a second property
		 ctx->ItemInputValue("##nodeproperties/##newpropertykey", "TestPropKey2");
		 ctx->ItemInputValue("##nodeproperties/##newpropertyvalue", "TestPropValue2");
		 ctx->ItemClick("##nodeproperties/###nodepropertyadd");
		 ctx->Yield();

		 const int propsAfterAdd2 = (int)node.properties().size();
		 IM_CHECK_EQ(propsAfterAdd2, propsBefore + 2);
	 };
 }

 } // namespace voxedit
