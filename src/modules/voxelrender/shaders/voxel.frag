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
		vec3 fdx = dFdx(v_pos);
		vec3 fdy = dFdy(v_pos);
		// http://www.aclockworkberry.com/shader-derivative-functions/
		// face normal (flat shading)
		normal = normalize(cross(fdx, fdy));
	}
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
#if r_normals == 0
	vec3 color3 = v_color.rgb;
#else
	vec3 color3 = v_normal;
#endif
	vec3 shadowColor = shadow(bias, color3, diffuse, u_ambient_color);
	vec3 color = checkerBoardColor(normal, v_pos, tonemapping(shadowColor * v_ambientocclusion));
	return vec4(color, v_color.a);
}

void main(void) {
	o_color = calcColor();
	if ((v_flags & FLAGOUTLINE) != 0u) {
		o_color = outline(v_pos, o_color);
	}
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / cl_gamma));
	o_glow = v_glow;
}
