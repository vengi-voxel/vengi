// attributes from the VAOs
in vec3 a_pos;
in vec3 a_norm;
in vec2 a_texcoords;

uniform float u_wavetime;
uniform float u_wavewidth;
uniform float u_waveheight;
uniform mat4 u_projection;
uniform mat4 u_model;
uniform mat4 u_view;
uniform vec3 u_diffusecolor;
uniform vec3 u_specularcolor;
uniform vec3 u_lightpos;

out vec3 v_pos;
out vec3 v_norm;
out vec2 v_texcoords;
out vec3 v_diffusecolor;
out vec3 v_specularcolor;
out vec3 v_lightpos;

void main(void) {
	mat4 modelview = u_view * u_model;
	vec4 pos4 = modelview * vec4(a_pos, 1.0);
	v_pos = vec3(pos4) / pos4.w;
	v_norm = a_norm;
	v_texcoords = a_texcoords;
	v_diffusecolor = u_diffusecolor;
	v_specularcolor = u_specularcolor;
	v_lightpos = u_lightpos;
	float posw = sin(u_wavewidth * a_pos.x + u_wavetime) * cos(u_wavewidth * a_pos.y + u_wavetime) * u_waveheight;
	gl_Position = u_projection * modelview * vec4(a_pos, posw);
}
