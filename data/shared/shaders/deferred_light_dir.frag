#include "_calctexcoord.frag"

uniform sampler2D u_pos;
uniform sampler2D u_color;
uniform sampler2D u_norm;

uniform vec3 u_lightdir;
uniform vec3 u_diffuse_color;

$out vec4 o_color;

vec3 calcDirectionalLight(vec3 norm) {
	vec3 diffuse = u_diffuse_color * clamp(dot(norm, u_lightdir), 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = diffuse + ambient;
	return lightvalue;
}

void main(void) {
	vec2 uv    = calcTexCoord();
	vec3 pos   = $texture2D(u_pos, uv).xyz;
	vec3 color = $texture2D(u_color, uv).xyz;
	vec3 norm  = normalize($texture2D(u_norm, uv).xyz);

	// cl_gamma is a cvar
	vec3 finalGamma = vec3(1.0 / cl_gamma);
	vec3 finalColor = color * calcDirectionalLight(norm);
	o_color = vec4(pow(finalColor, finalGamma), 1.0);
}
