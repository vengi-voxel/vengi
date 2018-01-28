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
	double _frameCounterResetTime = 0.0;

	typedef std::unordered_map<int32_t, int16_t> KeyMap;
	typedef KeyMap::const_iterator KeyMapConstIter;
	typedef KeyMap::iterator KeyMapIter;
	KeyMap _keys;
	util::BindMap _bindings;
	glm::ivec2 _mousePos;
	glm::ivec2 _mouseRelativePos;

	WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport = 17815);

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

	enum class OpenFileMode {
		Save, Open, Directory
	};

	/**
	 * @brief Opens a file dialog
	 * @param[in] mode @c OpenFileMode
	 * @param[in] filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 * @return The selected string of the file (or directory) - or an empty string if the selection was aborted.
	 */
	virtual std::string fileDialog(OpenFileMode mode, const std::string& filter = "");

	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	std::string saveDialog(const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	std::string openDialog(const std::string& filter = "");
	/**
	 * @brief Wrapper method for @c fileDialog()
	 */
	std::string directoryDialog();

	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onKeyRelease(int32_t key) override;
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

inline std::string WindowedApp::saveDialog(const std::string& filter) {
	return fileDialog(OpenFileMode::Save, filter);
}

inline std::string WindowedApp::openDialog(const std::string& filter) {
	return fileDialog(OpenFileMode::Open, filter);
}

inline std::string WindowedApp::directoryDialog() {
	return fileDialog(OpenFileMode::Directory);
}

}
