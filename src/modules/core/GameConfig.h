/**
 * @file
 */

#pragma once

namespace cfg {

constexpr const char *ClientMouseRotationSpeed = "cl_cammouserotspeed";
constexpr const char *ClientEmail = "cl_email";
constexpr const char *ClientPassword = "cl_password";
// name of the player that is visible by other players
constexpr const char *ClientName = "cl_name";
constexpr const char *ClientVSync = "cl_vsync";
constexpr const char *ClientDebugSeverity = "cl_debugseverity";
// the port where the server is listening on that the client wants to connect to
constexpr const char *ClientPort = "cl_port";
// the host where the server is running on that the client wants to connect to
constexpr const char *ClientHost = "cl_host";
constexpr const char *ClientFullscreen = "cl_fullscreen";
constexpr const char *ClientMultiSampleSamples = "cl_multisamplesamples";
constexpr const char *ClientMultiSampleBuffers = "cl_multisamplebuffers";
constexpr const char *ClientShadowMapSize = "cl_shadowmapsize";
constexpr const char *ClientOpenGLVersion = "cl_openglversion";
constexpr const char *ClientRenderUI = "cl_renderui";
constexpr const char *ClientWindowWidth = "cl_width";
constexpr const char *ClientWindowDisplay = "cl_display";
constexpr const char *ClientWindowHeight = "cl_height";
constexpr const char *ClientWindowHighDPI = "cl_highdpi";
constexpr const char *UIShowMetrics = "ui_showmetrics";
constexpr const char *UIFontSize = "ui_fontsize";
constexpr const char *UILastDirectory = "ui_lastdirectory";
constexpr const char *UILastFilter = "ui_lastfilter";
constexpr const char *UIStyle = "ui_style";
// filedialog
constexpr const char *UIFileDialogShowHidden = "ui_filedialogshowhidden";
constexpr const char *UINotifyDismissMillis = "ui_notifydismiss";
constexpr const char *UIBookmarks = "ui_bookmarks";

constexpr const char *ClientGamma = "cl_gamma";
constexpr const char *ClientShadowMap = "cl_shadowmap";
constexpr const char *ClientBloom = "cl_bloom";
constexpr const char *ClientCameraMinZoom = "cl_camminzoom";
constexpr const char *ClientCameraMaxZoom = "cl_cammaxzoom";

constexpr const char *ClientDebugShadowMapCascade = "cl_debug_cascade";
constexpr const char *ClientDebugShadow = "cl_debug_shadow";

constexpr const char *RenderOutline = "r_renderoutline";

constexpr const char *ConsoleCurses = "con_curses";

constexpr const char *CoreMaxFPS = "core_maxfps";
constexpr const char *CoreLogLevel = "core_loglevel";
constexpr const char *CoreSysLog = "core_syslog";
constexpr const char *CorePath = "core_path";

// The size of the chunk that is extracted with each step
constexpr const char *VoxelMeshSize = "voxel_meshsize";

constexpr const char *AppHomePath = "app_homepath";

constexpr const char *MetricPort = "metric_port";
constexpr const char *MetricHost = "metric_host";
constexpr const char *MetricFlavor = "metric_flavor";

constexpr const char *VoxelPalette = "palette";
constexpr const char *VoxformatMergequads = "voxformat_mergequads";
constexpr const char *VoxformatReusevertices = "voxformat_reusevertices";
constexpr const char *VoxformatAmbientocclusion = "voxformat_ambientocclusion";
constexpr const char *VoxformatScale = "voxformat_scale";
constexpr const char *VoxformatMerge = "voxformat_merge";
constexpr const char *VoxformatScaleX = "voxformat_scale_x";
constexpr const char *VoxformatScaleY = "voxformat_scale_y";
constexpr const char *VoxformatScaleZ = "voxformat_scale_z";
constexpr const char *VoxformatQuads = "voxformat_quads";
constexpr const char *VoxformatWithcolor = "voxformat_withcolor";
constexpr const char *VoxformatWithtexcoords = "voxformat_withtexcoords";
constexpr const char *VoxformatTransform = "voxformat_transform_mesh";
constexpr const char *VoxformatFillHollow = "voxformat_fillhollow";
constexpr const char *VoxformatVXLNormalType = "voxformat_vxlnormaltype";
constexpr const char *VoxformatQBTPaletteMode = "voxformat_qbtpalettemode";
constexpr const char *VoxformatQBTMergeCompounds = "voxformat_qbtmergecompounds";
constexpr const char *VoxformatVOXCreateLayers = "voxformat_voxcreatelayers";
constexpr const char *VoxformatVOXCreateGroups = "voxformat_voxcreategroups";
constexpr const char *VoxformatQBSaveLeftHanded = "voxformat_qbsavelefthanded";

}
