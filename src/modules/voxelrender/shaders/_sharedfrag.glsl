
layout(std140, binding = 1) uniform u_frag {
	mediump vec3 u_lightdir;
	lowp vec3 u_diffusecolor;
	lowp vec3 u_ambientcolor;
	vec2 u_depthsize;
	vec4 u_distances;
	mat4 u_cascades[4];
	vec4 u_selectiontint;
	uint u_timemillis;
	float u_gamma;
	int u_checkerboard;
	int u_debugshadow;
	int u_debugcascade;
	int u_tonemapping;
	int u_renderoutline;
	int u_shadowmap;
	vec4 u_camerapos;
	vec4 u_cameradir; // xyz = forward, w = orthographic projection
};

layout(location = 0) out vec4 o_color;
layout(location = 1) out vec4 o_glow;

in vec3 v_lightspacepos;
flat in vec3 v_nm0;
flat in vec3 v_nm1;
flat in vec3 v_nm2;
#pragma constant MaxDepthBuffers 4

layout(binding = 2) uniform sampler2DArrayShadow u_shadowmaptex;

vec3 cubicWorldNormal(vec3 pos);

// Compare at each texel's receiver-plane depth, rather than at the fragment's depth.
float sampleShadow(in int cascade, in vec3 uv, in vec2 sampleUV, in vec2 depthGradient) {
	// Only cover floating-point error; a large normalized bias detaches shadows in wide scenes.
	float compare = uv.z + dot(sampleUV - uv.xy, depthGradient) - 0.000002;
	// Clear-depth texels must stay lit even when the receiver plane extrapolates past the far plane.
	// Cascades have discontinuous UVs; implicit texture derivatives produce seams between them.
	return textureGrad(u_shadowmaptex, vec4(sampleUV, cascade, min(compare, 1.0 - 0.000002)), vec2(0.0), vec2(0.0));
}

float filteredShadow(in int cascade, in vec3 uv, in vec3 worldpos) {
#ifdef CUBICSHADOWRECEIVER
	// World-position derivatives lose precision on close, oblique cube faces.
	vec3 receiverNormal = cubicWorldNormal(v_pos);
#else
	vec3 receiverNormal = cross(dFdx(worldpos), dFdy(worldpos));
#endif
	vec3 toCamera = u_cameradir.w != 0.0 ? -u_cameradir.xyz : u_camerapos.xyz - worldpos;
	if (dot(receiverNormal, toCamera) < 0.0) {
		receiverNormal = -receiverNormal;
	}
	float facing = dot(receiverNormal, normalize(u_lightdir)) / max(length(receiverNormal), 1.0e-20);
	// Opaque alpha can interpolate just below 1.0 in perspective views.
	bool opaque = v_color.a >= 1.0 - 1.0e-5;
	// Rays toward the sun enter the solid volume on an opaque back-facing surface.
	if (opaque && facing <= 0.00001) {
		return 0.0;
	}
	// Orthographic cascade rows are orthogonal; avoid inverting a grazing screen-space Jacobian.
	mat3 rows = transpose(mat3(u_cascades[cascade]));
	vec3 planeNormal = vec3(dot(rows[0], receiverNormal), dot(rows[1], receiverNormal), dot(rows[2], receiverNormal));
	planeNormal /= vec3(dot(rows[0], rows[0]), dot(rows[1], rows[1]), dot(rows[2], rows[2]));
	planeNormal.xy *= 2.0;
#ifndef CLIPDEPTHZ0TO1
	planeNormal.z *= 2.0;
#endif
#ifdef CLIPORIGINUPPERLEFT
	planeNormal.y = -planeNormal.y;
#endif
	vec2 depthGradient = abs(planeNormal.z) > 1.0e-20 ? -planeNormal.xy / planeNormal.z : vec2(0.0);
	if (!opaque) {
		depthGradient *= smoothstep(0.0, 0.05, abs(facing));
	}
	vec2 size = max(u_depthsize, vec2(1.0));
	vec2 texelPosition = uv.xy * size - 0.5;
	vec2 base = (floor(texelPosition) + 0.5) / size;
	vec2 fraction = fract(texelPosition);
	vec2 texel = 1.0 / size;
	float s00 = sampleShadow(cascade, uv, base, depthGradient);
	float s10 = sampleShadow(cascade, uv, base + vec2(texel.x, 0.0), depthGradient);
	float s01 = sampleShadow(cascade, uv, base + vec2(0.0, texel.y), depthGradient);
	float s11 = sampleShadow(cascade, uv, base + texel, depthGradient);
	return mix(mix(s00, s10, fraction.x), mix(s01, s11, fraction.x), fraction.y);
}

