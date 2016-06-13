#include "_calctexcoord.frag"

uniform sampler2D u_pos;
uniform sampler2D u_color;
uniform sampler2D u_norm;

uniform vec3 u_lightpos;
uniform vec3 u_diffuse_color;

$out vec4 o_color;

vec4 calcDirectionalLight(vec3 pos, vec3 norm) {
	vec3 lightdir = normalize(u_lightpos - pos);
	vec3 diffuse = u_diffuse_color * clamp(dot(norm, lightdir), 0.0, 1.0) * 0.8;
	vec3 ambient = vec3(0.2);
	vec3 lightvalue = diffuse + ambient;
	return vec4(lightvalue, 1.0);
}

void main(void) {
	vec2 uv    = calcTexCoord();
	vec3 pos   = $texture2D(u_pos, uv).xyz;
	vec3 color = $texture2D(u_color, uv).xyz;
	vec3 norm  = normalize($texture2D(u_norm, uv).xyz);

	o_color = vec4(color, 1.0) * calcDirectionalLight(pos, norm);
}
