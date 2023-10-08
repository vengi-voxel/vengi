/**
 * @file
 * @brief This list contains the tip of the days that voxedit shows at startup. It contains placeholders for cvars and
 * command bindings.
 * @see util::replacePlaceholders()
 */

#pragma once

namespace voxedit {

// clang-format off
static const char *TIPOFTHEDAY[]{
	"Switch between scene and edit mode by pressing the <cmd:togglescene> key.",
	"Use the file dialog options for format specific options.",
	"You can record a video of your rotating model in the viewport by using the 'View' menu and the 'Video' option. Make sure to set the camera rotation speed to e.g. 1.",
	"When saving to a foreign format you might lose scene details if the format doesn't support particular features. Make sure to use the vengi format for highest compatibility.",
	"Change the color reduction mode to improve the quality of the palette especially for importing RGBA or mesh based formats.",
	"Drag a model from the assets panel to the stamp brush to use it.",
	"You can reset the camera simply by pressing <cmd:resetcamera>.",
	"The editor shows the last executed console command in the status bar. You can use these commands and bind when to keys.",
	"You can use wasd style scene movement by switching the 'Camera movement' to 'Eye' in the 'View' menu of the viewport.",
	"In order to use the path modifier, you have to place the reference position on top of another voxel and place the end of the path on another existing and connected voxel.",
	"When importing meshes, you can switch between different voxelization methods in the options menu.",
	"Delete voxels in edit mode by pressing <cmd:+actionexecutedelete> or by using the erase modifier.",
	"Switch between different color themes in the options menu.",
	"You can change your default key bindings to Magicavoxel, Blender, Qubicle or Vengi own style. Check the bindings window in the options menu."
};

// clang-format on

} // namespace voxedit
