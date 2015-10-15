// attributes from the VAOs
in vec3 a_pos;
in ivec2 a_materialdensity;

uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;
uniform vec3 u_diffusecolor;
uniform vec3 u_specularcolor;
uniform vec3 u_lightpos;

flat out int v_material;
flat out int v_density;
out vec3 v_pos;
out vec3 v_diffusecolor;
out vec3 v_specularcolor;
out vec3 v_lightpos;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * vec4(a_pos, 1.0);
	v_pos = vec3(pos4) / pos4.w;
	v_material = a_materialdensity.x;
	v_density = a_materialdensity.y;
	v_diffusecolor = u_diffusecolor;
	v_specularcolor = u_specularcolor;
	v_lightpos = u_lightpos;
	gl_Position = u_projection * modelview * vec4(a_pos, 1.0);
}
