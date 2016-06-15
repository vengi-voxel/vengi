$out vec4 o_color;
$in vec2 v_texcoord;

#ifdef PERSPECTIVE
uniform float u_nearplane;
uniform float u_farplane;

float linearizeDepth(float depth)
{
	float z = depth * 2.0 - 1.0;
	return (2.0 * u_nearplane * u_farplane) / (u_farplane + u_nearplane - z * (u_farplane - u_nearplane));
}
#endif

uniform sampler2D u_shadowmap;

void main()
{
	float v = $texture2D(u_shadowmap, v_texcoord).r;
#ifdef PERSPECTIVE
	o_color = vec4(vec3(linearizeDepth(depthValue) / u_farplane), 1.0);
#else
	// orthographic
	o_color = vec4(vec3(v), 1.0);
#endif
}
