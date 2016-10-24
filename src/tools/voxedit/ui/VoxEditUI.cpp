#include "VoxEditUI.h"
#include "ToolWindow.h"
#include "CameraWindow.h"

void registerWindows(VoxEdit* tool) {
	new ToolWindow(tool);
	new CameraWindow(tool);
}
