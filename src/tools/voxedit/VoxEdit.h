/**
 * @file
 */

#pragma once

#include "core/collection/DynamicArray.h"
#include "io/FormatDescription.h"
#include "ui/IMGUIApp.h"
#include "voxedit-util/SceneManager.h"

namespace voxedit {
class MainWindow;
class SceneManager;
typedef core::SharedPtr<SceneManager> SceneManagerPtr;
}

/**
 * @brief This is a voxel editor that can import and export multiple mesh/voxel formats.
 *
 * @ingroup Tools
 */
class VoxEdit: public ui::IMGUIApp {
private:
	using Super = ui::IMGUIApp;
	voxedit::MainWindow* _mainWindow = nullptr;
	core::DynamicArray<io::FormatDescription> _paletteFormats;
	voxedit::SceneManagerPtr _sceneMgr;

	core::String getSuggestedFilename(const char *extension = nullptr) const;

	// 0 is the default binding
	enum KeyBindings {
		Magicavoxel = 0,
		Blender = 1,
		Vengi = 2,
		Qubicle = 3,
		Max
	};

	void loadKeymap(int keymap) override;
protected:
	void printUsageHeader() const override;

public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::TimeProviderPtr& timeProvider, const voxedit::SceneManagerPtr& sceneMgr);

	void onRenderUI() override;

	void onDropFile(const core::String& file) override;

	bool allowedToQuit() override;

	const voxedit::SceneManagerPtr& sceneMgr() const;

	app::AppState onConstruct() override;
	app::AppState onInit() override;
	app::AppState onCleanup() override;
	app::AppState onRunning() override;

	void toggleScene();
};

inline const voxedit::SceneManagerPtr& VoxEdit::sceneMgr() const {
	return _sceneMgr;
}
