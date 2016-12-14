#include "_common.frag"

#if cl_shadowmap == 1

uniform sampler2DArrayShadow u_shadowmap;
$in vec3 v_lightspacepos;
uniform vec2 u_depthsize;
uniform vec4 u_distances;
uniform mat4 u_cascades[4];

/**
 * perform percentage-closer shadow map lookup
 * http://codeflow.org/entries/2013/feb/15/soft-shadow-mapping
 */
float sampleShadowPCF(int cascade, vec2 uv, float compare) {
	float result = 0.0;
	for (int x = -2; x <= 2; x++) {
		for (int y = -2; y <= 2; y++) {
			vec2 off = vec2(x, y) / u_depthsize;
			result += texture(u_shadowmap, vec4(uv + off, cascade, compare));
		}
	}
	return result / 25.0;
}

float calculateShadow(mat4 viewprojection) {
#if 1
	float viewz = gl_FragCoord.z;
#else
	float viewz = (viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif
#if 0
	int cascade = int(dot(vec4(greaterThan(vec4(viewz), u_distances)), vec4(1)));
#else
	int cascade = 3;
	if (viewz < u_distances[0])
		cascade = 0;
	else if (viewz < u_distances[1])
		cascade = 1;
	else if (viewz < u_distances[2])
		cascade = 2;
#endif
	vec4 lightp = u_cascades[cascade] * vec4(v_lightspacepos, 1.0);
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow */
	vec3 lightpt = (lightp.xyz / lightp.w) * 0.5 + 0.5;
	return sampleShadowPCF(cascade, lightpt.xy, lightpt.z);
}

#else // cl_shadowmap == 1

float calculateShadow(mat4 viewprojection) {
	return 1.0;
}

#endif // cl_shadowmap == 1
