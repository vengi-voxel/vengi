/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include "core/GLM.h"
#include "util/KeybindingParser.h"
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
	glm::ivec2 _dimension;
	float _aspect = 1.0f;
	int _fps = 0;
	uint32_t _frameCounter = 0;
	float _dpiFactor = 1.0f;
	double _frameCounterResetTime = 0.0;

	std::unordered_set<int32_t> _keys;
	util::BindMap _bindings;
	glm::ivec2 _mousePos;
	glm::ivec2 _mouseRelativePos;

	WindowedApp(const metric::MetricPtr& metric, const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	bool loadKeyBindings(const std::string& filename = "keybindings.cfg");

	virtual ~WindowedApp();

	void showCursor(bool show);

	void centerMousePosition();

	bool isRelativeMouseMode() const;

	void toggleRelativeMouseMode();

	void setRelativeMouseMode(bool mode);

public:
	const glm::ivec2& dimension() const;
	int width() const;
	int height() const;
	bool isPressed(int32_t key) const;

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
	void saveDialog(const std::function<void(const std::string&)>& callback, const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 */
	void openDialog(const std::function<void(const std::string&)>& callback, const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	void directoryDialog(const std::function<void(const std::string&)>& callback);

	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
	virtual void onWindowResize() override;

	static WindowedApp* getInstance();
};

inline const glm::ivec2& WindowedApp::dimension() const {
	return _dimension;
}

inline int WindowedApp::width() const {
	return _dimension.x;
}

inline int WindowedApp::height() const {
	return _dimension.y;
}

inline void WindowedApp::saveDialog(const std::function<void(const std::string&)>& callback, const std::string& filter) {
	fileDialog(callback, OpenFileMode::Save, filter);
}

inline void WindowedApp::openDialog(const std::function<void(const std::string&)>& callback, const std::string& filter) {
	fileDialog(callback, OpenFileMode::Open, filter);
}

inline void WindowedApp::directoryDialog(const std::function<void(const std::string&)>& callback) {
	fileDialog(callback, OpenFileMode::Directory);
}

inline bool WindowedApp::isPressed(int32_t key) const {
	return _keys.find(key) != _keys.end();
}

}
