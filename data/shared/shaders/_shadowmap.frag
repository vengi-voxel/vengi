#include "_shadowsampler.frag"

#if cl_shadowmap == 1

uniform sampler2D u_shadowmap;
$in vec4 v_lightspacepos;
uniform vec2 u_screensize;

float calculateShadow(float ndotl) {
	float s = $texture2D(u_shadowmap, v_lightspacepos.xy).z;
	float visibility = 0.5;
	float bias = 0.005 * tan(acos(ndotl));
	bias = clamp(bias, 0.0, 0.01);
	if (s < v_lightspacepos.z - bias) {
		visibility = 1.0;
	}
	return visibility;
}

#else

float calculateShadow(float ndotl) {
	return 1.0;
}

#endif
