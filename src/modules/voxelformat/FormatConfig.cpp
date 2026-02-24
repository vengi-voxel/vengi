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

	const core::VarDef voxformatMergequads(cfg::VoxformatMergequads, true, core::CV_NOPERSIST, N_("Merge similar quads to optimize the mesh"));
	core::registerVar(voxformatMergequads);
	const core::VarDef voxformatMeshMode(cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::Binary, core::CV_NOPERSIST,
				   NC_("Voxel mesh mode description", "0 = cubes, 1 = marching cubes, 2 = binary mesher, 3 = greedy texture"),
				   core::Var::minMaxValidator<(int)voxel::SurfaceExtractionType::Cubic,
											  (int)voxel::SurfaceExtractionType::Max - 1>);
	core::registerVar(voxformatMeshMode);
	const core::VarDef voxformatReusevertices(cfg::VoxformatReusevertices, true, core::CV_NOPERSIST,
				   N_("Reuse vertices or always create new ones"));
	core::registerVar(voxformatReusevertices);
	const core::VarDef voxformatRGBWeightedAverage(cfg::VoxformatRGBWeightedAverage, true, core::CV_NOPERSIST,
				   N_("If multiple triangles contribute to the same voxel the color values are averaged based on their "
					 "area contribution - otherwise only the biggest triangle counts"));
	core::registerVar(voxformatRGBWeightedAverage);
	const core::VarDef voxformatAmbientocclusion(cfg::VoxformatAmbientocclusion, false, core::CV_NOPERSIST,
				   N_("Extra vertices for ambient occlusion"));
	core::registerVar(voxformatAmbientocclusion);
	const core::VarDef voxformatRGBFlattenFactor(cfg::VoxformatRGBFlattenFactor, 0, core::CV_NOPERSIST,
				   N_("The RGB color flatten factor for importing color and mesh formats"), core::Var::minMaxValidator<0, 255>);
	core::registerVar(voxformatRGBFlattenFactor);
	const core::VarDef voxformatTargetColors(cfg::VoxformatTargetColors, 0, core::CV_NOPERSIST,
				   N_("Target number of colors after voxelization (0 = no limit, otherwise quantize to this amount)"),
				   core::Var::minMaxValidator<0, 256>);
	core::registerVar(voxformatTargetColors);
	const core::VarDef voxformatSaveVisibleOnly(cfg::VoxformatSaveVisibleOnly, false, core::CV_NOPERSIST, N_("Save only visible nodes"));
	core::registerVar(voxformatSaveVisibleOnly);
	const core::VarDef voxformatScale(cfg::VoxformatScale, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization on all axis by the given factor"));
	core::registerVar(voxformatScale);
	const core::VarDef voxformatScaleX(cfg::VoxformatScaleX, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization X axis by the given factor"));
	core::registerVar(voxformatScaleX);
	const core::VarDef voxformatScaleY(cfg::VoxformatScaleY, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization Y axis by the given factor"));
	core::registerVar(voxformatScaleY);
	const core::VarDef voxformatScaleZ(cfg::VoxformatScaleZ, 1.0f, core::CV_NOPERSIST,
				   N_("Scale the vertices for voxelization Z axis by the given factor"));
	core::registerVar(voxformatScaleZ);
	const core::VarDef voxformatQuads(cfg::VoxformatQuads, true, core::CV_NOPERSIST,
				   N_("Export as quads. If this false, triangles will be used."));
	core::registerVar(voxformatQuads);
	const core::VarDef voxformatWithColor(cfg::VoxformatWithColor, true, core::CV_NOPERSIST, N_("Export with vertex colors"));
	core::registerVar(voxformatWithColor);
	const core::VarDef voxformatWithNormals(cfg::VoxformatWithNormals, false, core::CV_NOPERSIST,
				   N_("Export smoothed normals for cubic meshes"));
	core::registerVar(voxformatWithNormals);
	const core::VarDef voxformatColorAsFloat(cfg::VoxformatColorAsFloat, true, core::CV_NOPERSIST,
				   N_("Export with vertex colors as float values (if vertex colors are exported)"));
	core::registerVar(voxformatColorAsFloat);
	const core::VarDef voxformatWithtexcoords(cfg::VoxformatWithtexcoords, true, core::CV_NOPERSIST,
				   N_("Export with uv coordinates of the palette image"));
	core::registerVar(voxformatWithtexcoords);
	const core::VarDef voxformatTransform(cfg::VoxformatTransform, true, core::CV_NOPERSIST,
				   N_("Apply the scene graph transform to mesh exports"));
	core::registerVar(voxformatTransform);
	const core::VarDef voxformatOptimize(cfg::VoxformatOptimize, false, core::CV_NOPERSIST, N_("Apply mesh optimization steps to meshes"));
	core::registerVar(voxformatOptimize);
	const core::VarDef voxformatFillHollow(cfg::VoxformatFillHollow, true, core::CV_NOPERSIST,
				   N_("Fill the hollows when voxelizing a mesh format"));
	core::registerVar(voxformatFillHollow);
	const core::VarDef voxformatVoxelizeMode(cfg::VoxformatVoxelizeMode, MeshFormat::VoxelizeMode::HighQuality, core::CV_NOPERSIST,
				   N_("0 = high quality, 1 = faster and less memory"), core::Var::minMaxValidator<0, 1>);
	core::registerVar(voxformatVoxelizeMode);
	const core::VarDef voxformatQBTPaletteMode(cfg::VoxformatQBTPaletteMode, true, core::CV_NOPERSIST,
				   N_("Use palette mode in qubicle qbt export"));
	core::registerVar(voxformatQBTPaletteMode);
	const core::VarDef voxformatQBTMergeCompounds(cfg::VoxformatQBTMergeCompounds, false, core::CV_NOPERSIST,
				   NC_("Merge compounds when loading Qubicle QBT files", "Merge compounds on load"));
	core::registerVar(voxformatQBTMergeCompounds);
	const core::VarDef voxformatMerge(cfg::VoxformatMerge, false, core::CV_NOPERSIST, N_("Merge all objects into one"));
	core::registerVar(voxformatMerge);
	const core::VarDef voxformatEmptyPaletteIndex(cfg::VoxformatEmptyPaletteIndex, -1, core::CV_NOPERSIST,
				   N_("The index of the empty color in the palette"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= -1 && type <= 255;
				   });
	core::registerVar(voxformatEmptyPaletteIndex);
	const core::VarDef voxformatVXLLoadHVA(cfg::VoxformatVXLLoadHVA, true, core::CV_NOPERSIST, N_("Load the hva for animations"));
	core::registerVar(voxformatVXLLoadHVA);
	const core::VarDef voxformatVOXCreateGroups(cfg::VoxformatVOXCreateGroups, true, core::CV_NOPERSIST,
				   NC_("Create groups when saving MagicaVoxel vox files", "Create groups for vox file"));
	core::registerVar(voxformatVOXCreateGroups);
	const core::VarDef voxformatVOXCreateLayers(cfg::VoxformatVOXCreateLayers, true, core::CV_NOPERSIST,
				   NC_("Create layers when saving MagicaVoxel vox files", "Create layers for vox file"));
	core::registerVar(voxformatVOXCreateLayers);
	const core::VarDef voxformatQBSaveLeftHanded(cfg::VoxformatQBSaveLeftHanded, true, core::CV_NOPERSIST,
				   N_("Toggle between left and right handed"));
	core::registerVar(voxformatQBSaveLeftHanded);
	const core::VarDef voxformatQBSaveCompressed(cfg::VoxformatQBSaveCompressed, true, core::CV_NOPERSIST,
				   NC_("Save qubicle voxel files with RLE compression enabled", "Save RLE compressed"));
	core::registerVar(voxformatQBSaveCompressed);
	const core::VarDef voxelCreatePalette(
		cfg::VoxelCreatePalette, true, core::CV_NOPERSIST,
		N_("Create own palette from textures or colors or remap the existing palette colors to a new palette"));
	core::registerVar(voxelCreatePalette);
	const core::VarDef voxformatPointCloudSize(cfg::VoxformatPointCloudSize, 1, core::CV_NOPERSIST,
				   N_("Specify the side length for the voxels when loading a point cloud"));
	core::registerVar(voxformatPointCloudSize);
	const core::VarDef voxformatGLTF_KHR_materials_pbrSpecularGlossiness(cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, true, core::CV_NOPERSIST,
				   N_("Apply KHR_materials_pbrSpecularGlossiness when saving into the glTF format"));
	core::registerVar(voxformatGLTF_KHR_materials_pbrSpecularGlossiness);
	const core::VarDef voxformatGLTF_KHR_materials_specular(cfg::VoxformatGLTF_KHR_materials_specular, false, core::CV_NOPERSIST,
				   N_("Apply KHR_materials_specular when saving into the glTF format"));
	core::registerVar(voxformatGLTF_KHR_materials_specular);
	const core::VarDef voxformatWithMaterials(cfg::VoxformatWithMaterials, true, core::CV_NOPERSIST,
				   N_("Try to export material properties if the formats support it"));
	core::registerVar(voxformatWithMaterials);
	const core::VarDef voxformatImageVolumeMaxDepth(cfg::VoxformatImageVolumeMaxDepth, 1, core::CV_NOPERSIST,
				   N_("The maximum depth of the volume when importing an image as volume"),
				   core::Var::minMaxValidator<1, 255>);
	core::registerVar(voxformatImageVolumeMaxDepth);
	const core::VarDef voxformatImageHeightmapMinHeight(cfg::VoxformatImageHeightmapMinHeight, 0, core::CV_NOPERSIST,
				   N_("The minimum height of the heightmap when importing an image as heightmap"),
				   core::Var::minMaxValidator<0, 255>);
	core::registerVar(voxformatImageHeightmapMinHeight);
	const core::VarDef voxformatImageVolumeBothSides(cfg::VoxformatImageVolumeBothSides, true, core::CV_NOPERSIST,
				   N_("Import the image as volume for both sides"));
	core::registerVar(voxformatImageVolumeBothSides);
	const core::VarDef voxformatTexturePath(cfg::VoxformatTexturePath, "", core::CV_NOPERSIST,
				   N_("Register an additional search path for texture lookups"));
	core::registerVar(voxformatTexturePath);
	const core::VarDef voxformatImageImportType(cfg::VoxformatImageImportType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   N_("0 = plane, 1 = heightmap, 2 = volume"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>);
	core::registerVar(voxformatImageImportType);
	const core::VarDef voxformatImageSaveType(cfg::VoxformatImageSaveType, PNGFormat::ImageType::Plane, core::CV_NOPERSIST,
				   NC_("Image save type", "0 = plane, 1 = heightmap, 2 = volume, 3 = thumbnail"),
				   core::Var::minMaxValidator<PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume>);
	core::registerVar(voxformatImageSaveType);
	const core::VarDef voxformatImageSliceOffsetAxis(cfg::VoxformatImageSliceOffsetAxis, "y", core::CV_NOPERSIST,
				   N_("The axis to offset the slices when importing images as volumes or heightmaps"),
				   [](const core::String &var) {
					   return math::toAxis(var) != math::Axis::None;
				   });
	core::registerVar(voxformatImageSliceOffsetAxis);
	const core::VarDef voxformatImageSliceOffset(cfg::VoxformatImageSliceOffset, 0, core::CV_NOPERSIST,
				   N_("The offset of the slices when importing images as volumes or heightmaps"));
	core::registerVar(voxformatImageSliceOffset);
	static_assert(PNGFormat::ImageType::Plane == 0, "Plane must be 0");
	static_assert(PNGFormat::ImageType::Volume == 2, "Volume must be 2");
	const core::VarDef voxformatSchematicType(cfg::VoxformatSchematicType, "mcedit2", core::CV_NOPERSIST,
				   N_("The type of schematic format to use when saving schematics"), schematicTypeValidator);
	core::registerVar(voxformatSchematicType);
	const core::VarDef voxformatBinvoxVersion(cfg::VoxformatBinvoxVersion, 2, core::CV_NOPERSIST,
				   NC_("Binvox format version", "Save in version 1, 2 or the unofficial version 3"), [](const core::String &var) {
					   const int type = var.toInt();
					   return type >= 1 && type <= 3;
				   });
	core::registerVar(voxformatBinvoxVersion);
	const core::VarDef voxformatSkinApplyTransform(cfg::VoxformatSkinApplyTransform, false, core::CV_NOPERSIST,
				   N_("Apply transforms to Minecraft skins"));
	core::registerVar(voxformatSkinApplyTransform);
	const core::VarDef voxformatSkinAddGroups(cfg::VoxformatSkinAddGroups, true, core::CV_NOPERSIST,
				   N_("Add groups for body parts of Minecraft skins"));
	core::registerVar(voxformatSkinAddGroups);
	const core::VarDef voxformatSkinMergeFaces(cfg::VoxformatSkinMergeFaces, false, core::CV_NOPERSIST,
				   N_("Merge face parts into single volume for Minecraft skins"));
	core::registerVar(voxformatSkinMergeFaces);
	const core::VarDef voxformatMeshSimplify(cfg::VoxformatMeshSimplify, false, core::CV_NOPERSIST,
				   N_("Simplify the mesh when voxelizing a mesh format"));
	core::registerVar(voxformatMeshSimplify);
	const core::VarDef voxformatGMLRegion(cfg::VoxformatGMLRegion, "", core::CV_NOPERSIST,
				   N_("World coordinate region filter for GML/CityGML import. Format: 'minX minY minZ maxX maxY maxZ' "
					 "in GML world coordinates. Only applied when the estimated voxel region exceeds the size threshold. "
					 "Objects fully inside this region are imported, others are skipped."));
	core::registerVar(voxformatGMLRegion);
	const core::VarDef voxformatGMLFilenameFilter(cfg::VoxformatGMLFilenameFilter, "", core::CV_NOPERSIST,
				   N_("Filename filter for GML/CityGML import. Only import files that contain this string in their "
					 "filename. Wildcards are supported."));
	core::registerVar(voxformatGMLFilenameFilter);
	const core::VarDef voxformatOSMURL(cfg::VoxformatOSMURL, "https://overpass-api.de/api/interpreter", core::CV_NOPERSIST,
				   N_("The URL of the Overpass API endpoint"));
	core::registerVar(voxformatOSMURL);
	const core::VarDef voxformatOSMMetersPerVoxel(cfg::VoxformatOSMMetersPerVoxel, 1.0f, core::CV_NOPERSIST,
				   N_("The number of real-world meters each voxel represents in OSM imports"));
	core::registerVar(voxformatOSMMetersPerVoxel);

	return true;
}

} // namespace voxelformat
