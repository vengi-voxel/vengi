#if cl_shadowmap == 1

#ifndef CUSTOM_SHADOW_TEXCOORD
vec2 calculateShadowTexcoord(in vec2 uv) {
	return uv;
}
#endif

uniform sampler2DArrayShadow u_shadowmap;
$in vec3 v_lightspacepos;
$in float v_viewz;
uniform vec2 u_depthsize;
uniform vec4 u_distances;
$constant MaxDepthBufferUniformName u_cascades
uniform mat4 u_cascades[4];

/**
 * perform percentage-closer shadow map lookup
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadowPCF(in int cascade, in vec2 uv, in float compare) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / u_depthsize;
			result += texture(u_shadowmap, vec4(uv + off, cascade, compare));
		}
	}
	return result / 25.0;
}

float calculateShadow(in int cascade, in mat4 viewprojection) {
	vec4 lightp = u_cascades[cascade] * vec4(v_lightspacepos, 1.0);
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow */
	vec3 lightpt = (lightp.xyz / lightp.w) * 0.5 + 0.5;
	return sampleShadowPCF(cascade, calculateShadowTexcoord(lightpt.xy), lightpt.z);
}

int calculateCascade() {
	int cascade = int(dot(vec4(greaterThan(vec4(v_viewz), u_distances)), vec4(1)));
	return cascade;
}

vec3 shadow(in mat4 viewprojection, vec3 color, in vec3 diffuse, in vec3 ambient) {
	int cascade = calculateCascade();
	float shadow = calculateShadow(cascade, viewprojection);
#if cl_debug_cascade
	if (cascade == 0) {
		color.r = 0.0;
		color.g = 1.0;
		color.b = 0.0;
	} else if (cascade == 1) {
		color.r = 0.0;
		color.g = 1.0;
		color.b = 1.0;
	} else if (cascade == 2) {
		color.r = 0.0;
		color.g = 0.0;
		color.b = 1.0;
	} else if (cascade == 3) {
		color.r = 0.0;
		color.g = 0.5;
		color.b = 0.5;
	} else {
		color.r = 1.0;
	}
#endif // cl_debug_cascade
#if cl_debug_shadow == 1
	// shadow only rendering
	return vec3(shadow);
#else // cl_debug_shadow
	vec3 lightvalue = ambient + (diffuse * shadow);
	return clamp(color * lightvalue, 0.0f, 1.0f);
#endif // cl_debug_shadow
}

#else // cl_shadowmap == 1

vec3 shadow(in mat4 viewprojection, in vec3 color, in vec3 diffuse, in vec3 ambient) {
	return clamp(color * (ambient + diffuse), 0.0f, 1.0f);
}

#endif // cl_shadowmap == 1
