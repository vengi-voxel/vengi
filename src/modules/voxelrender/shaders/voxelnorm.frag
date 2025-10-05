// shader for marching cubes

$in vec3 v_pos;
$in vec3 v_normal;
$in vec4 v_color;
$in vec4 v_glow;
flat $in uint v_flags;
#include "_shared.glsl"
#include "_sharedfrag.glsl"

vec4 calcColor(void) {
	vec3 normal = v_normal;
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec3 shadowColor = shadow(bias, v_color.rgb, diffuse, u_ambient_color);
	return vec4(shadowColor, v_color.a);
}

void main(void) {
	o_color = calcColor();
	if ((v_flags & FLAGOUTLINE) != 0u) {
		o_color = outline(v_pos, o_color);
	}
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / cl_gamma));
	o_glow = v_glow;
}
