
layout(std140) uniform u_frag {
	mediump vec3 u_lightdir;
	lowp vec3 u_diffuse_color;
	lowp vec3 u_ambient_color;
	vec2 u_depthsize;
	vec4 u_distances;
	mat4 u_cascades[4];
	vec4 u_biasParams; // x: constantBias, y: slopeBias, z: biasClamp, w: voxelEdgeBias
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
 * Calculate dynamic bias for voxel shadow mapping
 * Uses slope-scale depth bias to reduce shadow acne and self-shadowing
 */
float calculateVoxelBias(in vec3 normal, in vec3 lightDir, in int cascade) {
	// Calculate angle between surface normal and light direction
	float cosTheta = clamp(dot(normal, lightDir), 0.01, 1.0);

	// Calculate slope for slope-scale bias
	float tanTheta = sqrt(1.0 - cosTheta * cosTheta) / cosTheta;

	// Base constant bias scaled by cascade
	float cascadeScale = 1.0 + float(cascade) * 0.5;
	float constantBias = u_biasParams.x * cascadeScale;

	// Slope-scale bias - critical for voxel hard edges
	float slopeBias = u_biasParams.y * tanTheta;

	// Additional voxel edge bias
	float voxelBias = u_biasParams.w * cascadeScale;

	// Combine biases
	float totalBias = constantBias + slopeBias + voxelBias;

	// Adaptive bias for grazing angles
	float grazingFactor = 1.0 / (cosTheta * cosTheta);
	totalBias *= mix(1.0, grazingFactor, 0.2);

	// Clamp to prevent Peter Panning
	return min(totalBias, u_biasParams.z);
}

/**
 * perform percentage-closer shadow map lookup with adaptive sampling
 * Enhanced with voxel-specific bias calculations
 */
float sampleShadowPCF(in vec3 normal, in vec3 lightDir, in int cascade, in vec2 uv, in float compare) {
	float result = 0.0;

	// Calculate dynamic bias for voxel environments
	float bias = calculateVoxelBias(normal, lightDir, cascade);
	float biasedCompare = compare - bias;

	// Adaptive kernel size based on cascade level for better performance
	// Closer cascades get sharper shadows, distant ones can be softer
	int r = cascade == 0 ? 1 : (cascade == 1 ? 1 : 2);

	// Use a more efficient sampling pattern for better quality
	if (r == 1) {
		// 3x3 kernel for sharp shadows
		for (int x = -1; x <= 1; x++) {
			for (int y = -1; y <= 1; y++) {
				vec2 off = vec2(x, y) / u_depthsize;
				result += texture(u_shadowmap, vec4(uv + off, cascade, biasedCompare));
			}
		}
		return result / 9.0;
	} else {
		// Poisson disk sampling for larger kernels - better quality than regular grid
		const vec2 poissonDisk[16] = vec2[](
			vec2(-0.94201624, -0.39906216), vec2(0.94558609, -0.76890725),
			vec2(-0.094184101, -0.92938870), vec2(0.34495938, 0.29387760),
			vec2(-0.91588581, 0.45771432), vec2(-0.81544232, -0.87912464),
			vec2(-0.38277543, 0.27676845), vec2(0.97484398, 0.75648379),
			vec2(0.44323325, -0.97511554), vec2(0.53742981, -0.47373420),
			vec2(-0.26496911, -0.41893023), vec2(0.79197514, 0.19090188),
			vec2(-0.24188840, 0.99706507), vec2(-0.81409955, 0.91437590),
			vec2(0.19984126, 0.78641367), vec2(0.14383161, -0.14100790)
		);

		float kernelScale = float(r) * 0.5;
		for (int i = 0; i < 16; i++) {
			vec2 off = poissonDisk[i] * kernelScale / u_depthsize;
			result += texture(u_shadowmap, vec4(uv + off, cascade, biasedCompare));
		}
		return result / 16.0;
	}
}

vec3 calculateShadowUVZ(in vec4 lightspacepos, in int cascade) {
	vec4 lightp = u_cascades[cascade] * lightspacepos;
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow
	 * Also bring the ndc into the range [0-1] because the depth map
	 * is in that range */
	return (lightp.xyz / lightp.w) * 0.5 + 0.5;
}

/**
 * Enhanced shadow sampling with better edge detection for voxels
 */
float sampleShadowEnhanced(in vec3 normal, in vec3 lightDir, in int cascade, in vec2 uv, in float compare) {
	// Basic PCF sample with voxel-aware bias
	float shadowValue = sampleShadowPCF(normal, lightDir, cascade, uv, compare);

	// Dynamic bias for edge detection
	float bias = calculateVoxelBias(normal, lightDir, cascade);

	// Check if we're near a shadow boundary for extra sharpening
	float center = texture(u_shadowmap, vec4(uv, cascade, compare - bias));
	vec2 texelSize = 1.0 / u_depthsize;

	// Sample neighboring pixels to detect edges
	float left = texture(u_shadowmap, vec4(uv + vec2(-texelSize.x, 0.0), cascade, compare - bias));
	float right = texture(u_shadowmap, vec4(uv + vec2(texelSize.x, 0.0), cascade, compare - bias));
	float up = texture(u_shadowmap, vec4(uv + vec2(0.0, -texelSize.y), cascade, compare - bias));
	float down = texture(u_shadowmap, vec4(uv + vec2(0.0, texelSize.y), cascade, compare - bias));

	// Calculate edge strength
	float edgeStrength = abs(center - left) + abs(center - right) + abs(center - up) + abs(center - down);

	// Voxel environments benefit from sharper shadow transitions
	if (edgeStrength > 0.08) {
		// More aggressive sharpening for voxel hard edges
		shadowValue = shadowValue * shadowValue * (3.0 - 2.0 * shadowValue); // Smoothstep for sharper transition
	}

	return shadowValue;
}

vec3 shadow(in vec4 lightspacepos, in float bias, vec3 color, in vec3 diffuse, in vec3 ambient, in vec3 normal) {
	int cascade = int(dot(vec4(greaterThan(vec4(v_viewz), u_distances)), vec4(1)));
	vec3 uv = calculateShadowUVZ(lightspacepos, cascade);
	float shadow = sampleShadowEnhanced(normal, u_lightdir, cascade, uv.xy, uv.z);

	// Apply contrast enhancement for sharper voxel shadow edges
	shadow = smoothstep(0.15, 0.85, shadow);
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

vec3 shadow(in float bias, vec3 color, in vec3 diffuse, in vec3 ambient, in vec3 normal) {
	return shadow(vec4(v_lightspacepos, 1.0), bias, color, diffuse, ambient, normal);
}

#else // cl_shadowmap == 1

vec3 shadow(in vec4 lightspacepos, in float bias, in vec3 color, in vec3 diffuse, in vec3 ambient, in vec3 normal) {
	return color * (ambient + diffuse);
}

vec3 shadow(in float bias, in vec3 color, in vec3 diffuse, in vec3 ambient, in vec3 normal) {
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
