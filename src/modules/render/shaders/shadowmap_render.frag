$out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_near;
uniform float u_far;
uniform float u_cascade;
uniform sampler2DArray u_shadowmap;

float linearizedDepth(float depth) {
	return 2.0 * u_near * u_far / (u_far + u_near - (2.0 * depth - 1.0) * (u_far - u_near));
}

void main() {
	vec3 depth = $texture2D(u_shadowmap, vec3(v_texcoord, u_cascade)).xyz;
	o_color = vec4(vec3(linearizedDepth(depth.r)), 1.0);
}
