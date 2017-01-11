// for now they are the same
uniform float u_time;

vec2 calculateShadowTexcoord(vec2 uv) {
	float offset = cos(u_time / 1000.0) * 0.000125;
	vec2 coord = vec2(uv.x + offset, uv.y + offset);
	return coord;
}

#define CUSTOM_SHADOW_TEXCOORD
#include "world.frag"
