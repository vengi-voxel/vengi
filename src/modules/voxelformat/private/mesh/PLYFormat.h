/**
 * @file
 */

#pragma once

#include "MeshFormat.h"
#include "core/collection/DynamicArray.h"

namespace voxelformat {

/** TODO:
 * camera element support
 * range_grid element: https://www.cs.jhu.edu/~misha/Code/PoissonMesh/TextureStitcher/range-grid.ply.html
 */

/**
 * @brief Polygon File Format or Stanford Triangle Format
 *
 * http://paulbourke.net/dataformats/ply/
 * https://mathworks.com/help/vision/ug/the-ply-format.html
 *
 * @ingroup Formats
 */
class PLYFormat : public MeshFormat {
public:
	enum class PlyFormatType { Ascii, BinaryLittleEndian, BinaryBigEndian };

	enum class DataType {
		Int8,
		UInt8,
		Int16,
		UInt16,
		Int32,
		UInt32,
		Float32,
		Float64,

		Max
	};

	enum class PropertyUse { x, y, z, nx, ny, nz, red, green, blue, alpha, s, t, Max };

	struct Property {
		DataType type = DataType::Max;
		DataType countType = DataType::Max; // in case of a list
		PropertyUse use = PropertyUse::Max;
		core::String name;
		bool isList = false;
	};

	struct Element {
		core::String name;
		int count = 0;
		core::DynamicArray<Property> properties;
	};

	struct Header {
		core::DynamicArray<Element> elements;
		PlyFormatType format = PlyFormatType::Ascii;
		core::String version = "1.0";
	};

	struct Vertex {
		glm::vec3 position{0.0f};
		glm::vec3 normal{0.0f};
		glm::vec2 texCoord{0.0f};
		core::RGBA color{0, 0, 0, 255};
	};

	struct Face {
		int indices[3]{0, 0, 0};
	};
protected:
	static bool skipElementBinary(const Element &element, io::SeekableReadStream &stream, const Header &header);
	static int dataSize(DataType type);
	static DataType dataType(const core::String &in);
	static PropertyUse use(const core::String &in);
	static bool parseHeader(io::SeekableReadStream &stream, Header &header);
	bool parseFacesAscii(const Element &element, io::SeekableReadStream &stream, core::DynamicArray<Face> &faces) const;
	bool parseVerticesAscii(const Element &element, io::SeekableReadStream &stream,
							core::DynamicArray<Vertex> &vertices) const;

	bool parseFacesBinary(const Element &element, io::SeekableReadStream &stream, core::DynamicArray<Face> &faces,
						  const Header &header) const;
	bool parseVerticesBinary(const Element &element, io::SeekableReadStream &stream,
							 core::DynamicArray<Vertex> &vertices, const Header &header) const;

	bool parsePointCloudBinary(const core::String &filename, io::SeekableReadStream &stream,
							  scenegraph::SceneGraph &sceneGraph, const Header &header,
							  core::DynamicArray<Vertex> &vertices) const;
	bool parsePointCloudAscii(const core::String &filename, io::SeekableReadStream &stream,
							  scenegraph::SceneGraph &sceneGraph, const Header &header,
							  core::DynamicArray<Vertex> &vertices) const;
	bool parsePointCloud(const core::String &filename, io::SeekableReadStream &stream,
						 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header) const;

	bool parseMeshBinary(const core::String &filename, io::SeekableReadStream &stream,
						 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header,
						 TriCollection &tris) const;
	void convertToTris(TriCollection &tris, core::DynamicArray<Vertex> &vertices, core::DynamicArray<Face> &faces) const;
	bool parseMeshAscii(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header,
						TriCollection &tris) const;
	bool parseMesh(const core::String &filename, io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
				   const LoadContext &ctx, const Header &header);

	bool voxelizeGroups(const core::String &filename, io::SeekableReadStream &stream,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

public:
	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const Meshes &meshes,
					const core::String &filename, io::SeekableWriteStream &stream, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override;
};
} // namespace voxelformat
