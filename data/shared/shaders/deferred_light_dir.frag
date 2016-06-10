#include "_calctexcoord.frag"

uniform sampler2D u_pos;
uniform sampler2D u_color;
uniform sampler2D u_norm;

$out vec4 o_color;

vec4 calcDirectionalLight(vec3 pos, vec3 norm) {
	// TODO:
	return vec4(1.0);
}

void main(void) {
	vec2 uv    = calcTexCoord();
	vec3 pos   = $texture2D(u_pos, uv).xyz;
	vec3 color = $texture2D(u_color, uv).xyz;
	vec3 norm  = normalize($texture2D(u_norm, uv).xyz);

	o_color = vec4(color, 1.0) * calcDirectionalLight(pos, norm);
}
