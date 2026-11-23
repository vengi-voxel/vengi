/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"
#include "core/collection/DynamicArray.h"
#include "voxelformat/private/mesh/MeshMaterial.h"
#include "math/Vec3f.h"

namespace voxelformat {

/**
 * @brief Quake bsp format
 *
 * @ingroup Formats
 *
 * https://www.gamers.org/dEngine/quake/spec/quake-spec34/qkspec_4.htm#CBSPF
 */
class QuakeBSPFormat : public MeshFormat {
private:
	// ----------------------------------------------
	// internal bsp (ufoai) structures
	struct BspLump {
		uint32_t offset = 0;
		uint32_t len = 0;
	};
	static_assert(sizeof(BspLump) == 8, "Unexpected size of BspLump");

	struct BspHeader {
		uint32_t magic = 0;
		uint32_t version = 0;
		BspLump lumps[30];
	};

	// q1
	struct BspTextureBase {
		float vecS[3] = {0.0f, 0.0f, 0.0f};
		float distS = 0.0f;
		float vecT[3] = {0.0f, 0.0f, 0.0f};
		float distT = 0.0f;
		uint32_t surfaceFlags = 0; // miptex index in q1
		uint32_t value = 0;
	};

	struct BspTexture : public BspTextureBase {
		char name[32] = "";
	};
	static_assert(sizeof(BspTexture) == 72, "Unexpected size of BspTexture");

	struct BspModel {
		float mins[3]{0.0f, 0.0f, 0.0f};
		float maxs[3]{0.0f, 0.0f, 0.0f};
		float position[3]{0.0f, 0.0f, 0.0f};
		int32_t node = 0;
		int32_t faceId = 0;
		int32_t faceCount = 0;
	};
	static_assert(sizeof(BspModel) == 48, "Unexpected size of BspModel");

	struct BspVertex {
		float x = 0.0f;
		float y = 0.0f;
		float z = 0.0f;
	};
	static_assert(sizeof(BspVertex) == 12, "Unexpected size of BspVertex");

	struct BspFace {
		uint16_t planeId = 0;
		int16_t side = 0;

		int32_t edgeId = 0;
		int16_t edgeCount = 0;
		int16_t textureId = 0;

		int32_t lightofsDay = 0;
		int32_t lightofsNight = 0;
	};
	static_assert(sizeof(BspFace) == 20, "Unexpected size of BspFace");

	struct BspEdge {
		int16_t vertexIndices[2]{0, 0}; // negative means counter clock wise
	};
	static_assert(sizeof(BspEdge) == 4, "Unexpected size of BspEdge");

	struct Quake1Texinfo {
		char name[16];
		uint32_t width, height; // must be a multiple of 8
		uint32_t offset1; // offset to u_char Pix[width   * height]
		uint32_t offset2; // offset to u_char Pix[width/2 * height/2]
		uint32_t offset4; // offset to u_char Pix[width/4 * height/4]
		uint32_t offset8; // offset to u_char Pix[width/8 * height/8]
	};

	// ----------------------------------------------
	// structures used for loading the relevant parts

	struct Model {
		int32_t faceId = 0;
		int32_t faceCount = 0;
	};

	struct Face {
		int32_t edgeId = 0;
		int16_t edgeCount = 0;
		int16_t textureId = 0; // texture info idx
		int32_t index = -1;
		bool used = false;
		bool hidden = false; // clip, sky, trigger, liquid, ...
	};

	// hull 0 of a Quake 1 BSP, used to decide which voxels are solid
	struct Quake1Plane {
		float normal[3]{0.0f, 0.0f, 0.0f};
		float dist = 0.0f;
	};
	struct Quake1Node {
		int32_t plane = 0;
		int16_t children[2]{0, 0};
	};
	struct Quake1Leaf {
		int32_t contents = 0;
	};
	struct Quake1Model {
		int32_t headnode = -1;
		int32_t faceId = 0;
		int32_t faceCount = 0;
	};

	struct Texture : public BspTexture {
		MeshMaterialIndex materialIdx = -1;
	};

	bool voxelize(const core::DynamicArray<Texture> &textures, core::Buffer<Face> &faces,
				  const core::Buffer<BspEdge> &edges, const core::Buffer<int32_t> &surfEdges,
				  const core::Buffer<BspVertex> &vertices, scenegraph::SceneGraph &sceneGraph,
				  const core::String &name, const MeshMaterialArray &meshMaterialArray, core::IProgress *progress);

	int32_t validateLump(const BspLump &lump, size_t elementSize) const;

