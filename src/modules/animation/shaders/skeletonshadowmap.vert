/**
 * @brief Shader to fill the bound shadowmap with the depth values
 * @note Must have the same attributes as the skeleton.vert shader!
 */

$in vec3 a_pos;
$in uint a_bone_id;
//$in uint a_color_index;
//$in uint a_ambient_occlusion;

#define MAXBONES 16
$constant MaxBones MAXBONES
uniform mat4 u_bones[MAXBONES];

uniform mat4 u_lightviewprojection;
uniform mat4 u_model;

void main()
{
	vec4 worldpos = u_model * u_bones[a_bone_id] * vec4(a_pos, 1.0f);
	gl_Position = u_lightviewprojection * worldpos;
}
