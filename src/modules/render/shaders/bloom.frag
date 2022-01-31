layout(location = 0) $out vec4 o_color;
$in vec2 v_texcoord;

uniform sampler2D u_color0;
uniform sampler2D u_color1;

void main() {
	vec4 sceneColor = $texture2D(u_color0, v_texcoord);
	vec4 bloomColor = $texture2D(u_color1, v_texcoord);
	vec4 finalColor = sceneColor + bloomColor;
	if (finalColor.a < 0.0001) {
		discard;
	}
	o_color = finalColor;
}