	bool loadBsp(const core::String &filename, io::SeekableReadStream &stream, const io::ArchivePtr &archive,
				 scenegraph::SceneGraph &sceneGraph, core::IProgress *progress);
	bool loadQuake1Bsp(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
					   const BspHeader &header, core::IProgress *progress);
	bool loadQuake2Bsp(const core::String &filename, io::SeekableReadStream &stream, const io::ArchivePtr &archive,
					   scenegraph::SceneGraph &sceneGraph, const BspHeader &header, core::IProgress *progress);
	bool loadQuake2Textures(const core::String &filename, io::SeekableReadStream &stream, const io::ArchivePtr &archive,
							const BspHeader &header, core::DynamicArray<Texture> &textures,
							MeshMaterialMap &meshMaterials, MeshMaterialArray &meshMaterialArray);
	bool loadQuetooBsp(const core::String &filename, io::SeekableReadStream &stream, const io::ArchivePtr &archive,
					   scenegraph::SceneGraph &sceneGraph, const BspHeader &header, core::IProgress *progress);
	bool loadQuake1Textures(const core::String &filename, io::SeekableReadStream &stream, const BspHeader &header,
							core::DynamicArray<Texture> &textures, MeshMaterialMap &meshMaterials, MeshMaterialArray &meshMaterialArray);
	bool loadQuake1Faces(io::SeekableReadStream &stream, const BspHeader &header, core::Buffer<Face> &faces,
						 const core::DynamicArray<Texture> &textures);
	bool loadQuake1Edges(io::SeekableReadStream &stream, const BspHeader &header, core::Buffer<BspEdge> &edges,
						 core::Buffer<int32_t> &surfEdges);
	bool loadQuake1Vertices(io::SeekableReadStream &stream, const BspHeader &header,
							core::Buffer<BspVertex> &vertices);
	bool loadQuake1Planes(io::SeekableReadStream &stream, const BspHeader &header, core::Buffer<Quake1Plane> &planes);
	bool loadQuake1Nodes(io::SeekableReadStream &stream, const BspHeader &header, core::Buffer<Quake1Node> &nodes);
	bool loadQuake1Leaves(io::SeekableReadStream &stream, const BspHeader &header, core::Buffer<Quake1Leaf> &leaves);
	bool loadQuake1BrushModels(io::SeekableReadStream &stream, const BspHeader &header,
							   core::Buffer<Quake1Model> &models);
	static float distSqPointTriangle(const math::Vec3f &p, const math::Vec3f &a, const math::Vec3f &b, const math::Vec3f &c);
	static bool quakePointSolid(const core::Buffer<Quake1Plane> &planes, const core::Buffer<Quake1Node> &nodes,
								const core::Buffer<Quake1Leaf> &leaves, int root, const math::Vec3f &pos,
								const math::Vec3f &half);

	bool voxelizeQuake1Models(const core::DynamicArray<Texture> &textures, core::Buffer<Face> &faces,
							  const core::Buffer<BspEdge> &edges, const core::Buffer<int32_t> &surfEdges,
							  const core::Buffer<BspVertex> &vertices, const core::Buffer<Quake1Plane> &planes,
							  const core::Buffer<Quake1Node> &nodes, const core::Buffer<Quake1Leaf> &leaves,
							  const core::Buffer<Quake1Model> &models, scenegraph::SceneGraph &sceneGraph,
							  const core::String &name, const MeshMaterialArray &meshMaterialArray);

	bool loadUFOAlienInvasionBsp(const core::String &filename, io::SeekableReadStream &stream,
								 scenegraph::SceneGraph &sceneGraph, const BspHeader &header,
								 core::IProgress *progress);
	bool loadUFOAlienInvasionTextures(const core::String &filename, io::SeekableReadStream &stream,
									  const BspHeader &header, core::DynamicArray<Texture> &textures,
									  MeshMaterialMap &meshMaterials, MeshMaterialArray &meshMaterialArray);
	bool loadUFOAlienInvasionFaces(io::SeekableReadStream &stream, const BspHeader &header,
								   core::Buffer<Face> &faces);
	bool loadUFOAlienInvasionEdges(io::SeekableReadStream &stream, const BspHeader &header,
								   core::Buffer<BspEdge> &edges, core::Buffer<int32_t> &surfEdges);
	bool loadUFOAlienInvasionVertices(io::SeekableReadStream &stream, const BspHeader &header,
									  core::Buffer<BspVertex> &vertices);
	bool loadUFOAlienInvasionFacesForLevel(io::SeekableReadStream &stream, const BspHeader &header,
										   core::Buffer<Face> &faces, core::Buffer<Face> &facesLevel,
										   const core::Buffer<Model> &models, int level);
	bool loadUFOAlienInvasionModels(io::SeekableReadStream &stream, const BspHeader &header,
									core::Buffer<Model> &models);

	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const math::Vec3f &scale, bool quad,
					bool withColor, bool withTexCoords) override {
		return false;
	}

	static const io::FormatDescription &formatUFOAI() {
		static io::FormatDescription f{"UFO:Alien Invasion", "", {"bsp"}, {"IBSP"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

	static const io::FormatDescription &formatQuake1() {
		static io::FormatDescription f{"Quake 1", "", {"bsp"}, {"\x1d"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

	static const io::FormatDescription &formatQuake2() {
		static io::FormatDescription f{"Quake 2", "", {"bsp"}, {"IBSP"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

	static const io::FormatDescription &formatQuetoo() {
		static io::FormatDescription f{"Quetoo", "", {"bsp"}, {"IBSP"}, VOX_FORMAT_FLAG_MESH};
		return f;
	}

	// No zip magic here: a PK magic would claim every other zip format.
	static const io::FormatDescription &formatQuetooPk3() {
		static io::FormatDescription f{"Quetoo PK3", "", {"pk3"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
