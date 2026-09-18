/**
 * @file
 */

#include "FormatConfig.h"
#include "app/I18N.h"
#include "core/ArrayLength.h"
#include "core/ConfigVar.h"
#include "core/Path.h"
#include "core/String.h"
#include "core/Var.h"
#include "io/FormatDescription.h"
#include "io/Stream.h"
#include "palette/FormatConfig.h"
#include "voxel/SurfaceExtractor.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelformat/private/binvox/BinVoxFormat.h"
#include "voxelformat/private/commandconquer/VXLFormat.h"
#include "voxelformat/private/image/AsepriteFormat.h"
#include "voxelformat/private/image/PNGFormat.h"
#include "voxelformat/private/magicavoxel/VoxFormat.h"
#include "voxelformat/private/mesh/GLTFFormat.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelformat/private/mesh/OBJFormat.h"
#include "voxelformat/private/mesh/PLYFormat.h"
#include "voxelformat/private/mesh/STLFormat.h"
#include "voxelformat/private/mesh/gis/GMLFormat.h"
#include "voxelformat/private/mesh/gis/OSMFormat.h"
#include "voxelformat/private/mesh/lego/LDrawFormat.h"
#include "voxelformat/private/mesh/lego/LXFFormat.h"
#include "voxelformat/private/mesh/lego/StudioIOFormat.h"
#include "voxelformat/private/minecraft/DatFormat.h"
#include "voxelformat/private/minecraft/MCRFormat.h"
#include "voxelformat/private/minecraft/MCWorldFormat.h"
#include "voxelformat/private/minecraft/SchematicFormat.h"
#include "voxelformat/private/minecraft/SkinFormat.h"
#include "voxelformat/private/qubicle/QBFormat.h"
#include "voxelformat/private/qubicle/QBTFormat.h"
#include "voxelformat/private/vengi/VENGIFormat.h"

