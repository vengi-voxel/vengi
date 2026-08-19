// Per-draw instance data for multi-draw-indirect. Binding 3 is free
// (0=vert/block UBO, 1=frag UBO, 2=shadow sampler). Extensions are injected by Shader::getSource.
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
mat4 getModelMatrix() { return u_drawinstances[VENGIDRAWID].model; }
int getGrayFlag() { return u_drawinstances[VENGIDRAWID].gray; }
int getLockedFlag() { return u_drawinstances[VENGIDRAWID].locked; }
float getOpacity() { return u_drawinstances[VENGIDRAWID].opacity; }
