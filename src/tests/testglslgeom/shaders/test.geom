layout(points) in;
layout(line_strip, max_vertices = 64) out;

$in vec3 g_color[];
$out vec3 v_color;

uniform int u_sides;
uniform float u_radius;
uniform mat4 u_projection;

const float PI = 3.1415926;

void main() {
	v_color = g_color[0];

	float delta = PI * 2.0 / float(u_sides);
	float ang = 0.0f;
	for (int i = 0; i <= u_sides; i++) {
		vec4 offset = vec4(cos(ang) * u_radius, -sin(ang) * u_radius, 0.0, 0.0);
		gl_Position = u_projection * (gl_in[0].gl_Position + offset);
		ang += delta;
		EmitVertex();
	}

	EndPrimitive();
}
