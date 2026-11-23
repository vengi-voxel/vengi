/**
 * @file
 */

#pragma once

#include "MeshTri.h"
#include "PosSampling.h"
#include "core/Common.h"
#include "core/UUID.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicParallelMap.h"
#include "io/Archive.h"
#include "palette/NormalPalette.h"
#include "voxel/ChunkMesh.h"
#include "voxel/Mesh.h"
#include "voxelformat/Format.h"
#include "voxelformat/private/mesh/Mesh.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "core/IProgress.h"
#include <functional>
#include "math/Mat4f.h"
#include "math/Mat3f.h"
#include "math/Vec3i.h"
#include "math/Vec3f.h"
#include "math/Vec2f.h"

namespace palette {
class NormalPaletteLookup;
}

namespace scenegraph {
class SceneGraphTransform;
}

namespace voxelformat {

struct PointCloudVertex {
	math::Vec3f position{0.0f};
	color::RGBA color{0, 0, 0, 255};
};
using PointCloud = core::Buffer<PointCloudVertex, 4096>;
using MeshTriCollection = core::DynamicArray<voxelformat::MeshTri>;
using PosMap = core::DynamicParallelMap<int64_t, PosSampling, 3541, core::privdynamicmap::DefaultHasher,
										core::privdynamicmap::EqualCompare, 32768>;

/**
 * @brief Convert the volume data into a mesh
 *
 * http://research.michael-schwarz.com/publ/2010/vox/
 * http://research.michael-schwarz.com/publ/files/vox-siga10.pdf
 */
class MeshFormat : public Format {
public:
	static constexpr const uint8_t FillColorIndex = 2;

	/**
	 * Subdivide until we brought the triangles down to the size of 1 or smaller
	 */
	static bool subdivideTri(const voxelformat::MeshTri &meshTri, MeshTriCollection &tinyTris, int depth);
	static bool calculateAABB(const MeshTriCollection &tris, math::Vec3f &mins, math::Vec3f &maxs);
	static math::Vec3f getScale();
	/**
	 * @brief Checks whether the given triangles are axis aligned - usually true for voxel meshes
	 */
	static bool isVoxelMesh(const MeshTriCollection &tris);
private:

	/**
	 * @brief Voxelizes a mesh node and adds it to the scene graph.
	 *
	 * This function takes a collection of mesh triangles and voxelizes them into a volume. The resulting
	 * volume is then added as a node to the scene graph. The function supports different voxelization modes
	 * and can handle both axis-aligned and non-axis-aligned meshes. It also supports optional palette creation
	 * and filling of hollow spaces within the volume. Some of the functionality depends on @c core::Var
	 * settings.
	 *
	 * @param uuid The unique identifier for the new node.
	 * @param name The name of the new node.
	 * @param sceneGraph The scene graph to which the new node will be added.
	 * @param tris The collection of mesh triangles to be voxelized.
	 * @param parent The parent node ID in the scene graph. If no parent, pass 0 to attach it to the root node.
	 * @param resetOrigin If true, the origin of the volume will be reset to the lower corner of the region.
	 * @return The id of the newly created node in the scene graph, or InvalidNodeId if voxelization failed.
	 *
	 * @see voxelformat::MeshTri
	 * @see voxelizeGroups()
	 */
	int voxelizeNode(const core::UUID &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph,
					 MeshTriCollection &&tris, const MeshMaterialArray &meshMaterialArray, int parent = 0,
					 bool resetOrigin = true, core::IProgress *progress = nullptr) const;
	void sampleTriToPosMap(PosMap &posMap, const voxel::Region &region, const MeshTri &meshTri,
						   const MeshMaterialArray &meshMaterialArray,
						   palette::NormalPaletteLookup &normalLookup) const;
	void subdivideTriToPosMap(PosMap &posMap, const voxel::Region &region, const MeshTri &meshTri,
							  const MeshMaterialArray &meshMaterialArray,
							  palette::NormalPaletteLookup &normalLookup, int depth) const;
protected:
	/**
	 * @brief Color flatten factor - see @c PosSampling::getColor()
	 */
	bool _weightedAverage = true;

private:
	struct InputBounds {
		int nodeId;
		math::Vec3f origin;
		math::Vec3f mins;
		math::Vec3f maxs;
		core::DynamicArray<math::Vec3f> vertices;
		void calculateAABB(const math::Mat4f &matrix, math::Vec3f &worldMins, math::Vec3f &worldMaxs) const;
	};
	mutable core::DynamicArray<InputBounds> _inputBounds;
	bool _collectInputBounds = false;
	bool _useSceneInputScale = false;
	math::Vec3f _sceneInputScale{1.0f};

