/**
 * @file
 */

#pragma once

#include "ui/UIApp.h"
#include "frontend/RawVolumeRenderer.h"
#include "video/Camera.h"
#include "frontend/Axis.h"
#include "frontend/Plane.h"

/**
 * @brief This tool provides a UI to create noise images on-the-fly.
 */
class VoxEdit: public ui::UIApp {
private:
	using Super = ui::UIApp;
	bool _cameraMotion = true;
	bool _renderPlane = false;
	bool _renderAxis = true;
	video::Camera _camera;
	frontend::Axis _axis;
	frontend::Plane _plane;
	core::VarPtr _rotationSpeed;
	uint8_t _moveMask = 0;
	float _cameraSpeed = 0.1f;
	bool _dirty = false;
	bool _extract = false;
	core::VarPtr _lastDirectory;
	frontend::RawVolumeRenderer _rawVolumeRenderer;

	bool isDirty() const;
public:
	VoxEdit(const io::FilesystemPtr& filesystem, const core::EventBusPtr& eventBus, const core::TimeProviderPtr& timeProvider);

	bool saveFile(std::string_view file);
	bool loadFile(std::string_view file);
	bool newFile(bool force = false);

	core::AppState onInit() override;
	void beforeUI() override;
	core::AppState onCleanup() override;
	core::AppState onRunning() override;
	void onMouseMotion(int32_t x, int32_t y, int32_t relX, int32_t relY) override;
	void onMouseWheel(int32_t x, int32_t y) override;
	void onMouseButtonPress(int32_t x, int32_t y, uint8_t button) override;
	bool onKeyPress(int32_t key, int16_t modifier) override;
	void onWindowResize() override;
};

inline bool VoxEdit::isDirty() const {
	return _dirty;
}
