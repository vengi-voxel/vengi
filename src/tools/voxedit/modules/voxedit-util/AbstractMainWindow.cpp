/**
 * @file
 */

#include "AbstractMainWindow.h"
#include "voxedit-util/SceneManager.h"
#include "voxelformat/VolumeFormat.h"

namespace voxedit {

AbstractMainWindow::AbstractMainWindow(video::WindowedApp *app) : _app(app) {
}

bool AbstractMainWindow::prefab(const core::String &file) {
	if (file.empty()) {
		_app->openDialog([this](const core::String file) { prefab(file); }, voxelformat::SUPPORTED_VOXEL_FORMATS_LOAD);
		return true;
	}

	return sceneMgr().prefab(file);
}

bool AbstractMainWindow::saveScreenshot(const core::String& file) {
	if (file.empty()) {
		_app->saveDialog([this] (const core::String file) {saveScreenshot(file); }, "png");
		return true;
	}
	if (!saveImage(file.c_str())) {
		Log::warn("Failed to save screenshot");
		return false;
	}
	Log::info("Screenshot created at '%s'", file.c_str());
	return true;
}

void AbstractMainWindow::afterLoad(const core::String &file) {
	_lastOpenedFile->setVal(file);
	resetCamera();
}

bool AbstractMainWindow::importAsPlane(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importAsPlane(file); }, "png");
		return true;
	}

	return sceneMgr().importAsPlane(file);
}

bool AbstractMainWindow::importPalette(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importPalette(file); }, "png");
		return true;
	}

	return sceneMgr().importPalette(file);
}

bool AbstractMainWindow::importHeightmap(const core::String& file) {
	if (file.empty()) {
		_app->openDialog([this] (const core::String file) { importHeightmap(file); }, "png");
		return true;
	}

	return sceneMgr().importHeightmap(file);
}

}
