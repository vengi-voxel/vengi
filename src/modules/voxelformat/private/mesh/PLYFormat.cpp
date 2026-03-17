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
#include "app/Async.h"
#include "core/concurrent/Atomic.h"
#include "voxel/MaterialColor.h"
#include "voxel/Mesh.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/VoxelVertex.h"
#include "voxelutil/VolumeVisitor.h"

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

/**
 * @brief Compute the face normal for a surface voxel by checking which faces are exposed to air.
 * This always gives the correct outward direction regardless of wall thickness.
 */
static glm::vec3 computeFaceNormal(const voxel::RawVolume &volume, const glm::ivec3 &pos) {
	const voxel::Region &region = volume.region();
	glm::vec3 faceNormal(0.0f);
	static const glm::ivec3 faceOffsets[6] = {{1, 0, 0}, {-1, 0, 0}, {0, 1, 0}, {0, -1, 0}, {0, 0, 1}, {0, 0, -1}};
	for (int face = 0; face < 6; ++face) {
		const glm::ivec3 neighbor = pos + faceOffsets[face];
		if (!region.containsPoint(neighbor) || voxel::isAir(volume.voxel(neighbor).getMaterial())) {
			faceNormal += glm::vec3(faceOffsets[face]);
		}
	}
	const float len2 = glm::dot(faceNormal, faceNormal);
	if (len2 < 0.0001f) {
		return glm::vec3(0.0f, 1.0f, 0.0f);
	}
	return glm::normalize(faceNormal);
}

/**
 * @brief Estimate a smooth outward-facing normal for a surface voxel by weighted-average
 * of solid neighbor offsets within the given radius.
 *
 * First computes the exact face normal (always correct direction), then smooths it using
 * a weighted average of solid neighbor offsets. The face normal is used as a reference to
 * prevent normal flips on thin walls and concave surfaces.
 */
static glm::vec3 estimateWeightedNormal(const voxel::RawVolume &volume, const glm::ivec3 &pos, int radius) {
	const glm::vec3 faceNormal = computeFaceNormal(volume, pos);
	if (radius <= 1) {
		return faceNormal;
	}

	const voxel::Region &region = volume.region();
	glm::vec3 sum(0.0f);
	for (int dz = -radius; dz <= radius; ++dz) {
		for (int dy = -radius; dy <= radius; ++dy) {
			for (int dx = -radius; dx <= radius; ++dx) {
				if (dx == 0 && dy == 0 && dz == 0) {
					continue;
				}
				const glm::ivec3 neighbor = pos + glm::ivec3(dx, dy, dz);
				if (!region.containsPoint(neighbor)) {
					continue;
				}
				const float distSq = (float)(dx * dx + dy * dy + dz * dz);
				if (distSq > (float)(radius * radius)) {
					continue;
				}
				const voxel::Voxel &v = volume.voxel(neighbor);
				if (voxel::isAir(v.getMaterial())) {
					continue;
				}
				const float weight = 1.0f / glm::sqrt(distSq);
				sum += glm::vec3((float)dx, (float)dy, (float)dz) * weight;
			}
		}
	}
	if (glm::dot(sum, sum) < 0.0001f) {
		return faceNormal;
	}
	glm::vec3 smoothed = glm::normalize(-sum);
	// If the smoothed normal flips relative to the face normal, correct it
	if (glm::dot(smoothed, faceNormal) < 0.0f) {
		smoothed = -smoothed;
	}
	return smoothed;
}

