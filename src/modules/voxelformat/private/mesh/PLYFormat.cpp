/**
 * @file
 */

#include "PLYFormat.h"
#include "color/Color.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "engine-config.h"
#include "io/Archive.h"
#include "io/EndianStreamReadWrapper.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/VoxelVertex.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Failed to read ply " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) == false) {                                                                                             \
		Log::error("Failed to read ply " CORE_STRINGIFY(read));                                                        \
		return false;                                                                                                  \
	}

template<class T>
static T read(io::EndianStreamReadWrapper &es, PLYFormat::DataType type) {
	switch (type) {
	case PLYFormat::DataType::Int8: {
		int8_t count = 0;
		wrap(es.readInt8(count))
		return (T)(uint8_t)count;
	}
	case PLYFormat::DataType::UInt8: {
		uint8_t count = 0;
		wrap(es.readUInt8(count))
		return count;
	}
	case PLYFormat::DataType::Int16: {
		int16_t count = 0;
		wrap(es.readInt16(count))
		return (T)(uint16_t)count;
	}
	case PLYFormat::DataType::UInt16: {
		uint16_t count = 0;
		wrap(es.readUInt16(count))
		return count;
	}
	case PLYFormat::DataType::Int32: {
		int32_t count = 0;
		wrap(es.readInt32(count))
		return count;
	}
	case PLYFormat::DataType::UInt32: {
		uint32_t count = 0;
		wrap(es.readUInt32(count))
		return (T)(uint32_t)count;
	}
	case PLYFormat::DataType::Float32: {
		float count = 0;
		wrap(es.readFloat(count))
		return count;
	}
	case PLYFormat::DataType::Float64: {
		double count = 0;
		wrap(es.readDouble(count))
		return count;
	}
	case PLYFormat::DataType::Max:
		core_assert(false);
		break;
	}
	return (T)0;
}

static uint8_t readColor(io::EndianStreamReadWrapper &es, PLYFormat::DataType type) {
	switch (type) {
	case PLYFormat::DataType::Int8: {
		int8_t count = 0;
		wrap(es.readInt8(count))
		return (uint8_t)count;
	}
	case PLYFormat::DataType::UInt8: {
		uint8_t count = 0;
		wrap(es.readUInt8(count))
		return count;
	}
	case PLYFormat::DataType::Int16: {
		int16_t count = 0;
		wrap(es.readInt16(count))
		return (uint8_t)(((uint16_t)count) >> 8);
	}
	case PLYFormat::DataType::UInt16: {
		uint16_t count = 0;
		wrap(es.readUInt16(count))
		return (uint8_t)(count >> 8);
	}
	case PLYFormat::DataType::Float32: {
		float count = 0;
		wrap(es.readFloat(count))
		return (uint8_t)(count / 255.0f);
	}
	case PLYFormat::DataType::Float64: {
		double count = 0;
		wrap(es.readDouble(count))
		return (uint8_t)(count / 255.0);
	}
	case PLYFormat::DataType::Int32:
	case PLYFormat::DataType::UInt32:
	case PLYFormat::DataType::Max:
		core_assert(false);
		break;
	}
	return 0u;
}

int PLYFormat::dataSize(DataType type) {
	switch (type) {
	case DataType::Int8:
	case DataType::UInt8:
		return 1;
	case DataType::Int16:
	case DataType::UInt16:
		return 2;
	case DataType::Int32:
	case DataType::UInt32:
	case DataType::Float32:
		return 4;
	case DataType::Float64:
		return 8;
	case DataType::Max:
		break;
	}
	return 0;
}

PLYFormat::DataType PLYFormat::dataType(const core::String &in) {
	static const char *typeNames[] = {"char", "uchar", "short", "ushort", "int", "uint", "float", "double"};
	static_assert(lengthof(typeNames) == (int)DataType::Max, "Invalid typeNames length");
	for (int i = 0; i < (int)DataType::Max; ++i) {
		if (in == typeNames[i]) {
			return (DataType)i;
		}
	}
	if (in == "int8") {
		return DataType::Int8;
	} else if (in == "uint8") {
		return DataType::UInt8;
	} else if (in == "int16") {
		return DataType::Int16;
	} else if (in == "uint16") {
		return DataType::UInt16;
	} else if (in == "int32") {
		return DataType::Int32;
	} else if (in == "uint32") {
		return DataType::UInt32;
	} else if (in == "float32") {
		return DataType::Float32;
	} else if (in == "float64") {
		return DataType::Float64;
	}

	return DataType::Max;
}

