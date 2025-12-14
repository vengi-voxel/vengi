std = "lua54"
max_line_length = false
allow_defined = true
ignore = {
	"131", -- unused global variable
	"212", -- unused argument
	"431", -- shadowing upvalue
}

globals = {
	"math",
	"error",

	-- "arguments",
	"main",

	"g_cmd",
	"g_http",
	"g_log",
	"g_import",
	"g_noise",
	"g_palette",
	"g_region",
	"g_scenegraph",
	"g_shape",
	"g_var",

	"g_bvec2",
	"g_bvec3",
	"g_bvec4",
	"g_dvec2",
	"g_dvec3",
	"g_dvec4",
	"g_ivec2",
	"g_ivec3",
	"g_ivec4",
	"g_vec2",
	"g_vec3",
	"g_vec4",
	"g_quat",
	"g_sys",
	"g_io",
	"g_algorithm",

	"tracy"
}

read_globals = {
	"arguments",
	"main",
}
