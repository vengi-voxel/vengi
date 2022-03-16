/**
 * @file
 */

#pragma once

#include "Format.h"

namespace voxelformat {

/**
 * @brief Convert the volume data into a mesh
 */
class MeshExporter : public Format {
protected:
	struct Tri {
		glm::vec3 vertices[3];
		glm::vec2 uv[3];
		image::ImagePtr texture;
		uint32_t color = 0xFFFFFFFF;

		glm::vec2 centerUV() const {
			return (uv[0] + uv[1] + uv[2]) / 3.0f;
		}

		glm::vec3 center() const {
			return (vertices[0] + vertices[1] + vertices[2]) / 3.0f;
		}

		glm::vec3 mins() const {
			glm::vec3 v;
			for (int i = 0; i < 3; ++i) {
				v[i] = core_min(vertices[0][i], core_min(vertices[1][i], vertices[2][i]));
			}
			return v;
		}

		glm::vec3 maxs() const {
			glm::vec3 v;
			for (int i = 0; i < 3; ++i) {
				v[i] = core_max(vertices[0][i], core_max(vertices[1][i], vertices[2][i]));
			}
			return v;
		}

		uint32_t colorAt(const glm::vec2 &uv) const {
			if (texture) {
				const float w = (float)texture->width();
				const float h = (float)texture->height();
				float x = uv.x * w;
				float y = uv.y * h;
				while (x < 0.0f)
					x += w;
				while (x > w)
					x -= w;
				while (y < 0.0f)
					y += h;
				while (y > h)
					y -= h;

				const int xint = (int)glm::round(x - 0.5f);
				const int yint = texture->height() - (int)glm::round(y - 0.5f) - 1;
				const uint8_t *ptr = texture->at(xint, yint);
				return *(const uint32_t *)ptr;
			}
			return color;
		}

		// Sierpinski gasket with keeping the middle
		void subdivide(Tri out[4]) const {
			const glm::vec3 midv[]{glm::mix(vertices[0], vertices[1], 0.5f), glm::mix(vertices[1], vertices[2], 0.5f),
								   glm::mix(vertices[2], vertices[0], 0.5f)};
			const glm::vec2 miduv[]{glm::mix(uv[0], uv[1], 0.5f), glm::mix(uv[1], uv[2], 0.5f),
									glm::mix(uv[2], uv[0], 0.5f)};

			// the subdivided new three triangles
			out[0] = Tri{{vertices[0], midv[0], midv[2]}, {uv[0], miduv[0], miduv[2]}, texture, color};
			out[1] = Tri{{vertices[1], midv[1], midv[0]}, {uv[1], miduv[1], miduv[0]}, texture, color};
			out[2] = Tri{{vertices[2], midv[2], midv[1]}, {uv[2], miduv[2], miduv[1]}, texture, color};
			// keep the middle
			out[3] = Tri{{midv[0], midv[1], midv[2]}, {miduv[0], miduv[1], miduv[2]}, texture, color};
		}
	};

	/**
	 * Subdivide until we brought the triangles down to the size of 1 or smaller
	 */
	static void subdivideTri(const Tri &tri, core::DynamicArray<Tri> &tinyTris);

	static void voxelizeTris(voxel::RawVolume *volume, const core::DynamicArray<Tri> &tinyTris);

	struct MeshExt {
		MeshExt(voxel::Mesh *mesh, const SceneGraphNode &node, bool applyTransform);
		voxel::Mesh *mesh;
		core::String name;
		bool applyTransform = false;

		SceneGraphTransform transform;
		glm::vec3 size{0.0f};
		int nodeId = -1;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, io::SeekableWriteStream &stream,
							const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
							bool withTexCoords = true) = 0;

	static glm::vec3 getScale();
	// checks if the winding needs flipping if the scale values are negative
	static bool flipWinding(const glm::vec3 &scale);

public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &file, SceneGraph &sceneGraph) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override;
};

} // namespace voxel
