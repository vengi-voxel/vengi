/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/Var.h"
#include "core/collection/Buffer.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "ui/Panel.h"

#include <glm/vec3.hpp>

namespace scenegraph {
class SceneGraphNode;
class SceneGraphNodeCamera;
} // namespace scenegraph

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class NodeInspectorPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	core::VarPtr _localSpace;
	core::VarPtr _regionSizes;
	core::VarPtr _gridSize;
	core::Buffer<glm::ivec3> _validRegionSizes;
	glm::ivec3 _newRegionSize{32, 32, 32};
	SceneManagerPtr _sceneMgr;
	core::String _propertyKey;
	core::String _propertyValue;

	void modelRegions(command::CommandExecutionListener &listener, scenegraph::SceneGraphNode &node);
	void updateModelRegionSizes();
	void saveRegionSizes(const core::Buffer<glm::ivec3> &regionSizes);
	void modelProperties(scenegraph::SceneGraphNode &node);
	void modelView(command::CommandExecutionListener &listener);
	void modelViewMenuBar(scenegraph::SceneGraphNode &node);
	void keyFrameInterpolationSettings(scenegraph::SceneGraphNode &node, scenegraph::KeyFrameIndex keyFrameIdx);
	void sceneView(command::CommandExecutionListener &listener, scenegraph::SceneGraphNode &node);
	void sceneViewMenuBar(scenegraph::SceneGraphNode &node);
	void detailView(scenegraph::SceneGraphNode &node);
	/**
	 * @return @c true if the property was handled with a special ui input widget - @c false if it should just be a
	 * normal text input field
	 */
	bool handleCameraProperty(scenegraph::SceneGraphNodeCamera &node, const core::String &key,
							  const core::String &value);

public:
	NodeInspectorPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
		: Super(app, "nodeinspector"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void shutdown();
	void update(const char *id, bool sceneMode, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
