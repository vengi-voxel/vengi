vec3 emitRadiance(vec3 albedo, GpuMaterial mat) {
	float emission = max(mat.emission, 0.0);
	float flux = clamp(mat.flux, 0.0, 8.0);
	float scale = emission * exp2(flux);
	scale = mix(scale, emission, clamp(mat.ldr, 0.0, 1.0));
	return albedo * scale;
}

float shadowRay(vec3 rayOrigin, vec3 rayDir, float maxDist) {
	rayDir = avoidZero(rayDir);
	float visibility = 1.0;
	float walked = 0.0;
	for (int i = 0; i < 8; i++) {
		float remaining = maxDist - walked;
		if (remaining <= 1e-3) {
			return visibility;
		}
		float hitDist;
		ivec3 cell;
		uint index;
		if (!traceVoxels(rayOrigin, rayDir, remaining, false, hitDist, cell, index)) {
			return visibility;
		}
		walked += hitDist;
		if (walked >= maxDist - 0.51) {
			return visibility;
		}
		if (!indexTransmits(index)) {
			return 0.0;
		}
		vec4 materialRow = fetchMaterialRow(index, 1);
		float trans = max(materialRow.y, 0.25);
		visibility *= trans * exp(-materialRow.w * 2.0);
		if (visibility < 0.02) {
			return 0.0;
		}
		rayOrigin = rayOrigin + rayDir * (hitDist + 1e-3);
		walked += 1e-3;
	}
	return visibility;
}

float sunShadow(vec3 rayOrigin, vec3 rayDir) {
	return shadowRay(rayOrigin, rayDir, 1e30);
}

// Surface sampling and direct-light MIS must use the same diffuse/GGX mixture PDF.
// Reference: https://pbr-book.org/4ed/Light_Transport_I_Surface_Reflection/A_Better_Path_Tracer
float surfaceSpecularProbability(vec3 viewDir, vec3 normal, vec3 albedo, float metallic, float specularStrength) {
	if (specularStrength <= 0.0 && metallic <= 0.0) {
		return 0.0;
	}
	vec3 fresnel = fresnelSchlick(max(dot(viewDir, normal), 0.0), mix(vec3(0.04), albedo, metallic));
	return clamp(mix(max(fresnel.r, max(fresnel.g, fresnel.b)) * specularStrength, 1.0, metallic), 0.0, 1.0);
}

float surfacePdf(vec3 viewDir, vec3 lightDir, vec3 normal, float roughness, float specularProbability) {
	float cosine = dot(normal, lightDir);
	if (cosine <= 0.0) {
		return 0.0;
	}
	if (specularProbability <= 0.0) {
		return cosine / PI;
	}
	return mix(cosine / PI, pdfGGXVNDF(viewDir, lightDir, normal, roughness), specularProbability);
}

vec3 surfaceBrdf(vec3 viewDir, vec3 lightDir, vec3 normal, vec3 albedo, float roughness, float metallic, float specularStrength) {
	vec3 diffuse = albedo * (1.0 - metallic) / PI;
	if (specularStrength <= 0.0) {
		return diffuse;
	}
	return diffuse + (evalBrdf(viewDir, lightDir, normal, albedo, roughness, metallic) - diffuse) * specularStrength;
}

// Next-event estimation with balance-heuristic multiple importance sampling (MIS).
// https://pbr-book.org/4ed/Monte_Carlo_Integration/Improving_Efficiency
void addEnvironmentMis(inout vec3 radiance, vec3 pathWeight, vec3 scatterOrigin, vec3 normal, vec3 viewDir, vec3 albedo,
			  float roughness, float metallic, float specularStrength, float specularProbability, bool sampleBsdf) {
	if (u_skySamplingProbability <= 0.0 && all(equal(u_suncolor, vec3(0.0)))) {
		return;
	}
	vec3 lightDir = sampleEnvironment();
	float cosine = dot(normal, lightDir);
	if (cosine <= 0.0) {
		return;
	}
	float lightPdf = environmentPdf(lightDir);
	float bsdfPdf = sampleBsdf ? surfacePdf(viewDir, lightDir, normal, roughness, specularProbability) : 0.0;
	vec3 incoming = u_skySamplingProbability > 0.0 ? skyRadiance(lightDir, true) :
		(dot(lightDir, u_sundir) >= u_suncosangle ? u_suncolor / sunSolidAngle() : vec3(0.0));
	vec3 brdf = surfaceBrdf(viewDir, lightDir, normal, albedo, roughness, metallic, specularStrength);
	// Balance weight divided by the light PDF simplifies to 1 / (lightPdf + bsdfPdf).
	// Evaluate the contribution before traversal to shorten the lifetime of surface temporaries.
	vec3 contribution = pathWeight * brdf * incoming * cosine / max(lightPdf + bsdfPdf, 1e-12);
	if (any(greaterThan(contribution, vec3(0.0)))) {
		radiance += contribution * shadowRay(scatterOrigin, lightDir, 1e30);
	}
}

// Luminance-weighted emitter selection using Walker's alias method.
// https://pbr-book.org/4ed/Sampling_Algorithms/The_Alias_Method
void addEmitNee(inout vec3 radiance, vec3 pathWeight, vec3 scatterOrigin, vec3 normal, vec3 viewDir, vec3 albedo,
			   float roughness, float metallic, float specularStrength, ivec3 skipCell, bool skip) {
	if (u_lightcount <= 0 || u_inverseEmitterLuminance <= 0.0) {
		return;
	}
	float choice = nextRand() * float(u_lightcount);
	int lightIndex = min(int(choice), u_lightcount - 1);
	vec4 alias = texelFetch(u_lights, ivec2(0, lightIndex), 0);
	if (fract(choice) >= alias.w) {
		lightIndex = int(texelFetch(u_lights, ivec2(1, lightIndex), 0).w);
	}
	GpuEmitLight light = fetchEmitLight(lightIndex);
	float lightProbability = dot(vec3(light.radianceR, light.radianceG, light.radianceB), vec3(0.2126, 0.7152, 0.0722)) * u_inverseEmitterLuminance;
	ivec3 emitCell = gpuEmitLightCell(light);
	if (skip && emitCell == skipCell) {
		return;
	}
	vec3 lightCenter = vec3(emitCell) + vec3(0.5) + vec3(u_volumemin);
	vec3 lightP = lightCenter + (vec3(nextRand(), nextRand(), nextRand()) - 0.5) * 0.98;
	vec3 toLight = lightP - scatterOrigin;
	float dist2 = max(dot(toLight, toLight), 0.25);
	float dist = sqrt(dist2);
	vec3 lightDir = toLight / dist;
	float nDotL = dot(normal, lightDir);
	if (nDotL <= 0.0) {
		return;
	}
	vec3 brdf = surfaceBrdf(viewDir, lightDir, normal, albedo, roughness, metallic, specularStrength);
	float geom = min(nDotL / dist2, 4.0);
	vec3 contribution = pathWeight * brdf * vec3(light.radianceR, light.radianceG, light.radianceB) *
				geom / max(lightProbability, 1e-12);
	if (any(greaterThan(contribution, vec3(0.0)))) {
		radiance += contribution * shadowRay(scatterOrigin, lightDir, max(dist - 0.51, 1e-3));
	}
}
