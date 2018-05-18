$in vec4 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;

#include "_fog.frag"
#include "_shadowmap.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
$out vec4 o_color;

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl = dot(normal, u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);
	vec3 shadowColor = shadow(u_viewprojection, v_color.rgb, diffuse, u_ambient_color);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(linearColor, v_color.a);
}