PLYFormat::PropertyUse PLYFormat::use(const core::String &in) {
	static const char *useNames[] = {"x", "y", "z", "nx", "ny", "nz", "red", "green", "blue", "alpha", "s", "t"};
	static_assert(lengthof(useNames) == (int)PropertyUse::Max, "Invalid useNames length");
	for (int i = 0; i < (int)PropertyUse::Max; ++i) {
		if (in == useNames[i]) {
			return (PropertyUse)i;
		}
	}
	if (in == "diffuse_red") {
		return PropertyUse::red;
	}
	if (in == "diffuse_green") {
		return PropertyUse::green;
	}
	if (in == "diffuse_blue") {
		return PropertyUse::blue;
	}
	Log::debug("Failed to map %s", in.c_str());
	return PropertyUse::Max;
}

bool PLYFormat::parseHeader(io::SeekableReadStream &stream, Header &header) {
	core::String line;
	line.reserve(256);
	bool endHeader = false;
	core::DynamicArray<core::String> tokens;
	tokens.reserve(32);
	while (!endHeader) {
		// read header
		wrapBool(stream.readLine(line))

		tokens.clear();
		core::string::splitString(line, tokens);
		if (tokens.empty()) {
			continue;
		}

		if (tokens[0] == "format") {
			if (tokens.size() != 3) {
				Log::error("Invalid ply format: %s", line.c_str());
				return false;
			}
			if (tokens[1] == "ascii") {
				Log::debug("found ascii format");
				header.format = PlyFormatType::Ascii;
			} else if (tokens[1] == "binary_little_endian") {
				Log::debug("found binary_little_endian format");
				header.format = PlyFormatType::BinaryLittleEndian;
			} else if (tokens[1] == "binary_big_endian") {
				Log::debug("found binary_big_endian format");
				header.format = PlyFormatType::BinaryBigEndian;
			} else {
				Log::error("Invalid ply format: %s", line.c_str());
				return false;
			}
			header.version = tokens[2];
		} else if (tokens[0] == "element") {
			if (tokens.size() == 3) {
				Element element;
				// vertex, face, edge, material
				element.name = tokens[1];
				element.count = core::string::toInt(tokens[2]);
				header.elements.push_back(element);
			} else {
				Log::error("Invalid ply element: %s", line.c_str());
				return false;
			}
		} else if (tokens[0] == "property") {
			if (header.elements.empty()) {
				Log::error("Invalid ply property before element: %s", line.c_str());
				return false;
			}
			if (tokens.size() < 3) {
				Log::error("Invalid ply property: %s", line.c_str());
				return false;
			}
			Property property;
			if (tokens[1] == "list") {
				if (tokens.size() < 5) {
					Log::error("Invalid ply list property: %s", line.c_str());
					return false;
				}
				property.countType = dataType(tokens[2]);
				property.type = dataType(tokens[3]);
				property.name = tokens[4];
				property.use = use(tokens[4]);
				property.isList = true;
			} else {
				property.type = dataType(tokens[1]);
				property.name = tokens[2];
				property.use = use(tokens[2]);
				property.isList = false;
			}
			header.elements.back().properties.push_back(property);
		} else if (tokens[0] == "comment" || tokens[0] == "obj_info") {
			Log::debug("ply %s", line.c_str());
			header.comment = line;
		} else if (tokens[0] == "end_header") {
			endHeader = true;
		}
	}
	return true;
}

