/**
 * @file
 */

#pragma once

#include "core/App.h"
#include "io/IEventObserver.h"
#include "io/EventHandler.h"
#include "core/GLM.h"
#include "util/KeybindingParser.h"

struct SDL_Window;
typedef void *SDL_GLContext;

namespace video {

class WindowedApp: public core::App, public io::IEventObserver {
protected:
	class ProfilerGPU {
	private:
		unsigned int _id = 0u;
		double _avg = 0.0;
		double _min = 0.0;
		double _max = 0.0;
		int _state = 0;
	public:
		ProfilerGPU();
		~ProfilerGPU();

		bool init();
		void enter();
		void leave();
		double minimum() const;
		double maximum() const;
		double avg() const;
	};

	SDL_Window* _window;
	SDL_GLContext _glcontext;
	glm::ivec2 _dimension;
	float _aspect;

	typedef std::unordered_map<int32_t, int16_t> KeyMap;
	typedef KeyMap::const_iterator KeyMapConstIter;
	typedef KeyMap::iterator KeyMapIter;
	KeyMap _keys;
	util::BindMap _bindings;

	WindowedApp(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider, uint16_t traceport = 17815);

	bool loadKeyBindings(const std::string& filename = "keybindings.cfg");

	virtual ~WindowedApp() {
	}

	inline void centerMouseCursor() {
		SDL_WarpMouseInWindow(_window, width() / 2, height() / 2);
	}

	inline bool isRelativeMouseMode() const {
		return SDL_GetRelativeMouseMode();
	}

	inline void toggleRelativeMouseMode() {
		const bool current = isRelativeMouseMode();
		setRelativeMouseMode(current ? false : true);
	}

	inline void setRelativeMouseMode(bool mode) {
		SDL_SetRelativeMouseMode(mode ? SDL_TRUE : SDL_FALSE);
	}

public:
	inline const glm::ivec2& dimension() const {
		return _dimension;
	}

	inline int width() const {
		return _dimension.x;
	}

	inline int height() const {
		return _dimension.y;
	}

	enum class OpenFileMode {
		Save, Open, Directory
	};

	/**
	 * @param filter png,jpg;psd The default filter is for png and jpg files. A second filter is available for psd files. There is a wildcard option in a dropdown.
	 * @return The selected string of the file - or an empty string
	 */
	std::string fileDialog(OpenFileMode mode, const std::string& filter = "");
	std::string saveDialog(const std::string& filter = "");
	std::string openDialog(const std::string& filter = "");
	std::string directoryDialog();

	virtual core::AppState onRunning() override;
	virtual void onAfterRunning() override;
	virtual bool onKeyRelease(int32_t key) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual core::AppState onConstruct() override;
	virtual core::AppState onInit() override;
	virtual core::AppState onCleanup() override;
	virtual void onWindowResize() override;
};

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
