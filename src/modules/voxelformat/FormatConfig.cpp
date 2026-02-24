/**
 * @file
 */

#include "FormatConfig.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "palette/FormatConfig.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelformat/private/image/PNGFormat.h"
#include "voxelformat/private/mesh/MeshFormat.h"

namespace voxelformat {

bool FormatConfig::init() {
	palette::FormatConfig::init();

	const core::VarDef voxformatMergequads(cfg::VoxformatMergequads, true, N_("Merge quads"),
										   N_("Merge similar quads to optimize the mesh"), core::CV_NOPERSIST);
	core::registerVar(voxformatMergequads);
	const core::VarDef voxformatMeshMode(
		cfg::VoxformatMeshMode, (int)voxel::SurfaceExtractionType::Binary, (int)voxel::SurfaceExtractionType::Cubic,
		(int)voxel::SurfaceExtractionType::Max - 1, N_("Mesh mode"),
		NC_("Voxel mesh mode description", "0 = cubes, 1 = marching cubes, 2 = binary mesher, 3 = greedy texture"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatMeshMode);
	const core::VarDef voxformatReusevertices(cfg::VoxformatReusevertices, true, N_("Reuse vertices"),
											  N_("Reuse vertices or always create new ones"), core::CV_NOPERSIST);
	core::registerVar(voxformatReusevertices);
	const core::VarDef voxformatRGBWeightedAverage(
		cfg::VoxformatRGBWeightedAverage, true, N_("RGB weighted average"),
		N_("If multiple triangles contribute to the same voxel the color values are averaged based on their "
		   "area contribution - otherwise only the biggest triangle counts"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatRGBWeightedAverage);
	const core::VarDef voxformatAmbientocclusion(cfg::VoxformatAmbientocclusion, false, N_("Ambient occlusion"),
												 N_("Extra vertices for ambient occlusion"), core::CV_NOPERSIST);
	core::registerVar(voxformatAmbientocclusion);
	const core::VarDef voxformatRGBFlattenFactor(
		cfg::VoxformatRGBFlattenFactor, 0, 0, 255, N_("RGB flatten factor"),
		N_("The RGB color flatten factor for importing color and mesh formats"), core::CV_NOPERSIST);
	core::registerVar(voxformatRGBFlattenFactor);
	const core::VarDef voxformatTargetColors(
		cfg::VoxformatTargetColors, 0, 0, 256, N_("Target colors (0=no limit)"),
		N_("Target number of colors after voxelization (0 = no limit, otherwise quantize to this amount)"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatTargetColors);
	const core::VarDef voxformatSaveVisibleOnly(cfg::VoxformatSaveVisibleOnly, false, N_("Save visible only"),
												N_("Save only visible nodes"), core::CV_NOPERSIST);
	core::registerVar(voxformatSaveVisibleOnly);
	const core::VarDef voxformatScale(cfg::VoxformatScale, 1.0f, N_("Uniform scale"),
									  N_("Scale the vertices for voxelization on all axis by the given factor"),
									  core::CV_NOPERSIST);
	core::registerVar(voxformatScale);
	const core::VarDef voxformatScaleX(cfg::VoxformatScaleX, 1.0f, N_("X axis scale"),
									   N_("Scale the vertices for voxelization X axis by the given factor"),
									   core::CV_NOPERSIST);
	core::registerVar(voxformatScaleX);
	const core::VarDef voxformatScaleY(cfg::VoxformatScaleY, 1.0f, N_("Y axis scale"),
									   N_("Scale the vertices for voxelization Y axis by the given factor"),
									   core::CV_NOPERSIST);
	core::registerVar(voxformatScaleY);
	const core::VarDef voxformatScaleZ(cfg::VoxformatScaleZ, 1.0f, N_("Z axis scale"),
									   N_("Scale the vertices for voxelization Z axis by the given factor"),
									   core::CV_NOPERSIST);
	core::registerVar(voxformatScaleZ);
	const core::VarDef voxformatQuads(cfg::VoxformatQuads, true, N_("Exports quads"),
									  N_("Export as quads. If this false, triangles will be used."),
									  core::CV_NOPERSIST);
	core::registerVar(voxformatQuads);
	const core::VarDef voxformatWithColor(cfg::VoxformatWithColor, true, N_("Vertex colors"),
										  N_("Export with vertex colors"), core::CV_NOPERSIST);
	core::registerVar(voxformatWithColor);
	const core::VarDef voxformatWithNormals(cfg::VoxformatWithNormals, false, N_("Normals"),
											N_("Export smoothed normals for cubic meshes"), core::CV_NOPERSIST);
	core::registerVar(voxformatWithNormals);
	const core::VarDef voxformatColorAsFloat(
		cfg::VoxformatColorAsFloat, true, N_("Vertex colors as float"),
		N_("Export with vertex colors as float values (if vertex colors are exported)"), core::CV_NOPERSIST);
	core::registerVar(voxformatColorAsFloat);
	const core::VarDef voxformatWithtexcoords(cfg::VoxformatWithtexcoords, true, N_("Texture coordinates"),
											  N_("Export with uv coordinates of the palette image"),
											  core::CV_NOPERSIST);
	core::registerVar(voxformatWithtexcoords);
	const core::VarDef voxformatTransform(cfg::VoxformatTransform, true, N_("Apply transformations"),
										  N_("Apply the scene graph transform to mesh exports"), core::CV_NOPERSIST);
	core::registerVar(voxformatTransform);
	const core::VarDef voxformatOptimize(cfg::VoxformatOptimize, false, N_("Apply optimizations"),
										 N_("Apply mesh optimization steps to meshes"), core::CV_NOPERSIST);
	core::registerVar(voxformatOptimize);
	const core::VarDef voxformatFillHollow(cfg::VoxformatFillHollow, true, N_("Fill hollow"),
										   N_("Fill the hollows when voxelizing a mesh format"), core::CV_NOPERSIST);
	core::registerVar(voxformatFillHollow);
	const core::VarDef voxformatVoxelizeMode(cfg::VoxformatVoxelizeMode, MeshFormat::VoxelizeMode::HighQuality, 0, 1,
											 N_("Voxelize mode"), N_("0 = high quality, 1 = faster and less memory"),
											 core::CV_NOPERSIST);
	core::registerVar(voxformatVoxelizeMode);
	const core::VarDef voxformatQBTPaletteMode(cfg::VoxformatQBTPaletteMode, true, N_("Palette mode"),
											   N_("Use palette mode in qubicle qbt export"), core::CV_NOPERSIST);
	core::registerVar(voxformatQBTPaletteMode);
	const core::VarDef voxformatQBTMergeCompounds(
		cfg::VoxformatQBTMergeCompounds, false, N_("Merge compounds"),
		NC_("Merge compounds when loading Qubicle QBT files", "Merge compounds on load"), core::CV_NOPERSIST);
	core::registerVar(voxformatQBTMergeCompounds);
	const core::VarDef voxformatMerge(cfg::VoxformatMerge, false, N_("Single object"), N_("Merge all objects into one"),
									  core::CV_NOPERSIST);
	core::registerVar(voxformatMerge);
	const core::VarDef voxformatEmptyPaletteIndex(
		cfg::VoxformatEmptyPaletteIndex, -1, -1, 255, N_("Empty palette index"),
		N_("The index of the empty color in the palette"), core::CV_NOPERSIST);
	core::registerVar(voxformatEmptyPaletteIndex);
	const core::VarDef voxformatVXLLoadHVA(cfg::VoxformatVXLLoadHVA, true, N_("Load HVA"),
										   N_("Load the hva for animations"), core::CV_NOPERSIST);
	core::registerVar(voxformatVXLLoadHVA);
	const core::VarDef voxformatVOXCreateGroups(
		cfg::VoxformatVOXCreateGroups, true, N_("Create groups"),
		NC_("Create groups when saving MagicaVoxel vox files", "Create groups for vox file"), core::CV_NOPERSIST);
	core::registerVar(voxformatVOXCreateGroups);
	const core::VarDef voxformatVOXCreateLayers(
		cfg::VoxformatVOXCreateLayers, true, N_("Create layers"),
		NC_("Create layers when saving MagicaVoxel vox files", "Create layers for vox file"), core::CV_NOPERSIST);
	core::registerVar(voxformatVOXCreateLayers);
	const core::VarDef voxformatQBSaveLeftHanded(cfg::VoxformatQBSaveLeftHanded, true, N_("Left handed"),
												 N_("Toggle between left and right handed"), core::CV_NOPERSIST);
	core::registerVar(voxformatQBSaveLeftHanded);
	const core::VarDef voxformatQBSaveCompressed(
		cfg::VoxformatQBSaveCompressed, true, N_("Compressed"),
		NC_("Save qubicle voxel files with RLE compression enabled", "Save RLE compressed"), core::CV_NOPERSIST);
	core::registerVar(voxformatQBSaveCompressed);
	const core::VarDef voxelCreatePalette(
		cfg::VoxelCreatePalette, true, N_("Create palette"),
		N_("Create own palette from textures or colors or remap the existing palette colors to a new palette"),
		core::CV_NOPERSIST);
	core::registerVar(voxelCreatePalette);
	const core::VarDef voxformatPointCloudSize(cfg::VoxformatPointCloudSize, 1, N_("Point cloud size"),
											   N_("Specify the side length for the voxels when loading a point cloud"),
											   core::CV_NOPERSIST);
	core::registerVar(voxformatPointCloudSize);
	const core::VarDef voxformatGLTF_KHR_materials_pbrSpecularGlossiness(
		cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, true, N_("KHR_materials_pbrSpecularGlossiness"),
		N_("Apply KHR_materials_pbrSpecularGlossiness when saving into the glTF format"), core::CV_NOPERSIST);
	core::registerVar(voxformatGLTF_KHR_materials_pbrSpecularGlossiness);
	const core::VarDef voxformatGLTF_KHR_materials_specular(
		cfg::VoxformatGLTF_KHR_materials_specular, false, N_("KHR_materials_specular"),
		N_("Apply KHR_materials_specular when saving into the glTF format"), core::CV_NOPERSIST);
	core::registerVar(voxformatGLTF_KHR_materials_specular);
	const core::VarDef voxformatWithMaterials(cfg::VoxformatWithMaterials, true, N_("Export materials"),
											  N_("Try to export material properties if the formats support it"),
											  core::CV_NOPERSIST);
	core::registerVar(voxformatWithMaterials);
	const core::VarDef voxformatImageVolumeMaxDepth(
		cfg::VoxformatImageVolumeMaxDepth, 1, 1, 255, N_("Max depth"),
		N_("The maximum depth of the volume when importing an image as volume"), core::CV_NOPERSIST);
	core::registerVar(voxformatImageVolumeMaxDepth);
	const core::VarDef voxformatImageHeightmapMinHeight(
		cfg::VoxformatImageHeightmapMinHeight, 0, 0, 255, N_("Min height"),
		N_("The minimum height of the heightmap when importing an image as heightmap"), core::CV_NOPERSIST);
	core::registerVar(voxformatImageHeightmapMinHeight);
	const core::VarDef voxformatImageVolumeBothSides(cfg::VoxformatImageVolumeBothSides, true, N_("Both sides"),
													 N_("Import the image as volume for both sides"),
													 core::CV_NOPERSIST);
	core::registerVar(voxformatImageVolumeBothSides);
	const core::VarDef voxformatTexturePath(cfg::VoxformatTexturePath, "", N_("Texture search path"),
											N_("Register an additional search path for texture lookups"),
											core::CV_NOPERSIST);
	core::registerVar(voxformatTexturePath);
	const core::VarDef voxformatImageImportType(cfg::VoxformatImageImportType, PNGFormat::ImageType::Plane,
												PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume,
												N_("Image import type"), N_("0 = plane, 1 = heightmap, 2 = volume"),
												core::CV_NOPERSIST);
	core::registerVar(voxformatImageImportType);
	const core::VarDef voxformatImageSaveType(
		cfg::VoxformatImageSaveType, PNGFormat::ImageType::Plane, PNGFormat::ImageType::Plane,
		PNGFormat::ImageType::Volume, N_("Image save type"),
		NC_("Image save type", "0 = plane, 1 = heightmap, 2 = volume, 3 = thumbnail"), core::CV_NOPERSIST);
	core::registerVar(voxformatImageSaveType);
	const core::VarDef voxformatImageSliceOffsetAxis(
		cfg::VoxformatImageSliceOffsetAxis, "y", {"x", "y", "z"}, N_("Slice offset axis"),
		N_("The axis to offset the slices when importing images as volumes or heightmaps"), core::CV_NOPERSIST);
	core::registerVar(voxformatImageSliceOffsetAxis);
	const core::VarDef voxformatImageSliceOffset(
		cfg::VoxformatImageSliceOffset, 0, N_("Slice offset"),
		N_("The offset of the slices when importing images as volumes or heightmaps"), core::CV_NOPERSIST);
	core::registerVar(voxformatImageSliceOffset);
	static_assert(PNGFormat::ImageType::Plane == 0, "Plane must be 0");
	static_assert(PNGFormat::ImageType::Volume == 2, "Volume must be 2");
	const core::VarDef voxformatSchematicType(
		cfg::VoxformatSchematicType, "mcedit2", {"mcedit2", "worldedit", "schematica"}, N_("Schematic type"),
		N_("The type of schematic format to use when saving schematics"), core::CV_NOPERSIST);
	core::registerVar(voxformatSchematicType);
	const core::VarDef voxformatBinvoxVersion(
		cfg::VoxformatBinvoxVersion, 2, 1, 3, N_("Binvox version"),
		NC_("Binvox format version", "Save in version 1, 2 or the unofficial version 3"), core::CV_NOPERSIST);
	core::registerVar(voxformatBinvoxVersion);
	const core::VarDef voxformatSkinApplyTransform(cfg::VoxformatSkinApplyTransform, false, N_("Apply transformations"),
												   N_("Apply transforms to Minecraft skins"), core::CV_NOPERSIST);
	core::registerVar(voxformatSkinApplyTransform);
	const core::VarDef voxformatSkinAddGroups(cfg::VoxformatSkinAddGroups, true, N_("Add groups"),
											  N_("Add groups for body parts of Minecraft skins"), core::CV_NOPERSIST);
	core::registerVar(voxformatSkinAddGroups);
	const core::VarDef voxformatSkinMergeFaces(cfg::VoxformatSkinMergeFaces, false, N_("Merge faces"),
											   N_("Merge face parts into single volume for Minecraft skins"),
											   core::CV_NOPERSIST);
	core::registerVar(voxformatSkinMergeFaces);
	const core::VarDef voxformatMeshSimplify(cfg::VoxformatMeshSimplify, false, N_("Simplify"),
											 N_("Simplify the mesh when voxelizing a mesh format"), core::CV_NOPERSIST);
	core::registerVar(voxformatMeshSimplify);
	const core::VarDef voxformatGMLRegion(
		cfg::VoxformatGMLRegion, "", N_("Region filter"),
		N_("World coordinate region filter for GML/CityGML import. Format: 'minX minY minZ maxX maxY maxZ' "
		   "in GML world coordinates. Only applied when the estimated voxel region exceeds the size threshold. "
		   "Objects fully inside this region are imported, others are skipped."),
		core::CV_NOPERSIST);
	core::registerVar(voxformatGMLRegion);
	const core::VarDef voxformatGMLFilenameFilter(
		cfg::VoxformatGMLFilenameFilter, "", N_("Filename filter"),
		N_("Filename filter for GML/CityGML import. Only import files that contain this string in their "
		   "filename. Wildcards are supported."),
		core::CV_NOPERSIST);
	core::registerVar(voxformatGMLFilenameFilter);
	const core::VarDef voxformatOSMURL(cfg::VoxformatOSMURL, "https://overpass-api.de/api/interpreter",
									   N_("Overpass API URL"), N_("The URL of the Overpass API endpoint"),
									   core::CV_NOPERSIST);
	core::registerVar(voxformatOSMURL);
	const core::VarDef voxformatOSMMetersPerVoxel(
		cfg::VoxformatOSMMetersPerVoxel, 1.0f, N_("Meters per voxel"),
		N_("The number of real-world meters each voxel represents in OSM imports"), core::CV_NOPERSIST);
	core::registerVar(voxformatOSMMetersPerVoxel);

	return true;
}

} // namespace voxelformat
