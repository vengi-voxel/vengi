/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "core/String.h"
#include "core/collection/Buffer.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/StringMap.h"
#include <glm/mat4x4.hpp>
#include <glm/vec3.hpp>

namespace voxelformat {

/**
 * @brief 3D Studio Max format 3ds
 *
 * https://paulbourke.net/dataformats/3ds/
 *
 * @ingroup Formats
 */
class Autodesk3DSFormat : public MeshFormat {
private:
	struct Face3ds {
		glm::u16vec3 indices{};
		uint16_t flags = 0;
		int32_t smoothingGroup = -1;
		glm::vec3 normal{};
		core::String material;
	};

	struct Mesh3ds {
		core::String name;
		glm::mat4 matrix{1.0f};
		core::Buffer<glm::vec3> vertices;
		core::Buffer<glm::vec3> normals;
		core::Buffer<glm::vec2> texcoords;
		core::DynamicArray<Face3ds> faces;
		core::Buffer<color::RGBA> colors;
	};

	struct Chunk3ds {
		uint16_t id = 0u;
		uint32_t length = 0u;
	};

	struct MaterialTexture3ds {
		core::String name;
		// 0x2 means mirror, 0x10 is cut off (clamped)
		// TODO: MATERIAL: image::TextureWrap
		int16_t tiling = 0;
		float blur = 0.0f;
		float scaleU = 1.0f;
		float scaleV = 1.0f;
		float offsetU = 0.0f;
		float offsetV = 0.0f;
		image::ImagePtr texture;
	};

	struct Material3ds {
		core::String name;
		color::RGBA diffuseColor{0, 0, 0};
		color::RGBA ambientColor{0, 0, 0};
		color::RGBA specularColor{0, 0, 0};
		float shininess = 0.0f;	 // Specular intensity (specular factor)
		float shininess2 = 0.0f; // controls the size/shape of the specular highlight
		float transparency = 0.0f;
		float blur = 0.0f;
		MaterialTexture3ds diffuse;
	};

	struct Camera3ds {
		core::String name;
		glm::vec3 position;
		glm::vec3 target;
		float roll;
		float fieldOfView;
		uint8_t unknown;
		float nearPlane;
		float farPlane;
	};

	struct Node3ds {
		int16_t id = -1;
		int16_t parentId = -1;
		core::String name;
		core::String instanceName;
		glm::vec3 pivot{0.0f};
		uint32_t meshVersion = 0;
		float scale = 1.0f; // global scaling factor
		uint16_t flags1 = 0;
		uint16_t flags2 = 0;
		glm::vec3 min{0.0f};
		glm::vec3 max{0.0f};
		core::StringMap<Material3ds> materials;
		core::DynamicArray<Mesh3ds> meshes;
		core::DynamicArray<Camera3ds> cameras;
	};

	class ScopedChunk {
	private:
		int64_t _chunkPos;
		io::SeekableReadStream *_stream;

	public:
		Chunk3ds chunk;
		ScopedChunk(io::SeekableReadStream *stream);
		~ScopedChunk();
	};

	void skipUnknown(io::SeekableReadStream *stream, const Chunk3ds &chunk, const char *section) const;

	bool readDataFactor(io::SeekableReadStream *stream, Chunk3ds &parent, float &factor) const;
	bool readDataColor(io::SeekableReadStream *stream, Chunk3ds &parent, color::RGBA &color) const;

	bool readMeshFaces(io::SeekableReadStream *stream, Chunk3ds &parent, Mesh3ds &mesh) const;
	bool readMesh(io::SeekableReadStream *stream, Chunk3ds &parent, Mesh3ds &mesh) const;
	bool readCamera(io::SeekableReadStream *stream, Chunk3ds &parent, Camera3ds &camera) const;
	bool readKeyFrames(io::SeekableReadStream *stream, Chunk3ds &parent) const;
	bool readMaterial(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream *stream,
					  Chunk3ds &parent, Material3ds &material) const;
	bool readNode(const core::String &filename, const io::ArchivePtr &archive, io::SeekableReadStream *stream,
				  Chunk3ds &parent, Node3ds &node) const;
	bool readNodeChildren(io::SeekableReadStream *stream, Chunk3ds &parent, Node3ds &node) const;
	bool readMaterialTexture(const core::String &filename, const io::ArchivePtr &archive,
							 io::SeekableReadStream *stream, Chunk3ds &parent, MaterialTexture3ds &texture) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;
	bool saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
					const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
					const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Autodesk 3D Studio", "", {"3ds"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