	int collectInputBounds(const core::UUID &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph,
						   const Mesh &mesh, int parent, bool resetOrigin) const;
	bool calculateInputBounds(const scenegraph::SceneGraph &sceneGraph, math::Vec3f &mins, math::Vec3f &maxs) const;
	bool prepareInputScale(const core::String &filename, const io::ArchivePtr &archive, const LoadContext &ctx);
	int voxelizeMesh(const core::UUID &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph, Mesh &&mesh,
					 int parent, bool resetOrigin, core::IProgress *progress) const;

protected:
	struct ChunkMeshExt {
		ChunkMeshExt() = default;
		ChunkMeshExt(voxel::ChunkMesh *mesh, const scenegraph::SceneGraphNode &node, bool applyTransform);
		voxel::ChunkMesh *mesh;
		core::String name;
		image::ImagePtr texture;
		bool applyTransform = false;

		math::Vec3f size{0.0f};
		math::Vec3f pivot{0.0f};
		int nodeId = -1;

		math::Mat3f normalMatrix(const scenegraph::SceneGraphTransform &transform, const math::Vec3f &scale) const;
		void visitByMaterial(int materialIndex, const std::function<void(const voxel::Mesh &, voxel::IndexType,
																		voxel::IndexType, voxel::IndexType)> &callback) const;
	};
	using ChunkMeshes = core::DynamicArray<ChunkMeshExt>;
	virtual bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
							const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
							const math::Vec3f &scale = math::Vec3f(1.0f), bool quad = false, bool withColor = true,
							bool withTexCoords = true) = 0;
	virtual bool savePointCloud(const scenegraph::SceneGraph &sceneGraph, const PointCloud &pointCloud,
								const core::String &filename, const io::ArchivePtr &archive,
								const math::Vec3f &scale = math::Vec3f(1.0f), bool withColor = true) const {
		return false;
	}

	static ChunkMeshExt *getParent(const scenegraph::SceneGraph &sceneGraph, ChunkMeshes &meshes, int nodeId);
	math::Vec3f getInputScale(const math::Vec3f &meshMins, const math::Vec3f &meshMaxs) const;
	math::Vec3f sceneInputScale() const {
		return _collectInputBounds ? math::Vec3f(1.0f) : _useSceneInputScale ? _sceneInputScale : getScale();
	}
	bool collectingInputBounds() const {
		return _collectInputBounds;
	}
	bool savePointClouds(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const math::Vec3f &scale = math::Vec3f(1.0f),
							 bool withColor = true) const;