#include <string.h>

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
		cfg::VoxformatTargetColors, 0, 0, 256, N_("Target"),
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
	const core::VarDef voxformatVoxelizeChunked(cfg::VoxformatVoxelizeChunked, false, N_("Chunked voxelization"),
												N_("Enable chunked voxelization for large meshes"), core::CV_NOPERSIST);
	core::registerVar(voxformatVoxelizeChunked);
	const core::VarDef voxformatVoxelizeChunkSize(cfg::VoxformatVoxelizeChunkSize, 128, 16, 512,
												  N_("Chunk size"), N_("Chunk size for chunked voxelization"),
												  core::CV_NOPERSIST);
	core::registerVar(voxformatVoxelizeChunkSize);
	const core::VarDef voxformatVoxelSize(cfg::VoxformatVoxelSize, 0, 0, 1024,
										  N_("Voxel size"),
										  N_("The number of voxels on the largest axis (0 = disabled, use scale cvars instead). This only works for single mesh imports."),
										  core::CV_NOPERSIST);
	core::registerVar(voxformatVoxelSize);
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
	const core::VarDef voxelCropOnLoad(
		cfg::VoxelCropOnLoad, false, N_("Crop on load"),
		N_("Crop volumes to tight bounds on load to save memory"), core::CV_NOPERSIST);
	core::registerVar(voxelCropOnLoad);
	const core::VarDef voxelTextureDedupe(
		cfg::VoxelTextureDedupe, true, N_("Dedupe texture atlas"),
		N_("Reuse identical color patches in the greedy texture atlas to pack more faces"), core::CV_NOPERSIST);
	core::registerVar(voxelTextureDedupe);
	const core::VarDef voxformatVOXCreateGroups(
		cfg::VoxformatVOXCreateGroups, true, N_("Create groups"),
		NC_("Create groups when saving MagicaVoxel vox files", "Create groups for vox file"), core::CV_NOPERSIST);
	core::registerVar(voxformatVOXCreateGroups);
	const core::VarDef voxformatVOXCreateLayers(
		cfg::VoxformatVOXCreateLayers, true, N_("Create layers"),
		NC_("Create layers when saving MagicaVoxel vox files", "Create layers for vox file"), core::CV_NOPERSIST);
	core::registerVar(voxformatVOXCreateLayers);
	const core::VarDef voxformatVOXAnimAsNodes(
		cfg::VoxformatVOXAnimAsNodes, false, N_("Animation as nodes"),
		NC_("Import MagicaVoxel animation frames as dedicated single volume nodes instead of keyframe animations",
			"Import animation frames as nodes"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatVOXAnimAsNodes);
	const core::VarDef voxformatMVApplyTransform(
		cfg::VoxformatMVApplyTransform, true, N_("Apply transforms"),
		NC_("Bake MagicaVoxel nTRN transforms into voxels on load (default). Disable to keep shared models and node TRS",
			"Bake MagicaVoxel transforms into voxels"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatMVApplyTransform);
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
	const core::VarDef voxformatPointCloud(
		cfg::VoxformatPointCloud, false, N_("Save as point cloud"),
		N_("Export visible voxels as point samples for mesh formats that support point cloud saving"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatPointCloud);
	const core::VarDef voxformatPointCloudSize(cfg::VoxformatPointCloudSize, 1, N_("Point cloud size"),
											   N_("Specify the side length for the voxels when loading a point cloud"),
											   core::CV_NOPERSIST);
	core::registerVar(voxformatPointCloudSize);
	const core::VarDef voxformatGLTF_KHR_materials_pbrSpecularGlossiness(
		cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, false, N_("KHR_materials_pbrSpecularGlossiness"),
		N_("Apply KHR_materials_pbrSpecularGlossiness when saving into the glTF format (prefer KHR_materials_specular)"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatGLTF_KHR_materials_pbrSpecularGlossiness);
	const core::VarDef voxformatGLTF_KHR_materials_specular(
		cfg::VoxformatGLTF_KHR_materials_specular, true, N_("KHR_materials_specular"),
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
	const core::VarDef voxformatTexturePath(cfg::VoxformatTexturePath, core::Path(), N_("Texture search path"),
											N_("Register an additional search path for texture lookups"),
											core::CV_NOPERSIST, core::VarType::Directory);
	core::registerVar(voxformatTexturePath);
	const core::VarDef voxformatImageImportType(cfg::VoxformatImageImportType, PNGFormat::ImageType::Plane,
												PNGFormat::ImageType::Plane, PNGFormat::ImageType::Volume,
												N_("Image import type"), N_("0 = plane, 1 = heightmap, 2 = volume"),
												core::CV_NOPERSIST);
	core::registerVar(voxformatImageImportType);
	const core::VarDef voxformatImageSaveType(
		cfg::VoxformatImageSaveType, PNGFormat::ImageType::Plane, PNGFormat::ImageType::Plane,
		PNGFormat::ImageType::Thumbnail, N_("Image save type"),
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
	static_assert(PNGFormat::ImageType::Thumbnail == 3, "Thumbnail must be 3");
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
	const core::VarDef voxformatMCSeparateWater(
		cfg::VoxformatMCSeparateWater, false, N_("Separate water"),
		N_("Put Minecraft water voxels into a dedicated transparent volume as a child of each region"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatMCSeparateWater);
	const core::VarDef voxformatMeshSimplify(cfg::VoxformatMeshSimplify, false, N_("Simplify"),
											 N_("Simplify the mesh when voxelizing a mesh format"), core::CV_NOPERSIST);
	core::registerVar(voxformatMeshSimplify);
	const core::VarDef voxformatMeshSimplifyRatio(
		cfg::VoxformatMeshSimplifyRatio, 0.8f, 0.0f, 1.0f, N_("Mesh simplify ratio"),
		N_("Target fraction of triangle indices to keep when applying mesh optimization (0 = disable simplification)"),
		core::CV_NOPERSIST);
	core::registerVar(voxformatMeshSimplifyRatio);
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
	const core::VarDef voxformatLDrawDir(cfg::VoxformatLDrawDir,
#ifdef __linux__
										 core::Path(core::String("/usr/share/ldraw/")),
#else
										 core::Path(),
#endif
										 N_("LDraw library path"),
										 N_("Path to the LDraw parts library directory for resolving part references"),
										 core::CV_NOPERSIST, core::VarType::Directory);
	core::registerVar(voxformatLDrawDir);

	return true;
}

// Display order is the FileDialogOptions widget order (save entries first, then load).
static const char *const g_meshModeTitles[] = {N_("Cubes"), N_("Marching cubes"), N_("Binary"), N_("Greedy texture")};
static_assert(lengthof(g_meshModeTitles) == (int)voxel::SurfaceExtractionType::Max, "Update mesh mode value titles");

static const FormatVarMeta g_formatCVars[] = {
	// mesh save
	{cfg::VoxformatMergequads, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatReusevertices, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxelTextureDedupe, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatAmbientocclusion, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatTransform, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatOptimize, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatMeshSimplifyRatio, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatPointCloud, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatQuads, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatWithColor, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatWithNormals, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatColorAsFloat, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatWithtexcoords, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatGLTF_KHR_materials_pbrSpecularGlossiness, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {&GLTFFormat::format()}, {}},
	{cfg::VoxformatGLTF_KHR_materials_specular, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {&GLTFFormat::format()}, {}},
	{cfg::VoxformatWithMaterials, FormatCVarFlag_Save | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatMeshMode, FormatCVarFlag_Save | FormatCVarFlag_Mesh | FormatCVarFlag_Primary, {}, {g_meshModeTitles[0], g_meshModeTitles[1], g_meshModeTitles[2], g_meshModeTitles[3]}},
	{cfg::VoxformatBinvoxVersion, FormatCVarFlag_Save, {&BinVoxFormat::format()}, {nullptr, N_("Binvox 1 (white)"), N_("Binvox 2 (multi colors)"), N_("Binvox 3 (unofficial)")}},
	{cfg::VoxformatSchematicType, FormatCVarFlag_Save, {&SchematicFormat::format()}, {}},
	{cfg::VoxformatQBTPaletteMode, FormatCVarFlag_Save, {&QBTFormat::format()}, {}},
	{cfg::VoxformatVOXCreateGroups, FormatCVarFlag_Save, {&VoxFormat::format()}, {}},
	{cfg::VoxformatVOXCreateLayers, FormatCVarFlag_Save, {&VoxFormat::format()}, {}},
	{cfg::VoxformatVOXAnimAsNodes, FormatCVarFlag_Load | FormatCVarFlag_Save, {&VoxFormat::format()}, {}},
	{cfg::VoxformatQBSaveLeftHanded, FormatCVarFlag_Save, {&QBFormat::format()}, {}},
	{cfg::VoxformatQBSaveCompressed, FormatCVarFlag_Save, {&QBFormat::format()}, {}},
	{cfg::VoxformatImageSaveType, FormatCVarFlag_Save | FormatCVarFlag_Image, {&PNGFormat::format()}, {N_("Plane"), N_("Heightmap"), N_("Volume"), N_("Thumbnail")}},
	{cfg::VoxformatEmptyPaletteIndex, FormatCVarFlag_Save, {&VENGIFormat::format()}, {}},
	{cfg::VoxformatMerge, FormatCVarFlag_Save | FormatCVarFlag_All, {}, {}},
	{cfg::VoxformatSaveVisibleOnly, FormatCVarFlag_Save | FormatCVarFlag_All, {}, {}},
	{cfg::VoxformatVoxelSize, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_Primary, {}, {}},
	{cfg::VoxformatScale, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatScaleX, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatScaleY, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatScaleZ, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatTexturePath, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatFillHollow, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_Primary, {}, {}},
	{cfg::VoxformatPointCloudSize, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatMeshSimplify, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::NormalPalette, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatGMLRegion, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {&GMLFormat::format()}, {}},
	{cfg::VoxformatGMLFilenameFilter, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {&GMLFormat::format()}, {}},
	{cfg::VoxformatVoxelizeMode, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_Primary, {}, {N_("High quality"), N_("Fast")}},
	{cfg::VoxformatVoxelizeChunked, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatVoxelizeChunkSize, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatRGBWeightedAverage, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {}, {}},
	{cfg::VoxformatOSMURL, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {&OSMFormat::format()}, {}},
	{cfg::VoxformatOSMMetersPerVoxel, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {&OSMFormat::format()}, {}},
	{cfg::VoxformatLDrawDir, FormatCVarFlag_Load | FormatCVarFlag_Mesh, {&LDrawFormat::format(), &StudioIOFormat::format(), &LXFFormat::format()}, {}},
	{cfg::VoxformatImageImportType, FormatCVarFlag_Load | FormatCVarFlag_Image, {&PNGFormat::format()}, {N_("Plane"), N_("Heightmap"), N_("Volume")}},
	{cfg::VoxformatImageVolumeMaxDepth, FormatCVarFlag_Load | FormatCVarFlag_Image, {&PNGFormat::format()}, {}},
	{cfg::VoxformatImageVolumeBothSides, FormatCVarFlag_Load | FormatCVarFlag_Image, {&PNGFormat::format()}, {}},
	{cfg::VoxformatImageHeightmapMinHeight, FormatCVarFlag_Load | FormatCVarFlag_Image, {&PNGFormat::format()}, {}},
	{cfg::VoxformatImageSliceOffset, FormatCVarFlag_Load | FormatCVarFlag_Image, {&AsepriteFormat::format()}, {}},
	{cfg::VoxformatImageSliceOffsetAxis, FormatCVarFlag_Load | FormatCVarFlag_Image, {&AsepriteFormat::format()}, {}},
	{cfg::VoxformatSkinApplyTransform, FormatCVarFlag_Load, {&SkinFormat::format()}, {}},
	{cfg::VoxformatSkinAddGroups, FormatCVarFlag_Load, {&SkinFormat::format()}, {}},
	{cfg::VoxformatSkinMergeFaces, FormatCVarFlag_Load, {&SkinFormat::format()}, {}},
	{cfg::VoxformatMCSeparateWater, FormatCVarFlag_Load, {&MCRFormat::format(), &DatFormat::format(), &MCWorldFormat::format(), &SchematicFormat::format()}, {}},
	{cfg::VoxformatVXLLoadHVA, FormatCVarFlag_Load, {&VXLFormat::format()}, {}},
	{cfg::VoxformatMVApplyTransform, FormatCVarFlag_Load, {&VoxFormat::format()}, {}},
	{cfg::VoxformatQBTMergeCompounds, FormatCVarFlag_Load, {&QBTFormat::format()}, {}},
	{cfg::VoxelCropOnLoad, FormatCVarFlag_Load | FormatCVarFlag_All, {}, {}},
	{cfg::CoreColorReduction, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_RGB | FormatCVarFlag_Primary, {}, {}},
	{cfg::VoxformatRGBFlattenFactor, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_RGB, {}, {}},
	{cfg::VoxformatTargetColors, FormatCVarFlag_Load | FormatCVarFlag_Mesh | FormatCVarFlag_RGB, {}, {}},
	{cfg::VoxelCreatePalette, FormatCVarFlag_Load | FormatCVarFlag_All, {}, {}},
	{cfg::VoxelPalette, FormatCVarFlag_Load | FormatCVarFlag_All | FormatCVarFlag_Primary, {}, {}},
};

static_assert(MeshFormat::VoxelizeMode::HighQuality == 0, "HighQuality must be 0");
static_assert(MeshFormat::VoxelizeMode::Fast == 1, "Fast must be 1");

const FormatVarMeta *FormatConfig::varsMeta() {
	return g_formatCVars;
}

int FormatConfig::cvarCount() {
	return lengthof(g_formatCVars);
}

const FormatVarMeta *FormatConfig::findVarMeta(const char *name) {
	if (name == nullptr) {
		return nullptr;
	}
	for (int i = 0; i < lengthof(g_formatCVars); ++i) {
		if (strcmp(g_formatCVars[i].name, name) == 0) {
			return &g_formatCVars[i];
		}
	}
	return nullptr;
}

static bool hasFormats(const FormatVarMeta &meta) {
	return meta.formats[0] != nullptr;
}

static bool isImageFormat(const io::FormatDescription &desc) {
	return desc == PNGFormat::format() || desc == AsepriteFormat::format();
}

bool FormatConfig::meshSaveSupportsQuads(const io::FormatDescription &desc) {
	return desc == OBJFormat::format() || desc == PLYFormat::format();
}

bool FormatConfig::meshSaveSupportsColor(const io::FormatDescription &desc) {
	return !(desc == STLFormat::format());
}

bool FormatConfig::meshSaveSupportsTexCoords(const io::FormatDescription &desc) {
	return meshSaveSupportsColor(desc);
}

static bool meshSaveCvarAllowed(const FormatVarMeta &meta, const io::FormatDescription &desc) {
	if (!strcmp(meta.name, cfg::VoxformatQuads)) {
		return FormatConfig::meshSaveSupportsQuads(desc);
	}
	if (!strcmp(meta.name, cfg::VoxformatWithColor) || !strcmp(meta.name, cfg::VoxformatColorAsFloat)) {
		return FormatConfig::meshSaveSupportsColor(desc);
	}
	if (!strcmp(meta.name, cfg::VoxformatWithtexcoords)) {
		return FormatConfig::meshSaveSupportsTexCoords(desc);
	}
	return true;
}

static bool meshSaveCvarRestrictsFormats(const FormatVarMeta &meta) {
	return !strcmp(meta.name, cfg::VoxformatQuads) || !strcmp(meta.name, cfg::VoxformatWithColor) ||
		   !strcmp(meta.name, cfg::VoxformatColorAsFloat) || !strcmp(meta.name, cfg::VoxformatWithtexcoords);
}

bool FormatConfig::appliesTo(const FormatVarMeta &meta, bool save, const io::FormatDescription &desc) {
	if (save) {
		if ((meta.flags & FormatCVarFlag_Save) == 0u) {
			return false;
		}
	} else if ((meta.flags & FormatCVarFlag_Load) == 0u) {
		return false;
	}

	if (meta.flags & FormatCVarFlag_All) {
		return true;
	}

	if (hasFormats(meta)) {
		for (int i = 0; i < FormatVarMeta::MaxFormats; ++i) {
			if (meta.formats[i] == nullptr) {
				break;
			}
			if (*meta.formats[i] == desc) {
				return true;
			}
		}
		return false;
	}

	if ((meta.flags & FormatCVarFlag_Mesh) && isMeshFormat(desc)) {
		if (save) {
			return meshSaveCvarAllowed(meta, desc);
		}
		return true;
	}
	if ((meta.flags & FormatCVarFlag_Image) && isImageFormat(desc)) {
		return true;
	}
	if ((meta.flags & FormatCVarFlag_RGB) && isRGBFormat(desc)) {
		return true;
	}
	return false;
}

static void writeJsonBool(io::WriteStream &stream, const char *key, bool value) {
	stream.writeStringFormat(false, ",\"%s\": %s", key, value ? "true" : "false");
}

void FormatConfig::writeConfigJson(io::WriteStream &stream, const core::VarPtr &var) {
	if (!var) {
		return;
	}
	const FormatVarMeta *meta = findVarMeta(var->name());
	if (meta == nullptr) {
		return;
	}

	writeJsonBool(stream, "load", (meta->flags & FormatCVarFlag_Load) != 0u);
	writeJsonBool(stream, "save", (meta->flags & FormatCVarFlag_Save) != 0u);
	writeJsonBool(stream, "mesh", (meta->flags & FormatCVarFlag_Mesh) != 0u);
	writeJsonBool(stream, "image", (meta->flags & FormatCVarFlag_Image) != 0u);
	writeJsonBool(stream, "rgb", (meta->flags & FormatCVarFlag_RGB) != 0u);
	writeJsonBool(stream, "all", (meta->flags & FormatCVarFlag_All) != 0u);
	writeJsonBool(stream, "primary", (meta->flags & FormatCVarFlag_Primary) != 0u);
	stream.writeStringFormat(false, ",\"order\": %i", (int)(meta - g_formatCVars));

	if (hasFormats(*meta)) {
		stream.writeString(",\"formats\": [", false);
		bool first = true;
		for (int i = 0; i < FormatVarMeta::MaxFormats; ++i) {
			if (meta->formats[i] == nullptr) {
				break;
			}
			if (!first) {
				stream.write(",", 1);
			}
			first = false;
			stream.writeJsonString(meta->formats[i]->name.c_str());
		}
		stream.writeString("]", false);
	} else if (meshSaveCvarRestrictsFormats(*meta)) {
		stream.writeString(",\"formats\": [", false);
		bool first = true;
		for (const io::FormatDescription *desc = voxelSave(); desc->valid(); ++desc) {
			if (!isMeshFormat(*desc) || !meshSaveCvarAllowed(*meta, *desc)) {
				continue;
			}
			if (!first) {
				stream.write(",", 1);
			}
			first = false;
			stream.writeJsonString(desc->name.c_str());
		}
		stream.writeString("]", false);
	}

	int lastTitle = -1;
	for (int i = 0; i < FormatVarMeta::MaxValueTitles; ++i) {
		if (meta->valueTitles[i] != nullptr && meta->valueTitles[i][0] != '\0') {
			lastTitle = i;
		}
	}
	if (lastTitle < 0) {
		return;
	}
	stream.writeString(",\"value_titles\": [", false);
	for (int i = 0; i <= lastTitle; ++i) {
		if (i > 0) {
			stream.write(",", 1);
		}
		stream.writeJsonString(meta->valueTitles[i]);
	}
	stream.writeString("]", false);
}

} // namespace voxelformat