vec3 calculateShadowUVZ(in vec4 lightspacepos, in int cascade) {
	vec4 lightp = u_cascades[cascade] * lightspacepos;
	/* we manually have to do the perspective divide as there is no
	 * version of textureProj that can take a sampler2DArrayShadow
	 * Also bring the ndc into the range [0-1] because the depth map
	 * is in that range */
	vec3 ndc = lightp.xyz / lightp.w;
#ifdef CLIPDEPTHZ0TO1
	vec3 uv = vec3(ndc.xy * 0.5 + 0.5, ndc.z);
#else
	vec3 uv = ndc * 0.5 + 0.5;
#endif
#ifdef CLIPORIGINUPPERLEFT
	// Upper-left clip origin stores NDC +Y at texel row 0; flip so UV matches the map.
	uv.y = 1.0 - uv.y;
#endif
	return uv;
}

vec3 shadow(in vec4 lightspacepos, vec3 color, in vec3 diffuse, in vec3 ambient) {
	if (u_shadowmap == 0) {
		return color * (ambient + diffuse);
	}
	float viewz = dot(lightspacepos.xyz - u_camerapos.xyz, u_cameradir.xyz);
	int cascade = int(dot(vec4(greaterThan(vec4(viewz), u_distances)), vec4(1)));
	cascade = clamp(cascade, 0, MaxDepthBuffers - 1);

	vec3 uv = calculateShadowUVZ(lightspacepos, cascade);
	float shadow = filteredShadow(cascade, uv, lightspacepos.xyz);
	if (u_debugcascade != 0) {
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
	}
	if (u_debugshadow != 0) {
		// shadow only rendering
		return vec3(shadow);
	}
	// Apply the map to all incoming light. Key-only modulation was invisible
	// when ambient dominated (and in Unlit: diffuse is 0).
	vec3 lightvalue = (ambient + diffuse) * mix(0.25, 1.0, shadow);
	return color * lightvalue;
}

vec3 shadow(vec3 color, in vec3 diffuse, in vec3 ambient) {
	return shadow(vec4(v_lightspacepos, 1.0), color, diffuse, ambient);
}

/**
 * Greedy cube faces are axis-aligned in model space. Screen-space dFdx mixes
 * two faces at voxel edges, which skewed lighting. Snap to the dominant cube
 * axis, then transform to world.
 */
vec3 voxelFaceNormal(vec3 n) {
	vec3 an = abs(n);
	if (an.x >= an.y && an.x >= an.z) {
		return vec3(sign(n.x), 0.0, 0.0);
	}
	if (an.y >= an.z) {
		return vec3(0.0, sign(n.y), 0.0);
	}
	return vec3(0.0, 0.0, sign(n.z));
}

vec3 cubicWorldNormal(vec3 pos) {
	vec3 nModel = cross(dFdx(pos), dFdy(pos));
	if (dot(nModel, nModel) < 1.0e-20) {
		vec3 nWorld = cross(dFdx(v_lightspacepos), dFdy(v_lightspacepos));
		if (dot(nWorld, nWorld) < 1.0e-20) {
			return vec3(0.0, 1.0, 0.0);
		}
		return voxelFaceNormal(nWorld);
	}
	vec3 nFace = voxelFaceNormal(nModel);
	return normalize(v_nm0 * nFace.x + v_nm1 * nFace.y + v_nm2 * nFace.z);
}

