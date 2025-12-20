/**
 * @file
 */

#pragma once

#include "palette/PaletteCache.h"
#include "ui/IMGUIApp.h"
#include "voxelgenerator/LUAApi.h"
#include "voxelui/LUAApiWidget.h"

/**
 * @brief This is a basic ui for voxconvert
 *
 * @ingroup Tools
 */
class VoxConvertUI : public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	core::String _output;
	core::String _inputFile;
	core::String _voxconvertBinary = "vengi-voxconvert";
	core::String _outputFile;
	core::String _oldOutputFile;
	bool _outputFileExists = false;
	bool _overwriteOutputFile = false;
	palette::PaletteCache _paletteCache;

	struct ScriptExecutorContext : public voxelui::LUAApiExecutorContext {
		core::String _scriptFilename;
		core::DynamicArray<core::String> _args;

		void notify(const core::String &scriptFilename, const core::DynamicArray<core::String> &args) override {
			_scriptFilename = scriptFilename;
			_args = args;
		}
	};
	ScriptExecutorContext _luaApiCtx;
	voxelui::LUAApiWidget _luaApiWidget;
	voxelgenerator::LUAApi _luaApi;

	core::String _droppedFile;

protected:
	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;

	void onDropFile(void *windowHandle, const core::String &file) override;
	void onDropText(void *windowHandle, const core::String& text) override {
		onDropFile(nullptr, text);
	}

	void onRenderUI() override;

public:
	VoxConvertUI(const io::FilesystemPtr &filesystem, const core::TimeProviderPtr &timeProvider);
};
