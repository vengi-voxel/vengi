layout(location = 0) in vec2 a_pos;
layout(location = 1) in vec2 a_texcoord;
layout(location = 2) in vec4 a_color;
layout(location = 3) in vec4 a_clip;
out vec2 v_uv;
out vec4 v_color;
out vec2 v_screen;
flat out vec4 v_clip;
void main() {
    gl_Position = vec4(a_pos, 0.0, 1.0);
#ifdef CLIPORIGINUPPERLEFT
    gl_Position.y = -gl_Position.y;
#endif
    v_uv = a_texcoord;
    v_color = a_color;
    v_screen = vec2(a_pos.x * 0.5 + 0.5, 0.5 - a_pos.y * 0.5);
    v_clip = a_clip;
}
