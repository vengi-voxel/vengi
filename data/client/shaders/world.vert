// attributes from the VAOs
in vec3 a_pos;
in ivec2 a_materialdensity;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_lightpos;
uniform float u_fogrange;
uniform float u_viewdistance;
uniform sampler2D u_texture;

out vec3 v_pos;
out vec3 v_color;
out vec3 v_lightpos;
out float v_fogrange;
out float v_viewdistance;
out float v_ambientocclusion;

vec3 materialColor[8] = vec3[](
	vec3(0.0,   0.0,   0.0),		// air
	vec3(0.419, 0.258, 0.149),		// dirt
	vec3(0.427, 0.776, 0.007),		// grass
	vec3(0.75,  0.75,  0.9),		// clouds
	vec3(1.0,   0.0,   0.0),		// water
	vec3(0.0,   0.5,   0.0),		// leaves
	vec3(0.419, 0.258, 0.149),		// trunk
	vec3(0.75,  0.75,  0.9)			// clouds
);

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * vec4(a_pos, 1.0);
	v_pos = vec3(pos4) / pos4.w;
	v_ambientocclusion = clamp(float(a_materialdensity.y) / 255.0, 0.0, 1.0);
	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;

	vec4 noisepos = u_model * vec4(a_pos, 1.0);
	vec3 colornoise = vec3(texture(u_texture, noisepos.xz / 256.0 / 10.0));
	v_color = materialColor[a_materialdensity.x] * colornoise;
	v_color = clamp(v_color, 0.0, 1.0);

	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
