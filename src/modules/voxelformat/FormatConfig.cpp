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

	core::registerVar(core::VarDef(cfg::VoxformatMergequads, true, core::CV_NOPERSIST, N_("Merge similar quads to optimize the mesh")));
	core::registerVar(core::VarDef(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::Binary, core::CV_NOPERSIST,
				   NC_("Voxel mesh mode description", "0 = cubes, 1 = marching cubes, 2 = binary mesher, 3 = greedy texture"),
				   core::Var::minMaxValidator<(int)voxel::SurfaceExtractionType::Cubic,
											  (int)voxel::SurfaceExtractionType::Max - 1>));
	core::registerVar(core::VarDef(cfg::VoxformatReusevertices, true, core::CV_NOPERSIST,
				   N_("Reuse vertices or always create new ones")));
	core::registerVar(core::VarDef(cfg::VoxformatRGBWeightedAverage, true, core::CV_NOPERSIST,
				   N_("If multiple triangles contribute to the same voxel the color values are averaged based on their "
					 "area contribution - otherwise only the biggest triangle counts")));
	core::registerVar(core::VarDef(cfg::VoxformatAmbientocclusion, false, core::CV_NOPERSIST,
				   N_("Extra vertices for ambient occlusion")));
	core::registerVar(core::VarDef(cfg::VoxformatRGBFlattenFactor, 0, core::CV_NOPERSIST,
				   N_("The RGB color flatten factor for importing color and mesh formats"), core::Var::minMaxValidator<0, 255>));
	core::registerVar(core::VarDef(cfg::VoxformatTargetColors, 0, core::CV_NOPERSIST,
				   N_("Target number of colors after voxelization (0 = no limit, otherwise quantize to this amount)"),
				   core::Var::minMaxValidator<0, 256>));
	core::registerVar(core::VarDef(cfg::VoxformatSaveVisibleOnly, false, core::CV_NOPERSIST, N_("Save only visible nodes")));
	core::registerVar(core::VarDef(cfg::VoxformatScale, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization on all axis by the given factor")));
	core::registerVar(core::VarDef(cfg::VoxformatScaleX, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization X axis by the given factor")));
	core::registerVar(core::VarDef(cfg::VoxformatScaleY, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization Y axis by the given factor")));
	core::registerVar(core::VarDef(cfg::VoxformatScaleZ, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization Z axis by the given factor")));
	core::registerVar(core::VarDef(cfg::VoxformatQuads, true, core::CV_NOPERSIST,
				   N_("Export as quads. If this false, triangles will be used.")));
	core::registerVar(core::VarDef(cfg::VoxformatWithColor, true, core::CV_NOPERSIST, N_("Export with vertex colors")));
	core::registerVar(core::VarDef(cfg::VoxformatWithNormals, false, core::CV_NOPERSIST,
				   N_("Export smoothed normals for cubic meshes")));
	core::registerVar(core::VarDef(cfg::VoxformatColorAsFloat, true, core::CV_NOPERSIST,
				   N_("Export with vertex colors as float values (if vertex colors are exported)")));
	core::registerVar(core::VarDef(cfg::VoxformatWithtexcoords, true, core::CV_NOPERSIST,
				   N_("Export with uv coordinates of the palette image")));
	core::registerVar(core::VarDef(cfg::VoxformatTransform, true, core::CV_NOPERSIST,
				   N_("Apply the scene graph transform to mesh exports")));
	core::registerVar(core::VarDef(cfg::VoxformatOptimize, false, core::CV_NOPERSIST, N_("Apply mesh optimization steps to meshes")));
	core::registerVar(core::VarDef(cfg::VoxformatFillHollow, true, core::CV_NOPERSIST,
				   N_("Fill the hollows when voxelizing a mesh format")));
	core::registerVar(core::VarDef(cfg::VoxformatVoxelizeMode, MeshFormat::VoxelizeMode::HighQuality, core::CV_NOPERSIST,
				   N_("0 = high quality, 1 = faster and less memory"), core::Var::minMaxValidator<0, 1>));
	core::registerVar(core::VarDef(cfg::VoxformatQBTPaletteMode, true, core::CV_NOPERSIST,
				   N_("Use palette mode in qubicle qbt export")));
	core::registerVar(core::VarDef(cfg::VoxformatQBTMergeCompounds, false, core::CV_NOPERSIST,
				   NC_("Merge compounds when loading Qubicle QBT files", "Merge compounds on load")));
	core::registerVar(core::VarDef(cfg::VoxformatMerge, false, core::CV_NOPERSIST, N_("Merge all objects into one")));
	core::registerVar(core::VarDef(cfg::VoxformatEmptyPaletteIndex, -1, core::CV_NOPERSIST,
				   N_("The index of the empty color in the palette"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= -1 && type <= 255;
				   }));
	core::registerVar(core::VarDef(cfg::VoxformatVXLLoadHVA, true, core::CV_NOPERSIST, N_("Load the hva for animations")));
	core::registerVar(core::VarDef(cfg::VoxformatVOXCreateGroups, true, core::CV_NOPERSIST,
				   NC_("Create groups when saving MagicaVoxel vox files", "Create groups for vox file")));
	core::registerVar(core::VarDef(cfg::VoxformatVOXCreateLayers, true, core::CV_NOPERSIST,
				   NC_("Create layers when saving MagicaVoxel vox files", "Create layers for vox file")));
	core::registerVar(core::VarDef(cfg::VoxformatQBSaveLeftHanded, true, core::CV_NOPERSIST,
				   N_("Toggle between left and right handed")));
	core::registerVar(core::VarDef(cfg::VoxformatQBSaveCompressed, true, core::CV_NOPERSIST,
				   NC_("Save qubicle voxel files with RLE compression enabled", "Save RLE compressed")));
	core::registerVar(core::VarDef(
		cfg::VoxelCreatePalette, true, core::CV_NOPERSIST,
		N_("Create own palette from textures or colors or remap the existing palette colors to a new palette")));
	core::registerVar(core::VarDef(cfg::VoxformatPointCloudSize, 1, core::CV_NOPERSIST,
				   N_("Specify the side length for the voxels when loading a point cloud")));
	core::registerVar(core::VarDef(cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, true, core::CV_NOPERSIST,
				   N_("Apply KHR_materials_pbrSpecularGlossiness when saving into the glTF format")));
	core::registerVar(core::VarDef(cfg::VoxformatGLTF_KHR_materials_specular, false, core::CV_NOPERSIST,
				   N_("Apply KHR_materials_specular when saving into the glTF format")));
	core::registerVar(core::VarDef(cfg::VoxformatWithMaterials, true, core::CV_NOPERSIST,
				   N_("Try to export material properties if the formats support it")));
	core::registerVar(core::VarDef(cfg::VoxformatImageVolumeMaxDepth, 1, core::CV_NOPERSIST,
				   N_("The maximum depth of the volume when importing an image as volume"),
				   core::Var::minMaxValidator<1, 255>));
	core::registerVar(core::VarDef(cfg::VoxformatImageHeightmapMinHeight, 0, core::CV_NOPERSIST,
				   N_("The minimum height of the heightmap when importing an image as heightmap"),
				   core::Var::minMaxValidator<0, 255>));
	core::registerVar(core::VarDef(cfg::VoxformatImageVolumeBothSides, true, core::CV_NOPERSIST,
				   N_("Import the image as volume for both sides")));
	core::registerVar(core::VarDef(cfg::VoxformatTexturePath, "", core::CV_NOPERSIST,
				   N_("Register an additional search path for texture lookups")));
	core::registerVar(core::VarDef(cfg::VoxformatImageImportType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   N_("0 = plane, 1 = heightmap, 2 = volume"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>));
	core::registerVar(core::VarDef(cfg::VoxformatImageSaveType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   NC_("Image save type", "0 = plane, 1 = heightmap, 2 = volume, 3 = thumbnail"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>));
	core::registerVar(core::VarDef(cfg::VoxformatImageSliceOffsetAxis, "y", core::CV_NOPERSIST,
				   N_("The axis to offset the slices when importing images as volumes or heightmaps"),
				   [](const core::String &var) {
					   return math::toAxis(var) != math::Axis::None;
				   }));
	core::registerVar(core::VarDef(cfg::VoxformatImageSliceOffset, 0, core::CV_NOPERSIST,
				   N_("The offset of the slices when importing images as volumes or heightmaps")));
	static_assert(PNGFormat::ImageType::Plane == 0, "Plane must be 0");
	static_assert(PNGFormat::ImageType::Volume == 2, "Volume must be 2");
	core::registerVar(core::VarDef(cfg::VoxformatSchematicType, "mcedit2", core::CV_NOPERSIST,
				   N_("The type of schematic format to use when saving schematics"), schematicTypeValidator));
	core::registerVar(core::VarDef(cfg::VoxformatBinvoxVersion, 2, core::CV_NOPERSIST,
				   NC_("Binvox format version", "Save in version 1, 2 or the unofficial version 3"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= 1 && type <= 3;
				   }));
	core::registerVar(core::VarDef(cfg::VoxformatSkinApplyTransform, false, core::CV_NOPERSIST,
				   N_("Apply transforms to Minecraft skins")));
	core::registerVar(core::VarDef(cfg::VoxformatSkinAddGroups, true, core::CV_NOPERSIST,
				   N_("Add groups for body parts of Minecraft skins")));
	core::registerVar(core::VarDef(cfg::VoxformatSkinMergeFaces, false, core::CV_NOPERSIST,
				   N_("Merge face parts into single volume for Minecraft skins")));
	core::registerVar(core::VarDef(cfg::VoxformatMeshSimplify, false, core::CV_NOPERSIST,
				   N_("Simplify the mesh when voxelizing a mesh format")));
	core::registerVar(core::VarDef(cfg::VoxformatGMLRegion, "", core::CV_NOPERSIST,
				   N_("World coordinate region filter for GML/CityGML import. Format: 'minX minY minZ maxX maxY maxZ' "
					 "in GML world coordinates. Only applied when the estimated voxel region exceeds the size threshold. "
					 "Objects fully inside this region are imported, others are skipped.")));
	core::registerVar(core::VarDef(cfg::VoxformatGMLFilenameFilter, "", core::CV_NOPERSIST,
				   N_("Filename filter for GML/CityGML import. Only import files that contain this string in their "
					 "filename. Wildcards are supported.")));
	core::registerVar(core::VarDef(cfg::VoxformatOSMURL, "https://overpass-api.de/api/interpreter", core::CV_NOPERSIST,
				   N_("The URL of the Overpass API endpoint")));
	core::registerVar(core::VarDef(cfg::VoxformatOSMMetersPerVoxel, 1.0f, core::CV_NOPERSIST,
				   N_("The number of real-world meters each voxel represents in OSM imports")));

	return true;
}

} // namespace voxelformat
