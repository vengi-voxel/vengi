/**
 * @file
 */

#include "PathTracerConfig.h"
#include "PathTracerState.h"
#include "app/I18N.h"
#include "core/ConfigVar.h"
#include "core/String.h"
#include "core/Var.h"
#include "voxelrender/RenderUtil.h"

namespace voxelpathtracer {

namespace {

core::String formatVec3(const glm::vec3 &v) {
	return core::String::format("%f %f %f", v.x, v.y, v.z);
}

glm::vec3 readVec3(const char *name, const glm::vec3 &fallback) {
	const core::VarPtr &var = core::getVar(name);
	if (!var) {
		return fallback;
	}
	glm::vec3 v = fallback;
	var->vec3Val(&v[0]);
	return v;
}

void writeVec3(const char *name, const glm::vec3 &v) {
	const core::VarPtr &var = core::getVar(name);
	if (var) {
		var->setVal(formatVec3(v));
	}
}

} // namespace

bool PathTracerConfig::init() {
	const PathTracerParams defaults;

	core::registerVar(core::VarDef(cfg::PathTracerSamples, defaults.samples, 1, 16384, N_("Samples"),
								   N_("Per-pixel pathtracer samples"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerBounces, defaults.bounces, 1, 64, N_("Bounces"),
								   N_("Maximum light bounces for the pathtracer"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSamplesThisPass, defaults.samplesThisPass, 1, 64,
								   N_("Samples per pass"), N_("Pathtracer samples computed per frame"),
								   core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerResolution, defaults.resolution, 1, 8192, N_("Resolution"),
								   N_("Square pathtracer output size in pixels"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerExposure, defaults.exposure, -5.0f, 5.0f, N_("Exposure"),
								   N_("Pathtracer exposure compensation in stops"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerToneMapping, defaults.toneMapping, 0,
								   (int)voxelrender::ToneMappingMax - 1, N_("Tone mapping"),
								   N_("Pathtracer tone mapping operator"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerContrast, defaults.contrast, 0.5f, 1.5f, N_("Contrast"),
								   N_("Pathtracer contrast"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSaturation, defaults.saturation, 0.0f, 2.0f, N_("Saturation"),
								   N_("Pathtracer saturation"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerVignette, defaults.vignette, 0.0f, 1.0f, N_("Vignette"),
								   N_("Pathtracer vignette strength"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerAperture, defaults.aperture, 0.0f, 0.5f, N_("Aperture"),
								   N_("Pathtracer lens aperture for depth of field"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFocusDistance, defaults.focusDistance, 1.0f, 256.0f,
								   N_("Focus distance"), N_("Pathtracer focus distance for depth of field"),
								   core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSunElevation, defaults.sunElevation, -90.0f, 90.0f,
								   N_("Sun elevation"), N_("Pathtracer sun elevation angle in degrees"),
								   core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSunAzimuth, defaults.sunAzimuth, 0.0f, 360.0f, N_("Sun azimuth"),
								   N_("Pathtracer sun azimuth angle in degrees"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSunIntensity, defaults.sunIntensity, 0.0f, 10.0f,
								   N_("Sun intensity"), N_("Pathtracer sun intensity"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSunSoftness, defaults.sunSoftness, 0.05f, 20.0f,
								   N_("Sun softness"), N_("Pathtracer sun disk size in degrees"),
								   core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSunColor, formatVec3(defaults.sunColor), N_("Sun color"),
								   N_("Pathtracer sun color (RGB)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSkyTop, formatVec3(defaults.skyTop), N_("Sky top"),
								   N_("Pathtracer sky top color (RGB)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSkyHorizon, formatVec3(defaults.skyHorizon), N_("Sky horizon"),
								   N_("Pathtracer sky horizon color (RGB)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerSkyIntensity, defaults.skyIntensity, 0.0f, 4.0f,
								   N_("Sky intensity"), N_("Pathtracer sky intensity"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFloor, defaults.floorEnabled, N_("Floor"),
								   N_("Enable the pathtracer ground plane"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFloorColor, formatVec3(defaults.floorColor), N_("Floor color"),
								   N_("Pathtracer ground plane color (RGB)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFloorY, defaults.floorY, N_("Floor Y"),
								   N_("Pathtracer ground plane height"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerDepthOfField, defaults.depthOfField, N_("Depth of field"),
								   N_("Enable pathtracer depth of field"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerBackground, defaults.background, PathTracerBackgroundSky,
								   PathTracerBackgroundTransparent, N_("Background"),
								   N_("0 = sky, 1 = solid, 2 = transparent"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerBackgroundColor, formatVec3(defaults.backgroundColor),
								   N_("Background color"), N_("Pathtracer solid background color (RGB)"),
								   core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFogDensity, defaults.fogDensity, 0.0f, 0.05f, N_("Fog density"),
								   N_("Pathtracer fog density"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerFogColor, formatVec3(defaults.fogColor), N_("Fog color"),
								   N_("Pathtracer fog color (RGB)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerVoxelEdge, defaults.voxelEdge, 0.0f, 1.0f, N_("Voxel edge"),
								   N_("Fillet exposed voxel corners (0 = hard cube)"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerBloom, defaults.bloom, 0.0f, 2.0f, N_("Bloom"),
								   N_("Pathtracer bloom strength"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerAtmosphere, defaults.atmosphere, 0.0f, 0.05f, N_("Atmosphere"),
								   N_("Pathtracer world haze that brightens toward the sun"), core::CV_NOPERSIST));
	core::registerVar(core::VarDef(cfg::PathTracerRenderScale, defaults.renderScale, 0.25f, 1.0f, N_("Render scale"),
								   N_("Trace at a fraction of the output resolution, then upsample"),
								   core::CV_NOPERSIST));
	return true;
}

void PathTracerConfig::loadParams(PathTracerParams &params) {
	const PathTracerParams defaults;
	params.samples = core::getVar(cfg::PathTracerSamples)->intVal();
	params.bounces = core::getVar(cfg::PathTracerBounces)->intVal();
	params.samplesThisPass = core::getVar(cfg::PathTracerSamplesThisPass)->intVal();
	params.resolution = core::getVar(cfg::PathTracerResolution)->intVal();
	params.exposure = core::getVar(cfg::PathTracerExposure)->floatVal();
	params.toneMapping = core::getVar(cfg::PathTracerToneMapping)->intVal();
	params.contrast = core::getVar(cfg::PathTracerContrast)->floatVal();
	params.saturation = core::getVar(cfg::PathTracerSaturation)->floatVal();
	params.vignette = core::getVar(cfg::PathTracerVignette)->floatVal();
	params.aperture = core::getVar(cfg::PathTracerAperture)->floatVal();
	params.focusDistance = core::getVar(cfg::PathTracerFocusDistance)->floatVal();
	params.sunElevation = core::getVar(cfg::PathTracerSunElevation)->floatVal();
	params.sunAzimuth = core::getVar(cfg::PathTracerSunAzimuth)->floatVal();
	params.sunIntensity = core::getVar(cfg::PathTracerSunIntensity)->floatVal();
	params.sunSoftness = core::getVar(cfg::PathTracerSunSoftness)->floatVal();
	params.sunColor = readVec3(cfg::PathTracerSunColor, defaults.sunColor);
	params.skyTop = readVec3(cfg::PathTracerSkyTop, defaults.skyTop);
	params.skyHorizon = readVec3(cfg::PathTracerSkyHorizon, defaults.skyHorizon);
	params.skyIntensity = core::getVar(cfg::PathTracerSkyIntensity)->floatVal();
	params.floorEnabled = core::getVar(cfg::PathTracerFloor)->boolVal();
	params.floorColor = readVec3(cfg::PathTracerFloorColor, defaults.floorColor);
	params.floorY = core::getVar(cfg::PathTracerFloorY)->floatVal();
	params.depthOfField = core::getVar(cfg::PathTracerDepthOfField)->boolVal();
	params.background = core::getVar(cfg::PathTracerBackground)->intVal();
	params.backgroundColor = readVec3(cfg::PathTracerBackgroundColor, defaults.backgroundColor);
	params.fogDensity = core::getVar(cfg::PathTracerFogDensity)->floatVal();
	params.fogColor = readVec3(cfg::PathTracerFogColor, defaults.fogColor);
	params.voxelEdge = core::getVar(cfg::PathTracerVoxelEdge)->floatVal();
	params.bloom = core::getVar(cfg::PathTracerBloom)->floatVal();
	params.atmosphere = core::getVar(cfg::PathTracerAtmosphere)->floatVal();
	params.renderScale = core::getVar(cfg::PathTracerRenderScale)->floatVal();
}

void PathTracerConfig::saveParams(const PathTracerParams &params) {
	core::getVar(cfg::PathTracerSamples)->setVal(params.samples);
	core::getVar(cfg::PathTracerBounces)->setVal(params.bounces);
	core::getVar(cfg::PathTracerSamplesThisPass)->setVal(params.samplesThisPass);
	core::getVar(cfg::PathTracerResolution)->setVal(params.resolution);
	core::getVar(cfg::PathTracerExposure)->setVal(params.exposure);
	core::getVar(cfg::PathTracerToneMapping)->setVal(params.toneMapping);
	core::getVar(cfg::PathTracerContrast)->setVal(params.contrast);
	core::getVar(cfg::PathTracerSaturation)->setVal(params.saturation);
	core::getVar(cfg::PathTracerVignette)->setVal(params.vignette);
	core::getVar(cfg::PathTracerAperture)->setVal(params.aperture);
	core::getVar(cfg::PathTracerFocusDistance)->setVal(params.focusDistance);
	core::getVar(cfg::PathTracerSunElevation)->setVal(params.sunElevation);
	core::getVar(cfg::PathTracerSunAzimuth)->setVal(params.sunAzimuth);
	core::getVar(cfg::PathTracerSunIntensity)->setVal(params.sunIntensity);
	core::getVar(cfg::PathTracerSunSoftness)->setVal(params.sunSoftness);
	writeVec3(cfg::PathTracerSunColor, params.sunColor);
	writeVec3(cfg::PathTracerSkyTop, params.skyTop);
	writeVec3(cfg::PathTracerSkyHorizon, params.skyHorizon);
	core::getVar(cfg::PathTracerSkyIntensity)->setVal(params.skyIntensity);
	core::getVar(cfg::PathTracerFloor)->setVal(params.floorEnabled);
	writeVec3(cfg::PathTracerFloorColor, params.floorColor);
	core::getVar(cfg::PathTracerFloorY)->setVal(params.floorY);
	core::getVar(cfg::PathTracerDepthOfField)->setVal(params.depthOfField);
	core::getVar(cfg::PathTracerBackground)->setVal(params.background);
	writeVec3(cfg::PathTracerBackgroundColor, params.backgroundColor);
	core::getVar(cfg::PathTracerFogDensity)->setVal(params.fogDensity);
	writeVec3(cfg::PathTracerFogColor, params.fogColor);
	core::getVar(cfg::PathTracerVoxelEdge)->setVal(params.voxelEdge);
	core::getVar(cfg::PathTracerBloom)->setVal(params.bloom);
	core::getVar(cfg::PathTracerAtmosphere)->setVal(params.atmosphere);
	core::getVar(cfg::PathTracerRenderScale)->setVal(params.renderScale);
}

} // namespace voxelpathtracer
