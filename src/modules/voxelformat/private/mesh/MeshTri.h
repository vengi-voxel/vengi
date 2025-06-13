/**
 * @file
 */

#pragma once

#include "MeshMaterial.h"
#include "math/Tri.h"

namespace voxelformat {

class MeshTri : public math::Tri {
private:
	glm::vec2 uv[3]{};
public:
	MeshTri() = default;
	inline MeshTri(const glm::vec3 (&_vertices)[3], const glm::vec2 (&_uv)[3], const MeshMaterialPtr &_material,
					   const core::RGBA (&_color)[3])
		: math::Tri::Tri(_vertices, _color) {
		setUVs(_uv[0], _uv[1], _uv[2]);
		this->material = _material;
	}

	virtual ~MeshTri() = default;

	MeshMaterialPtr material;

	void setUVs(const glm::vec2 &uv1, const glm::vec2 &uv2, const glm::vec2 &uv3);
	[[nodiscard]] glm::vec2 centerUV() const;

	/**
	 * @return @c false if the given position is not within the triangle area. The value of uv should not be used in
	 * this case.
	 */
	[[nodiscard]] bool calcUVs(const glm::vec3 &pos, glm::vec2 &uv) const;
	[[nodiscard]] core::RGBA colorAt(const glm::vec2 &uv, bool originUpperLeft = false) const;
	[[nodiscard]] core::RGBA extracted() const;
	[[nodiscard]] core::RGBA centerColor() const;
	[[nodiscard]] core::RGBA blendedColor() const;
};
static_assert(sizeof(MeshTri) == 88, "MeshTri size unexpected");

// Sierpinski gasket with keeping the middle
template<class MESHTRI>
inline void subdivide(const MESHTRI &in, MESHTRI out[4]) {
	const glm::vec3 midv[]{glm::mix(in.vertices[0], in.vertices[1], 0.5f), glm::mix(in.vertices[1], in.vertices[2], 0.5f),
						   glm::mix(in.vertices[2], in.vertices[0], 0.5f)};
	const glm::vec2 miduv[]{glm::mix(in.uv[0], in.uv[1], 0.5f), glm::mix(in.uv[1], in.uv[2], 0.5f), glm::mix(in.uv[2], in.uv[0], 0.5f)};
	const core::RGBA midc[]{core::RGBA::mix(in.color[0], in.color[1]), core::RGBA::mix(in.color[1], in.color[2]),
							core::RGBA::mix(in.color[2], in.color[0])};

	// the subdivided new three triangles
	out[0] = MESHTRI{
		{in.vertices[0], midv[0], midv[2]}, {in.uv[0], miduv[0], miduv[2]}, in.material, {in.color[0], midc[0], midc[2]}};
	out[1] = MESHTRI{
		{in.vertices[1], midv[1], midv[0]}, {in.uv[1], miduv[1], miduv[0]}, in.material, {in.color[1], midc[1], midc[0]}};
	out[2] = MESHTRI{
		{in.vertices[2], midv[2], midv[1]}, {in.uv[2], miduv[2], miduv[1]}, in.material, {in.color[2], midc[2], midc[1]}};
	// keep the middle
	out[3] =
		MESHTRI{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, in.material, {midc[0], midc[1], midc[2]}};
}

} // namespace voxelformat
