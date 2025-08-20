/**
 * @file
 */

#include "PLYFormat.h"
#include "core/Color.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
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
#include "voxelformat/external/earcut.hpp"
#include <array>

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

bool PLYFormat::parseFacesAscii(const Element &element, io::SeekableReadStream &stream,
								core::Buffer<PLYFace> &faces, core::DynamicArray<PLYPolygon> &polygons) const {
	core::DynamicArray<core::String> tokens;
	tokens.reserve(32);

	faces.reserve(element.count);
	for (int idx = 0; idx < element.count; ++idx) {
		core::String line;
		wrapBool(stream.readLine(line))
		tokens.clear();
		core::string::splitString(line, tokens);
		if (tokens.size() < element.properties.size()) {
			Log::error("Invalid ply face: %s", line.c_str());
			return false;
		}
		const int indices = core::string::toInt(tokens[0]);
		if (indices == 3) {
			PLYFace face;
			face.indices[0] = core::string::toInt(tokens[1]);
			face.indices[1] = core::string::toInt(tokens[2]);
			face.indices[2] = core::string::toInt(tokens[3]);
			faces.push_back(face);
		} else if (indices == 4) {
			// triangle fan
			PLYFace face1;
			face1.indices[0] = core::string::toInt(tokens[1]);
			face1.indices[1] = core::string::toInt(tokens[2]);
			face1.indices[2] = core::string::toInt(tokens[3]);
			faces.push_back(face1);

			PLYFace face2;
			face2.indices[0] = core::string::toInt(tokens[1]);
			face2.indices[1] = core::string::toInt(tokens[3]);
			face2.indices[2] = core::string::toInt(tokens[4]);
			faces.push_back(face2);
		} else {
			PLYPolygon polygon;
			polygon.indices.reserve(indices);
			for (int64_t i = 0; i < indices; ++i) {
				const int polygonIdx = core::string::toInt(tokens[i + 1]);
				polygon.indices.push_back(polygonIdx);
			}
			polygons.push_back(polygon);
		}
	}
	return true;
}

bool PLYFormat::parseVerticesAscii(const Element &element, io::SeekableReadStream &stream,
								   core::Buffer<MeshVertex> &vertices) const {
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
		vertex.color = core::RGBA(0, 0, 0, 255);
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
									  core::Buffer<MeshVertex> &vertices) const {
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
									 core::Buffer<MeshVertex> &vertices) const {
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
	core::Buffer<MeshVertex> vertices;
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
	return voxelizePointCloud(filename, sceneGraph, pointCloud) != InvalidNodeId;
}

void PLYFormat::convertToTris(MeshTriCollection &tris, core::Buffer<MeshVertex> &vertices,
							  core::Buffer<PLYFace> &faces) const {
	tris.reserve(tris.size() + faces.size());
	for (int i = 0; i < (int)faces.size(); ++i) {
		const PLYFace &face = faces[i];
		voxelformat::MeshTri meshTri;
		const MeshVertex &vertex0 = vertices[face.indices[0]];
		const MeshVertex &vertex1 = vertices[face.indices[1]];
		const MeshVertex &vertex2 = vertices[face.indices[2]];
		meshTri.setUVs(vertex0.uv, vertex1.uv, vertex2.uv);
		meshTri.setColor(vertex0.color, vertex1.color, vertex2.color);
		meshTri.setVertices(vertex0.pos, vertex1.pos, vertex2.pos);
		tris.emplace_back(meshTri);
	}
}

/**
 * @param[out] faces The triangulated faces
 * @param[in] polygons The indices of the polygon
 */
