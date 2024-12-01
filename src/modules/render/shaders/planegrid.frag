// https://dev.to/javiersalcedopuyo/simple-infinite-grid-shader-5fah

#extension GL_ARB_shading_language_420pack : enable

in VertexOut {
	vec4 position;
	vec3 camera_pos;
	vec2 coords;
} v_in;

out vec4 o_color;

#include "planeconstant.glsl"

// Fragment shader
void main() {
	// Offset coordinates for grid alignment
	vec2 cell_coords = mod(v_in.coords + half_cell_size, cell_size);
	vec2 subcell_coords = mod(v_in.coords + half_subcell_size, subcell_size);

	// Distance to edges
	vec2 distance_to_cell = abs(cell_coords - half_cell_size);
	vec2 distance_to_subcell = abs(subcell_coords - half_subcell_size);

	// Line thickness adjustment
	vec2 d = fwidth(v_in.coords);
	float adjusted_cell_line_thickness = 0.5 * (cell_line_thickness + d.x);
	float adjusted_subcell_line_thickness = 0.5 * (subcell_line_thickness + d.x);

	vec4 color = vec4(0.0);
	if (any(lessThan(distance_to_subcell, vec2(adjusted_subcell_line_thickness)))) {
		color = subcell_color;
	}
	if (any(lessThan(distance_to_cell, vec2(adjusted_cell_line_thickness)))) {
		color = cell_color;
	}

	// Fade out effect
	float opacity_falloff;
	{
		float distance_to_camera = length(v_in.coords - v_in.camera_pos.xz);
		float fade_distance = abs(v_in.camera_pos.y) * height_to_fade_distance_ratio;
		fade_distance = clamp(fade_distance, min_fade_distance, max_fade_distance);
		opacity_falloff = smoothstep(1.0, 0.0, distance_to_camera / fade_distance);
	}

	o_color = color * opacity_falloff;
}
