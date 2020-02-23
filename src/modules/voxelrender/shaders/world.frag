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
	float cx = floor(repeats * pos.x);
	float cy = floor(repeats * pos.y);
	float cz = floor(repeats * pos.z);
	float result = mod(cx + cy + cz, 2.0);
	return sign(result);
}

void main(void) {
	vec3 fdx = dFdx(v_pos);
	vec3 fdy = dFdy(v_pos);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	vec3 voxelColor = v_color.rgb;
	ivec2 xz = ivec2(v_pos.x * 0.5, v_pos.z * 0.5);
	float checkerBoardIntensity = random(vec2(xz.x, xz.y));
	float variance = 0.05 * checkerBoardIntensity;
	float checkerBoardFactor = mix(0.95 + variance, 1.0 - variance, checker(v_pos, 0.5));
	voxelColor *= checkerBoardFactor;
	vec3 shadowColor = shadow(u_viewprojection, voxelColor, diffuse, ambientColor);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(v_pos, linearColor, v_color.a);
}
