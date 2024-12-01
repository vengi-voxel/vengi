// https://dev.to/javiersalcedopuyo/simple-infinite-grid-shader-5fah
#extension GL_ARB_shading_language_420pack : enable

layout(std140, binding = 0) uniform u_uniformblock {
	vec3 camera_pos;
	mat4 view;
	mat4 proj;
};

#include "planeconstant.glsl"

// Vertex input and output
layout(location = 0) in uint a_vertex_id;

out VertexOut {
	vec4 position;
	vec3 camera_pos;
	vec2 coords;
} v_out;

// Vertex shader
void main() {
	vec4 world_pos = positions[a_vertex_id];
	world_pos.xyz *= grid_size;
	world_pos.xz += camera_pos.xz; // Make the quad follows the camera for "infinity"

	v_out.position = proj * view * world_pos;
	v_out.camera_pos = camera_pos;
	v_out.coords = world_pos.xz;

	gl_Position = v_out.position;
}
