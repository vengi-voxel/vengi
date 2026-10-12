$in vec2 v_texcoord;

layout(binding = 0) uniform usampler3D u_bricks;
layout(binding = 1) uniform usampler3D u_indirection;
layout(binding = 2) uniform sampler2D u_palette;
layout(binding = 3) uniform sampler2D u_materials;
layout(binding = 4) uniform sampler2D u_accumulationTexture;
layout(binding = 5) uniform sampler2D u_lights;

layout(std140, binding = 0) uniform u_params {
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
};

layout(location = 0) $out vec4 o_fragcolor;

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

struct SceneHit {
	bool voxel;
	bool floor;
	float dist;
	float voxelDist;
	float floorDist;
	ivec3 cell;
	uint index;
	vec3 pos;
};

struct MediumState {
	bool inside;
	float ior;
	float roughness;
	vec3 albedo;
	float attenuation;
	float density;
	float phase;
	int mediaKind;
	float emission;
};

MediumState mediumNone() {
	MediumState medium;
	medium.inside = false;
	medium.ior = 1.5;
	medium.roughness = 0.0;
	medium.albedo = vec3(1.0);
	medium.attenuation = 0.0;
	medium.density = 0.0;
	medium.phase = 0.5;
	medium.mediaKind = 0;
	medium.emission = 0.0;
	return medium;
}

SceneHit intersectScene(vec3 rayOrigin, vec3 rayDir, bool inside) {
	SceneHit hit;
	bool intersectsFloor = !inside && u_floorenabled == 1 && rayDir.y < 0.0 && rayOrigin.y > u_floory;
	hit.floorDist = 1e30;
	if (intersectsFloor) {
		hit.floorDist = (u_floory - rayOrigin.y) / rayDir.y;
	}
	// The floor occludes all voxels beyond its intersection distance.
	hit.voxel = traceVoxels(rayOrigin, rayDir, hit.floorDist, inside, hit.voxelDist, hit.cell, hit.index);
	hit.floor = intersectsFloor && (!hit.voxel || hit.floorDist < hit.voxelDist);
	hit.dist = hit.floor ? hit.floorDist : hit.voxelDist;
	hit.pos = rayOrigin + rayDir * hit.dist;
	return hit;
}

bool scatterInMedium(inout vec3 rayOrigin, inout vec3 rayDir, inout vec3 pathWeight, SceneHit hit, MediumState medium) {
	if (!medium.inside || medium.density <= 0.0 || (medium.mediaKind != 1 && medium.mediaKind != 3)) {
		return false;
	}
	float sigma = medium.density * 2.0;
	float freePath = -log(max(nextRand(), 1e-5)) / max(sigma, 1e-4);
	float maxFreePath = hit.voxel ? hit.voxelDist : 64.0;
	if (hit.floor) {
		maxFreePath = min(maxFreePath, hit.floorDist);
	}
	if (freePath >= maxFreePath) {
		return false;
	}
	vec3 pos = rayOrigin + rayDir * freePath;
	pathWeight *= exp(-medium.attenuation * freePath * 4.0 * (vec3(1.0) - medium.albedo));
	if (medium.mediaKind == 3) {
		pathWeight *= mix(vec3(1.0), medium.albedo, 0.6);
	}
	rayDir = samplePhaseFunction(rayDir, medium.phase);
	rayOrigin = pos;
	return true;
}

vec3 evaluateMiss(vec3 rayDir, int bounce, bool specularBounce, float scatterPdf, inout float alpha) {
	if (bounce == 0) {
		if (u_bgmode == 2) {
			alpha = 0.0;
			return vec3(0.0);
		}
		if (u_bgmode == 1) {
			return u_bgcolor;
		}
		return skyRadiance(rayDir, false);
	}
	vec3 env = skyRadiance(rayDir, false);
	float lightPdf = environmentPdf(rayDir);
	float misWeight = specularBounce || scatterPdf <= 0.0 ? 1.0 : scatterPdf / max(scatterPdf + lightPdf, 1e-12);
	if (u_skySamplingProbability > 0.0) {
		env *= misWeight;
	}
	if (dot(rayDir, u_sundir) >= u_suncosangle) {
		env += (u_suncolor / sunSolidAngle()) * misWeight;
	}
	return env;
}

