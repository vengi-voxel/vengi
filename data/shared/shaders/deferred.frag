$in vec3 v_pos;
$in vec4 v_color;
$in vec4 v_worldpos;

$out vec3 o_pos;
$out vec3 o_color;
$out vec3 o_norm;

void main(void) {
	vec3 fdx = dFdx(v_pos.xyz);
	vec3 fdy = dFdy(v_pos.xyz);
	vec3 normal = normalize(cross(fdx, fdy));

	o_pos = v_pos;
	o_color = v_color;
	o_norm = normal;
}
