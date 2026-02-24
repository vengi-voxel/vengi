/**
 * @file
 */

#include "FormatConfig.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "math/Axis.h"
#include "palette/FormatConfig.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelformat/private/image/PNGFormat.h"
#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {

static bool schematicTypeValidator(const core::String &value) {
	static const char *schematicTypes[] = {"mcedit2", "worldedit", "schematica"};
	for (const char *type : schematicTypes) {
		if (value == type) {
			return true;
		}
	}
	return false;
}

bool FormatConfig::init() {
	palette::FormatConfig::init();

	core::Var::registerVar(cfg::VoxformatMergequads, "true", core::CV_NOPERSIST, _("Merge similar quads to optimize the mesh"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::Binary, core::CV_NOPERSIST,
				   C_("Voxel mesh mode description", "0 = cubes, 1 = marching cubes, 2 = binary mesher, 3 = greedy texture"),
				   core::Var::minMaxValidator<(int)voxel::SurfaceExtractionType::Cubic,
											  (int)voxel::SurfaceExtractionType::Max - 1>);
	core::Var::registerVar(cfg::VoxformatReusevertices, "true", core::CV_NOPERSIST,
				   _("Reuse vertices or always create new ones"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatRGBWeightedAverage, "true", core::CV_NOPERSIST,
				   _("If multiple triangles contribute to the same voxel the color values are averaged based on their "
					 "area contribution - otherwise only the biggest triangle counts"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatAmbientocclusion, "false", core::CV_NOPERSIST,
				   _("Extra vertices for ambient occlusion"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatRGBFlattenFactor, "0", core::CV_NOPERSIST,
				   _("The RGB color flatten factor for importing color and mesh formats"), core::Var::minMaxValidator<0, 255>);
	core::Var::registerVar(cfg::VoxformatTargetColors, "0", core::CV_NOPERSIST,
				   _("Target number of colors after voxelization (0 = no limit, otherwise quantize to this amount)"),
				   core::Var::minMaxValidator<0, 256>);
	core::Var::registerVar(cfg::VoxformatSaveVisibleOnly, "false", core::CV_NOPERSIST, _("Save only visible nodes"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatScale, "1.0", core::CV_NOPERSIST,
				   _("Scale the vertices for voxelization on all axis by the given factor"));
	core::Var::registerVar(cfg::VoxformatScaleX, "1.0", core::CV_NOPERSIST,
				   _("Scale the vertices for voxelization X axis by the given factor"));
	core::Var::registerVar(cfg::VoxformatScaleY, "1.0", core::CV_NOPERSIST,
				   _("Scale the vertices for voxelization Y axis by the given factor"));
	core::Var::registerVar(cfg::VoxformatScaleZ, "1.0", core::CV_NOPERSIST,
				   _("Scale the vertices for voxelization Z axis by the given factor"));
	core::Var::registerVar(cfg::VoxformatQuads, "true", core::CV_NOPERSIST,
				   _("Export as quads. If this false, triangles will be used."), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatWithColor, "true", core::CV_NOPERSIST, _("Export with vertex colors"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatWithNormals, "false", core::CV_NOPERSIST,
				   _("Export smoothed normals for cubic meshes"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatColorAsFloat, "true", core::CV_NOPERSIST,
				   _("Export with vertex colors as float values (if vertex colors are exported)"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatWithtexcoords, "true", core::CV_NOPERSIST,
				   _("Export with uv coordinates of the palette image"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatTransform, "true", core::CV_NOPERSIST,
				   _("Apply the scene graph transform to mesh exports"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatOptimize, "false", core::CV_NOPERSIST, _("Apply mesh optimization steps to meshes"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatFillHollow, "true", core::CV_NOPERSIST,
				   _("Fill the hollows when voxelizing a mesh format"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatVoxelizeMode, MeshFormat::VoxelizeMode::HighQuality, core::CV_NOPERSIST,
				   _("0 = high quality, 1 = faster and less memory"), core::Var::minMaxValidator<0, 1>);
	core::Var::registerVar(cfg::VoxformatQBTPaletteMode, "true", core::CV_NOPERSIST,
				   _("Use palette mode in qubicle qbt export"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatQBTMergeCompounds, "false", core::CV_NOPERSIST,
				   C_("Merge compounds when loading Qubicle QBT files", "Merge compounds on load"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatMerge, "false", core::CV_NOPERSIST, _("Merge all objects into one"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatEmptyPaletteIndex, "-1", core::CV_NOPERSIST,
				   _("The index of the empty color in the palette"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= -1 && type <= 255;
				   });
	core::Var::registerVar(cfg::VoxformatVXLLoadHVA, "true", core::CV_NOPERSIST, _("Load the hva for animations"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatVOXCreateGroups, "true", core::CV_NOPERSIST,
				   C_("Create groups when saving MagicaVoxel vox files", "Create groups for vox file"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatVOXCreateLayers, "true", core::CV_NOPERSIST,
				   C_("Create layers when saving MagicaVoxel vox files", "Create layers for vox file"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatQBSaveLeftHanded, "true", core::CV_NOPERSIST,
				   _("Toggle between left and right handed"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatQBSaveCompressed, "true", core::CV_NOPERSIST,
				   C_("Save qubicle voxel files with RLE compression enabled", "Save RLE compressed"),
				   core::Var::boolValidator);
	core::Var::registerVar(
		cfg::VoxelCreatePalette, "true", core::CV_NOPERSIST,
		_("Create own palette from textures or colors or remap the existing palette colors to a new palette"),
		core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatPointCloudSize, "1", core::CV_NOPERSIST,
				   _("Specify the side length for the voxels when loading a point cloud"));
	core::Var::registerVar(cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, "true", core::CV_NOPERSIST,
				   _("Apply KHR_materials_pbrSpecularGlossiness when saving into the glTF format"),
				   core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatGLTF_KHR_materials_specular, "false", core::CV_NOPERSIST,
				   _("Apply KHR_materials_specular when saving into the glTF format"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatWithMaterials, "true", core::CV_NOPERSIST,
				   _("Try to export material properties if the formats support it"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatImageVolumeMaxDepth, "1", core::CV_NOPERSIST,
				   _("The maximum depth of the volume when importing an image as volume"),
				   core::Var::minMaxValidator<1, 255>);
	core::Var::registerVar(cfg::VoxformatImageHeightmapMinHeight, "0", core::CV_NOPERSIST,
				   _("The minimum height of the heightmap when importing an image as heightmap"),
				   core::Var::minMaxValidator<0, 255>);
	core::Var::registerVar(cfg::VoxformatImageVolumeBothSides, "true", core::CV_NOPERSIST,
				   _("Import the image as volume for both sides"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatTexturePath, "", core::CV_NOPERSIST,
				   _("Register an additional search path for texture lookups"));
	core::Var::registerVar(cfg::VoxformatImageImportType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   _("0 = plane, 1 = heightmap, 2 = volume"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>);
	core::Var::registerVar(cfg::VoxformatImageSaveType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   C_("Image save type", "0 = plane, 1 = heightmap, 2 = volume, 3 = thumbnail"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>);
	core::Var::registerVar(cfg::VoxformatImageSliceOffsetAxis, "y", core::CV_NOPERSIST,
				   _("The axis to offset the slices when importing images as volumes or heightmaps"),
				   [](const core::String &var) {
					   return math::toAxis(var) != math::Axis::None;
				   });
	core::Var::registerVar(cfg::VoxformatImageSliceOffset, "0", core::CV_NOPERSIST,
				   _("The offset of the slices when importing images as volumes or heightmaps"));
	static_assert(PNGFormat::ImageType::Plane == 0, "Plane must be 0");
	static_assert(PNGFormat::ImageType::Volume == 2, "Volume must be 2");
	core::Var::registerVar(cfg::VoxformatSchematicType, "mcedit2", core::CV_NOPERSIST,
				   _("The type of schematic format to use when saving schematics"), schematicTypeValidator);
	core::Var::registerVar(cfg::VoxformatBinvoxVersion, "2", core::CV_NOPERSIST,
				   C_("Binvox format version", "Save in version 1, 2 or the unofficial version 3"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= 1 && type <= 3;
				   });
	core::Var::registerVar(cfg::VoxformatSkinApplyTransform, "false", core::CV_NOPERSIST,
				   _("Apply transforms to Minecraft skins"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatSkinAddGroups, "true", core::CV_NOPERSIST,
				   _("Add groups for body parts of Minecraft skins"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatSkinMergeFaces, "false", core::CV_NOPERSIST,
				   _("Merge face parts into single volume for Minecraft skins"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatMeshSimplify, "false", core::CV_NOPERSIST,
				   _("Simplify the mesh when voxelizing a mesh format"), core::Var::boolValidator);
	core::Var::registerVar(cfg::VoxformatGMLRegion, "", core::CV_NOPERSIST,
				   _("World coordinate region filter for GML/CityGML import. Format: 'minX minY minZ maxX maxY maxZ' "
					 "in GML world coordinates. Only applied when the estimated voxel region exceeds the size threshold. "
					 "Objects fully inside this region are imported, others are skipped."));
	core::Var::registerVar(cfg::VoxformatGMLFilenameFilter, "", core::CV_NOPERSIST,
				   _("Filename filter for GML/CityGML import. Only import files that contain this string in their "
					 "filename. Wildcards are supported."));
	core::Var::registerVar(cfg::VoxformatOSMURL, "https://overpass-api.de/api/interpreter", core::CV_NOPERSIST,
				   _("The URL of the Overpass API endpoint"));
	core::Var::registerVar(cfg::VoxformatOSMMetersPerVoxel, "1.0", core::CV_NOPERSIST,
				   _("The number of real-world meters each voxel represents in OSM imports"));

	return true;
}

} // namespace voxelformat
