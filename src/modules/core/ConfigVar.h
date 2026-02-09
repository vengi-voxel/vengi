/**
 * @file
 */

#pragma once

namespace cfg {

constexpr const char *ClientMouseRotationSpeed = "cl_cammouserotspeed";
constexpr const char *ClientVSync = "cl_vsync";
constexpr const char *ClientDebugSeverity = "cl_debugseverity";
constexpr const char *ClientMultiSampleSamples = "cl_multisamplesamples";
constexpr const char *ClientMultiSampleBuffers = "cl_multisamplebuffers";
constexpr const char *ClientShadowMapSize = "cl_shadowmapsize";
constexpr const char *ClientOpenGLVersion = "cl_openglversion";
constexpr const char *ClientRenderUI = "cl_renderui";
constexpr const char *ClientWindowDisplay = "cl_display";
constexpr const char *ClientWindowHighDPI = "cl_highdpi";
constexpr const char *UIShowMetrics = "ui_showmetrics";
constexpr const char *UIFontSize = "ui_fontsize";
constexpr const char *UIKeyMap = "ui_keymap";
constexpr const char *UILastDirectory = "ui_lastdirectory";
constexpr const char *UILastFilterSave = "ui_lastfiltersave";
constexpr const char *UILastFilterOpen = "ui_lastfilteropen";
constexpr const char *UIStyle = "ui_style";
// filedialog
constexpr const char *UIFileDialogShowHidden = "ui_filedialogshowhidden";
constexpr const char *UIFileDialogLastFile = "ui_filedialoglastfile";
constexpr const char *UIFileDialogLastFiles = "ui_filedialoglastfiles";
constexpr const char *UINotifyDismissMillis = "ui_notifydismiss";
constexpr const char *UIMultiMonitor = "ui_multimonitor";
constexpr const char *UIBookmarks = "ui_bookmarks";

constexpr const char *AssetPanelLocalDirectory = "ui_assetlocaldirectory";

constexpr const char *HttpConnectTimeout = "http_connecttimeout";
constexpr const char *HttpTimeout = "http_timeout";
constexpr const char *HttpConnectTimeoutAssets = "http_connecttimeout_assets";
constexpr const char *HttpTimeoutAssets = "http_timeout_assets";

constexpr const char *ClientGamma = "cl_gamma";
constexpr const char *ClientShadowMap = "cl_shadowmap";
constexpr const char *ClientBloom = "cl_bloom";
constexpr const char *ClientCameraMinZoom = "cl_camminzoom";
constexpr const char *ClientCameraMaxZoom = "cl_cammaxzoom";
constexpr const char *ClientCameraZoomSpeed = "cl_camzoomspeed";

constexpr const char *ClientDebugShadowMapCascade = "cl_debug_cascade";
constexpr const char *ClientDebugShadow = "cl_debug_shadow";

constexpr const char *RenderCullBuffers = "r_cullbuffers";
constexpr const char *RenderCullNodes = "r_cullnodes";
constexpr const char *RenderOutline = "r_renderoutline";
constexpr const char *RenderNormals = "r_normals";
constexpr const char *ToneMapping = "r_tonemapping";
constexpr const char *RenderCheckerBoard = "r_checkerboard";
constexpr const char *MaxAnisotropy = "r_maxanisotropy";

constexpr const char *CoreMaxFPS = "core_maxfps";
constexpr const char *CoreLogLevel = "core_loglevel";
constexpr const char *CorePath = "core_path";
constexpr const char *CoreColorReduction = "core_colorreduction";
constexpr const char *CoreLanguage = "core_language";

// The size of the mesh chunk
constexpr const char *VoxelMeshSize = "voxel_meshsize";
constexpr const char *VoxRenderMeshMode = "vox_meshmode";
constexpr const char *VoxformatMeshMode = "voxformat_meshmode";

constexpr const char *AppPipe = "app_pipe";
constexpr const char *AppHomePath = "app_homepath";
constexpr const char *AppVersion = "app_version";
constexpr const char *AppUserName = "app_username";

constexpr const char *MetricPort = "metric_port";
constexpr const char *MetricHost = "metric_host";
constexpr const char *MetricJsonUrl = "metric_json_url";
constexpr const char *MetricFlavor = "metric_flavor";
constexpr const char *MetricUUID = "metric_uuid";

constexpr const char *VoxelPalette = "palette";
constexpr const char *NormalPalette = "normalpalette";
constexpr const char *PalformatRGB6Bit = "palformat_rgb6bit";
constexpr const char *PalformatMaxSize = "palformat_maxsize";
constexpr const char *PalformatGimpRGBA = "palformat_gimprgba";
constexpr const char *VoxConvertDepthFactor2D = "voxconvert_depthfactor2d";
constexpr const char *VoxelCreatePalette = "voxformat_createpalette";
constexpr const char *VoxformatMergequads = "voxformat_mergequads";
constexpr const char *VoxformatReusevertices = "voxformat_reusevertices";
constexpr const char *VoxformatAmbientocclusion = "voxformat_ambientocclusion";
constexpr const char *VoxformatRGBFlattenFactor = "voxformat_rgbflattenfactor";
constexpr const char *VoxformatRGBWeightedAverage = "voxformat_rgbweightedaverage";
constexpr const char *VoxformatScale = "voxformat_scale";
constexpr const char *VoxformatSaveVisibleOnly = "voxformat_savevisibleonly";
constexpr const char *VoxformatMerge = "voxformat_merge";
constexpr const char *VoxformatEmptyPaletteIndex = "voxformat_emptypaletteindex";
constexpr const char *VoxformatScaleX = "voxformat_scale_x";
constexpr const char *VoxformatScaleY = "voxformat_scale_y";
constexpr const char *VoxformatScaleZ = "voxformat_scale_z";
constexpr const char *VoxformatQuads = "voxformat_quads";
constexpr const char *VoxformatWithColor = "voxformat_withcolor";
constexpr const char *VoxformatWithNormals = "voxformat_withnormals";
constexpr const char *VoxformatWithMaterials = "voxformat_withmaterials";
constexpr const char *VoxformatColorAsFloat = "voxformat_colorasfloat";
constexpr const char *VoxformatWithtexcoords = "voxformat_withtexcoords";
constexpr const char *VoxformatPointCloudSize = "voxformat_pointcloudsize";
constexpr const char *VoxformatTransform = "voxformat_transform_mesh";
constexpr const char *VoxformatOptimize = "voxformat_optimize";
constexpr const char *VoxformatMeshSimplify = "voxformat_mesh_simplify";
constexpr const char *VoxformatFillHollow = "voxformat_fillhollow";
constexpr const char *VoxformatVoxelizeMode = "voxformat_voxelizemode";
constexpr const char *VoxformatQBTPaletteMode = "voxformat_qbtpalettemode";
constexpr const char *VoxformatQBTMergeCompounds = "voxformat_qbtmergecompounds";
constexpr const char *VoxformatVOXCreateLayers = "voxformat_voxcreatelayers";
constexpr const char *VoxformatVOXCreateGroups = "voxformat_voxcreategroups";
constexpr const char *VoxformatVXLLoadHVA = "voxformat_vxllodhva";
constexpr const char *VoxformatQBSaveLeftHanded = "voxformat_qbsavelefthanded";
constexpr const char *VoxformatQBSaveCompressed = "voxformat_qbsavecompressed";
constexpr const char *VoxformatGLTF_KHR_materials_pbrSpecularGlossiness = "voxformat_gltf_khr_materials_pbrspecularglossiness";
constexpr const char *VoxformatGLTF_KHR_materials_specular = "voxformat_gltf_khr_materials_specular";
constexpr const char *VoxformatImageVolumeMaxDepth = "voxformat_imagevolumemaxdepth";
constexpr const char *VoxformatImageHeightmapMinHeight = "voxformat_imageheightmapminheight";
constexpr const char *VoxformatImageVolumeBothSides = "voxformat_imagevolumebothsides";
constexpr const char *VoxformatImageImportType = "voxformat_imageimporttype";
constexpr const char *VoxformatImageSliceOffsetAxis = "voxformat_imagesliceoffsetaxis";
constexpr const char *VoxformatImageSliceOffset = "voxformat_imagesliceoffset";
constexpr const char *VoxformatImageSaveType = "voxformat_imagesavetype";
constexpr const char *VoxformatTexturePath = "voxformat_texturepath";
constexpr const char *VoxformatSchematicType = "voxformat_schematictype";
constexpr const char *VoxformatBinvoxVersion = "voxformat_binvoxversion";
constexpr const char *VoxformatSkinApplyTransform = "voxformat_skinapplytransform";
constexpr const char *VoxformatSkinAddGroups = "voxformat_skinaddgroups";
constexpr const char *VoxformatSkinMergeFaces = "voxformat_skinmergefaces";
constexpr const char *VoxformatGMLRegion = "voxformat_gmlregion";
constexpr const char *VoxformatGMLFilenameFilter = "voxformat_gmlfilenamefilter";
constexpr const char *VoxformatOSMURL = "voxformat_osmurl";

constexpr const char *GameModeClipping = "g_clipping";
constexpr const char *GameModeApplyGravity = "g_applygravity";
constexpr const char *GameModeJumpVelocity = "g_jumpvelocity";
constexpr const char *GameModeBodyHeight = "g_bodyheight";
constexpr const char *GameModeMovementSpeed = "g_movementspeed";
constexpr const char *GameModeGravity = "g_gravity";
constexpr const char *GameModeFriction = "g_friction";
constexpr const char *GameModeBodySize = "g_bodysize";

} // namespace cfg