	/**
	 * @brief Voxelizes the input mesh
	 *
	 * Convert your input mesh into @c voxelformat::MeshTri instances and use the methods of this class to help
	 * voxelizing those.
	 * @see voxelizeNode()
	 */
	virtual bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx);
	/**
	 * @brief Voxelizes a point cloud and adds it to the scene graph.
	 *
	 * This function takes a collection of point cloud vertices and voxelizes them into a volume. The resulting
	 * volume is then added as a node to the scene graph. The function scales the input vertices, calculates the
	 * bounding box, and creates voxels for each point in the point cloud. It also supports setting the size of
	 * each point in the voxel grid.
	 *
	 * @param filename The name of the file from which the point cloud was loaded.
	 * @param sceneGraph The scene graph to which the new node will be added.
	 * @param vertices The collection of point cloud vertices to be voxelized.
	 * @return The node id of the newly created node in the scene graph, or InvalidNodeId if voxelization failed.
	 */
	int voxelizePointCloud(const core::String &filename, scenegraph::SceneGraph &sceneGraph,
						   PointCloud &&vertices) const;
	void convertToScaledTris(MeshTriCollection &tris, const core::DynamicArray<MeshVertex> &vertices,
							 voxel::IndexArray &indices) const;
	void triangulatePolygons(const core::DynamicArray<voxel::IndexArray> &polygons,
							 const core::DynamicArray<MeshVertex> &vertices, voxel::IndexArray &indices) const;
	/**
	 * Import a mesh, collecting bounds during measurement and voxelizing during the actual import.
	 */
	int importMesh(const core::String &name, scenegraph::SceneGraph &sceneGraph, Mesh &&mesh, int parent = 0,
					 bool resetOrigin = true, core::IProgress *progress = nullptr) const {
		return importMesh(core::UUID(), name, sceneGraph, core::move(mesh), parent, resetOrigin, progress);
	}
	int importMesh(const core::UUID &uuid, const core::String &name, scenegraph::SceneGraph &sceneGraph, Mesh &&mesh,
					 int parent = 0, bool resetOrigin = true, core::IProgress *progress = nullptr) const;

	/**
	 * @return A particular uv value for the palette image for the given color index
	 * @sa image::Image::uv()
	 */
	static math::Vec2f paletteUV(int colorIndex);

	size_t simplify(voxel::IndexArray &indices, const core::DynamicArray<MeshVertex> &vertices) const;
	void simplifyPointCloud(PointCloud &vertices) const;

	/**
	 * @brief A map with positions and colors that can get averaged from the input triangles
	 */
	void addToPosMap(PosMap &posMap, const voxel::Region &region, color::RGBA rgba, uint32_t area, uint8_t normalIdx, const math::Vec3i &pos,
					 MeshMaterialIndex material) const;

	/**
	 * @brief Convert the given input triangles into a list of positions to place the voxels at
	 *
	 * @param[in] tris The triangles to voxelize
	 * @param[out] posMap The PosMap instance to fill with positions and colors
	 * @sa transformTrisAxisAligned()
	 * @sa voxelizeTris()
	 */
	void transformTris(const voxel::Region &region, const MeshTriCollection &tris, PosMap &posMap,
					   const MeshMaterialArray &meshMaterialArray, const palette::NormalPalette &normalPalette,
					   core::IProgress *progress = nullptr) const;
	/**
	 * @brief Convert the given input triangles into a list of positions to place the voxels at. This version is for
	 * aligned aligned triangles. This is usually the case for meshes that were exported from voxels.
	 *
	 * @param[in] tris The triangles to voxelize
	 * @param[out] posMap The @c PosMap instance to fill with positions and colors
	 * @sa transformTris()
	 * @sa voxelizeTris()
	 */
	void transformTrisAxisAligned(const voxel::Region &region, const MeshTriCollection &tris, PosMap &posMap,
								  const MeshMaterialArray &meshMaterialArray,
								  const palette::NormalPalette &normalPalette,
								  core::IProgress *progress = nullptr) const;
	/**
	 * @brief Convert the given @c PosMap into a volume
	 *
	 * @note The @c PosMap values can get calculated by @c transformTris() or @c transformTrisAxisAligned()
	 * @param[in] posMap The @c PosMap values with voxel positions and colors
	 * @param[in] fillHollow Fill the inner parts of a voxel volume
	 * @param[out] node The node to create the volume in
	 */
	void voxelizeTris(scenegraph::SceneGraphNode &node, const PosMap &posMap, const MeshMaterialArray &meshMaterialArray, bool fillHollow) const;

	int voxelizeNodeChunked(const core::String &name, scenegraph::SceneGraph &sceneGraph,
							MeshTriCollection &&tris, const MeshMaterialArray &meshMaterialArray,
							const math::Vec3f &trisMins, const voxel::Region &region,
							const palette::NormalPalette &normalPalette, int parent, bool resetOrigin,
							core::IProgress *progress = nullptr) const;

public:
	MeshFormat();
	bool loadGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
					const LoadContext &ctx) override;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	enum VoxelizeMode { HighQuality = 0, Fast = 1 };
};

} // namespace voxelformat