void attenuateInMedium(float dist, MediumState medium, inout vec3 radiance, inout vec3 pathWeight) {
	pathWeight *= exp(-medium.attenuation * dist * 4.0 * (vec3(1.0) - medium.albedo));
	if (medium.mediaKind == 2) {
		radiance += pathWeight * medium.albedo * medium.emission * min(dist, 8.0);
	}
}

void applyAtmosphere(vec3 rayDir, float dist, inout vec3 radiance, inout vec3 pathWeight) {
	float sigmaFog = u_fogdensity + u_atmosphere;
	if (sigmaFog <= 0.0) {
		return;
	}
	// Beer-Lambert transmittance for homogeneous fog.
	// https://pbr-book.org/4ed/Volume_Scattering/Transmittance
	float transmittance = exp(-sigmaFog * dist);
	vec3 inscatter = u_fogcolor * (1.0 - transmittance);
	if (u_atmosphere > 0.0) {
		float phase = henyeyGreensteinEval(dot(rayDir, u_sundir), 0.4);
		float atmosphereWeight = u_atmosphere / sigmaFog;
		inscatter += u_suncolor * phase * (1.0 - transmittance) * atmosphereWeight;
		inscatter += skyRadiance(rayDir, false) * (1.0 - transmittance) * 0.15 * atmosphereWeight;
	} else {
		inscatter *= 0.6;
	}
	radiance += pathWeight * inscatter;
	pathWeight *= transmittance;
}

vec3 voxelHitNormal(vec3 rayOrigin, vec3 rayDir, vec3 pos, ivec3 cell) {
	vec3 normal = faceNormal(rayOrigin, rayDir, cell);
	if (u_edgeradius > 1e-4) {
		vec3 rounded = roundedVoxelNormal(pos, cell);
		if (dot(rounded, normal) > 0.0) {
			normal = rounded;
		}
	}
	return normal;
}

void loadSurface(uint index, out vec3 albedo, out GpuMaterial mat) {
	vec4 palColor = fetchPalette(index);
	albedo = srgbToLinear(palColor.rgb);
	mat = fetchMaterial(index);
	if (palColor.a < 0.999) {
		if (gpuMaterialType(mat) != MaterialBlend) {
			mat.type = float(MaterialGlass);
		}
		mat.transparency = max(mat.transparency, 1.0 - palColor.a);
	} else if (mat.transparency > 0.0 && gpuMaterialType(mat) != MaterialBlend && gpuMaterialType(mat) != MaterialMedia &&
			   gpuMaterialType(mat) != MaterialGlass) {
		mat.type = float(MaterialGlass);
	}
}

// Unpolarized dielectric Fresnel equations, including total internal reflection.
// https://pbr-book.org/4ed/Reflection_Models/Dielectric_BSDF
float interfaceReflectance(float incidentCosine, float incidentOverTransmittedIor) {
	float transmittedSineSquared = incidentOverTransmittedIor * incidentOverTransmittedIor *
		(1.0 - incidentCosine * incidentCosine);
	if (transmittedSineSquared >= 1.0) {
		return 1.0;
	}
	float transmittedCosine = sqrt(1.0 - transmittedSineSquared);
	float perpendicular = (incidentOverTransmittedIor * incidentCosine - transmittedCosine) /
		max(incidentOverTransmittedIor * incidentCosine + transmittedCosine, 1e-7);
	float parallel = (incidentCosine - incidentOverTransmittedIor * transmittedCosine) /
		max(incidentCosine + incidentOverTransmittedIor * transmittedCosine, 1e-7);
	return 0.5 * (perpendicular * perpendicular + parallel * parallel);
}

