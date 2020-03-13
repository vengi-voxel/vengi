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

float checker(in vec2 pos) {
	vec2 c = floor(pos);
	float variance = 0.05 * random(c);
	float checker = mod(c.x + c.y, 2.0);
	return mix(0.95 + variance, 1.0 - variance, checker);
}

vec3 baseColor(in vec3 normal) {
	float checkerBoardFactor = 1.0;
	if (abs(normal.y) >= 0.999) {
		checkerBoardFactor = checker(v_pos.xz);
	} else if (abs(normal.x) >= 0.999) {
		checkerBoardFactor = checker(v_pos.yz);
	} else if (abs(normal.z) >= 0.999) {
		checkerBoardFactor = checker(v_pos.xy);
	}
	return v_color.rgb * checkerBoardFactor;
}

void main(void) {
	vec3 fdx = dFdx(v_pos);
	vec3 fdy = dFdy(v_pos);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl = abs(dot(normal, u_lightdir));
	vec3 diffuse = u_diffuse_color * ndotl;
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	vec3 voxelColor = baseColor(normal);
	float bias = max(0.05 * (1.0 - ndotl), 0.005);
	vec3 shadowColor = shadow(vec4(v_lightspacepos, 1.0), bias, voxelColor, diffuse, ambientColor);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(v_pos, linearColor, v_color.a);
}
