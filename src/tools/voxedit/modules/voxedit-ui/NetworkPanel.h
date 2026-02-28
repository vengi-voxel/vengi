/**
 * @file
 */

#pragma once

#include "command/CommandHandler.h"
#include "core/SharedPtr.h"
#include "core/String.h"
#include "core/collection/DynamicArray.h"
#include "ui/Panel.h"

namespace voxedit {

class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;

class NetworkPanel : public ui::Panel {
private:
	using Super = ui::Panel;
	SceneManagerPtr _sceneMgr;
	core::String _command;
	core::String _chatInput;
	bool _scrollToBottom = false;
	bool _chatCallbackRegistered = false;
	bool _refocusChatInput = false;

	/**
	 * @brief Render the chat area (messages + input) shared by client and server tabs
	 */
	void renderChat();

	/**
	 * @brief Check for @mention autocomplete and render autocomplete popup
	 */
	void handleMentionAutocomplete();

public:
	NetworkPanel(ui::IMGUIApp *app, const SceneManagerPtr &sceneMgr) : Super(app, "network"), _sceneMgr(sceneMgr) {
	}
	virtual ~NetworkPanel() = default;

	void init();
	void update(const char *id, command::CommandExecutionListener &listener);
#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

} // namespace voxedit