// GGX visible-normal dielectric sampling, with f * abs(cos(theta)) / pdf
// simplified to G / G1 after canceling D, Fresnel and the direction Jacobian.
// Radiance transmission also carries (incident IOR / transmitted IOR)^2.
// https://pbr-book.org/4ed/Reflection_Models/Rough_Dielectric_BSDF
bool sampleDielectricInterface(vec3 incoming, vec3 normal, float eta, float roughness,
							   inout vec3 pathWeight, inout float refractionScale, out vec3 outgoing, out bool transmitted) {
	vec3 microNormal = normal;
	float cosView = max(dot(-incoming, normal), 1e-6);
	float alpha = max(roughness * roughness, 0.001);
	if (roughness > 0.001) {
		vec3 tangent, bitangent;
		buildONB(normal, tangent, bitangent);
		vec3 localView = vec3(dot(-incoming, tangent), dot(-incoming, bitangent), cosView);
		vec3 localNormal = sampleGGXVNDF(localView, roughness);
		microNormal = tangent * localNormal.x + bitangent * localNormal.y + normal * localNormal.z;
	}
	float cosMicro = clamp(dot(-incoming, microNormal), 0.0, 1.0);
	float reflectance = interfaceReflectance(cosMicro, eta);
	transmitted = nextRand() >= reflectance;
	outgoing = transmitted ? refract(incoming, microNormal, eta) : reflect(incoming, microNormal);
	float cosOutgoing = dot(outgoing, normal);
	// Invalid microfacet directions represent zero contribution; do not resample them.
	if ((transmitted && cosOutgoing >= 0.0) || (!transmitted && cosOutgoing <= 0.0) ||
		dot(outgoing, outgoing) < 1e-12) {
		return false;
	}
	if (roughness > 0.001) {
		float lambdaView = lambdaSmith(cosView, alpha);
		float lambdaOutgoing = lambdaSmith(abs(cosOutgoing), alpha);
		pathWeight *= (1.0 + lambdaView) / (1.0 + lambdaView + lambdaOutgoing);
	}
	if (transmitted) {
		pathWeight *= eta * eta;
		// Remove temporary refractive radiance scaling from termination probabilities.
		// https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
		refractionScale /= eta * eta;
	}
	return true;
}

bool sampleDielectricExit(inout vec3 rayOrigin, inout vec3 rayDir, inout MediumState medium,
						  inout vec3 pathWeight, inout float refractionScale, vec3 pos, vec3 normal) {
	vec3 outgoing;
	bool transmitted;
	if (!sampleDielectricInterface(rayDir, normal, medium.ior, medium.roughness, pathWeight, refractionScale, outgoing, transmitted)) {
		return false;
	}
	rayDir = outgoing;
	rayOrigin = pos + normal * (transmitted ? -1e-3 : 1e-3);
	if (transmitted) {
		medium.inside = false;
	}
	return true;
}

bool sampleDielectricEnter(inout vec3 rayOrigin, inout vec3 rayDir, inout MediumState medium,
						   inout vec3 pathWeight, inout float refractionScale, vec3 pos, vec3 normal,
						   vec3 albedo, GpuMaterial mat, int mediaKind, float roughness) {
	float ior = max(mat.ior, 1.0001);
	vec3 outgoing;
	bool transmitted;
	if (!sampleDielectricInterface(rayDir, normal, 1.0 / ior, roughness, pathWeight, refractionScale, outgoing, transmitted)) {
		return false;
	}
	rayDir = outgoing;
	rayOrigin = pos + normal * (transmitted ? -1e-3 : 1e-3);
	if (transmitted) {
		medium.inside = true;
		medium.ior = ior;
		medium.roughness = roughness;
		medium.albedo = albedo;
		medium.attenuation = max(mat.attenuation, 0.05);
		medium.density = mat.density;
		medium.phase = mat.phase;
		medium.mediaKind = mediaKind;
		medium.emission = mat.emission * exp2(clamp(mat.flux, 0.0, 8.0));
	}
	return true;
}

