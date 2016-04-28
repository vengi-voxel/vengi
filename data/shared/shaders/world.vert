// attributes from the VAOs
in vec3 a_pos;
in int a_material;

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
out vec3 v_fogcolor;
out float v_fogrange;
out float v_viewdistance;
out float v_ambientocclusion;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = u_model * vec4(a_pos, 1.0);
	v_pos = pos4.xyz;
	// TODO: use a_pos.w to encode the ao for each vertex - per voxel is not enough
	v_ambientocclusion = 1.0; //clamp(float(a_materialdensity.y) / 255.0, 0.0, 1.0);
	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;

	vec4 noisepos = u_model * vec4(a_pos, 1.0);
	vec3 colornoise = texture(u_texture, abs(noisepos.xz) / 256.0 / 10.0).rgb;
	v_color = u_materialcolor[a_material].rgb * colornoise * 1.8;
	v_color = clamp(v_color, 0.0, 1.0);

	// use the air color as fog color, too
	v_fogcolor = u_materialcolor[0].rgb;

	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
