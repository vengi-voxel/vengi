/**
 * @file
 */

#pragma once

#include "app/App.h"
#include "video/IEventObserver.h"
#include "video/EventHandler.h"
#include "core/GLM.h"
#include "io/FormatDescription.h"
#include "util/KeybindingParser.h"
#include "util/KeybindingHandler.h"
#include "video/Types.h"
#include "video/Trace.h"
#include "video/Version.h"
#include <SDL_main.h>
#include <functional>
#include <glm/vec2.hpp>

struct SDL_Window;

namespace video {

/**
 * @brief An application with the usual lifecycle, but with a window attached to it.
 * @ingroup Video
 *
 * This application also receives input events (and others) from @c io::IEventObserver
 */
class WindowedApp: public app::App, public io::IEventObserver {
private:
	using Super = app::App;
protected:
	SDL_Window* _window = nullptr;
	RendererContext _rendererContext = nullptr;
	glm::ivec2 _frameBufferDimension;
	glm::ivec2 _windowDimension;
	float _aspect = 1.0f;
	double _fps = 0.0;
	float _dpiFactor = 1.0f;
	float _dpiHorizontalFactor = 1.0f;
	float _dpiVerticalFactor = 1.0f;
	bool _allowRelativeMouseMode = true;
	bool _showWindow = true;
	bool _singleWindowMode = false;

	util::KeyBindingHandler _keybindingHandler;
	/**
	 * @brief The current mouse position in the window
	 */
	glm::ivec2 _mousePos;
	/**
	 * @brief Delta of the mouse movement since the last frame
	 */
	glm::ivec2 _mouseRelativePos;

	WindowedApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);

	bool handleKeyPress(int32_t key, int16_t modifier, uint16_t count = 1u);
	bool handleKeyRelease(int32_t key, int16_t modifier);

	virtual ~WindowedApp();

	void showCursor(bool show);

	void centerMousePosition();

	bool toggleRelativeMouseMode();

	/**
	 * @brief (Un-)Grabs the mouse
	 */
	bool setRelativeMouseMode(bool mode);

public:
	bool isRelativeMouseMode() const;
	bool isSingleWindowMode() const;

	void *windowHandle();

	/**
	 * @brief This may differ from windowDimension() if we're rendering to a high-DPI
	 * drawable, i.e. the window was created with high-DPI support (Apple calls this
	 * "Retina").
	 */
	const glm::ivec2& frameBufferDimension() const;
	/**
	 * @brief This is the window size
	 *
	 * The window size in screen coordinates may differ from the size in pixels, if
	 * the window was created with high-dpi support (e.g. iOS or OS X). Use
	 * pixelDimension() to get the real client area size in pixels.
	 */
	const glm::ivec2& windowDimension() const;
	int frameBufferWidth() const;
	int frameBufferHeight() const;

	core::String getKeyBindingsString(const char *cmd) const;

	enum class OpenFileMode {
		Save, Open, Directory
	};

	/**
	 * @brief Opens a file dialog
	 * @param[in] mode @c OpenFileMode
	 * @param[in] formats nullptr terminated list of formats that are used to filter the entries
	 */
	virtual void fileDialog(const std::function<void(const core::String&)>& callback, OpenFileMode mode, const io::FormatDescription* formats = nullptr);

	/**
	 * @brief Wrapper method for @c fileDialog()
	 * @param[in] formats nullptr terminated list of formats that are used to filter the entries
	 */
	void saveDialog(const std::function<void(const core::String)>& callback, const io::FormatDescription* formats = nullptr);
	/**
	 * @brief Wrapper method for @c fileDialog()
	 * @param[in] formats nullptr terminated list of formats that are used to filter the entries
	 */
	void openDialog(const std::function<void(const core::String)>& callback, const io::FormatDescription* formats = nullptr);
	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	void directoryDialog(const std::function<void(const core::String)>& callback);

	virtual app::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onCleanup() override;

	/**
	 * Minimize the application window but continue running
	 */
	void minimize();

	static double fps() {
		return getInstance()->_fps;
	}
	static WindowedApp* getInstance();
};

inline void* WindowedApp::windowHandle() {
	return _window;
}

inline bool WindowedApp::isSingleWindowMode() const {
	return _singleWindowMode;
}

inline const glm::ivec2& WindowedApp::frameBufferDimension() const {
	return _frameBufferDimension;
}

inline const glm::ivec2& WindowedApp::windowDimension() const {
	return _windowDimension;
}

inline int WindowedApp::frameBufferWidth() const {
	return _frameBufferDimension.x;
}

inline int WindowedApp::frameBufferHeight() const {
	return _frameBufferDimension.y;
}

inline void WindowedApp::saveDialog(const std::function<void(const core::String)>& callback, const io::FormatDescription* formats) {
	fileDialog(callback, OpenFileMode::Save, formats);
}

inline void WindowedApp::openDialog(const std::function<void(const core::String)>& callback, const io::FormatDescription* formats) {
	fileDialog(callback, OpenFileMode::Open, formats);
}

inline void WindowedApp::directoryDialog(const std::function<void(const core::String)>& callback) {
	fileDialog(callback, OpenFileMode::Directory);
}

}
