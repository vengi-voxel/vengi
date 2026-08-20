// Per-draw instance data for multi-draw-indirect. Binding 3 is free
// (0=vert UBO, 1=frag UBO, 2=shadow sampler). Extensions are injected by Shader::getSource.
// Prefer gl_DrawIDARB so MDI works below GLSL 460 when ARB_shader_draw_parameters is present.
struct DrawInstance {
	mat4 model;
	int gray;
	int locked;
	float opacity;
	int pad;
};
layout(std430, binding = 3) readonly buffer DrawInstanceBuffer {
	DrawInstance u_drawinstances[];
};
#if __VERSION__ >= 460
#define VENGIDRAWID gl_DrawID
#else
#define VENGIDRAWID gl_DrawIDARB
#endif
