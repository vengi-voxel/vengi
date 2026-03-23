// shader for marching cubes

// attributes from the VAOs
layout (location = 0) $in vec3 a_pos;
layout (location = 1) $in uvec2 a_info;
layout (location = 2) $in vec3 a_normal;

#include "_sharedvert.glsl"
#include "_shared.glsl"

void main(void) {
	uint a_flags = ((a_info[0] & ~3u) >> 2u);
	uint a_colorindex = a_info[1];
	vec4 pos = u_model * vec4(a_pos, 1.0);
	v_pos = a_pos;
	v_normal = a_normal;

	int materialColorIndex = int(a_colorindex);
	vec4 materialColor = u_materialcolor[materialColorIndex];
	vec4 glowColor = u_glowcolor[materialColorIndex];
	v_flags = 0u;
	if (u_has_selection == 0) {
#if r_renderoutline == 0
		if ((a_flags & FLAGOUTLINE) != 0u) {
			v_flags |= FLAGOUTLINE;
		}
#else
		v_flags |= FLAGOUTLINE;
#endif
	}

	if (u_gray != 0 || (u_has_selection != 0 && (a_flags & FLAGOUTLINE) == 0u)) {
		float gray = (0.21 * materialColor.r + 0.72 * materialColor.g + 0.07 * materialColor.b) / 3.0;
		v_color = vec4(gray, gray, gray, materialColor.a);
	} else if (u_locked != 0) {
		// blue tint: reduce red/green to visually distinguish locked nodes
		const float lockedTintR = 0.6;
		const float lockedTintG = 0.8;
		const float lockedTintB = 1.0;
		v_color = vec4(materialColor.r * lockedTintR, materialColor.g * lockedTintG, materialColor.b * lockedTintB, materialColor.a);
	} else {
		v_color = materialColor;
	}
	v_glow = glowColor;

#if cl_shadowmap == 1
	v_lightspacepos = pos.xyz;
	v_viewz = (u_viewprojection * vec4(v_lightspacepos, 1.0)).w;
#endif // cl_shadowmap

	gl_Position = u_viewprojection * pos;
}
