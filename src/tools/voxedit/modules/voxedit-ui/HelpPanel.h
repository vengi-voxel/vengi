/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "ui/Panel.h"
#include "video/TexturePool.h"

namespace voxedit {

class MainWindow;

/**
 * @brief This help panel renders parts of the markdown documentation from the docs/ directory.
 *
 * See the @c CMakeLists.txt file for voxedit to get a list of all included markdown files.
 */
class HelpPanel : public ui::Panel {
public:
	struct State {
		core::String _basePath;
		core::String _filename;
	};

private:
	using Super = ui::Panel;

	MainWindow *_mainWindow = nullptr;
	core::DynamicArray<State> _history;
	int _historyPosition = 0;
	core::String _markdown;
	core::String _pendingMarkdown;
	bool _hasPendingUpdate = false;

	void setMarkdownState(const State &state);
	void loadCurrentState();

public:
	HelpPanel(MainWindow *mainWindow, ui::IMGUIApp *app) : Super(app, "help"), _mainWindow(mainWindow) {
	}
	virtual ~HelpPanel() = default;
	void init();
	void update(const char *id);
	void setMarkdownFile(const core::String &file);
	void setMarkdown(const core::String &markdown);
	const video::TexturePoolPtr &texturePool() const;
	void goBack();
	void goForward();
	bool canGoBack() const;
	bool canGoForward() const;

	const State &c() const {
		return _history[_historyPosition];
	}

#ifdef IMGUI_ENABLE_TEST_ENGINE
	void registerUITests(ImGuiTestEngine *engine, const char *id) override;
#endif
};

inline void HelpPanel::setMarkdown(const core::String &markdown) {
	_pendingMarkdown = markdown;
	_hasPendingUpdate = true;
}

} // namespace voxedit