// One sun (key) is shadowed. Back faces get fill + sky, not a fake opposite sun.
vec3 shadeLit(in vec3 normal, in vec3 color) {
	vec3 n = normalize(normal);
	vec3 sun = normalize(u_lightdir);
	float key = max(dot(n, sun), 0.0);
	vec3 fillDir = normalize(-sun + vec3(0.0, 0.35, 0.0));
	float fill = max(dot(n, fillDir), 0.0);
	float sky = 0.5 + 0.5 * n.y;
	float lit = step(1.0e-5, dot(u_diffusecolor, u_diffusecolor));
	vec3 ambient = u_ambientcolor * mix(1.0, mix(0.70, 1.0, sky), lit);
	vec3 keyDiffuse = u_diffusecolor * key;
	vec3 fillDiffuse = u_diffusecolor * fill * 0.35 * lit;
	return shadow(color, keyDiffuse, ambient + fillDiffuse);
}

// https://thebookofshaders.com
float checker(in vec2 pos, float strength) {
	vec2 c = floor(pos);
	float checker = mod(c.x + c.y, 2.0);
	return mix(0.95 + strength, 1.0 - strength, checker);
}

vec3 checkerBoardColor(in vec3 normal, in vec3 pos, in vec3 color) {
	if (u_checkerboard != 0) {
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
	return color;
}

/**
 * Weighted blended OIT (McGuire / Bavoil). Both MRT targets use additive blending.
 * Color0: accum = sum(rgb * a * w, a * w)
 * Color1.r: sum(log(1 - a)) so revealage = exp(Color1.r) = product(1 - a)
 */
void writeOIT(vec4 color) {
	float a = clamp(color.a, 0.0, 1.0);
	if (a < 1.0e-4) {
		discard;
	}
	a = min(a, 0.999);
	float z = gl_FragCoord.z;
	float w = clamp(pow(min(1.0, a * 10.0) + 0.01, 3.0) * 1.0e8 * pow(max(1.0 - z, 1.0e-4), 3.0), 1.0e-2, 3.0e3);
	o_color = vec4(color.rgb * a, a) * w;
	o_glow = vec4(log(1.0 - a), 0.0, 0.0, 0.0);
}

vec4 darken(vec4 color) {
	return vec4(color.rgb * 0.3, color.a);
}

vec4 brighten(vec4 color) {
	return clamp(vec4(color.rgb * vec3(1.5, 1.5, 1.5), color.a), 0.0, 1.0);
}

// Object-space voxel grid overlay. Lines stay ~1px via fwidth (https://iquilezles.org/).
// Works on greedy-merged quads because pos is still voxel coordinates.
// pulse: 0.0 = no edges, 1.0 = full edges (selection pulse uses values in between).
vec4 outline(vec3 pos, vec4 color, vec3 normal, float pulse) {
	vec3 faceN = abs(cross(dFdx(pos), dFdy(pos)));
	if (dot(faceN, faceN) < 1.0e-12) {
		faceN = abs(normal);
	}
	vec2 f = (faceN.x > faceN.y && faceN.x > faceN.z) ? pos.yz : ((faceN.y > faceN.z) ? pos.xz : pos.xy);
	vec2 d = max(fwidth(f), vec2(1.0e-5));
	vec2 g = abs(fract(f - 0.5) - 0.5) / d;
	float line = 1.0 - min(min(g.x, g.y), 1.0);
	float strength = clamp(line * pulse, 0.0, 1.0);
	vec4 edgeColor = (color.r < 0.1 && color.g < 0.1 && color.b < 0.1) ? brighten(color) : darken(color);
	return mix(color, edgeColor, strength);
}
