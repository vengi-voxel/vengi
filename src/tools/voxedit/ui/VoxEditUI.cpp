#include "VoxEditUI.h"
#include "ToolWindow.h"

void registerWindows(VoxEdit* tool) {
	new ToolWindow(tool);
}
