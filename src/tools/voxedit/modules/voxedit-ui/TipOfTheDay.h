#pragma once

#include "app/I18N.h"

static const char *tips[]{
	// clang-format off
	N_("Switch between scene and edit mode (not in simple UI mode) by pressing the <cmd:togglescene> key."),
	N_("Use the file dialog options for format specific options."),
	N_("You can record a video of your rotating model in the viewport by using the 'View' menu and the 'Video' option. Make sure to set the camera rotation speed to e.g. 1."),
	N_("When saving to a foreign format you might lose scene details if the format doesn't support particular features. Make sure to use the vengi format for highest compatibility."),
	N_("Change the color reduction mode to improve the quality of the palette especially for importing RGBA or mesh based formats."),
	N_("Drag a model from the assets panel to the stamp brush to use it."),
	N_("You can reset the camera simply by pressing <cmd:resetcamera>."),
	N_("The editor shows the last executed console command in the status bar. You can use these commands and bind them to keys."),
	N_("You can use wasd style scene movement by switching the 'Camera movement' to 'Eye' in the 'View' menu of the viewport."),
	N_("In order to use the path modifier, you have to place the reference position on top of another voxel and place the end of the path on another existing and connected voxel."),
	N_("When importing meshes, you can switch between different voxelization methods in the options menu."),
	N_("Delete voxels in edit mode by pressing <cmd:+actionexecutedelete> or by using the erase modifier."),
	N_("Switch between different color themes in the options menu."),
	N_("Please activate anonymous usage metrics in the options or the about dialog."),
	N_("You can change your default key bindings to Magicavoxel, Blender, Qubicle or Vengi own style. Check the bindings window in the options menu."),
	N_("You can save your viewport as AVI video or as screenshot."),
	N_("Press <cmd:pickcolor> to pick a color from the scene."),
	N_("Press <cmd:mirroraxisshapebrushx>, <cmd:mirroraxisshapebrushy> or <cmd:mirroraxisshapebrushz> to mirror the current selection."),
	N_("Use ctrl+tab to switch between the panels of the application."),
	N_("It's possible to have multiple viewports and align them next to each other to get a left, top and so on view of the scene at the same time.")
	// clang-format on
};
