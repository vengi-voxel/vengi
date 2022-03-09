$in vec4 v_pos;
$in vec4 v_color;
$in vec4 v_glow;
$in float v_ambientocclusion;
flat $in uint v_flags;
uniform mat4 u_viewprojection;

#include "_shadowmap.frag"
#include "_voxelflags.glsl"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

#ifndef cl_gamma
#define cl_gamma 2.2
#endif

vec4 calcColor(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec3 shadowColor = shadow(bias, v_color.rgb, diffuse, u_ambient_color);
	return vec4(shadowColor * v_ambientocclusion, v_color.a);
}

void main(void) {
	if ((v_flags & FLAGOUTLINE) != 0u) {
		const float epsilona = 0.025;
		const float epsilonb = 0.0001;
		float xx = fract(v_pos.x);
		float yy = fract(v_pos.y);
		float zz = fract(v_pos.z);
		bool nearX = (xx >= -epsilona && xx <= epsilona);
		bool nearY = (yy >= -epsilona && yy <= epsilona);
		bool nearZ = (zz >= -epsilona && zz <= epsilona);
		bool overX = (xx >= -epsilonb && xx <= epsilonb);
		bool overY = (yy >= -epsilonb && yy <= epsilonb);
		bool overZ = (zz >= -epsilonb && zz <= epsilonb);
		if ((nearX && !overX) || (nearY && !overY) || (nearZ && !overZ)) {
			o_color = vec4(v_color.rgb * vec3(0.3, 0.3, 0.3), v_color.a);
		} else {
			o_color = calcColor();
		}
	} else {
		o_color = calcColor();
	}
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / cl_gamma));
	if ((v_flags & FLAGBLOOM) != 0u) {
		o_glow = o_color;
	} else {
		o_glow = v_glow;
	}
}
