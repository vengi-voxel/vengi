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
uniform vec4 u_materialcolor[32];

out vec3 v_pos;
out vec3 v_color;
out vec3 v_lightpos;
out float v_fogrange;
out float v_viewdistance;
out float v_ambientocclusion;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * vec4(a_pos, 1.0);
	v_pos = pos4.xyz;
	// TODO: use a_pos.w to encode the ao for each vertex - per voxel is not enough
	v_ambientocclusion = clamp(float(a_materialdensity.y) / 255.0, 0.0, 1.0);
	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;

	vec4 noisepos = u_model * vec4(a_pos, 1.0);
	vec3 colornoise = texture(u_texture, noisepos.xz / 256.0 / 10.0).rgb;
	int material = a_materialdensity.x;
	v_color = u_materialcolor[material].rgb;
	v_color = clamp(v_color, 0.0, 1.0);

	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
