
layout(std140) uniform u_frag {
	mediump vec3 u_lightdir;
	lowp vec3 u_diffuse_color;
	lowp vec3 u_ambient_color;
	vec2 u_depthsize;
	vec4 u_distances;
	mat4 u_cascades[4];
	vec4 u_outlinecolor;
	vec4 u_selectedoutlinecolor;
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
	float compareDepth = compare - bias;
	const int r = 2;
	for (int x = -r; x <= r; x++) {
		for (int y = -r; y <= r; y++) {
			vec2 off = vec2(x, y) / u_depthsize;
			result += texture(u_shadowmap, vec4(uv + off, cascade, compareDepth));
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

vec3 shadow(in vec4 lightspacepos, in float bias, in vec3 normal, in vec3 lightDir, vec3 color, in vec3 diffuse, in vec3 ambient) {
	int cascade = int(dot(vec4(greaterThan(vec4(v_viewz), u_distances)), vec4(1)));
	cascade = clamp(cascade, 0, MaxDepthBuffers - 1);
	float cascadeNear = (cascade == 0) ? 0.0 : u_distances[cascade - 1];
	float cascadeFar = u_distances[cascade];
	float cascadeRange = max(cascadeFar - cascadeNear, 0.001);

	vec3 normalizedLightDir = normalize(lightDir);
	vec3 normalizedNormal = normalize(normal);
	float NdotL = dot(normalizedNormal, normalizedLightDir);
	float alignment = max(NdotL, 0.0);

	// Calculate texel size for this cascade (approximate world space size)
	// Higher cascades cover more area, so texels are larger in world space
	float cascadeScale = 1.0 + float(cascade) * 0.5;
	float texelSize = cascadeScale / u_depthsize.x;

	// Normal offset bias: offset along surface normal to avoid shadow acne on slopes
	// This is the primary technique to fix both shadow acne and peter panning
	// The offset is proportional to the texel size and increases with surface slope
	float normalOffsetScale = texelSize * 2.5;
	float slopeScale = clamp(1.0 - NdotL, 0.0, 1.0);
	vec3 normalOffset = normalizedNormal * normalOffsetScale * (1.0 + slopeScale * 3.0);

	// Small constant depth bias to handle depth precision issues
	// This should be minimal to avoid peter panning
	float depthBias = bias * (0.5 + float(cascade) * 0.25);

	// Apply normal offset to position before transforming to shadow space
	vec3 offsetPos = lightspacepos.xyz + normalOffset;
	vec3 uv = calculateShadowUVZ(vec4(offsetPos, 1.0), cascade);

	// Additional slope-scaled depth bias for the comparison
	// Use a smaller bias since we already applied normal offset
	float slopeBias = depthBias * (0.1 + slopeScale * 0.4);
	float shadow = sampleShadowPCF(slopeBias, cascade, uv.xy, uv.z);
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

vec3 shadow(in float bias, in vec3 normal, in vec3 lightDir, vec3 color, in vec3 diffuse, in vec3 ambient) {
	return shadow(vec4(v_lightspacepos, 1.0), bias, normal, lightDir, color, diffuse, ambient);
}

#else // cl_shadowmap == 1

vec3 shadow(in vec4 lightspacepos, in float bias, in vec3 normal, in vec3 lightDir, in vec3 color, in vec3 diffuse, in vec3 ambient) {
	return color * (ambient + diffuse);
}

vec3 shadow(in float bias, in vec3 normal, in vec3 lightDir, in vec3 color, in vec3 diffuse, in vec3 ambient) {
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
// outlineColor: vec4(-1) means auto darken/brighten, otherwise use the given color's rgb
// thickness: controls the edge detection threshold
vec4 outline(vec3 pos, vec4 color, vec3 normal, vec4 outlineColor, float thickness) {
	const float epsilonb = 0.0001;
	vec3 frac = abs(fract(pos));
	bool nearX = (frac.x <= thickness || 1.0 - frac.x <= thickness);
	bool nearY = (frac.y <= thickness || 1.0 - frac.y <= thickness);
	bool nearZ = (frac.z <= thickness || 1.0 - frac.z <= thickness);
	bool overX = (frac.x <= epsilonb || 1.0 - frac.x <= epsilonb);
	bool overY = (frac.y <= epsilonb || 1.0 - frac.y <= epsilonb);
	bool overZ = (frac.z <= epsilonb || 1.0 - frac.z <= epsilonb);
	if ((nearX && !overX) || (nearY && !overY) || (nearZ && !overZ)) {
		if (outlineColor.r < 0.0) {
			// auto mode: darken or brighten based on voxel color
			if (color.r < 0.1 && color.g < 0.1 && color.b < 0.1) {
				color = brighten(color);
			} else {
				color = darken(color);
			}
		} else {
			color = vec4(outlineColor.rgb, color.a);
		}
	}
	return color;
}

vec4 outline(vec3 pos, vec4 color, vec3 normal) {
	return outline(pos, color, normal, vec4(-1.0), 0.025);
}
