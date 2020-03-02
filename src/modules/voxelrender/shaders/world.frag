$in vec3 v_pos;
$in vec4 v_color;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;

#include "_fog.frag"
#include "_shadowmap.frag"
#include "_daynight.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
uniform float u_time;
$out vec4 o_color;

// https://thebookofshaders.com
float random (in vec2 st) {
	return fract(sin(dot(st.xy, vec2(12.9898, 78.233))) * 43758.5453123);
}

float checker(vec3 pos, float repeats) {
	vec3 c = floor(repeats * pos);
	return mod(c.x + c.y + c.z, 2.0);
}

void main(void) {
	vec3 fdx = dFdx(v_pos);
	vec3 fdy = dFdy(v_pos);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	float checkerBoardIntensity = random(floor(v_pos.xz * 0.5));
	float variance = 0.05 * checkerBoardIntensity;
	float checkerBoardFactor = mix(0.95 + variance, 1.0 - variance, checker(v_pos, 0.5));
	vec3 voxelColor = clamp(v_color.rgb * checkerBoardFactor, vec3(0.0), vec3(1.0));
	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec3 shadowColor = shadow(vec4(v_lightspacepos, 1.0), bias, voxelColor, diffuse, ambientColor);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(v_pos, linearColor, v_color.a);
}
