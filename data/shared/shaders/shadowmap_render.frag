$out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_nearplane;
uniform float u_farplane;
uniform sampler2D u_shadowmap;

float linearizeDepth(float z)
{
	return (2.0 * u_nearplane) / (u_farplane + u_nearplane - z * (u_farplane - u_nearplane));
}

void main()
{
	float v = $texture2D(u_shadowmap, v_texcoord).x;
	if (v_texcoord.x < 0.5) {
		o_color = vec4(vec3(linearizeDepth(v)), 1.0);
	} else {
		o_color = vec4(vec3(v), 1.0);
	}
}
