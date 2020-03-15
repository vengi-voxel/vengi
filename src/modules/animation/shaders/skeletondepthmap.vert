/**
 * @brief Shader to fill the bound depthmap with the depth values
 * @note Must have the same attributes as the skeleton.vert shader!
 */

$in vec3 a_pos;
$in uint a_bone_id;
//$in uint a_color_index;
//$in uint a_ambient_occlusion;

#define MAX_BONES 16
$constant MaxBones MAX_BONES
uniform mat4 u_bones[MAX_BONES];

uniform mat4 u_viewprojection;
uniform mat4 u_model;

void main(void)
{
	vec4 worldpos = u_model * u_bones[a_bone_id] * vec4(a_pos, 1.0);
	gl_Position = u_viewprojection * worldpos;
}