bool PLYFormat::savePointCloud(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							   const io::ArchivePtr &archive) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	const int normalRadius = core::getVar(cfg::VoxformatPointCloudNormalRadius)->intVal();
	const bool applyTransform = core::getVar(cfg::VoxformatTransform)->boolVal();

	struct PointVertex {
		glm::vec3 pos;
		glm::vec3 normal;
		color::RGBA color;
	};

	core::DynamicArray<PointVertex> allPoints;

	for (const auto &entry : sceneGraph.nodes()) {
		const scenegraph::SceneGraphNode &node = entry->second;
		if (!node.isAnyModelNode()) {
			continue;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (volume == nullptr) {
			continue;
		}
		const palette::Palette &palette = node.palette();

		scenegraph::KeyFrameIndex keyFrameIdx = 0;
		const scenegraph::SceneGraphTransform &transform = node.transform(keyFrameIdx);

		// Pass 1: collect surface voxel positions and colors (fast)
		struct SurfaceVoxel {
			glm::ivec3 pos;
			color::RGBA color;
		};
		core::DynamicArray<SurfaceVoxel> surfaceVoxels;
		const voxel::Region &volRegion = volume->region();
		const size_t estimatedSurface = (size_t)volRegion.voxels() / 6;
		surfaceVoxels.reserve(estimatedSurface);
		auto visitor = [&](int x, int y, int z, const voxel::Voxel &voxel) {
			SurfaceVoxel sv;
			sv.pos = glm::ivec3(x, y, z);
			sv.color = palette.color(voxel.getColor());
			surfaceVoxels.push_back(sv);
		};
		voxelutil::visitSurfaceVolume(*volume, visitor);

		// Pass 2: compute normals in parallel (expensive)
		const int count = (int)surfaceVoxels.size();
		Log::debug("Computing normals for %i surface voxels (radius %i)", count, normalRadius);
		core::DynamicArray<PointVertex> nodePoints;
		nodePoints.resize(count);
		core::AtomicInt processed(0);
		const int logInterval = count / 10 > 0 ? count / 10 : 1;
		app::for_parallel(0, count, [&](int start, int end) {
			for (int i = start; i < end; ++i) {
				const SurfaceVoxel &sv = surfaceVoxels[i];
				nodePoints[i].pos = glm::vec3(sv.pos) + glm::vec3(0.5f);
				nodePoints[i].normal = estimateWeightedNormal(*volume, sv.pos, normalRadius);
				nodePoints[i].color = sv.color;
				const int done = processed.increment() + 1;
				if (done % logInterval == 0) {
					Log::debug("Normal estimation: %i%%", (int)(done * 100 / count));
				}
			}
		});
		Log::debug("Normal estimation complete");

		if (applyTransform) {
			const glm::vec3 pivot = node.pivot() * glm::vec3(node.region().getDimensionsInVoxels());
			const glm::mat4 &mat = transform.worldMatrix();
			const glm::mat3 normalMat = glm::mat3(glm::transpose(glm::inverse(mat)));
			for (PointVertex &pv : nodePoints) {
				pv.pos = glm::vec3(mat * glm::vec4(pv.pos - pivot, 1.0f));
				pv.normal = glm::normalize(normalMat * pv.normal);
			}
		}

		allPoints.reserve(allPoints.size() + nodePoints.size());
		for (const PointVertex &pv : nodePoints) {
			allPoints.push_back(pv);
		}
	}

	if (allPoints.empty()) {
		Log::warn("No surface voxels found for point cloud export");
		return false;
	}

	Log::info("Point cloud export: %i surface voxels", (int)allPoints.size());

	stream->writeStringFormat(false, "ply\nformat ascii 1.0\n");
	stream->writeStringFormat(false, "comment version " PROJECT_VERSION " github.com/vengi-voxel/vengi\n");
	stream->writeStringFormat(false, "comment Point cloud export from voxel data\n");
	stream->writeStringFormat(false, "element vertex %i\n", (int)allPoints.size());
	stream->writeStringFormat(false, "property float x\n");
	stream->writeStringFormat(false, "property float y\n");
	stream->writeStringFormat(false, "property float z\n");
	stream->writeStringFormat(false, "property float nx\n");
	stream->writeStringFormat(false, "property float ny\n");
	stream->writeStringFormat(false, "property float nz\n");
	stream->writeStringFormat(false, "property uchar red\n");
	stream->writeStringFormat(false, "property uchar green\n");
	stream->writeStringFormat(false, "property uchar blue\n");
	stream->writeStringFormat(false, "property uchar alpha\n");
	stream->writeStringFormat(false, "end_header\n");

	for (const PointVertex &pv : allPoints) {
		stream->writeStringFormat(false, "%f %f %f %f %f %f %u %u %u %u\n", pv.pos.x, pv.pos.y, pv.pos.z,
								  pv.normal.x, pv.normal.y, pv.normal.z, pv.color.r, pv.color.g, pv.color.b,
								  pv.color.a);
	}

	return true;
}

bool PLYFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	const bool pointCloudExport = core::getVar(cfg::VoxformatPointCloudExport)->boolVal();
	if (pointCloudExport) {
		return savePointCloud(sceneGraph, filename, archive);
	}
	return MeshFormat::saveGroups(sceneGraph, filename, archive, ctx);
}

bool PLYFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
						   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
						   const glm::vec3 &scale, bool quad, bool withColor, bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	// if no transform are applied, and no scale is wanted, we can just export integers
	const bool applyTransform = core::getVar(cfg::VoxformatTransform)->boolVal();
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
		if (meshExt.texture && meshExt.texture->isLoaded()) {
			Log::error("PLY format does not support textures");
			return false;
		}
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
