$in vec3 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;
uniform float u_time;
uniform samplerCube u_cubemap;
uniform vec3 u_camerapos;

vec2 calculateShadowTexcoord(vec2 uv) {
	float offset = cos(u_time / 1000.0) * 0.000125;
	vec2 coord = vec2(uv.x + offset, uv.y + offset);
	return coord;
}

#define CUSTOM_SHADOW_TEXCOORD
#include "_fog.frag"
#include "_shadowmap.frag"
#include "_daynight.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
$out vec4 o_color;

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 I = normalize(vec3(v_pos) - u_camerapos);
	vec3 R = reflect(I, normal);
	vec3 cubeColor = texture(u_cubemap, R).rgb;
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec3 shadowColor = shadow(bias, u_viewprojection, mix(cubeColor, v_color.rgb, 0.01), diffuse, ambientColor);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(v_pos.xyz, linearColor, v_color.a);
}
