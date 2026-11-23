layout(binding = 0) uniform sampler2D u_texture;
in vec2 v_uv;
in vec4 v_color;
in vec2 v_screen;
flat in vec4 v_clip;
layout(location = 0) out vec4 o_color;
void main() {
    if (v_screen.x < v_clip.x || v_screen.y < v_clip.y || v_screen.x >= v_clip.z || v_screen.y >= v_clip.w) discard;
    // Canvas sampling uses the base level, including for the atlas's single white texel.
    o_color = textureLod(u_texture, v_uv, 0.0) * v_color;
}
