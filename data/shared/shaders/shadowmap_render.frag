$out vec4 o_color;
$in vec2 v_texcoord;

uniform float u_nearplane;
uniform float u_farplane;
uniform sampler2DShadow u_shadowmap;

float linearizeDepth(float z)
{
	return (2.0 * u_nearplane) / (u_farplane + u_nearplane - z * (u_farplane - u_nearplane));
}

void main()
{
	// TODO: this doesn't work - 1.0 as z coordinate is just to let it compile
	float v = $shadow2D(u_shadowmap, vec3(v_texcoord, 1.0));
	if (v_texcoord.x < 0.5) {
		o_color = vec4(vec3(linearizeDepth(v)), 1.0);
	} else {
		o_color = vec4(vec3(v), 1.0);
	}
}