int resolveBsdfType(inout GpuMaterial mat, inout int mediaKind) {
	int bsdfType = gpuMaterialType(mat);
	if (bsdfType == MaterialBlend) {
		if (mat.transparency > 0.0 && nextRand() < mat.transparency) {
			bsdfType = MaterialGlass;
		} else if (mat.metallic > 0.0 && nextRand() < mat.metallic) {
			bsdfType = MaterialMetal;
		} else {
			bsdfType = MaterialDiffuse;
		}
	}
	if (bsdfType == MaterialMedia) {
		bsdfType = MaterialGlass;
		if (mediaKind == 0) {
			mediaKind = 1;
		}
		mat.ior = max(mat.ior, 1.01);
		mat.transparency = max(mat.transparency, 1.0);
	}
	return bsdfType;
}

// Sample either lobe, then evaluate the complete BRDF over its mixture PDF.
// Reference: https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
bool scatterFromSurface(inout vec3 rayOrigin, inout vec3 rayDir, inout vec3 pathWeight,
					   out float scatterPdf, vec3 scatterOrigin, vec3 normal, vec3 viewDir, vec3 albedo,
					   float roughness, float metallic, float specularStrength, float specularProbability) {
	if (nextRand() < specularProbability) {
		vec3 tangent, bitangent;
		buildONB(normal, tangent, bitangent);
		vec3 localView = vec3(dot(viewDir, tangent), dot(viewDir, bitangent), dot(viewDir, normal));
		vec3 microNormalLocal = sampleGGXVNDF(localView, roughness);
		vec3 microNormal = tangent * microNormalLocal.x + bitangent * microNormalLocal.y + normal * microNormalLocal.z;
		rayDir = reflect(-viewDir, microNormal);
	} else {
		rayDir = cosineSampleHemi(normal);
	}
	scatterPdf = surfacePdf(viewDir, rayDir, normal, roughness, specularProbability);
	if (scatterPdf <= 0.0) {
		return false;
	}
	pathWeight *= surfaceBrdf(viewDir, rayDir, normal, albedo, roughness, metallic, specularStrength) *
		max(dot(normal, rayDir), 0.0) / scatterPdf;
	rayOrigin = scatterOrigin;
	return true;
}

bool continuePathProbabilistically(int bounce, inout vec3 pathWeight, float refractionScale) {
	if (bounce < 3) {
		return true;
	}
	// Probabilistic path termination with unbiased path-weight compensation.
	// Reference: https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
	float keepWeight = min(max(pathWeight.r, max(pathWeight.g, pathWeight.b)) * refractionScale, 1.0);
	if (keepWeight >= 1.0) {
		return true;
	}
	if (keepWeight <= 0.0 || nextRand() >= keepWeight) {
		return false;
	}
	pathWeight /= keepWeight;
	return true;
}

