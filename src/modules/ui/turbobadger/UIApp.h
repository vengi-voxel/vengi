/**
 * @file
 */

#pragma once

#include "video/WindowedApp.h"
#include "Window.h"
#include "Console.h"
#include <tb_widgets_listener.h>

namespace ui {
namespace turbobadger {

/**
 * @ingroup UI
 */
class UIApp: public video::WindowedApp, private tb::TBWidgetListener {
private:
	using Super = video::WindowedApp;
protected:
	static constexpr uint32_t _logId = Log::logid("UIAPP");
	tb::TBWidget* _root = nullptr;
	Console _console;
	core::VarPtr _uiFontSize;
	core::VarPtr _renderUI;
	core::VarPtr _lastDirectory;
	int _lastShowTextY = -1;
	core::String _applicationSkin;
	bool _uiInitialized = false;

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;

	virtual void onWidgetFocusChanged(tb::TBWidget *widget, bool focused) override;

	bool invokeKey(int key, tb::SPECIAL_KEY special, tb::MODIFIER_KEYS mod, bool down);
	void showStr(int x, int y, const glm::vec4& color, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(5);
	void enqueueShowStr(int x, const glm::vec4& color, CORE_FORMAT_STRING const char *fmt, ...) CORE_PRINTF_VARARG_FUNC(4);

	tb::MODIFIER_KEYS getModifierKeys() const;
public:
	UIApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~UIApp();

	virtual void beforeUI() {
	}

	template<class T>
	T* getWidgetByType(const char *name);

	tb::TBWidget* getWidget(const char *name);
	tb::TBWidget* getWidgetAt(int x, int y, bool includeChildren = true);

	// hook that is called directory before the ui is rendered. Your last chance to let the app contribute
	// something in the ui context and the ui drawcalls (like debug text or rendering an in-game console on
	// top of the ui)
	virtual void afterRootWidget();

	void addChild(Window* window);

	/**
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 */
	void fileDialog(const std::function<void(const core::String&)>& callback, OpenFileMode mode, const core::String& filter) override;

	virtual void onWindowResize(int windowWidth, int windowHeight) override;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual app::AppState onCleanup() override;
};

template<class T>
inline T* UIApp::getWidgetByType(const char *name) {
	return _root->getWidgetByIDAndType<T>(tb::TBID(name));
}

}
}
