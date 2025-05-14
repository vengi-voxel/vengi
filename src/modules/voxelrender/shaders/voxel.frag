$in vec4 v_pos;
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
		vec3 fdx = dFdx(v_pos.xyz);
		vec3 fdy = dFdy(v_pos.xyz);
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
	vec3 color = checkerBoardColor(normal, v_pos.xyz, tonemapping(shadowColor * v_ambientocclusion));
	return vec4(color, v_color.a);
}

vec4 darken(vec4 color) {
	return vec4(color.rgb * vec3(0.3, 0.3, 0.3), o_color.a);
}

vec4 brighten(vec4 color) {
	return clamp(vec4(color.rgb * vec3(1.5, 1.5, 1.5), o_color.a), 0.0, 1.0);
}

void main(void) {
	o_color = calcColor();
	if ((v_flags & FLAGOUTLINE) != 0u) {
		// TODO: these must be zoom, scale and view related
		const float epsilona = 0.025;
		const float epsilonb = 0.0001;
		vec4 frac = abs(fract(v_pos));
		bool nearX = (frac.x <= epsilona || 1.0 - frac.x <= epsilona);
		bool nearY = (frac.y <= epsilona || 1.0 - frac.y <= epsilona);
		bool nearZ = (frac.z <= epsilona || 1.0 - frac.z <= epsilona);
		bool overX = (frac.x <= epsilonb || 1.0 - frac.x <= epsilonb);
		bool overY = (frac.y <= epsilonb || 1.0 - frac.y <= epsilonb);
		bool overZ = (frac.z <= epsilonb || 1.0 - frac.z <= epsilonb);
		if ((nearX && !overX) || (nearY && !overY) || (nearZ && !overZ)) {
			if (o_color.r < 0.1 && o_color.g < 0.1 && o_color.b < 0.1) {
				o_color = brighten(o_color);
			} else {
				o_color = darken(o_color);
			}
		}
	}
	o_color.rgb = pow(o_color.rgb, vec3(1.0 / cl_gamma));
	o_glow = v_glow;
}
