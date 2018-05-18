$out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_near;
uniform float u_far;
uniform float u_cascade;
uniform sampler2DArray u_shadowmap;

void main() {
	vec3 depth = $texture2D(u_shadowmap, vec3(v_texcoord, u_cascade)).xyz;
	vec3 linearizedDepth = (2.0 * u_near) / (u_far + u_near - depth * (u_far - u_near));
	o_color = vec4(linearizedDepth, 1.0);
}