void PLYFormat::triangulatePolygons(const core::DynamicArray<PLYPolygon> &polygons,
									const core::Buffer<MeshVertex> &vertices,
									core::Buffer<PLYFace> &faces) const {
	if (polygons.empty()) {
		Log::debug("No polygons to triangulate");
		return;
	}

	Log::debug("triangulate %i polygons", (int)polygons.size());

	// this code was taken from tinyobjloader
	for (const PLYPolygon &p : polygons) {
		const size_t nPolygons = p.indices.size();
		glm::vec3 norm(0.0f);
		for (size_t k = 0; k < nPolygons; ++k) {
			const int i0 = p.indices[k % nPolygons];
			const int i0_2 = p.indices[(k + 1) % nPolygons];
			const glm::vec3 &point1 = vertices[i0].pos;
			const glm::vec3 &point2 = vertices[i0_2].pos;
			const glm::vec3 a(point1 - point2);
			const glm::vec3 b(point1 + point2);
			norm += glm::dot(a, b);
		}
		const float len = glm::length(norm);
		if (len <= 0.0f) {
			continue;
		}
		const float invLength = -1.0f / len;
		norm *= invLength;

		const glm::vec3 &axis_w = norm;
		glm::vec3 a;
		if (glm::abs(axis_w.x) > 0.9999999f) {
			a = glm::vec3(0.0f, 1.0f, 0.0f);
		} else {
			a = glm::vec3(1.0f, 0.0f, 0.0f);
		}
		const glm::vec3 axis_v = glm::normalize(glm::cross(axis_w, a));
		const glm::vec3 axis_u = glm::cross(axis_w, axis_v);
		// TODO: VOXELFORMAT: reduce code duplication with Polygon class
		using Point = std::array<float, 2>;
		using Points = std::vector<Point>;
		Points polyline;
		std::vector<Points> polygon;

		for (size_t k = 0; k < nPolygons; k++) {
			const glm::vec3 &polypoint = vertices[p.indices[k]].pos;
			const glm::vec3 loc(glm::dot(polypoint, axis_u), glm::dot(polypoint, axis_v), glm::dot(polypoint, axis_w));
			polyline.push_back({loc.x, loc.y});
		}

		polygon.push_back(polyline);

		std::vector<voxel::IndexType> indices = mapbox::earcut<voxel::IndexType>(polygon);
		core_assert((int)indices.size() % 3 == 0);
		Log::debug("triangulated %i tris", (int)indices.size() / 3);

		for (size_t k = 0; k < indices.size() / 3; k++) {
			const int idx0 = indices[3 * k + 0];
			const int idx1 = indices[3 * k + 1];
			const int idx2 = indices[3 * k + 2];

			faces.push_back(PLYFace{idx0, idx1, idx2});
		}
	}
}

bool PLYFormat::parseFacesBinary(const Element &element, io::SeekableReadStream &stream,
								 core::Buffer<PLYFace> &faces, core::DynamicArray<PLYPolygon> &polygons,
								 const Header &header) const {
	io::EndianStreamReadWrapper es(stream, header.format == PlyFormatType::BinaryBigEndian);
	Log::debug("loading %i faces", element.count);
	faces.reserve(element.count);
	for (int i = 0; i < element.count; ++i) {
		for (size_t j = 0; j < element.properties.size(); ++j) {
			const Property &prop = element.properties[j];
			if (!prop.isList) {
				Log::error("Invalid ply face property: %s", prop.name.c_str());
				return false;
			}
			const int64_t indices = read<int64_t>(es, prop.countType);
			if (indices == 3) {
				PLYFace face;
				face.indices[0] = read<int>(es, prop.type);
				face.indices[1] = read<int>(es, prop.type);
				face.indices[2] = read<int>(es, prop.type);
				faces.push_back(face);
			} else if (indices == 4) {
				// triangle fan
				PLYFace face1;
				face1.indices[0] = read<int>(es, prop.type);
				face1.indices[1] = read<int>(es, prop.type);
				face1.indices[2] = read<int>(es, prop.type);
				faces.push_back(face1);

				PLYFace face2;
				face2.indices[0] = face1.indices[0];
				face2.indices[1] = face1.indices[2];
				face2.indices[2] = read<int>(es, prop.type);
				faces.push_back(face2);
			} else {
				PLYPolygon polygon;
				polygon.indices.reserve(indices);
				for (int64_t k = 0; k < indices; ++k) {
					const int idx = read<int>(es, prop.type);
					polygon.indices.push_back(idx);
				}
				polygons.push_back(polygon);
			}
		}
	}
	return true;
}

