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
	bool usePrimaryLight = ndotl1 >= ndotl2;
	vec3 lightDir = usePrimaryLight ? u_lightdir : -u_lightdir;
	float ndotl = max(ndotl1, ndotl2);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);
	// Base bias for shadow mapping - kept small since normal offset is the primary technique
	// The bias is increased slightly for surfaces facing away from the light
	float slopeFactor = 1.0 - ndotl;
	float bias = 0.0005 + 0.001 * slopeFactor;
	vec3 shadowColor = shadow(bias, normal, lightDir, v_color.rgb, diffuse, u_ambient_color);
	vec3 color = checkerBoardColor(normal, v_pos, tonemapping(shadowColor * v_ambientocclusion));
	vec4 ocolor = vec4(color, v_color.a);
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
