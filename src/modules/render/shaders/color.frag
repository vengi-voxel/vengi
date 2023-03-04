$in vec4 v_color;
layout(location = 0) $out vec4 o_color;
layout(location = 1) $out vec4 o_glow;

void main()
{
	o_color = v_color;
	o_glow = vec4(0.0, 0.0, 0.0, 0.0);
}