bool PLYFormat::parseFacesAscii(const Element &element, io::SeekableReadStream &stream, voxel::IndexArray &indices,
								core::DynamicArray<voxel::IndexArray> &polygons) const {
	core::DynamicArray<core::String> tokens;
	tokens.reserve(32);

	indices.reserve(element.count * 6);
	for (int idx = 0; idx < element.count; ++idx) {
		core::String line;
		wrapBool(stream.readLine(line))
		tokens.clear();
		core::string::splitString(line, tokens);
		if (tokens.size() < element.properties.size()) {
			Log::error("Invalid ply face: %s", line.c_str());
			return false;
		}
		const int indexCnt = core::string::toInt(tokens[0]);
		if (indexCnt == 3) {
			indices.push_back(core::string::toInt(tokens[1]));
			indices.push_back(core::string::toInt(tokens[2]));
			indices.push_back(core::string::toInt(tokens[3]));
		} else if (indexCnt == 4) {
			// triangle fan
			indices.push_back(core::string::toInt(tokens[1]));
			indices.push_back(core::string::toInt(tokens[2]));
			indices.push_back(core::string::toInt(tokens[3]));

			indices.push_back(core::string::toInt(tokens[1]));
			indices.push_back(core::string::toInt(tokens[3]));
			indices.push_back(core::string::toInt(tokens[4]));
		} else {
			voxel::IndexArray polygon;
			polygon.reserve(indexCnt);
			for (int64_t i = 0; i < indexCnt; ++i) {
				const int polygonIdx = core::string::toInt(tokens[i + 1]);
				polygon.push_back(polygonIdx);
			}
			polygons.emplace_back(core::move(polygon));
		}
	}
	return true;
}

bool PLYFormat::parseVerticesAscii(const Element &element, io::SeekableReadStream &stream,
								   core::DynamicArray<MeshVertex> &vertices) const {
	core::DynamicArray<core::String> tokens;
	tokens.reserve(32);

	vertices.reserve(element.count);
	for (int idx = 0; idx < element.count; ++idx) {
		core::String line;
		wrapBool(stream.readLine(line))
		tokens.clear();
		core::string::splitString(line, tokens);
		if (tokens.size() < element.properties.size()) {
			Log::error("Invalid ply vertex: %s", line.c_str());
			return false;
		}
		MeshVertex vertex;
		vertex.color = color::RGBA(0, 0, 0, 255);
		for (size_t i = 0; i < element.properties.size(); ++i) {
			const Property &prop = element.properties[i];
			Log::trace("%s: %s", prop.name.c_str(), tokens[i].c_str());
			switch (prop.use) {
			case PropertyUse::x:
				vertex.pos.x = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::y:
				vertex.pos.y = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::z:
				vertex.pos.z = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::nx:
				vertex.normal.x = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::ny:
				vertex.normal.y = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::nz:
				vertex.normal.z = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::red:
				vertex.color.r = core::string::toInt(tokens[i]);
				break;
			case PropertyUse::green:
				vertex.color.g = core::string::toInt(tokens[i]);
				break;
			case PropertyUse::blue:
				vertex.color.b = core::string::toInt(tokens[i]);
				break;
			case PropertyUse::alpha:
				vertex.color.a = core::string::toInt(tokens[i]);
				break;
			case PropertyUse::s:
				vertex.uv.x = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::t:
				vertex.uv.y = core::string::toFloat(tokens[i]);
				break;
			case PropertyUse::Max:
				break;
			}
		}
		vertices.push_back(vertex);
	}
	return true;
}

bool PLYFormat::parsePointCloudBinary(const core::String &filename, io::SeekableReadStream &stream,
									  scenegraph::SceneGraph &sceneGraph, const Header &header,
									  core::DynamicArray<MeshVertex> &vertices) const {
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name == "vertex") {
			if (!parseVerticesBinary(element, stream, vertices, header)) {
				return false;
			}
		} else {
			if (!skipElementBinary(element, stream, header)) {
				return false;
			}
		}
	}
	return true;
}

bool PLYFormat::parsePointCloudAscii(const core::String &filename, io::SeekableReadStream &stream,
									 scenegraph::SceneGraph &sceneGraph, const Header &header,
									 core::DynamicArray<MeshVertex> &vertices) const {
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name != "vertex") {
			for (int skip = 0; skip < element.count; ++skip) {
				core::String line;
				wrapBool(stream.readLine(line))
			}
			continue;
		}
		if (!parseVerticesAscii(element, stream, vertices)) {
			return false;
		}
	}
	return true;
}