vec4 runTrace(vec3 rayOrigin, vec3 rayDir) {
	vec3 radiance = vec3(0.0);
	vec3 pathWeight = vec3(1.0);
	float alpha = 1.0;
	float refractionScale = 1.0;
	MediumState medium = mediumNone();
	bool specularBounce = false;
	float scatterPdf = 0.0;

	int maxBounce = min(MaxBounces, u_bounces);
	for (int bounce = 0; bounce <= maxBounce; bounce++) {
		rayDir = avoidZero(rayDir);
		SceneHit hit = intersectScene(rayOrigin, rayDir, medium.inside);
		if (scatterInMedium(rayOrigin, rayDir, pathWeight, hit, medium)) {
			specularBounce = false;
			scatterPdf = 0.0;
			if (!continuePathProbabilistically(bounce, pathWeight, refractionScale)) {
				break;
			}
			continue;
		}

		if (!hit.floor && !hit.voxel) {
			medium.inside = false;
			radiance += pathWeight * evaluateMiss(rayDir, bounce, specularBounce, scatterPdf, alpha);
			break;
		}

		if (medium.inside) {
			attenuateInMedium(hit.dist, medium, radiance, pathWeight);
		} else {
			applyAtmosphere(rayDir, hit.dist, radiance, pathWeight);
		}

		vec3 normal;
		vec3 albedo;
		GpuMaterial mat;
		if (hit.floor) {
			normal = vec3(0.0, 1.0, 0.0);
			albedo = u_floorcolor;
			mat = gpuMaterialFloor();
		} else {
			normal = voxelHitNormal(rayOrigin, rayDir, hit.pos, hit.cell);
			if (medium.inside && (hit.index == 0u || indexTransmits(hit.index))) {
				if (!sampleDielectricExit(rayOrigin, rayDir, medium, pathWeight, refractionScale, hit.pos, normal)) {
					break;
				}
				specularBounce = true;
				scatterPdf = 0.0;
				if (!continuePathProbabilistically(bounce, pathWeight, refractionScale)) {
					break;
				}
				continue;
			}
			loadSurface(hit.index, albedo, mat);
			medium.inside = false;
		}

		int mediaKind = gpuMaterialMediaType(mat);
		int bsdfType = resolveBsdfType(mat, mediaKind);
		float roughness = max(mat.roughness, mat.sp);
		float metallic = mat.metallic;

		if (bsdfType == MaterialEmit || mat.emission > 0.0) {
			radiance += pathWeight * emitRadiance(albedo, mat);
		} else if (bsdfType == MaterialGlass) {
			if (!sampleDielectricEnter(rayOrigin, rayDir, medium, pathWeight, refractionScale, hit.pos, normal, albedo, mat, mediaKind, roughness)) {
				break;
			}
			specularBounce = true;
			scatterPdf = 0.0;
			if (!continuePathProbabilistically(bounce, pathWeight, refractionScale)) {
				break;
			}
			continue;
		}

		vec3 viewDir = -rayDir;
		vec3 scatterOrigin = hit.pos + normal * 1e-3;
		float specularStrength = bsdfType == MaterialMetal ? 1.0 : clamp(mat.specular, 0.0, 1.0);
		float specularProbability = surfaceSpecularProbability(viewDir, normal, albedo, metallic, specularStrength);
		addEnvironmentMis(radiance, pathWeight, scatterOrigin, normal, viewDir, albedo, roughness, metallic,
			specularStrength, specularProbability, bounce < maxBounce);
		addEmitNee(radiance, pathWeight, scatterOrigin, normal, viewDir, albedo, roughness, metallic, specularStrength, hit.cell, !hit.floor);
		if (bounce == maxBounce) {
			break;
		}
		if (!scatterFromSurface(rayOrigin, rayDir, pathWeight, scatterPdf, scatterOrigin, normal,
			viewDir, albedo, roughness, metallic, specularStrength, specularProbability)) {
			break;
		}
		specularBounce = false;
		if (!continuePathProbabilistically(bounce, pathWeight, refractionScale)) {
			break;
		}
	}

	return vec4(min(radiance, vec3(32.0)), alpha);
}

void main(void) {
	ivec2 pixel = ivec2(gl_FragCoord.xy);
	uint cameraPixelSeed = uint(pixel.x) ^ pcgHash(uint(pixel.y));

	int sampleCount = clamp(u_samplesPerPass, 1, MaxSamplesThisPass);
	vec4 runningAverage = vec4(0.0);
	if (u_frame > 0) {
		runningAverage = texelFetch(u_accumulationTexture, pixel, 0);
	}
	for (int sampleIndex = 0; sampleIndex < sampleCount; ++sampleIndex) {
		vec3 rayOrigin, rayDir;
		// The sample-only hash is shared by every pixel and prepared on the CPU.
		uint frameHash = uint(u_camerasamples[sampleIndex].z) | (uint(u_camerasamples[sampleIndex].w) << 16u);
		g_rng = pcgHash(uint(pixel.x) + pcgHash(uint(pixel.y) + frameHash));
		sampleCameraRay(cameraSobol(cameraPixelSeed, sampleIndex, 0u), cameraPixelSeed, sampleIndex, rayOrigin, rayDir);
		vec4 sampleColor = runTrace(rayOrigin, rayDir);
		if (any(isnan(sampleColor)) || any(isinf(sampleColor))) {
			sampleColor = vec4(0.0, 0.0, 0.0, 1.0);
		}
		float reciprocalCount = 1.0 / float(u_frame + sampleIndex + 1);
		runningAverage += (sampleColor - runningAverage) * reciprocalCount;
	}
	o_fragcolor = runningAverage;
}