bool PLYFormat::parseVerticesBinary(const Element &element, io::SeekableReadStream &stream,
									core::Buffer<MeshVertex> &vertices, const Header &header) const {
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

bool PLYFormat::parseMeshBinary(const core::String &filename, io::SeekableReadStream &stream,
								scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header,
								MeshTriCollection &tris) const {
	core::Buffer<MeshVertex> vertices;
	core::Buffer<PLYFace> faces;
	core::DynamicArray<PLYPolygon> polygons;
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name == "vertex") {
			if (!parseVerticesBinary(element, stream, vertices, header)) {
				return false;
			}
		} else if (element.name == "face") {
			if (!parseFacesBinary(element, stream, faces, polygons, header)) {
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

	triangulatePolygons(polygons, vertices, faces);
	convertToTris(tris, vertices, faces);

	return true;
}

bool PLYFormat::parseMeshAscii(const core::String &filename, io::SeekableReadStream &stream,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header,
							   MeshTriCollection &tris) const {
	core::Buffer<MeshVertex> vertices;
	core::Buffer<PLYFace> faces;
	core::DynamicArray<PLYPolygon> polygons;
	for (int i = 0; i < (int)header.elements.size(); ++i) {
		const Element &element = header.elements[i];
		if (element.name == "vertex") {
			if (!parseVerticesAscii(element, stream, vertices)) {
				return false;
			}
		} else if (element.name == "face") {
			if (!parseFacesAscii(element, stream, faces, polygons)) {
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

	triangulatePolygons(polygons, vertices, faces);
	convertToTris(tris, vertices, faces);

	return true;
}

bool PLYFormat::parseMesh(const core::String &filename, io::SeekableReadStream &stream,
						  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx, const Header &header) {
	MeshTriCollection tris;
	if (header.format == PlyFormatType::Ascii) {
		if (!parseMeshAscii(filename, stream, sceneGraph, ctx, header, tris)) {
			return false;
		}
	} else if (header.format == PlyFormatType::BinaryLittleEndian || header.format == PlyFormatType::BinaryBigEndian) {
		if (!parseMeshBinary(filename, stream, sceneGraph, ctx, header, tris)) {
			return false;
		}
	}

	const glm::vec3 scale = getInputScale();
	for (voxelformat::MeshTri &meshTri : tris) {
		meshTri.scaleVertices(scale);
	}

	if (!header.comment.empty()) {
		scenegraph::SceneGraphNode &root = sceneGraph.node(0);
		root.setProperty(scenegraph::PropDescription, header.comment);
	}

	return voxelizeNode(filename, sceneGraph, core::move(tris), {});
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

	return parseMesh(filename, *stream, sceneGraph, ctx, header);
}

#undef wrapBool
#undef wrap

bool PLYFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph, const Meshes &meshes,
						   const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale,
						   bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
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

	const core::String paletteName = core::string::replaceExtension(voxel::getPalette().name(), "png");
	stream->writeStringFormat(false, "ply\nformat ascii 1.0\n");
	stream->writeStringFormat(false, "comment version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n");
	stream->writeStringFormat(false, "comment TextureFile %s\n", paletteName.c_str());

	stream->writeStringFormat(false, "element vertex %i\n", elementsCnt);
	stream->writeStringFormat(false, "property float x\n");
	stream->writeStringFormat(false, "property float z\n");
	stream->writeStringFormat(false, "property float y\n");
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
				pos *= scale;
				stream->writeStringFormat(false, "%f %f %f", pos.x, pos.y, pos.z);
				if (withTexCoords) {
					const glm::vec2 &uv = paletteUV(v.colorIndex);
					stream->writeStringFormat(false, " %f %f", uv.x, uv.y);
				}
				if (withColor) {
					const core::RGBA color = palette.color(v.colorIndex);
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
