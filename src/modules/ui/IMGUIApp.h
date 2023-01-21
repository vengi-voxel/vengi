/**
 * @file
 */

#pragma once

#include "FileDialog.h"
#include "video/WindowedApp.h"
#include "video/Buffer.h"
#include "RenderShaders.h"
#include "Console.h"
#include "core/collection/Array.h"

struct SDL_Cursor;

namespace ui {

// https://github.com/aiekick/ImGuiFileDialog
/**
 * @ingroup UI
 */
class IMGUIApp: public video::WindowedApp {
private:
	using Super = video::WindowedApp;
	void loadFonts();
protected:
	core::Set<int32_t> _keys;
	core::VarPtr _renderUI;
	core::VarPtr _showMetrics;
	core::VarPtr _uiFontSize;
	video::Id _texture = video::InvalidId;
	Console _console;
	core::String _writePathIni;
	core::String _writePathLog;
	core::VarPtr _lastDirectory;
	core::VarPtr _uistyle;

	bool _showBindingsDialog = false;
	bool _showTexturesDialog = false;
	bool _showFileDialog = false;
	bool _persistUISettings = true;
	bool _imguiBackendInitialized = false;

	/**
	 * If anything in the ui has changed that makes the saved ini file invalid, you can
	 * just bump this version to reset to default instead of leaving the user with a broken
	 * ui
	 */
	int _iniVersion = 0;

	video::OpenFileMode _fileDialogMode = video::OpenFileMode::Directory;
	video::FileDialogSelectionCallback _fileDialogCallback {};
	video::FileDialogOptions _fileDialogOptions {};

	ImFont* _defaultFont = nullptr;
	ImFont* _bigFont = nullptr;
	ImFont* _smallFont = nullptr;
	ImFont* _bigIconFont = nullptr;

	FileDialog _fileDialog;

	void setColorTheme();

	virtual bool onKeyRelease(int32_t key, int16_t modifier) override;
	virtual bool onKeyPress(int32_t key, int16_t modifier) override;
	virtual bool onTextInput(const core::String& text) override;
	virtual void onMouseMotion(void *windowHandle, int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	virtual bool onMouseWheel(int32_t x, int32_t y) override;
	virtual void onMouseButtonRelease(int32_t x, int32_t y, uint8_t button) override;
	virtual void onMouseButtonPress(int32_t x, int32_t y, uint8_t button, uint8_t clicks) override;
	virtual bool handleSDLEvent(SDL_Event& event) override;
public:
	IMGUIApp(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, size_t threadPoolSize = 1);
	virtual ~IMGUIApp();

	virtual void beforeUI();

	int fontSize() const;
	virtual app::AppState onConstruct() override;
	virtual app::AppState onInit() override;
	virtual app::AppState onRunning() override;
	virtual void onRenderUI() = 0;
	virtual app::AppState onCleanup() override;

	ImFont *defaultFont();
	ImFont *bigFont();
	ImFont *bigIconFont();
	ImFont *smallFont();

	void showBindingsDialog();
	void showTexturesDialog();
	void fileDialog(const video::FileDialogSelectionCallback& callback, const video::FileDialogOptions& options, video::OpenFileMode mode, const io::FormatDescription* formats = nullptr, const core::String &filename = "") override;
};

inline void IMGUIApp::showBindingsDialog() {
	_showBindingsDialog = true;
}

inline void IMGUIApp::showTexturesDialog() {
	_showTexturesDialog = true;
}

inline ImFont *IMGUIApp::defaultFont() {
	return _defaultFont;
}

inline ImFont *IMGUIApp::bigFont() {
	return _bigFont;
}

inline ImFont *IMGUIApp::bigIconFont() {
	return _bigIconFont;
}

inline ImFont *IMGUIApp::smallFont() {
	return _smallFont;
}

inline int IMGUIApp::fontSize() const {
	return _uiFontSize->intVal();
}

}

inline ui::IMGUIApp* imguiApp() {
	return (ui::IMGUIApp*)video::WindowedApp::getInstance();
}
