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
	bool usePrimaryLight = ndotl1 >= ndotl2;
	vec3 lightDir = usePrimaryLight ? u_lightdir : -u_lightdir;
	float ndotl = max(ndotl1, ndotl2);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);
	float bias = max(0.0015 * (1.0 - ndotl), 0.00035);
	vec3 shadowColor = shadow(bias, normal, lightDir, v_color.rgb, diffuse, u_ambient_color);
	vec4 ocolor = vec4(shadowColor, v_color.a);
	if ((v_flags & FLAGOUTLINE) != 0u) {
		return outline(v_pos, ocolor, normal);
	}
	return ocolor;
}

void main(void) {
	o_color = calcColor();
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / cl_gamma));
	o_glow = v_glow;
}
