/**
 * @file
 */

#include "FormatConfig.h"
#include "core/Color.h"
#include "core/GameConfig.h"
#include "core/Var.h"
#include "voxel/Palette.h"

namespace voxelformat {

static bool colorReductionValidator(const core::String &value) {
	return core::Color::toColorReductionType(value.c_str()) != core::Color::ColorReductionType::Max;
}

bool FormatConfig::init() {
	core::Var::get(cfg::CoreColorReduction,
				   core::Color::toColorReductionTypeString(core::Color::ColorReductionType::MedianCut),
				   "Controls the algorithm that is used to perform the color reduction", colorReductionValidator);
	core::Var::get(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST, "Merge similar quads to optimize the mesh",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxelMeshMode, "0", core::CV_SHADER, "0 = cubes, 1 = marching cubes", core::Var::minMaxValidator<0, 1>);
	core::Var::get(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST, "Reuse vertices or always create new ones",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST, "Extra vertices for ambient occlusion",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatRGBFlattenFactor, "0", core::CV_NOPERSIST,
				   "To flatten factor for RGBA and mesh formats", [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= 0 && type <= 255;
				   });
	core::Var::get(cfg::VoxformatSaveVisibleOnly, "false", core::CV_NOPERSIST, "Save only visible nodes",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST,
				   "Scale the vertices on all axis by the given factor");
	core::Var::get(cfg::VoxformatScaleX, "1.0", core::CV_NOPERSIST, "Scale the vertices on X axis by the given factor");
	core::Var::get(cfg::VoxformatScaleY, "1.0", core::CV_NOPERSIST, "Scale the vertices on Y axis by the given factor");
	core::Var::get(cfg::VoxformatScaleZ, "1.0", core::CV_NOPERSIST, "Scale the vertices on Z axis by the given factor");
	core::Var::get(cfg::VoxformatQuads, "true", core::CV_NOPERSIST,
				   "Export as quads. If this false, triangles will be used.", core::Var::boolValidator);
	core::Var::get(cfg::VoxformatWithColor, "true", core::CV_NOPERSIST, "Export with vertex colors",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatColorAsFloat, "true", core::CV_NOPERSIST, "Export with vertex colors as float values (if vertex colors are exported)",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST,
				   "Export with uv coordinates of the palette image", core::Var::boolValidator);
	core::Var::get(cfg::VoxformatTransform, "true", core::CV_NOPERSIST,
				   "Apply the scene graph transform to mesh exports", core::Var::boolValidator);
	core::Var::get(cfg::VoxformatFillHollow, "true", core::CV_NOPERSIST,
				   "Fill the hollows when voxelizing a mesh format", core::Var::boolValidator);
	core::Var::get(cfg::VoxformatVoxelizeMode, "0", core::CV_NOPERSIST,
				   "0 = high quality, 1 = faster and less memory", core::Var::minMaxValidator<0, 1>);
	core::Var::get(cfg::VoxformatVXLNormalType, "2", core::CV_NOPERSIST,
				   "Normal type for VXL format - 2 (TS) or 4 (RedAlert2)", [](const core::String &var) {
					   const int type = var.toInt();
					   return type == 2 || type == 4;
				   });
	core::Var::get(cfg::VoxformatQBTPaletteMode, "true", core::CV_NOPERSIST, "Use palette mode in qubicle qbt export",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatQBTMergeCompounds, "false", core::CV_NOPERSIST, "Merge compounds on load",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxelPalette, voxel::Palette::getDefaultPaletteName(),
				   "This is the NAME part of palette-<NAME>.png or absolute png file to use (1x256)");
	core::Var::get(cfg::VoxformatMerge, "false", core::CV_NOPERSIST, "Merge all objects into one",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatVOXCreateGroups, "true", core::CV_NOPERSIST, "Merge compounds on load",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatVOXCreateLayers, "true", core::CV_NOPERSIST, "Merge compounds on load",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatQBSaveLeftHanded, "true", core::CV_NOPERSIST, "Toggle between left and right handed",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxelCreatePalette, "true", core::CV_NOPERSIST,
				   "Create own palette from textures or colors - not used for palette formats",
				   core::Var::boolValidator);
	core::Var::get(cfg::VoxformatPointCloudSize, "1", core::CV_NOPERSIST, "Specify the side length for the voxels when loading a point cloud");

	return true;
}

} // namespace voxelformat