bool PLYFormat::parsePointCloud(const core::String &filename, io::SeekableReadStream &stream,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx,
								const Header &header) const {
	core::DynamicArray<MeshVertex> vertices;
	if (header.format == PlyFormatType::Ascii) {
		if (!parsePointCloudAscii(filename, stream, sceneGraph, header, vertices)) {
			return false;
		}
	} else if (header.format == PlyFormatType::BinaryLittleEndian || header.format == PlyFormatType::BinaryBigEndian) {
		if (!parsePointCloudBinary(filename, stream, sceneGraph, header, vertices)) {
			return false;
		}
	}
	PointCloud pointCloud;
	pointCloud.resize(vertices.size());
	for (int i = 0; i < (int)vertices.size(); ++i) {
		pointCloud[i].position = vertices[i].pos;
		pointCloud[i].color = vertices[i].color;
	}
	return voxelizePointCloud(filename, sceneGraph, core::move(pointCloud)) != InvalidNodeId;
}

bool PLYFormat::parseFacesBinary(const Element &element, io::SeekableReadStream &stream, voxel::IndexArray &indices,
								 core::DynamicArray<voxel::IndexArray> &polygons, const Header &header) const {
	io::EndianStreamReadWrapper es(stream, header.format == PlyFormatType::BinaryBigEndian);
	Log::debug("loading %i faces", element.count);
	indices.reserve(element.count * 6);
	for (int i = 0; i < element.count; ++i) {
		for (size_t j = 0; j < element.properties.size(); ++j) {
			const Property &prop = element.properties[j];
			if (!prop.isList) {
				Log::error("Invalid ply face property: %s", prop.name.c_str());
				return false;
			}
			const int64_t indexCnt = read<int64_t>(es, prop.countType);
			if (indexCnt == 3) {
				indices.push_back(read<int>(es, prop.type));
				indices.push_back(read<int>(es, prop.type));
				indices.push_back(read<int>(es, prop.type));
			} else if (indexCnt == 4) {
				// triangle fan
				const int idx0 = read<int>(es, prop.type);
				const int idx1 = read<int>(es, prop.type);
				const int idx2 = read<int>(es, prop.type);
				indices.push_back(idx0);
				indices.push_back(idx1);
				indices.push_back(idx2);

				indices.push_back(idx0);
				indices.push_back(idx2);
				indices.push_back(read<int>(es, prop.type));
			} else {
				voxel::IndexArray polygon;
				polygon.reserve(indexCnt);
				for (int64_t k = 0; k < indexCnt; ++k) {
					const int idx = read<int>(es, prop.type);
					polygon.push_back(idx);
				}
				polygons.emplace_back(core::move(polygon));
			}
		}
	}
	return true;
}

bool PLYFormat::parseVerticesBinary(const Element &element, io::SeekableReadStream &stream,
									core::DynamicArray<MeshVertex> &vertices, const Header &header) const {
	io::EndianStreamReadWrapper es(stream, header.format == PlyFormatType::BinaryBigEndian);
	vertices.reserve(element.count);
	Log::debug("loading %i vertices", element.count);
	for (int i = 0; i < element.count; ++i) {
		MeshVertex vertex;
		for (size_t j = 0; j < element.properties.size(); ++j) {
			const Property &prop = element.properties[j];
			switch (prop.use) {
			case PropertyUse::x:
				vertex.pos.x = read<float>(es, prop.type);
				break;
			case PropertyUse::y:
				vertex.pos.y = read<float>(es, prop.type);
				break;
			case PropertyUse::z:
				vertex.pos.z = read<float>(es, prop.type);
				break;
			case PropertyUse::nx:
				vertex.normal.x = read<float>(es, prop.type);
				break;
			case PropertyUse::ny:
				vertex.normal.y = read<float>(es, prop.type);
				break;
			case PropertyUse::nz:
				vertex.normal.z = read<float>(es, prop.type);
				break;
			case PropertyUse::red:
				vertex.color.r = readColor(es, prop.type);
				break;
			case PropertyUse::green:
				vertex.color.g = readColor(es, prop.type);
				break;
			case PropertyUse::blue:
				vertex.color.b = readColor(es, prop.type);
				break;
			case PropertyUse::alpha:
				vertex.color.a = readColor(es, prop.type);
				break;
			case PropertyUse::s:
				vertex.uv.x = read<float>(es, prop.type);
				break;
			case PropertyUse::t:
				vertex.uv.y = read<float>(es, prop.type);
				break;
			case PropertyUse::Max:
				break;
			}
		}
		vertices.push_back(vertex);
	}
	return true;
}

