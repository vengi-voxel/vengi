/**
 * @file
 */

#pragma once

#include "TexturedTri.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "io/Archive.h"
#include "voxel/ChunkMesh.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Convert the volume data into a mesh
 *
 * http://research.michael-schwarz.com/publ/2010/vox/
 * http://research.michael-schwarz.com/publ/files/vox-siga10.pdf
 * https://citeseerx.ist.psu.edu/viewdoc/summary?doi=10.1.1.12.6294
 */
class MeshFormat : public Format {
public:
	static constexpr const uint8_t FillColorIndex = 2;
	using TriCollection = core::DynamicArray<voxelformat::TexturedTri, 512>;

	/**
	 * Subdivide until we brought the triangles down to the size of 1 or smaller
	 */
	static void subdivideTri(const voxelformat::TexturedTri &tri, TriCollection &tinyTris);
	static bool calculateAABB(const TriCollection &tris, glm::vec3 &mins, glm::vec3 &maxs);
	/**
	 * @brief Checks whether the given triangles are axis aligned - usually true for voxel meshes
	 */
	static bool isVoxelMesh(const TriCollection &tris);

protected:
	/**
	 * @brief Color flatten factor - see @c PosSampling::getColor()
	 */
	uint8_t _flattenFactor;
	bool _weightedAverage = true;

	struct PointCloudVertex {
		glm::vec3 position{0.0f};
		core::RGBA color{0, 0, 0, 255};
	};

	struct MeshExt {
		MeshExt(voxel::ChunkMesh *mesh, const scenegraph::SceneGraphNode &node, bool applyTransform);
		voxel::ChunkMesh *mesh;
		core::String name;
		bool applyTransform = false;

		glm::vec3 size{0.0f};
		glm::vec3 pivot{0.0f};
		int nodeId = -1;
	};
	using Meshes = core::DynamicArray<MeshExt>;
	virtual bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
							const Meshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
							const glm::vec3 &scale = glm::vec3(1.0f), bool quad = false, bool withColor = true,
							bool withTexCoords = true) = 0;

	static MeshExt *getParent(const scenegraph::SceneGraph &sceneGraph, Meshes &meshes, int nodeId);
	static glm::vec3 getInputScale();

	/**
	 * @brief Voxelizes the input mesh
	 *
	 * Convert your input mesh into @c voxelformat::TexturedTri instances and use the methods of this class to help
	 * voxelizing those.
	 * @see voxelizeNode()
	 */
	virtual bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	bool voxelizePointCloud(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
							const core::DynamicArray<PointCloudVertex> &vertices) const;

	/**
	 * @return A particular uv value for the palette image for the given color index
	 * @sa image::Image::uv()
	 */
	static glm::vec2 paletteUV(int colorIndex);

	/**
	 * @see voxelizeGroups()
	 */
	int voxelizeNode(const core::String &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph,
					 const TriCollection &tris, int parent = 0, bool resetOrigin = true) const;
	int voxelizeNode(const core::String &name, scenegraph::SceneGraph &sceneGraph, const TriCollection &tris,
					 int parent = 0, bool resetOrigin = true) const {
		return voxelizeNode("", name, sceneGraph, tris, parent, resetOrigin);
	}

	/**
	 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
	 * during voxelization
	 */
	struct PosSamplingEntry {
		inline PosSamplingEntry(float _area, core::RGBA _color) : area(_area), color(_color) {
		}
		float area;
		core::RGBA color;
	};

	/**
	 * @brief Weighted color entry for a position for averaging the voxel color value over all positions that were found
	 * during voxelization
	 */
	struct PosSampling {
		core::DynamicArray<PosSamplingEntry> entries;
		inline PosSampling(float area, core::RGBA color) {
			entries.emplace_back(area, color);
		}
		core::RGBA getColor(uint8_t flattenFactor, bool weightedAverage) const;
	};

	/**
	 * @brief A map with positions and colors that can get averaged from the input triangles
	 */
	typedef core::Map<glm::ivec3, PosSampling, 64, glm::hash<glm::ivec3>> PosMap;

	/**
	 * @brief Convert the given input triangles into a list of positions to place the voxels at
	 *
	 * @param[in] tris The triangles to voxelize
	 * @param[out] posMap The PosMap instance to fill with positions and colors
	 * @sa transformTrisAxisAligned()
	 * @sa voxelizeTris()
	 */
	static void transformTris(const voxel::Region &region, const TriCollection &tris, PosMap &posMap);
	/**
	 * @brief Convert the given input triangles into a list of positions to place the voxels at. This version is for
	 * aligned aligned triangles. This is usually the case for meshes that were exported from voxels.
	 *
	 * @param[in] tris The triangles to voxelize
	 * @param[out] posMap The @c PosMap instance to fill with positions and colors
	 * @sa transformTris()
	 * @sa voxelizeTris()
	 */
	static void transformTrisAxisAligned(const voxel::Region &region, const TriCollection &tris, PosMap &posMap);
	/**
	 * @brief Convert the given @c PosMap into a volume
	 *
	 * @note The @c PosMap values can get calculated by @c transformTris() or @c transformTrisAxisAligned()
	 * @param[in] posMap The @c PosMap values with voxel positions and colors
	 * @param[in] fillHollow Fill the inner parts of a voxel volume
	 * @param[out] node The node to create the volume in
	 */
	void voxelizeTris(scenegraph::SceneGraphNode &node, const PosMap &posMap, bool fillHollow) const;

public:
	static core::String lookupTexture(const core::String &meshFilename, const core::String &in);

	MeshFormat();
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	enum VoxelizeMode { HighQuality = 0, Fast = 1 };
};

} // namespace voxelformat
