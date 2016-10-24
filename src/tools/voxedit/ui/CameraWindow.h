/**
 * @file
 */

#pragma once

#include "ui/Window.h"
#include "core/Common.h"

class VoxEdit;

class CameraWindow: public ui::Window {
public:
	CameraWindow(VoxEdit* tool) :
			ui::Window(tool) {
		core_assert_always(loadResourceFile("ui/window/camera.tb.txt"));
	}
};
