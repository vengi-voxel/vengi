/**
 * @file
 */

#pragma once

namespace cfg {

constexpr const char *ClientMouseRotationSpeed = "cl_cammouserotspeed";
constexpr const char *ClientMouseSpeed = "cl_cammousespeed";
constexpr const char *ClientEmail = "cl_email";
constexpr const char *ClientPassword = "cl_password";
constexpr const char *ClientAutoLogin = "cl_autologin";
// name of the player that is visible by other players
constexpr const char *ClientName = "cl_name";
constexpr const char *ClientVSync = "cl_vsync";
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

constexpr const char *ClientGamma = "cl_gamma";
constexpr const char *ClientDeferred = "cl_deferred";
constexpr const char *ClientShadowMap = "cl_shadowmap";
constexpr const char *ClientCameraSun = "cl_cameralookthroughsun";
constexpr const char *ClientCameraMaxTargetDistance = "cl_cameramaxtargetdistance";

constexpr const char *ClientDebugDeferred = "cl_debug_deferred";
constexpr const char *ClientDebugShadowMap = "cl_debug_shadowmap";
constexpr const char *ClientDebugGeometry = "cl_debug_geometry";
constexpr const char *ClientDebugShadow = "cl_debug_shadow";

constexpr const char *ServerAutoRegister = "sv_autoregister";
constexpr const char *ServerUserTimeout = "sv_usertimeout";
// the server side seed that is used to create the world
constexpr const char *ServerSeed = "sv_seed";
constexpr const char *ServerHost = "sv_host";
constexpr const char *ServerPort = "sv_port";
constexpr const char *ServerMaxClients = "sv_maxclients";

constexpr const char *ShapeToolExtractRadius = "sh_extractradius";

constexpr const char *CoreLogLevel = "core_loglevel";

// The size of the chunk that is extracted with each step
constexpr const char *VoxelMeshSize = "voxel_meshsize";

constexpr const char *DatabaseName = "db_name";
constexpr const char *DatabaseHost = "db_host";
constexpr const char *DatabasePassword = "db_pw";
constexpr const char *DatabaseUser = "db_user";
constexpr const char *DatabaseMinConnections = "db_minconnections";
constexpr const char *DatabaseMaxConnections = "db_maxconnections";

constexpr const char *AppHomePath = "app_homepath";
constexpr const char *AppBasePath = "app_basepath";

constexpr const char *HTTPBaseURL = "http_baseurl";

}
