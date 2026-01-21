/**
 * @file
 */

#pragma once

#include "FileDialog.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/RingBuffer.h"
#include "video/TexturePool.h"
#include "video/WindowedApp.h"
#include "IMGUIConsole.h"
#include "Style.h"
#ifdef IMGUI_ENABLE_TEST_ENGINE
#include "ui/dearimgui/imgui_test_engine/imgui_te_engine.h"
#endif

struct SDL_Cursor;

namespace ui {

class Panel;

// https://github.com/aiekick/ImGuiFileDialog
/**
 * @ingroup UI
 */
class IMGUIApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	void loadFonts();

protected:
	command::CommandExecutionListener _lastExecutedCommand;
	core::Set<int32_t> _keys;
	core::VarPtr _renderUI;
	core::VarPtr _showMetrics;
	core::VarPtr _uiFontSize;
	IMGUIConsole _console;
	core::String _writePathIni;
	core::String _writePathLog;
	core::VarPtr _lastDirectory;
	core::VarPtr _uistyle;
	core::VarPtr _languageVar;
	core::VarPtr _lastOpenedFiles;
	core::VarPtr _lastOpenedFile;

	/**
	 * The current keymap as index to the registered keymaps from the
	 * particular application
	 */
	core::VarPtr _uiKeyMap;
	/**
	 * the array indices are the keymap ids given to @c loadKeyMap()
	 * the strings in this array are used to print the names in the
	 * key binding dialog
	 *
	 * each ui application can register its own keymaps by pushing
	 * strings into this array.
	 */
	core::DynamicArray<core::String> _uiKeyMaps;
	/**
	 * string to filter the bindings in the binding dialog
	 */
	core::String _bindingsFilter;
	/**
	 * Reset the custom keybindings and load the default keymap
	 */
	bool _resetKeybindings = false;
	bool _showBindingsDialog = false;
	bool _showTexturesDialog = false;
	bool _showCommandDialog = false;
	bool _showCvarDialog = false;
	bool _showFPSDialog = false;
	bool _closeModalPopup = false;
	bool _showFileDialog = false;
	bool _imguiBackendInitialized = false;

	bool _persistUISettings = true;
	bool _showConsole = true;

	/**
	 * If anything in the ui has changed that makes the saved ini file invalid, you can
	 * just bump this version to reset to default instead of leaving the user with a broken
	 * ui
	 */
	int _iniVersion = 0;

	float _dpiScale = 1.0f;

	video::OpenFileMode _fileDialogMode = video::OpenFileMode::Directory;
	video::FileDialogSelectionCallback _fileDialogCallback {};
	video::FileDialogOptions _fileDialogOptions {};
	ui::LastOpenedFiles _lastOpenedFilesRingBuffer;

#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine *_imguiTestEngine = nullptr;
	int _startedFromCommandlineFrameDelay = 3;
	// used for the imgui test engine (IM_REGISTER_TEST)
	virtual bool registerUITests() {
		_fileDialog.registerUITests(_imguiTestEngine, "filedialog");
		return true;
	}
#endif

	core::Buffer<Panel*> _panels;
	FileDialog _fileDialog;
	core::RingBuffer<float, 4096> _fpsData;

	/**
	 * @brief Convert semicolon-separated string into the @c _lastOpenedFilesRingBuffer array
	 */
	void loadLastOpenedFiles(const core::String &string);

	void setColorTheme();
	/**
	 * @brief Configure the default keymap bindings for the application
	 * @param[in] keymap The application can support several different keymaps - you get the keymap id here to load the desired keymap
	 * @note this is saved in the cvar @c cfg::UIKeyMap
	 */
	virtual void loadKeymap(int keymap);

	bool onKeyRelease(void *windowHandle, int32_t key, int16_t modifier) override;
	bool onKeyPress(void *windowHandle, int32_t key, int16_t modifier) override;
	bool onTextInput(void *windowHandle, const core::String& text) override;
	void onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY, int32_t mouseId) override;
	bool onMouseWheel(void *windowHandle, float x, float y, int32_t mouseId) override;
	void onMouseButtonRelease(void *windowHandle, int32_t x, int32_t y, uint8_t button, int32_t mouseId) override;
	void onMouseButtonPress(void *windowHandle, int32_t x, int32_t y, uint8_t button, uint8_t clicks, int32_t mouseId) override;
	bool handleSDLEvent(SDL_Event& event) override;

	void renderBindingsDialog();
	void renderTexturesDialog();
	void renderCvarDialog();
	void renderFPSDialog();
	void renderCommandDialog();
public:
	IMGUIApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~IMGUIApp();

	virtual void beforeUI();

	command::CommandExecutionListener &commandListener() {
		return _lastExecutedCommand;
	}

	int fontSize() const;
	int bigFontSize() const;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	// the main window should get the id "###app"
	virtual void onRenderUI() = 0;
	virtual app::AppState onCleanup() override;
	const glm::vec4 &color(style::StyleColor color);

	void languageOption();
	void colorReductionOptions();
	bool keyMapOption();
	const ui::LastOpenedFiles& lastOpenedFiles() const;
	void addLastOpenedFile(const core::String &file);
	core::String windowTitle() const;
	void lastOpenedMenu(const char *loadCommand = "load");

#ifdef IMGUI_ENABLE_TEST_ENGINE
	ImGuiTestEngine *imguiTestEngine() const {
		return _imguiTestEngine;
	}
#endif

	void addPanel(Panel *panel);
	void removePanel(Panel *panel);
	Panel* getPanel(const core::String &title);

	void showBindingsDialog();
	void showTexturesDialog();
	void showCvarDialog();
	void showFPSDialog();
	void showCommandDialog();
	void fileDialog(const video::FileDialogSelectionCallback& callback, const video::FileDialogOptions& options, video::OpenFileMode mode, const io::FormatDescription* formats = nullptr, const core::String &filename = "") override;
};

inline const ui::LastOpenedFiles& IMGUIApp::lastOpenedFiles() const {
	return _lastOpenedFilesRingBuffer;
}

inline void IMGUIApp::showBindingsDialog() {
	_showBindingsDialog = true;
}

inline void IMGUIApp::showTexturesDialog() {
	_showTexturesDialog = true;
}

inline void IMGUIApp::showCvarDialog() {
	_showCvarDialog = true;
}

inline void IMGUIApp::showFPSDialog() {
	_showFPSDialog = true;
}

inline void IMGUIApp::showCommandDialog() {
	_showCommandDialog = true;
}

inline int IMGUIApp::bigFontSize() const {
	return fontSize() * 1.6f;
}

inline int IMGUIApp::fontSize() const {
	return _uiFontSize->intVal();
}

}

inline ui::IMGUIApp* imguiApp() {
	return (ui::IMGUIApp*)video::WindowedApp::getInstance();
}
