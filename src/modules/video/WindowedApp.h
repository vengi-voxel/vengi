/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include "core/GLM.h"
#include "util/KeybindingParser.h"
#include "util/KeybindingHandler.h"
#include "video/Types.h"
#include "video/Version.h"

struct SDL_Window;

namespace video {

/**
 * @brief An application with the usual lifecycle, but with a window attached to it.
 * @ingroup Video
 *
 * This application also receives input events (and others) from @c io::IEventObserver
 */
class WindowedApp: public core::App, public io::IEventObserver {
private:
	using Super = core::App;
protected:
	SDL_Window* _window = nullptr;
	RendererContext _rendererContext = nullptr;
	glm::ivec2 _frameBufferDimension;
	glm::ivec2 _windowDimension;
	float _aspect = 1.0f;
	int _fps = 0;
	uint32_t _frameCounter = 0;
	float _dpiFactor = 1.0f;
	float _dpiHorizontalFactor = 1.0f;
	float _dpiVerticalFactor = 1.0f;
	double _frameCounterResetTime = 0.0;
	bool _allowRelativeMouseMode = true;

	util::KeyBindingHandler _keybindingHandler;
	/**
	 * @brief The current mouse position in the window
	 */
	glm::ivec2 _mousePos;
	/**
	 * @brief Delta of the mouse movement since the last frame
	 */
	glm::ivec2 _mouseRelativePos;

	WindowedApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	bool handleKeyPress(int32_t key, int16_t modifier);
	bool handleKeyRelease(int32_t key, int16_t modifier);

	virtual ~WindowedApp();

	void showCursor(bool show);

	void centerMousePosition();

	bool isRelativeMouseMode() const;

	bool toggleRelativeMouseMode();

	bool setRelativeMouseMode(bool mode);

public:
	/**
	 * @brief This may differ from screenDimension() if we're rendering to a high-DPI
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

	std::string getKeyBindingsString(const char *cmd) const;

	enum class OpenFileMode {
		Save, Open, Directory
	};

	/**
	 * @brief Opens a file dialog
	 * @param[in] mode @c OpenFileMode
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 */
	virtual void fileDialog(const std::function<void(const std::string&)>& callback, OpenFileMode mode, const std::string& filter = "");

	/**
	 * @brief Wrapper method for @c fileDialog()
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 */
	void saveDialog(const std::function<void(const std::string)>& callback, const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 */
	void openDialog(const std::function<void(const std::string)>& callback, const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	void directoryDialog(const std::function<void(const std::string)>& callback);

	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;

	static int fps() {
		return getInstance()->_fps;
	}
	static WindowedApp* getInstance();
};

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

inline void WindowedApp::saveDialog(const std::function<void(const std::string)>& callback, const std::string& filter) {
	fileDialog(callback, OpenFileMode::Save, filter);
}

inline void WindowedApp::openDialog(const std::function<void(const std::string)>& callback, const std::string& filter) {
	fileDialog(callback, OpenFileMode::Open, filter);
}

inline void WindowedApp::directoryDialog(const std::function<void(const std::string)>& callback) {
	fileDialog(callback, OpenFileMode::Directory);
}

}
