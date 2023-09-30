// shader for marching cubes

$in vec4 v_pos;
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
	if ((v_flags & FLAGOUTLINE) != 0u) {
		// TODO: these must be zoom, scale and view related
		const float epsilona = 0.025;
		const float epsilonb = 0.0001;
		float xx = abs(fract(v_pos.x));
		float yy = abs(fract(v_pos.y));
		float zz = abs(fract(v_pos.z));
		bool nearX = (xx <= epsilona || 1.0 - xx <= epsilona);
		bool nearY = (yy <= epsilona || 1.0 - yy <= epsilona);
		bool nearZ = (zz <= epsilona || 1.0 - zz <= epsilona);
		bool overX = (xx <= epsilonb || 1.0 - xx <= epsilonb);
		bool overY = (yy <= epsilonb || 1.0 - yy <= epsilonb);
		bool overZ = (zz <= epsilonb || 1.0 - zz <= epsilonb);
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
