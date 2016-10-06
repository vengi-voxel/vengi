#if cl_shadowmap == 1
uniform mat4 u_light;
uniform float u_farplanes[2];
uniform mat4 u_shadowmapmatrix1;
uniform mat4 u_shadowmapmatrix2;

$out vec4 v_lightspacepos;
$out vec4 v_texcoord1;
$out vec4 v_texcoord2;
#endif
