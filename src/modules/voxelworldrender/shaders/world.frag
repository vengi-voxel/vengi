$in vec3 v_pos;
$in vec4 v_color;
$in vec4 v_clipspace;
$in float v_ambientocclusion;
uniform mat4 u_viewprojection;
uniform sampler2DShadow u_entitiesdepthmap;

#include "_fog.frag"
#include "_shadowmap.frag"
#include "_daynight.frag"
#include "_checker.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
uniform float u_time;
layout(location = 0) $out vec4 o_color;

vec3 clipSpaceToTexCoords(vec4 clipSpace){
	vec3 ndc = (clipSpace.xyz / clipSpace.w);
	vec3 texCoords = ndc / 2.0 + 0.5;
	return clamp(texCoords, 0.0, 1.0);
}

/**
 * @brief If the player is behind some objects - we will make those fragments that
 * are between the camera eye and the player transparent
 */
float resolveAlpha() {
	vec3 ndc = clipSpaceToTexCoords(v_clipspace);
	float depth = gl_FragCoord.z;
	float characterDepth = $texture2D(u_entitiesdepthmap, ndc);
	if (depth > characterDepth) {
		return 0.3;
	}
	return v_color.a;
}

void main(void) {
	float alpha = resolveAlpha();
	if (alpha <= 0.01) {
		discard;
	}
	vec3 fdx = dFdx(v_pos);
	vec3 fdy = dFdy(v_pos);
	vec3 normal = normalize(cross(fdx, fdy));
	float ndotl = abs(dot(normal, u_lightdir));
	vec3 diffuse = u_diffuse_color * ndotl;
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	vec3 voxelColor = checkerBoardColor(normal, v_pos, v_color.rgb);
	float bias = max(0.05 * (1.0 - ndotl), 0.005);
	vec3 shadowColor = shadow(bias, voxelColor, diffuse, ambientColor);
	vec3 linearColor = shadowColor * v_ambientocclusion;
	o_color = fog(v_pos, linearColor, alpha);
}
