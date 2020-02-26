$in vec3 v_pos;
$in vec4 v_clipspace;
$in vec2 v_uv;
$in vec3 v_camdist;

uniform mat4 u_viewprojection;
uniform float u_time;
uniform samplerCube u_cubemap;
uniform sampler2D u_reflection;
uniform sampler2D u_refraction;
uniform sampler2D u_distortion;

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

const float c_wavestrength = 0.02;

void main(void) {
	vec2 ndc = (v_clipspace.xy / v_clipspace.w) / 2.0 + 0.5;
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));
	vec3 I = normalize(v_camdist);
	vec3 R = reflect(I, normal);
	vec3 cubeColor = texture(u_cubemap, R).rgb;
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);

	float moveFactor = fract(u_time / 20000.0);
	vec2 distortion1 = c_wavestrength * ($texture2D(u_distortion, vec2(v_uv.x + moveFactor, v_uv.y)).rg * 2.0 - 1.0);
	vec2 distortion2 = c_wavestrength * ($texture2D(u_distortion, vec2(-v_uv.x + moveFactor, v_uv.y + moveFactor)).rg * 2.0 - 1.0);
	vec2 totalDistortion = distortion1 + distortion2;

	vec2 refractTexcoords = ndc;
	refractTexcoords += totalDistortion;
	refractTexcoords = clamp(refractTexcoords, 0.001, 0.999);
	vec4 refractColor = $texture2D(u_refraction, refractTexcoords);

	vec2 reflectTexcoords = vec2(ndc.x, -ndc.y);
	reflectTexcoords += totalDistortion;
	reflectTexcoords.x = clamp(reflectTexcoords.x, 0.001, 0.999);
	reflectTexcoords.y = clamp(reflectTexcoords.y, -0.999, -0.001);
	vec4 reflectColor = texture(u_reflection, reflectTexcoords);
	float refractiveFactor = dot(I, vec3(0.0, 1.0, 0.0));
	refractiveFactor = pow(refractiveFactor, 10.0);

	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec3 shadowColor = shadow(bias, u_viewprojection, mix(cubeColor, vec3(0.0, 0.2, 0.5), 0.01), diffuse, ambientColor);
	vec4 waterColor = mix(reflectColor, refractColor, refractiveFactor);
	o_color = fog(v_pos.xyz, mix(shadowColor, waterColor.xyz, 0.5), 0.5);
}
