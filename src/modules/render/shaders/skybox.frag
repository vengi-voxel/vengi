$in vec3 v_uv;

uniform samplerCube u_cubemap;

layout(location = 0) $out vec4 o_color;

void main() {
	o_color = texture(u_cubemap, v_uv);
}
