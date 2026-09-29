$in vec3 v_pos;
$in vec3 v_normal;
$in vec4 v_color;
$in vec4 v_glow;
$in float v_ambientocclusion;
flat $in uint v_flags;
#include "_shared.glsl"
#include "_sharedfrag.glsl"
#include "_tonemapping.glsl"

vec4 calcColor(void) {
	vec3 normal;
	if ((v_flags & FLAGHASNORMALPALETTECOLOR) != 0u) {
		normal = v_normal;
	} else {
		normal = cubicWorldNormal(v_pos);
	}
	vec3 shadowColor = shadeLit(normal, v_color.rgb);
	vec3 color = checkerBoardColor(normal, v_pos, tonemapping(shadowColor * v_ambientocclusion));
	vec4 ocolor = vec4(color, v_color.a);
	if ((v_flags & FLAGOUTLINE) != 0u) {
		if (u_renderoutline != 0) {
			if ((v_flags & FLAGOUTLINEPULSE) != 0u) {
				ocolor.rgb = mix(ocolor.rgb, u_selectiontint.rgb, u_selectiontint.a);
				float pulse = 0.5 + 0.5 * sin(float(u_timemillis) * 0.005);
				return outline(v_pos, ocolor, normal, pulse);
			}
			return outline(v_pos, ocolor, normal, 1.0);
		}
		ocolor.rgb = mix(ocolor.rgb, u_selectiontint.rgb, u_selectiontint.a);
		return outline(v_pos, ocolor, normal, 1.0);
	}
	return ocolor;
}

void main(void) {
	o_color = calcColor();
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / u_gamma));
	o_glow = v_glow;
}