bool PLYFormat::skipElementBinary(const Element &element, io::SeekableReadStream &stream, const Header &header) {
	for (size_t i = 0; i < element.properties.size(); ++i) {
		const Property &prop = element.properties[i];
		if (prop.isList) {
			io::EndianStreamReadWrapper es(stream, header.format == PlyFormatType::BinaryBigEndian);
			const int64_t listCount = read<int64_t>(es, prop.countType);
			stream.skip(listCount * dataSize(prop.type));
		} else {
			stream.skip(dataSize(prop.type));
		}
	}
	return true;
}

bool PLYFormat::parseMeshBinary(io::SeekableReadStream &stream, const Header &header, Mesh &mesh) const {
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name == "vertex") {
			if (!parseVerticesBinary(element, stream, mesh.vertices, header)) {
				return false;
			}
		} else if (element.name == "face") {
			if (!parseFacesBinary(element, stream, mesh.indices, mesh.polygons, header)) {
				return false;
			}
		} else {
			if (!skipElementBinary(element, stream, header)) {
				Log::error("Failed to skip element %s", element.name.c_str());
				return false;
			}
			continue;
		}
	}

	return true;
}

bool PLYFormat::parseMeshAscii(io::SeekableReadStream &stream, const Header &header, Mesh &mesh) const {
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name == "vertex") {
			if (!parseVerticesAscii(element, stream, mesh.vertices)) {
				return false;
			}
		} else if (element.name == "face") {
			if (!parseFacesAscii(element, stream, mesh.indices, mesh.polygons)) {
				return false;
			}
		} else {
			for (int skip = 0; skip < element.count; ++skip) {
				core::String line;
				wrapBool(stream.readLine(line))
			}
			continue;
		}
	}

	return true;
}

bool PLYFormat::parseMesh(const core::String &filename, io::SeekableReadStream &stream,
						  scenegraph::SceneGraph &sceneGraph, const Header &header) {
	Mesh mesh;
	if (header.format == PlyFormatType::Ascii) {
		if (!parseMeshAscii(stream, header, mesh)) {
			return false;
		}
	} else if (header.format == PlyFormatType::BinaryLittleEndian || header.format == PlyFormatType::BinaryBigEndian) {
		if (!parseMeshBinary(stream, header, mesh)) {
			return false;
		}
	}

	if (!header.comment.empty()) {
		scenegraph::SceneGraphNode &root = sceneGraph.node(0);
		root.setProperty(scenegraph::PropDescription, header.comment);
	}
	return voxelizeMesh(filename, sceneGraph, core::move(mesh));
}

bool PLYFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	core::String line;
	wrapBool(stream->readLine(line))
	if (line != "ply") {
		Log::error("Invalid ply header");
		return false;
	}

	Header header;
	if (!parseHeader(*stream, header)) {
		return false;
	}

	// if only vertex elements available, this might be a point cloud, if face
	// element is available too, this is a mesh
	auto predicate = [](const Element &e) { return e.name == "face"; };
	if (core::find_if(header.elements.begin(), header.elements.end(), predicate) == header.elements.end()) {
		return parsePointCloud(filename, *stream, sceneGraph, ctx, header);
	}

	return parseMesh(filename, *stream, sceneGraph, header);
}

#undef wrapBool
#undef wrap

