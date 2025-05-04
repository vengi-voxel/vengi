/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "scenegraph/SceneGraphNode.h"
#include "ui/Panel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class NodePropertiesPanel : public ui::Panel {
private:
	using Super = ui ::Panel;
	SceneManagerPtr _sceneMgr;
	core::String _propertyKey;
	core::String _propertyValue;

	/**
	 * @return @c true if the property was handled with a special ui input widget - @c false if it should just be a
	 * normal text input field
	 */
	bool handleCameraProperty(const scenegraph::SceneGraphNodeCamera &node, const core::String &key,
							  const core::String &value);

public:
	NodePropertiesPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr)
		: Super(app, "nodeproperties"), _sceneMgr(sceneMgr) {
	}
	bool init();
	void shutdown();
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
