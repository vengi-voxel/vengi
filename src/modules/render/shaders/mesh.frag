$in vec3 v_norm;
$in vec2 v_texcoords;
$in vec3 v_pos;
$in vec4 v_color;

$out vec4 o_color;

uniform sampler2D u_texture;
uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;
uniform vec3 u_ambient_color;
uniform mat4 u_viewprojection;

#include "_fog.frag"
#include "_shadowmap.frag"

void main(void) {
	vec3 color = $texture2D(u_texture, v_texcoords).rgb + v_color.rgb;
	float ndotl = dot(v_norm, u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, ndotl);
	vec3 linearColor = shadow(u_viewprojection, color, diffuse, u_ambient_color);
	o_color = fog(v_pos, linearColor, v_color.a);
}
