
layout(std140) uniform u_frag {
	mediump vec3 u_lightdir;
	lowp vec3 u_diffuse_color;
	lowp vec3 u_ambient_color;
	vec2 u_depthsize;
	vec4 u_distances;
	mat4 u_cascades[4];
};

layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

#ifndef cl_gamma
#define cl_gamma 1.0
#endif

#if cl_shadowmap == 1

$in vec3 v_lightspacepos;
$in float v_viewz;
$constant MaxDepthBuffers 4

uniform sampler2DArrayShadow u_shadowmap;

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

vec3 calculateShadowUVZ(in vec4 lightspacepos, in int cascade) {
	vec4 lightp = u_cascades[cascade] * lightspacepos;
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow
	 * Also bring the ndc into the range [0-1] because the depth map
	 * is in that range */
	return (lightp.xyz / lightp.w) * 0.5 + 0.5;
}

vec3 shadow(in vec4 lightspacepos, in float bias, vec3 color, in vec3 diffuse, in vec3 ambient) {
	int cascade = int(dot(vec4(greaterThan(vec4(v_viewz), u_distances)), vec4(1)));
	vec3 uv = calculateShadowUVZ(lightspacepos, cascade);
	float shadow = sampleShadowPCF(bias, cascade, uv.xy, uv.z);
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
	return color * lightvalue;
#endif // cl_debug_shadow
}

vec3 shadow(in float bias, vec3 color, in vec3 diffuse, in vec3 ambient) {
	return shadow(vec4(v_lightspacepos, 1.0), bias, color, diffuse, ambient);
}

#else // cl_shadowmap == 1

vec3 shadow(in vec4 lightspacepos, in float bias, in vec3 color, in vec3 diffuse, in vec3 ambient) {
	return color * (ambient + diffuse);
}

vec3 shadow(in float bias, in vec3 color, in vec3 diffuse, in vec3 ambient) {
	return color * (ambient + diffuse);
}

#endif // cl_shadowmap == 1

#if r_checkerboard == 1

// https://thebookofshaders.com
float checker(in vec2 pos, float strength) {
	vec2 c = floor(pos);
	float checker = mod(c.x + c.y, 2.0);
	return mix(0.95 + strength, 1.0 - strength, checker);
}

vec3 checkerBoardColor(in vec3 normal, in vec3 pos, in vec3 color) {
	float checkerBoardFactor = 1.0;
	if (abs(normal.y) >= 0.999) {
		checkerBoardFactor = checker(pos.xz, 0.2);
	} else if (abs(normal.x) >= 0.999) {
		checkerBoardFactor = checker(pos.yz, 0.2);
	} else if (abs(normal.z) >= 0.999) {
		checkerBoardFactor = checker(pos.xy, 0.2);
	}
	return color * checkerBoardFactor;
}

#else // r_checkerboard == 1

vec3 checkerBoardColor(in vec3 normal, in vec3 pos, in vec3 color) {
	return color;
}

#endif // r_checkerboard == 1


vec4 darken(vec4 color) {
	return vec4(color.rgb * vec3(0.3, 0.3, 0.3), color.a);
}

vec4 brighten(vec4 color) {
	return clamp(vec4(color.rgb * vec3(1.5, 1.5, 1.5), color.a), 0.0, 1.0);
}

// pos is in object space
vec4 outline(vec3 pos, vec4 color) {
	const float epsilona = 0.025;
	const float epsilonb = 0.0001;
	vec3 frac = abs(fract(pos));
	bool nearX = (frac.x <= epsilona || 1.0 - frac.x <= epsilona);
	bool nearY = (frac.y <= epsilona || 1.0 - frac.y <= epsilona);
	bool nearZ = (frac.z <= epsilona || 1.0 - frac.z <= epsilona);
	bool overX = (frac.x <= epsilonb || 1.0 - frac.x <= epsilonb);
	bool overY = (frac.y <= epsilonb || 1.0 - frac.y <= epsilonb);
	bool overZ = (frac.z <= epsilonb || 1.0 - frac.z <= epsilonb);
	if ((nearX && !overX) || (nearY && !overY) || (nearZ && !overZ)) {
		if (color.r < 0.1 && color.g < 0.1 && color.b < 0.1) {
			color = brighten(color);
		} else {
			color = darken(color);
		}
	}
	return color;
}
