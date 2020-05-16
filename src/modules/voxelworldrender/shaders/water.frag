$in vec3 v_pos;
$in vec4 v_clipspace;
$in vec2 v_uv;
$in vec3 v_camdist;

uniform mat4 u_viewprojection;
uniform float u_time;
uniform float u_near;
uniform float u_far;
uniform samplerCube u_cubemap;
uniform sampler2D u_distortion;
uniform sampler2D u_normalmap;
#if cl_water
uniform sampler2D u_reflection;
uniform sampler2D u_refraction;
uniform sampler2D u_depthmap;
#endif

#include "_fog.frag"
#include "_shadowmap.frag"
#include "_daynight.frag"
#include "_checker.frag"

uniform mediump vec3 u_lightdir;
uniform lowp vec3 u_diffuse_color;
uniform lowp vec3 u_ambient_color;
layout(location = 0) $out vec4 o_color;

const float c_wavestrength = 0.01;
const float c_reflectivity = 0.5;
const float c_shinedamper = 20.0;
const float c_wavesizefactor = 10.0;

float depthToDistance(float depth) {
	float ndcz = depth * 2.0 - 1.0;
	return (2.0 * u_near * u_far) / (u_far + u_near - ndcz * (u_far - u_near));
}

vec2 clipSpaceToTexCoords(vec4 clipSpace){
	vec2 ndc = (clipSpace.xy / clipSpace.w);
	vec2 texCoords = ndc / 2.0 + 0.5;
	return clamp(texCoords, 0.002, 0.998);
}

void main(void) {
	float moveFactor = fract(u_time / 40000.0);
	vec2 distortedTexCoords = $texture2D(u_distortion, c_wavesizefactor * vec2(v_uv.x + moveFactor, v_uv.y)).rg * 0.01;
	distortedTexCoords = v_uv + vec2(distortedTexCoords.x, distortedTexCoords.y + moveFactor);

#if cl_water
	vec2 ndc = clipSpaceToTexCoords(v_clipspace);
	float floorDistance = depthToDistance($texture2D(u_depthmap, ndc).r);
	float waterDistance = depthToDistance(gl_FragCoord.z);
	float depthWater = floorDistance - waterDistance;
	distortedTexCoords *= clamp(depthWater, 0.0, 1.0) ;
#endif

	// b component of the normal map is upward (so our y)
	// r and g are mapped onto x and z and converted to allow negative values
	vec4 normalMapColor = $texture2D(u_normalmap, distortedTexCoords);
	vec3 normal = vec3(normalMapColor.r * 2.0 - 1.0, normalMapColor.b * 3.0, normalMapColor.g * 2.0 - 1.0);
	normal = normalize(normal);

	vec3 I = normalize(v_camdist);
	vec3 R = reflect(I, normal);
	vec3 cubeColor = texture(u_cubemap, R).rgb;
	float ndotl1 = dot(normal, u_lightdir);
	float ndotl2 = dot(normal, -u_lightdir);
	vec3 diffuse = u_diffuse_color * max(0.0, max(ndotl1, ndotl2));
	float specular = max(dot(R, I), 0.0);
	specular = pow(specular, c_shinedamper);
	vec3 ambientColor = dayTimeColor(u_ambient_color, u_time);
	vec3 lightColor = ambientColor * specular * c_reflectivity;

	vec2 totalDistortion = ($texture2D(u_distortion, distortedTexCoords).rg * 2.0 - 1.0) * c_wavestrength;

#if cl_water
	vec2 refractTexcoords = ndc;
	refractTexcoords += totalDistortion;
	refractTexcoords = clamp(refractTexcoords, 0.001, 0.999);
	vec4 refractColor = $texture2D(u_refraction, refractTexcoords);

	vec2 reflectTexcoords = vec2(ndc.x, 1.0 - ndc.y);
	reflectTexcoords += totalDistortion;
	vec4 reflectColor = $texture2D(u_reflection, reflectTexcoords);
	float refractiveFactor = dot(I, normal);
	refractiveFactor = clamp(pow(refractiveFactor, 4.0), 0.0, 1.0);
	vec4 waterColor = mix(reflectColor, refractColor, refractiveFactor);
#else
	vec4 waterColor = vec4(0.0, 0.0, 0.0, 1.0);
#endif

	float bias = max(0.05 * (1.0 - ndotl1), 0.005);
	vec4 lightspacepos = vec4(v_lightspacepos.x + totalDistortion.x, v_lightspacepos.y, v_lightspacepos.z + totalDistortion.y, 1.0);
	vec3 shadowColor = shadow(lightspacepos, bias, mix(cubeColor, vec3(0.0, 0.2, 0.5), 0.01), diffuse, ambientColor);
	o_color = fog(v_pos.xyz, mix(shadowColor, waterColor.xyz, 0.5), 0.5);
	// add a blue tint and the specular highlights
	const vec3 tint = vec3(0.0, 0.3, 0.5);
	vec3 waterColorTint = tint * checker(v_pos.xz);
	o_color = mix(o_color, vec4(waterColorTint, 1.0), 0.2) + vec4(lightColor, 0.0);
#if cl_water
	o_color.a = clamp(depthWater / 5.0, 0.0, 1.0);
#else
	o_color.a = 0.6;
#endif
}
