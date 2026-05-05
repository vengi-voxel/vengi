#extension GL_EXT_ray_query : require

in vec2 v_texcoord;

layout(binding = 0) uniform usampler3D u_bricks;
layout(binding = 1) uniform usampler3D u_indirection;
layout(binding = 2) uniform sampler2D u_palette;
layout(binding = 3) uniform sampler2D u_materials;
layout(binding = 4) uniform sampler2D u_accumulationTexture;
layout(binding = 5) uniform sampler2D u_lights;

layout(std140, binding = 6) uniform u_params {
	mat4 u_invviewproj;
	vec4 u_camerasamples[16];
	vec4 u_skybands[16];
	float u_skySamplingProbability;
	float u_inverseEmitterLuminance;
	vec3 u_camerapos;
	vec3 u_cameraright;
	vec3 u_cameraup;
	vec3 u_cameraforward;
	vec3 u_sundir;
	vec3 u_suncolor;
	vec3 u_skytop;
	vec3 u_skyhorizon;
	vec3 u_floorcolor;
	vec3 u_bgcolor;
	vec3 u_fogcolor;
	vec2 u_resolution;
	float u_suncosangle;
	float u_floory;
	float u_fogdensity;
	float u_aperture;
	float u_focusdistance;
	int u_volumeactive;
	int u_ortho;
	int u_frame;
	int u_samplesPerPass;
	int u_floorenabled;
	int u_bgmode;
	int u_bounces;
	float u_edgeradius;
	float u_atmosphere;
	int u_lightcount;
	ivec3 u_volumemin;
	ivec3 u_volumesize;
	int u_bricksperrow;
	int u_brickspercol;
};

layout(binding = 7) uniform sampler2D u_brickcoords;
layout(binding = 8) uniform accelerationStructureEXT u_tlas;

layout(location = 0) out vec4 o_fragcolor;

#define VENGIRAYQUERY 1

#include "_brdf.glsl"
#include "_tonemapping.glsl"

#include "_pathtracershared.glsl"
#include "_gpuscene.inc"
#include "_struct.glsl"
#include "_dda.glsl"
#include "_sky.glsl"
#include "_nee.glsl"
#include "_sobol.glsl"
#include "_camera.glsl"
#include "_pathtracermain.glsl"