bool PLYFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	// if no transform are applied, and no scale is wanted, we can just export integers
	const bool applyTransform = core::Var::getSafe(cfg::VoxformatTransform)->boolVal();
	const bool exportIntegers = glm::all(glm::equal(scale, glm::vec3(1.0f))) && !applyTransform;
	int elementsCnt = 0;
	int indicesCnt = 0;
	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			elementsCnt += (int)mesh.getNoOfVertices();
			indicesCnt += (int)mesh.getNoOfIndices();
		}
	}

	if (elementsCnt == 0 || indicesCnt == 0) {
		return false;
	}

	core::String palFilename = voxel::getPalette().filename();
	if (palFilename.empty()) {
		palFilename = "palette";
	}
	const core::String paletteName = core::string::replaceExtension(palFilename, "png");
	stream->writeStringFormat(false, "ply\nformat ascii 1.0\n");
	stream->writeStringFormat(false, "comment version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n");
	stream->writeStringFormat(false, "comment TextureFile %s\n", paletteName.c_str());

	stream->writeStringFormat(false, "element vertex %i\n", elementsCnt);
	if (exportIntegers) {
		stream->writeStringFormat(false, "property int x\n");
		stream->writeStringFormat(false, "property int z\n");
		stream->writeStringFormat(false, "property int y\n");
	} else {
		stream->writeStringFormat(false, "property float x\n");
		stream->writeStringFormat(false, "property float z\n");
		stream->writeStringFormat(false, "property float y\n");
	}
	if (withTexCoords) {
		stream->writeStringFormat(false, "property float s\n");
		stream->writeStringFormat(false, "property float t\n");
	}
	if (withColor) {
		stream->writeStringFormat(false, "property uchar red\n");
		stream->writeStringFormat(false, "property uchar green\n");
		stream->writeStringFormat(false, "property uchar blue\n");
		stream->writeStringFormat(false, "property uchar alpha\n");
	}

	int faces;
	if (quad) {
		faces = indicesCnt / 6;
	} else {
		faces = indicesCnt / 3;
	}

	stream->writeStringFormat(false, "element face %i\n", faces);
	stream->writeStringFormat(false, "property list uchar uint vertex_indices\n");
	stream->writeStringFormat(false, "end_header\n");

	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			const int nv = (int)mesh.getNoOfVertices();
			const voxel::VoxelVertex *vertices = mesh.getRawVertexData();
			const scenegraph::SceneGraphNode &graphNode = sceneGraph.node(meshExt.nodeId);
			scenegraph::KeyFrameIndex keyFrameIdx = 0;
			const scenegraph::SceneGraphTransform &transform = graphNode.transform(keyFrameIdx);
			const palette::Palette &palette = graphNode.palette();

			for (int j = 0; j < nv; ++j) {
				const voxel::VoxelVertex &v = vertices[j];
				glm::vec3 pos;
				if (meshExt.applyTransform) {
					pos = transform.apply(v.position, meshExt.pivot * meshExt.size);
				} else {
					pos = v.position;
				}
				if (exportIntegers) {
					stream->writeStringFormat(false, "%d %d %d", (int)pos.x, (int)pos.y, (int)pos.z);
				} else {
					pos *= scale;
					stream->writeStringFormat(false, "%f %f %f", pos.x, pos.y, pos.z);
				}
				if (withTexCoords) {
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					stream->writeStringFormat(false, " %f %f", uv.x, uv.y);
				}
				if (withColor) {
					const color::RGBA color = palette.color(v.colorIndex);
					stream->writeStringFormat(false, " %u %u %u %u", color.r, color.g, color.b, color.a);
				}
				stream->writeStringFormat(false, "\n");
			}
		}
	}

	int idxOffset = 0;
	for (const auto &meshExt : meshes) {
		for (int i = 0; i < voxel::ChunkMesh::Meshes; ++i) {
			const voxel::Mesh &mesh = meshExt.mesh->mesh[i];
			if (mesh.isEmpty()) {
				continue;
			}
			const int ni = (int)mesh.getNoOfIndices();
			const int nv = (int)mesh.getNoOfVertices();
			if (ni % 3 != 0) {
				Log::error("Unexpected indices amount");
				return false;
			}
			const voxel::IndexType *indices = mesh.getRawIndexData();
			if (quad) {
				for (int j = 0; j < ni; j += 6) {
					const uint32_t one = idxOffset + indices[j + 0];
					const uint32_t two = idxOffset + indices[j + 1];
					const uint32_t three = idxOffset + indices[j + 2];
					const uint32_t four = idxOffset + indices[j + 5];
					stream->writeStringFormat(false, "4 %i %i %i %i\n", (int)one, (int)two, (int)three, (int)four);
				}
			} else {
				for (int j = 0; j < ni; j += 3) {
					const uint32_t one = idxOffset + indices[j + 0];
					const uint32_t two = idxOffset + indices[j + 1];
					const uint32_t three = idxOffset + indices[j + 2];
					stream->writeStringFormat(false, "3 %i %i %i\n", (int)one, (int)two, (int)three);
				}
			}
			idxOffset += nv;
		}
	}
	return sceneGraph.firstPalette().save(paletteName.c_str());
}
} // namespace voxelformat
