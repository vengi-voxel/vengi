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
$constant MaxDepthBuffers 4
uniform mat4 u_cascades[4];

/**
 * perform percentage-closer shadow map lookup
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadowPCF(in float bias, in int cascade, in vec2 uv, in float compare) {
	float result = 0.0;
	const int r = 2;
	for (int x = -r; x <= r; x++) {
		for (int y = -r; y <= r; y++) {
			vec2 off = vec2(x, y) / u_depthsize;
			result += texture(u_shadowmap, vec4(uv + off, cascade, compare));
		}
	}
	const float size = 2.0 * float(r) + 1.0;
	const float elements = size * size;
	return result / elements;
}

float calculateShadow(in float bias, in int cascade, in mat4 viewprojection) {
	vec4 lightp = u_cascades[cascade] * vec4(v_lightspacepos, 1.0);
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow
	 * Also bring the ndc into the range [0-1] because the depth map
	 * is in that range */
	vec3 lightpt = (lightp.xyz / lightp.w) * 0.5 + 0.5;
	return sampleShadowPCF(bias, cascade, calculateShadowTexcoord(lightpt.xy), lightpt.z);
}

int calculateCascade() {
	int cascade = int(dot(vec4(greaterThan(vec4(v_viewz), u_distances)), vec4(1)));
	return cascade;
}

vec3 shadow(in float bias, in mat4 viewprojection, vec3 color, in vec3 diffuse, in vec3 ambient) {
	int cascade = calculateCascade();
	float shadow = calculateShadow(bias, cascade, viewprojection);
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

vec3 shadow(in float bias, in mat4 viewprojection, in vec3 color, in vec3 diffuse, in vec3 ambient) {
	return clamp(color * (ambient + diffuse), 0.0f, 1.0f);
}

#endif // cl_shadowmap == 1
