// attributes from the VAOs
in vec3 a_pos;
in int a_ao;
in int a_material;

uniform mat4 u_model;
uniform mat4 u_view;
uniform mat4 u_projection;
uniform vec3 u_lightpos;
uniform vec3 u_diffuse_color;
uniform float u_fogrange;
uniform float u_viewdistance;
uniform sampler2D u_texture;
uniform vec4 u_materialcolor[32];
uniform float u_debug_color;

out vec3 v_pos;
out vec4 v_color;
out vec3 v_lightpos;
out vec3 v_diffuse_color;
out vec3 v_fogcolor;
out float v_fogrange;
out float v_viewdistance;
out float v_ambientocclusion;
out float v_debug_color;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = u_model * vec4(a_pos, 1.0);
	v_pos = pos4.xyz;

	//vec3 aocolor[4] = vec3[](
	//	vec3(0.0, 0.0, 0.0),
	//	vec3(0.0, 0.0, 1.0),
	//	vec3(1.0, 0.0, 0.0),
	//	vec3(1.0, 1.0, 1.0));
	v_ambientocclusion = (float(a_ao) + 1.0) / 4.0;
	const float aovalues[] = float[](0.15, 0.6, 0.8, 1.0);
	v_ambientocclusion = aovalues[a_ao];

	v_fogrange = u_fogrange;
	v_viewdistance = u_viewdistance;
	v_lightpos = u_lightpos;
	v_diffuse_color = u_diffuse_color;
	v_debug_color = u_debug_color;

	vec3 materialColor = u_materialcolor[a_material].rgb;
	vec3 colornoise = texture(u_texture, abs(pos4.xz) / 256.0 / 10.0).rgb;
	v_color = vec4(materialColor * colornoise * 1.8, u_materialcolor[a_material].a);
	v_color = clamp(v_color, 0.0, 1.0);

	// use the air color as fog color, too
	v_fogcolor = u_materialcolor[0].rgb;

	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
