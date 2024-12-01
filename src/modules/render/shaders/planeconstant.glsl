const float grid_size = 100.0;
const float cell_size = 1.0;
const float half_cell_size = cell_size * 0.5;
const float cell_line_thickness = 0.005;

const float subcell_size = 0.1;
const float half_subcell_size = subcell_size * 0.5;
const float subcell_line_thickness = 0.001;

const vec4 cell_color = vec4(0.75, 0.75, 0.75, 0.5);
const vec4 subcell_color = vec4(0.5, 0.5, 0.5, 0.5);

const float height_to_fade_distance_ratio = 25.0;
const float min_fade_distance = grid_size * 0.05;
const float max_fade_distance = grid_size * 0.5;

const vec4 positions[4] = vec4[4](
	vec4(-0.5, 0.0,  0.5, 1.0),
	vec4( 0.5, 0.0,  0.5, 1.0),
	vec4(-0.5, 0.0, -0.5, 1.0),
	vec4( 0.5, 0.0, -0.5, 1.0)
);
