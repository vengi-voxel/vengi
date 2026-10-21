/**
 * @file
 */

#include "GodotSceneFormat.h"
#include "color/ColorUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "core/collection/StringMap.h"
#include "image/Image.h"
#include "io/Base64.h"
#include "io/BufferedReadWriteStream.h"
#include "io/MemoryReadStream.h"
#include "io/Stream.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "scenegraph/FrameTransform.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/VoxelVertex.h"
#include <float.h>
#include <limits.h>
#include <math.h>
#include <stdlib.h>
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/quaternion.hpp>
#include <glm/gtx/compatibility.hpp>

namespace voxelformat {

#define wrapBool(read)                                                                                                 \
	if (!(read)) {                                                                                                     \
		Log::error("Could not save escn file");                                                                        \
		return false;                                                                                                  \
	}

namespace {

// The text resource syntax is a Variant syntax, not JSON. Keep constructor names
// so resource references, packed arrays and numeric values remain distinguishable.
struct GodotValue {
	core::String text;
	core::DynamicArray<uint8_t> bytes;
	core::DynamicArray<core::String> keys;
	core::DynamicArray<GodotValue> values;

	const GodotValue &get(const core::String &key) const {
		for (size_t i = 0; i < keys.size(); ++i) {
			if (keys[i] == key) {
				return values[i];
			}
		}
		static const GodotValue empty;
		return empty;
	}
	bool has(const core::String &key) const {
		for (const core::String &k : keys) {
			if (k == key) {
				return true;
			}
		}
		return false;
	}
	double number(double fallback = 0.0) const {
		double n;
		return core::string::parseDouble(text, n) ? n : fallback;
	}
};

struct GodotSection {
	core::String tag;
	GodotValue attributes;
	GodotValue properties;
};

struct GodotDocument {
	core::String filename;
	core::DynamicArray<GodotSection> sections;
};

class GodotParser {
private:
	const char *_p;
	bool _valid = true;

	void whitespace() {
		if (!_valid)
			return;
		for (;;) {
			while (*_p && core::string::isspace(*_p)) {
				++_p;
			}
			if (*_p != ';' && *_p != '#') {
				return;
			}
			while (*_p && *_p != '\n') {
				++_p;
			}
		}
	}
	bool take(char c) {
		if (!_valid)
			return false;
		whitespace();
		if (*_p != c) {
			return false;
		}
		++_p;
		return true;
	}
	core::String token() {
		whitespace();
		core::String result;
		if (*_p == '&' || *_p == '^') {
			++_p; // StringName and NodePath literals
		}
		if (*_p == '"') {
			_valid = core::string::parseQuotedString(_p, result);
			return result;
		}
		while (*_p && !core::string::isspace(*_p) && !strchr("[]{}(),:=;#", *_p)) {
			result += *_p++;
		}
		if (result.empty())
			_valid = false;
		return result;
	}
	GodotValue value(int depth = 0) {
		GodotValue v;
		if (!_valid || depth > 64) {
			_valid = false;
			return v;
		}
		whitespace();
		char close = '\0';
		bool dictionary = false;
		if (take('{')) {
			v.text = "Dictionary";
			close = '}';
			dictionary = true;
		} else if (take('[')) {
			v.text = "Array";
			close = ']';
		} else {
			const bool quoted = *_p == '"' || *_p == '&' || *_p == '^';
			v.text = token();
			if (!quoted && (v.text == "Array" || v.text == "Dictionary") && take('[')) {
				// Typed container annotations do not affect mesh data.
				while (_valid && !take(']')) {
					value(depth + 1);
					take(',');
				}
			}
			if (!quoted && take('(')) {
				whitespace();
				if (v.text == "PackedByteArray") {
					if (*_p == '"') {
						io::BufferedReadWriteStream decoded;
						_valid = io::Base64::decode(decoded, token()) && take(')');
						if (_valid)
							v.bytes.append(decoded.getBuffer(), decoded.size());
					} else {
						while (_valid && !take(')')) {
							double n;
							if (!core::string::parseDouble(token(), n) || n < 0 || n > 255 || glm::floor(n) != n) {
								_valid = false;
								break;
							}
							if (v.bytes.size() == v.bytes.capacity())
								v.bytes.reserve(glm::max((size_t)32, v.bytes.size() * 2));
							v.bytes.push_back((uint8_t)n);
							if (take(')'))
								break;
							if (!take(','))
								_valid = false;
						}
					}
					return v;
				}
				close = ')';
			}
		}
		if (close) {
			if (take(close))
				return v;
			while (_valid) {
				if (dictionary) {
					v.keys.push_back(token());
					if (!take(':'))
						_valid = false;
				}
				if (v.values.size() == v.values.capacity())
					v.values.reserve(glm::max((size_t)32, v.values.size() * 2));
				v.values.push_back(value(depth + 1));
				if (take(close))
					break;
				if (!take(','))
					_valid = false;
				if (take(close))
					break;
			}
		}
		return v;
	}
	void assignment(GodotValue &v) {
		v.keys.push_back(token());
		if (!take('='))
			_valid = false;
		v.values.push_back(value());
	}

public:
	explicit GodotParser(const char *text) : _p(text) {
	}
	bool parse(GodotDocument &doc) {
		whitespace();
		while (*_p && _valid) {
			if (!take('['))
				return false;
			GodotSection section;
			section.tag = token();
			while (_valid && !take(']'))
				assignment(section.attributes);
			whitespace();
			while (*_p && *_p != '[' && _valid) {
				assignment(section.properties);
				whitespace();
			}
			doc.sections.push_back(core::move(section));
		}
		return _valid && !doc.sections.empty();
	}
};

static bool readDocument(const core::String &filename, const io::ArchivePtr &archive, GodotDocument &doc) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream || stream->size() < 0 || stream->size() > INT_MAX)
		return false;
	core::String text;
	if (!stream->readString((int)stream->size(), text))
		return false;
	doc.filename = filename;
	GodotParser parser(text.c_str());
	return parser.parse(doc);
}

// Resource ids are local to each file. Keep the owning document with every
// resolved resource, including references inside external .tres materials/meshes.
struct GodotResource {
	const GodotDocument *document = nullptr;
	const GodotSection *section = nullptr;

	explicit operator bool() const {
		return section != nullptr;
	}
};

static core::String findGodotProject(core::String directory, const io::ArchivePtr &archive) {
	for (;;) {
		core::String filename = core::string::path(directory, "project.godot");
		if (archive->exists(filename)) {
			return filename;
		}
		if (directory.empty() || core::string::isRootPath(directory)) {
			return "";
		}
		directory = core::string::extractDir(directory.substr(0, directory.size() - 1));
	}
}

class GodotResources {
private:
	const io::ArchivePtr &_archive;
	core::StringMap<core::SharedPtr<GodotDocument>> _documents;

public:
	explicit GodotResources(const io::ArchivePtr &archive) : _archive(archive) {
	}
	core::String path(const GodotDocument &doc, const core::String &name) const {
		if (core::string::startsWith(name, "res://")) {
			const core::String project = findGodotProject(core::string::extractDir(doc.filename), _archive);
			if (!project.empty())
				return core::string::extractDir(project) + name.substr(6);
			return name.substr(6);
		}
		if (core::string::isAbsolutePath(name))
			return name;
		return core::string::extractDir(doc.filename) + name;
	}
	const GodotDocument *externalDocument(const GodotDocument &doc, const GodotValue &ref) {
		if (ref.text != "ExtResource" || ref.values.size() != 1)
			return nullptr;
		for (const GodotSection &s : doc.sections) {
			if (s.tag != "ext_resource" || s.attributes.get("id").text != ref.values[0].text)
				continue;
			const core::String filename = path(doc, s.attributes.get("path").text);
			auto entry = _documents.find(filename);
			if (entry == _documents.end()) {
				auto external = core::make_shared<GodotDocument>();
				if (!readDocument(filename, _archive, *external.get()))
					return nullptr;
				_documents.put(filename, external);
				entry = _documents.find(filename);
			}
			return entry->value.get();
		}
		return nullptr;
	}
	GodotResource resolve(const GodotDocument &doc, const GodotValue &ref) {
		if (ref.values.size() != 1)
			return {};
		if (ref.text == "SubResource") {
			for (const GodotSection &s : doc.sections) {
				if (s.tag == "sub_resource" && s.attributes.get("id").text == ref.values[0].text)
					return {&doc, &s};
			}
		} else if (const GodotDocument *external = externalDocument(doc, ref)) {
			for (const GodotSection &resource : external->sections) {
				if (resource.tag == "resource")
					return {external, &resource};
			}
		}
		return {};
	}
	core::String type(const GodotResource &resource) const {
		if (resource.section->tag == "sub_resource")
			return resource.section->attributes.get("type").text;
		return resource.document->sections[0].attributes.get("type").text;
	}
	image::ImagePtr texture(const GodotDocument &doc, const GodotValue &ref) {
		if (ref.text != "ExtResource" || ref.values.size() != 1)
			return {};
		for (const GodotSection &s : doc.sections) {
			if (s.tag != "ext_resource" || s.attributes.get("id").text != ref.values[0].text)
				continue;
			const core::String filename = path(doc, s.attributes.get("path").text);
			core::ScopedPtr<io::SeekableReadStream> stream(_archive->readStream(filename));
			return stream ? image::loadImage(filename, *stream) : image::ImagePtr();
		}
		return {};
	}
};

static glm::vec3 godotVec3(const GodotValue &v, const glm::vec3 &fallback = glm::vec3(0.0f)) {
	if (v.values.size() != 3)
		return fallback;
	return glm::vec3(v.values[0].number(), v.values[1].number(), v.values[2].number());
}

static color::RGBA godotColor(const GodotValue &v) {
	if (v.values.size() != 4)
		return color::RGBA(255, 255, 255, 255);
	glm::u8vec4 rgba;
	for (int i = 0; i < 4; ++i)
		rgba[i] = (uint8_t)glm::round(glm::clamp((float)v.values[i].number(), 0.0f, 1.0f) * 255.0f);
	return color::RGBA(rgba.r, rgba.g, rgba.b, rgba.a);
}

static MeshMaterialPtr loadGodotMaterial(GodotResources &resources, const GodotDocument &doc, const GodotValue &ref) {
	const GodotResource resource = resources.resolve(doc, ref);
	if (!resource)
		return {};
	const core::String type = resources.type(resource);
	if (type != "StandardMaterial3D" && type != "ORMMaterial3D")
		return {};
	const GodotValue &p = resource.section->properties;
	MeshMaterialPtr result = createMaterial(p.get("resource_name").text);
	result->baseColor = godotColor(p.get("albedo_color"));
	result->baseColorFactor = 1.0f;
	if (p.get("transparency").number() != 0.0) {
		result->transparency = 1.0f - result->baseColor.a / 255.0f;
	}
	if (!p.has("metadata/vengi_type")) {
		result->material.setValue(palette::MaterialRoughness, (float)p.get("roughness").number(1.0));
		result->material.setValue(palette::MaterialSpecular, (float)p.get("metallic_specular").number(0.5));
	}
	result->texture = resources.texture(*resource.document, p.get("albedo_texture"));
	if (p.has("albedo_texture") && p.get("albedo_texture").text != "null" && !result->texture)
		return {};
	if (result->texture) {
		// MeshMaterial blends base color; Godot multiplies it with the texture.
		// Bake the tint into this import's private image before voxel sampling.
		for (int y = 0; y < result->texture->height(); ++y) {
			for (int x = 0; x < result->texture->width(); ++x) {
				const uint8_t *pixel = result->texture->data() + (y * result->texture->width() + x) * 4;
				result->texture->setColor(color::RGBA((uint8_t)(pixel[0] * result->baseColor.r / 255),
													  (uint8_t)(pixel[1] * result->baseColor.g / 255),
													  (uint8_t)(pixel[2] * result->baseColor.b / 255), pixel[3]),
										  x, y);
			}
		}
		result->baseColorFactor = 0.0f;
	}
	if (p.get("texture_repeat").text == "false") {
		result->wrapS = result->wrapT = image::TextureWrap::ClampToEdge;
	}
	struct Mapping {
		const char *name;
		palette::MaterialProperty property;
	};
	static const Mapping mapping[] = {{"metallic", palette::MaterialMetal},
									  {"roughness", palette::MaterialRoughness},
									  {"metallic_specular", palette::MaterialSpecular}};
	for (const Mapping &m : mapping) {
		if (p.has(m.name))
			result->material.setValue(m.property, (float)p.get(m.name).number());
	}
	if (p.get("emission_enabled").text == "true") {
		const color::RGBA emission = godotColor(p.get("emission"));
		result->emitColor = emission;
		const float energy = (float)p.get("emission_energy_multiplier").number(1.0);
		const float brightness = glm::max(emission.r, glm::max(emission.g, emission.b)) / 255.0f;
		result->material.setValue(palette::MaterialEmit, energy * brightness);
	}
	// Preserve vengi properties which have no direct StandardMaterial3D equivalent.
	for (int i = palette::MaterialMetal; i < (int)palette::MaterialMax; ++i) {
		const auto prop = (palette::MaterialProperty)i;
		const core::String key = core::String("metadata/vengi_") + palette::MaterialPropertyName(prop);
		if (p.has(key))
			result->material.setValue(prop, (float)p.get(key).number());
	}
	if (p.has("metadata/vengi_type")) {
		const int typeId = (int)p.get("metadata/vengi_type").number();
		if (typeId >= 0 && typeId <= (int)palette::MaterialType::Media)
			result->material.type = (palette::MaterialType)typeId;
	} else if (result->material.emit > 0.0f)
		result->material.type = palette::MaterialType::Emit;
	else if (result->transparency > 0.0f)
		result->material.type = palette::MaterialType::Glass;
	else if (result->material.metal > 0.0f)
		result->material.type = palette::MaterialType::Metal;
	return result;
}

static bool godotBytes(const GodotValue &v, core::DynamicArray<uint8_t> &bytes) {
	if (v.text != "PackedByteArray")
		return false;
	bytes = v.bytes;
	return true;
}

static bool loadGodotSurface(const GodotValue &surface, Mesh &mesh, MeshMaterialIndex materialIdx, bool vertexColors) {
	const int primitive = (int)surface.get("primitive").number(-1);
	if (primitive != 3 && primitive != 4) {
		Log::error("Unsupported Godot primitive %i (expected triangles or triangle strip)", primitive);
		return false;
	}
	const size_t first = mesh.vertices.size();
	voxel::IndexArray indices;
	const GodotValue &arrays = surface.get("arrays");
	if (!arrays.values.empty()) {
		if (arrays.values.size() != 13)
			return false;
		const GodotValue &positions = arrays.values[0];
		if (positions.text != "PackedVector3Array" || positions.values.size() % 3)
			return false;
		const size_t count = positions.values.size() / 3;
		const GodotValue &colors = arrays.values[3];
		const GodotValue &uvs = arrays.values[4];
		if (!colors.values.empty() && colors.values.size() != count * 4)
			return false;
		if (!uvs.values.empty() && uvs.values.size() != count * 2)
			return false;
		for (size_t i = 0; i < count; ++i) {
			MeshVertex v;
			for (int j = 0; j < 3; ++j)
				v.pos[j] = (float)positions.values[i * 3 + j].number(NAN);
			if (vertexColors && !colors.values.empty()) {
				v.color =
					color::getRGBA(glm::vec4(colors.values[i * 4].number(), colors.values[i * 4 + 1].number(),
											 colors.values[i * 4 + 2].number(), colors.values[i * 4 + 3].number()));
			}
			if (!uvs.values.empty())
				v.uv = glm::vec2(uvs.values[i * 2].number(), uvs.values[i * 2 + 1].number());
			v.materialIdx = materialIdx;
			mesh.vertices.push_back(v);
		}
		for (const GodotValue &index : arrays.values[12].values) {
			const double n = index.number(-1);
			if (n < 0 || n >= count || glm::floor(n) != n)
				return false;
			indices.push_back((voxel::IndexType)n);
		}
	} else {
		const double countValue = surface.get("vertex_count").number(-1);
		const double formatValue = surface.get("format").number(-1);
		if (countValue <= 0 || countValue > INT_MAX || glm::floor(countValue) != countValue || formatValue < 0 ||
			formatValue >= (double)(1ULL << 44) || glm::floor(formatValue) != formatValue)
			return false;
		const size_t count = (size_t)countValue;
		const uint64_t format = (uint64_t)formatValue;
		// Godot 4: positions precede a separate normal/tangent block. Attributes
		// (RGBA8 colors, UVs and custom channels) are interleaved in another buffer.
		const bool compressed = (format & (1ULL << 29)) != 0;
		if (!(format & 1) || (format & (1ULL << 25)))
			return false;
		core::DynamicArray<uint8_t> vertices, attributes, indexData;
		if (!godotBytes(surface.get("vertex_data"), vertices))
			return false;
		const size_t positionStride = compressed ? 8 : 12;
		const size_t normalStride = ((format & 2) ? 4 : 0) + ((format & 4) && !compressed ? 4 : 0);
		if (vertices.size() < count * (positionStride + normalStride))
			return false;
		size_t attributeStride = (format & 8) ? 4 : 0;
		const size_t uvOffset = attributeStride;
		if (format & 16)
			attributeStride += compressed ? 4 : 8;
		if (format & 32)
			attributeStride += compressed ? 4 : 8;
		static const size_t customSizes[] = {4, 4, 4, 8, 4, 8, 12, 16};
		for (int c = 0; c < 4; ++c) {
			if (format & (1ULL << (6 + c)))
				attributeStride += customSizes[(format >> (13 + c * 3)) & 7];
		}
		if (attributeStride &&
			(!godotBytes(surface.get("attribute_data"), attributes) || attributes.size() < count * attributeStride))
			return false;
		const GodotValue &aabb = surface.get("aabb");
		if (compressed && aabb.values.size() != 6)
			return false;
		for (size_t i = 0; i < count; ++i) {
			MeshVertex v;
			io::MemoryReadStream positions(vertices.data() + i * positionStride, positionStride);
			for (int j = 0; j < 3; ++j) {
				if (compressed) {
					uint16_t n;
					positions.readUInt16(n);
					v.pos[j] = (float)(aabb.values[j].number() + n / 65535.0 * aabb.values[j + 3].number());
				} else {
					positions.readFloat(v.pos[j]);
				}
			}
			if (vertexColors && (format & 8)) {
				const uint8_t *c = attributes.data() + i * attributeStride;
				v.color = color::RGBA(c[0], c[1], c[2], c[3]);
			}
			if (format & 16) {
				io::MemoryReadStream uv(attributes.data() + i * attributeStride + uvOffset, compressed ? 4 : 8);
				if (compressed) {
					const GodotValue &uvScale = surface.get("uv_scale");
					for (int j = 0; j < 2; ++j) {
						const float scale = uvScale.values.size() == 4 ? (float)uvScale.values[j].number() : 0.0f;
						uint16_t packed;
						uv.readUInt16(packed);
						const float n = packed / 65535.0f;
						v.uv[j] = scale == 0.0f ? n : (n - 0.5f) * scale;
					}
				} else {
					uv.readFloat(v.uv.x);
					uv.readFloat(v.uv.y);
				}
			}
			v.materialIdx = materialIdx;
			mesh.vertices.push_back(v);
		}
		const double indexCountValue = surface.get("index_count").number();
		if (indexCountValue < 0 || indexCountValue > INT_MAX || glm::floor(indexCountValue) != indexCountValue)
			return false;
		const size_t indexCount = (size_t)indexCountValue;
		if (indexCount) {
			const size_t stride = count <= 65536 ? 2 : 4;
			if (!godotBytes(surface.get("index_data"), indexData) || indexData.size() != indexCount * stride)
				return false;
			io::MemoryReadStream indexStream(indexData.data(), indexData.size());
			for (size_t i = 0; i < indexCount; ++i) {
				uint32_t index;
				if (stride == 2) {
					uint16_t n;
					indexStream.readUInt16(n);
					index = n;
				} else {
					indexStream.readUInt32(index);
				}
				if (index >= count)
					return false;
				indices.push_back(index);
			}
		}
	}
	const size_t count = mesh.vertices.size() - first;
	if (count == 0)
		return false;
	for (size_t i = first; i < mesh.vertices.size(); ++i) {
		mesh.vertices[i].uv.y = 1.0f - mesh.vertices[i].uv.y;
		const glm::vec3 &p = mesh.vertices[i].pos;
		if (!glm::all(glm::isfinite(p)))
			return false;
	}
	if (indices.empty()) {
		for (size_t i = 0; i < count; ++i)
			indices.push_back((voxel::IndexType)i);
	}
	if (primitive == 3 && indices.size() % 3)
		return false;
	if (indices.size() < 3)
		return false;
	for (size_t i = 0; i + 2 < indices.size(); i += primitive == 3 ? 3 : 1) {
		// Godot front faces use clockwise winding.
		const bool odd = primitive == 4 && (i & 1);
		mesh.indices.push_back((voxel::IndexType)first + indices[i + (odd ? 1 : 2)]);
		mesh.indices.push_back((voxel::IndexType)first + indices[i + (odd ? 2 : 1)]);
		mesh.indices.push_back((voxel::IndexType)first + indices[i]);
	}
	return true;
}

static bool loadGodotPrimitive(GodotResources &resources, const GodotResource &resource, const GodotDocument &doc,
							   const GodotValue &nodeProperties, Mesh &mesh) {
	const core::String type = resources.type(resource);
	const GodotValue &p = resource.section->properties;
	const GodotValue &ref =
		nodeProperties.has("material_override") && nodeProperties.get("material_override").text != "null"
			? nodeProperties.get("material_override")
			: p.get("material");
	const GodotDocument &materialDoc = &ref == &p.get("material") ? *resource.document : doc;
	MeshMaterialPtr material = loadGodotMaterial(resources, materialDoc, ref);
	if (!material && !ref.text.empty() && ref.text != "null")
		return false;
	if (material)
		mesh.materials.push_back(material);
	const MeshMaterialIndex matIdx = material ? 0 : -1;
	if (type == "BoxMesh") {
		const glm::vec3 size = godotVec3(p.get("size"), glm::vec3(1.0f));
		for (int i = 0; i < 8; ++i) {
			MeshVertex v;
			v.pos = glm::vec3(i & 1 ? 0.5f : -0.5f, i & 2 ? 0.5f : -0.5f, i & 4 ? 0.5f : -0.5f) * size;
			v.materialIdx = matIdx;
			mesh.vertices.push_back(v);
		}
		static const voxel::IndexType indices[] = {0, 2, 1, 1, 2, 3, 4, 5, 6, 5, 7, 6, 0, 1, 4, 1, 5, 4,
												   2, 6, 3, 3, 6, 7, 0, 4, 2, 2, 4, 6, 1, 3, 5, 3, 7, 5};
		mesh.indices.append(indices, lengthof(indices));
		return true;
	}
	if (type == "PlaneMesh" || type == "QuadMesh") {
		const GodotValue &sizeValue = p.get("size");
		const glm::vec2 size = sizeValue.values.size() == 2
								   ? glm::vec2(sizeValue.values[0].number(), sizeValue.values[1].number())
								   : glm::vec2(type == "PlaneMesh" ? 2.0f : 1.0f);
		const int orientation = (int)p.get("orientation").number(type == "PlaneMesh" ? 1 : 2);
		const glm::vec3 offset = godotVec3(p.get("center_offset"));
		for (int i = 0; i < 4; ++i) {
			MeshVertex v;
			v.uv = glm::vec2(i & 1 ? 1.0f : 0.0f, i & 2 ? 0.0f : 1.0f);
			const float x = (i & 1 ? 0.5f : -0.5f) * size.x;
			const float y = (i & 2 ? 0.5f : -0.5f) * size.y;
			v.pos = (orientation == 0	? glm::vec3(0, x, y)
					 : orientation == 1 ? glm::vec3(x, 0, -y)
										: glm::vec3(x, y, 0)) +
					offset;
			v.materialIdx = matIdx;
			mesh.vertices.push_back(v);
		}
		static const voxel::IndexType indices[] = {0, 1, 2, 1, 3, 2};
		mesh.indices.append(indices, lengthof(indices));
		return true;
	}
	Log::error("Unsupported Godot mesh resource %s", type.c_str());
	return false;
}

static bool loadGodotMesh(GodotResources &resources, const GodotDocument &doc, const GodotValue &ref,
						  const GodotValue &nodeProperties, Mesh &mesh) {
	const GodotResource resource = resources.resolve(doc, ref);
	if (!resource) {
		Log::error("Missing Godot mesh resource");
		return false;
	}
	if (resources.type(resource) != "ArrayMesh")
		return loadGodotPrimitive(resources, resource, doc, nodeProperties, mesh);
	const GodotValue &surfaces = resource.section->properties.get("_surfaces");
	if (surfaces.values.empty())
		return false;
	for (size_t i = 0; i < surfaces.values.size(); ++i) {
		const GodotValue &surface = surfaces.values[i];
		const GodotValue *materialRef = &surface.get("material");
		const GodotDocument *materialDoc = resource.document;
		const core::String overrideKey = core::String::format("surface_material_override/%i", (int)i);
		if (nodeProperties.has(overrideKey) && nodeProperties.get(overrideKey).text != "null") {
			materialRef = &nodeProperties.get(overrideKey);
			materialDoc = &doc;
		}
		if (nodeProperties.has("material_override") && nodeProperties.get("material_override").text != "null") {
			materialRef = &nodeProperties.get("material_override");
			materialDoc = &doc;
		}
		MeshMaterialPtr material = loadGodotMaterial(resources, *materialDoc, *materialRef);
		if (!material && materialRef->text != "null" && !materialRef->text.empty()) {
			Log::error("Unsupported or missing Godot material");
			return false;
		}
		bool vertexColors = false;
		if (material) {
			const GodotResource m = resources.resolve(*materialDoc, *materialRef);
			vertexColors = m.section->properties.get("vertex_color_use_as_albedo").text == "true";
			if (vertexColors)
				material->baseColorFactor = 0.0f;
		}
		const MeshMaterialIndex idx = material ? (MeshMaterialIndex)mesh.materials.size() : -1;
		if (material)
			mesh.materials.push_back(material);
		if (!loadGodotSurface(surface, mesh, idx, vertexColors))
			return false;
	}
	return !mesh.indices.empty();
}

static bool godotTransform(const GodotValue &properties, glm::mat4 &m) {
	if (properties.has("transform")) {
		const GodotValue &v = properties.get("transform");
		if (v.text != "Transform3D" || v.values.size() != 12)
			return false;
		for (int i = 0; i < 12; ++i) {
			const float n = (float)v.values[i].number(NAN);
			if (!glm::isfinite(n))
				return false;
			// Transform3D's text form stores basis rows, then translation.
			if (i < 9)
				m[i % 3][i / 3] = n;
			else
				m[3][i - 9] = n;
		}
	} else {
		const glm::vec3 position = godotVec3(properties.get("position"));
		const glm::vec3 rotation = godotVec3(properties.get("rotation"));
		const glm::vec3 scale = godotVec3(properties.get("scale"), glm::vec3(1.0f));
		// Node3D's default Euler order is YXZ.
		m = glm::translate(glm::mat4(1.0f), position) *
			glm::rotate(glm::rotate(glm::rotate(glm::mat4(1.0f), rotation.y, glm::vec3(0, 1, 0)), rotation.x,
									glm::vec3(1, 0, 0)),
						rotation.z, glm::vec3(0, 0, 1)) *
			glm::scale(glm::mat4(1.0f), scale);
	}
	return true;
}

// Vengi uses frame indices rather than seconds, as does the glTF importer.
static constexpr double GodotAnimationFPS = 24.0;
static constexpr int GodotMaxAnimationSamples = 1000000;
static constexpr const char *GodotAnimationLength = "godot_animation_length/";
static constexpr const char *GodotAnimationLoop = "godot_animation_loop/";

static core::String godotString(const GodotValue &v) {
	return v.values.size() == 1 && (v.text == "NodePath" || v.text == "StringName") ? v.values[0].text : v.text;
}

static core::String godotNodePath(const core::String &base, const core::String &relative) {
	if (!relative.empty() && relative[0] == '/')
		return "";
	core::DynamicArray<core::String> parts, resolved;
	core::string::splitString(base + "/" + relative, parts, "/");
	for (const core::String &part : parts) {
		if (part == "." || part.empty())
			continue;
		if (part == "..") {
			if (resolved.empty())
				return "";
			resolved.pop();
		} else
			resolved.push_back(part);
	}
	const core::String result = core::string::join(resolved, "/");
	return result.empty() ? core::String(".") : result;
}

struct GodotAnimationKey {
	double time = 0.0;
	float transition = 1.0f;
	glm::vec4 value{0.0f};
};

struct GodotAnimationTrack {
	int nodeId = InvalidNodeId;
	int channel = 0; // position, quaternion, scale
	int interpolation = 1;
	bool loopWrap = true;
	core::DynamicArray<GodotAnimationKey> keys;
};

static float godotEase(float t, float curve) {
	if (curve > 0.0f)
		return curve < 1.0f ? 1.0f - glm::pow(1.0f - t, 1.0f / curve) : glm::pow(t, curve);
	if (curve < 0.0f)
		return t < 0.5f ? glm::pow(t * 2.0f, -curve) * 0.5f
						: (1.0f - glm::pow(1.0f - (t - 0.5f) * 2.0f, -curve)) * 0.5f + 0.5f;
	return 0.0f;
}

static glm::vec4 sampleGodotTrack(const GodotAnimationTrack &track, double time, double length, bool loop) {
	const auto &keys = track.keys;
	if (keys.size() == 1)
		return keys[0].value;
	size_t next = 0, upper = keys.size();
	while (next < upper) {
		const size_t middle = next + (upper - next) / 2;
		if (keys[middle].time <= time + 1e-8)
			next = middle + 1;
		else
			upper = middle;
	}
	const GodotAnimationKey *a;
	const GodotAnimationKey *b;
	double start, end;
	if (next == 0 || next == keys.size()) {
		if (!loop || !track.loopWrap)
			return keys[next == 0 ? 0 : keys.size() - 1].value;
		a = &keys.back();
		b = &keys.front();
		start = a->time - (next == 0 ? length : 0.0);
		end = b->time + (next == 0 ? 0.0 : length);
	} else {
		a = &keys[next - 1];
		b = &keys[next];
		start = a->time;
		end = b->time;
	}
	if (end <= start || track.interpolation == 0)
		return a->value;
	const float t = godotEase(glm::clamp((float)((time - start) / (end - start)), 0.0f, 1.0f), a->transition);
	if (track.channel == 1) {
		const glm::quat q = glm::normalize(glm::slerp(glm::quat(a->value.w, a->value.x, a->value.y, a->value.z),
													  glm::quat(b->value.w, b->value.x, b->value.y, b->value.z), t));
		return glm::vec4(q.x, q.y, q.z, q.w);
	}
	return glm::mix(a->value, b->value, t);
}

static bool readGodotTracks(const GodotValue &properties, const core::String &rootPath,
							const core::StringMap<int> &paths, core::DynamicArray<GodotAnimationTrack> &tracks) {
	if (properties.has("_compression")) {
		Log::error("Compressed Godot animation tracks are not supported");
		return false;
	}
	// Properties, rather than a numeric loop, allow disabled and unrelated tracks
	// to be skipped without requiring contiguous supported track indices.
	for (const core::String &key : properties.keys) {
		if (!core::string::startsWith(key, "tracks/") || !core::string::endsWith(key, "/type"))
			continue;
		const core::String prefix = key.substr(0, key.size() - 4);
		if (properties.get(prefix + "enabled").text == "false")
			continue;
		const core::String &type = properties.get(key).text;
		GodotAnimationTrack track;
		if (type == "position_3d")
			track.channel = 0;
		else if (type == "rotation_3d")
			track.channel = 1;
		else if (type == "scale_3d")
			track.channel = 2;
		else {
			Log::warn("Skipping unsupported Godot animation track type %s", type.c_str());
			continue;
		}
		track.interpolation = (int)properties.get(prefix + "interp").number(1);
		if (track.interpolation != 0 && track.interpolation != 1) {
			Log::error("Unsupported Godot transform interpolation %i", track.interpolation);
			return false;
		}
		track.loopWrap = properties.get(prefix + "loop_wrap").text != "false";
		core::String path = godotString(properties.get(prefix + "path"));
		const size_t colon = path.find(":");
		if (colon != core::String::npos)
			path = path.substr(0, colon);
		const core::String resolved = godotNodePath(rootPath, path);
		const auto entry = paths.find(resolved);
		if (entry == paths.end()) {
			Log::error("Missing Godot animation target %s", path.c_str());
			return false;
		}
		track.nodeId = entry->value;
		const GodotValue &data = properties.get(prefix + "keys");
		const size_t stride = track.channel == 1 ? 6 : 5;
		if ((data.text != "PackedFloat32Array" && data.text != "PackedFloat64Array") || data.values.size() % stride)
			return false;
		for (size_t i = 0; i < data.values.size(); i += stride) {
			GodotAnimationKey k;
			k.time = data.values[i].number(NAN);
			k.transition = (float)data.values[i + 1].number(NAN);
			if (!glm::isfinite(k.time) || k.time < 0 || !glm::isfinite(k.transition) ||
				(!track.keys.empty() && k.time <= track.keys.back().time))
				return false;
			for (size_t c = 0; c < stride - 2; ++c) {
				k.value[c] = (float)data.values[i + 2 + c].number(NAN);
				if (!glm::isfinite(k.value[c]))
					return false;
			}
			if (track.channel == 1) {
				const float len = glm::length(k.value);
				if (len < 1e-6f)
					return false;
				k.value /= len;
			}
			track.keys.push_back(k);
		}
		if (!track.keys.empty())
			tracks.push_back(core::move(track));
	}
	return true;
}

static bool loadGodotAnimations(GodotResources &resources, const GodotDocument &doc, const core::StringMap<int> &paths,
								scenegraph::SceneGraph &graph, const glm::vec3 &scale) {
	const core::String originalAnimation = graph.activeAnimation();
	core::Map<int, scenegraph::SceneGraphTransform> rest;
	for (const auto &entry : graph.nodes())
		rest.put(entry->value.id(), entry->value.transform(0));
	core::String firstAnimation, autoplay;
	for (const GodotSection &section : doc.sections) {
		if (section.tag != "node" || section.attributes.get("type").text != "AnimationPlayer")
			continue;
		const GodotValue &p = section.properties;
		const core::String parent = section.attributes.has("parent") ? section.attributes.get("parent").text : ".";
		const core::String playerPath =
			parent == "." ? section.attributes.get("name").text : parent + "/" + section.attributes.get("name").text;
		const core::String rootPath =
			godotNodePath(playerPath, p.has("root_node") ? godotString(p.get("root_node")) : core::String(".."));
		GodotValue libraries = p.get("libraries");
		for (size_t i = 0; i < p.keys.size(); ++i) {
			if (core::string::startsWith(p.keys[i], "libraries/")) {
				libraries.keys.push_back(p.keys[i].substr(10));
				libraries.values.push_back(p.values[i]);
			}
		}
		for (size_t lib = 0; lib < libraries.keys.size(); ++lib) {
			const GodotResource library = resources.resolve(doc, libraries.values[lib]);
			if (!library || resources.type(library) != "AnimationLibrary")
				return false;
			const GodotValue &animations = library.section->properties.get("_data");
			for (size_t ai = 0; ai < animations.keys.size(); ++ai) {
				const GodotResource animation = resources.resolve(*library.document, animations.values[ai]);
				if (!animation || resources.type(animation) != "Animation")
					return false;
				const GodotValue &props = animation.section->properties;
				core::String name =
					libraries.keys[lib].empty() ? animations.keys[ai] : libraries.keys[lib] + "/" + animations.keys[ai];
				const core::String playbackName = name;
				if (props.has("metadata/vengi_name"))
					name = props.get("metadata/vengi_name").text;
				if (name.empty())
					return false;
				if (name != "Default") {
					while (graph.hasAnimation(name))
						name += "_2";
				}
				const double length = props.get("length").number(1.0);
				const double loopMode = props.get("loop_mode").number();
				if (length <= 0 || !glm::isfinite(length) || length * GodotAnimationFPS > GodotMaxAnimationSamples ||
					loopMode < 0 || loopMode > 2 || glm::floor(loopMode) != loopMode)
					return false;
				core::DynamicArray<GodotAnimationTrack> tracks;
				if (!readGodotTracks(props, rootPath, paths, tracks))
					return false;
				if (tracks.empty())
					continue;
				const int frames = (int)glm::ceil(length * GodotAnimationFPS - 1e-6);
				if ((int64_t)(frames + 1) * tracks.size() > GodotMaxAnimationSamples) {
					Log::error("Godot animation exceeds the import sample limit");
					return false;
				}
				if (!graph.hasAnimation(name))
					graph.addAnimation(name);
				for (const auto &entry : graph.nodes()) {
					auto &node = entry->value;
					node.setAnimation(name);
					scenegraph::SceneGraphKeyFrames keyframes;
					scenegraph::SceneGraphKeyFrame pose;
					pose.setTransform(rest.find(node.id())->value);
					keyframes.push_back(pose);
					node.setKeyFrames(keyframes);
				}
				core::Map<int, bool> targets;
				for (const GodotAnimationTrack &track : tracks)
					targets.put(track.nodeId, true);
				for (const auto &target : targets) {
					auto &node = graph.node(target->key);
					scenegraph::SceneGraphKeyFrames keyframes;
					keyframes.reserve(frames + 1);
					for (int frame = 0; frame <= frames; ++frame) {
						scenegraph::SceneGraphKeyFrame keyframe;
						keyframe.frameIdx = frame;
						keyframe.setTransform(rest.find(node.id())->value);
						auto &transform = keyframe.transform();
						const double time = glm::min(frame / GodotAnimationFPS, length);
						for (const GodotAnimationTrack &track : tracks) {
							if (track.nodeId != node.id())
								continue;
							const glm::vec4 v = sampleGodotTrack(track, time, length, loopMode != 0);
							if (track.channel == 0)
								transform.setLocalTranslation(glm::vec3(v) * scale);
							else if (track.channel == 1)
								transform.setLocalOrientation(glm::quat(v.w, v.x, v.y, v.z));
							else
								transform.setLocalScale(glm::vec3(v));
						}
						keyframes.push_back(keyframe);
					}
					node.setKeyFrames(keyframes);
				}
				graph.node(0).setProperty(core::String(GodotAnimationLength) + name,
										  core::String::format("%.9g", length));
				graph.node(0).setProperty(core::String(GodotAnimationLoop) + name,
										  core::String::format("%i", (int)loopMode));
				if (firstAnimation.empty() && playbackName != "RESET")
					firstAnimation = name;
				if (godotString(p.get("autoplay")) == playbackName)
					autoplay = name;
			}
		}
	}
	if (!autoplay.empty())
		graph.node(0).setProperty("godot_animation_autoplay", autoplay);
	graph.setAnimation(!autoplay.empty() ? autoplay : firstAnimation.empty() ? originalAnimation : firstAnimation);
	graph.updateTransforms();
	return true;
}

} // namespace

bool GodotSceneFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
									  scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	GodotDocument doc;
	if (!readDocument(filename, archive, doc) || doc.sections[0].tag != "gd_scene" ||
		(doc.sections[0].attributes.has("format") && doc.sections[0].attributes.get("format").number() != 3)) {
		Log::error("Invalid Godot 4 text scene");
		return false;
	}
	GodotResources resources(archive);
	core::StringMap<int> paths;
	bool root = true;
	for (const GodotSection &section : doc.sections) {
		if (section.tag != "node")
			continue;
		if (stopExecution())
			return false;
		const GodotValue &a = section.attributes;
		const GodotValue &p = section.properties;
		if (a.has("instance")) {
			Log::error("Instanced Godot scenes are not supported; make the scene local before importing");
			return false;
		}
		int parent = 0;
		core::String path = ".";
		if (!root) {
			const core::String &parentPath = a.get("parent").text;
			auto entry = paths.find(parentPath);
			if (entry == paths.end()) {
				Log::error("Missing Godot parent node %s", parentPath.c_str());
				return false;
			}
			parent = entry->value;
			path = parentPath == "." ? a.get("name").text : parentPath + "/" + a.get("name").text;
		} else if (a.has("parent"))
			return false;
		if (paths.hasKey(path))
			return false;
		const core::String &type = a.get("type").text;
		if (type == "AnimationPlayer" || type == "AnimationTree") {
			paths.put(path, parent);
			root = false;
			continue;
		}
		int nodeId;
		if (type == "MeshInstance3D" && p.has("mesh") && p.get("mesh").text != "null") {
			Mesh mesh;
			if (!loadGodotMesh(resources, doc, p.get("mesh"), p, mesh))
				return false;
			nodeId = importMesh(a.get("name").text, sceneGraph, core::move(mesh), parent, false, ctx.progress);
		} else {
			scenegraph::SceneGraphNodeType nodeType = scenegraph::SceneGraphNodeType::Group;
			if (type == "Camera3D")
				nodeType = scenegraph::SceneGraphNodeType::Camera;
			else if (type == "Marker3D")
				nodeType = scenegraph::SceneGraphNodeType::Point;
			if (root && nodeType == scenegraph::SceneGraphNodeType::Group) {
				nodeId = sceneGraph.root().id();
				sceneGraph.node(nodeId).setName(a.get("name").text);
			} else {
				scenegraph::SceneGraphNode node(nodeType);
				node.setName(a.get("name").text);
				nodeId = sceneGraph.emplace(core::move(node), parent);
			}
		}
		if (nodeId == InvalidNodeId)
			return false;
		scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
		glm::mat4 matrix(1.0f);
		if (!godotTransform(p, matrix))
			return false;
		matrix[3] *= glm::vec4(sceneInputScale(), 1.0f);
		node.transform(0).setLocalMatrix(matrix);
		node.setVisible(p.get("visible").text != "false");
		if (node.isCameraNode()) {
			auto &camera = scenegraph::toCameraNode(node);
			camera.setNearPlane((float)p.get("near").number(0.05));
			camera.setFarPlane((float)p.get("far").number(4000.0));
			camera.setFieldOfView((int)p.get("fov").number(75));
			if (p.get("projection").number() == 1)
				camera.setOrthographic();
			else
				camera.setPerspective();
		}
		for (size_t i = 0; i < p.keys.size(); ++i) {
			if (core::string::startsWith(p.keys[i], "metadata/"))
				node.setProperty(p.keys[i].substr(9), p.values[i].text);
		}
		paths.put(path, nodeId);
		root = false;
	}
	sceneGraph.updateTransforms();
	return !root && loadGodotAnimations(resources, doc, paths, sceneGraph, sceneInputScale());
}

// https://docs.godotengine.org/en/stable/classes/class_transform3d.html
static core::String createTransform(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
									const glm::vec3 &scale) {
	glm::mat4 m;
	if (node.allKeyFrames().hasKey("Default") && !node.keyFrames("Default").empty()) {
		m = node.keyFrames("Default")[0].transform().calculateLocalMatrix();
	} else {
		m = sceneGraph.transformForFrame(node, 0).worldMatrix();
		if (!node.isRootNode()) {
			const int parent = sceneGraph.parentId(node);
			m = glm::inverse(sceneGraph.transformForFrame(sceneGraph.node(parent), 0).worldMatrix()) * m;
		}
	}
	m[3] *= glm::vec4(scale, 1.0f);
	return core::String::format("Transform3D(%.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g, %.9g)",
								m[0][0], m[1][0], m[2][0], m[0][1], m[1][1], m[2][1], m[0][2], m[1][2], m[2][2],
								m[3][0], m[3][1], m[3][2]);
}

// https://docs.godotengine.org/de/4.x/classes/class_color.html
static core::String createColor(const color::RGBA &color) {
	const glm::vec4 &colorf = color::fromRGBA(color);
	return core::String::format("Color(%f, %f, %f, %f)", colorf.r, colorf.g, colorf.b, colorf.a);
}

// https://docs.godotengine.org/de/4.x/classes/class_standardmaterial3d.html
static bool saveMaterial(const scenegraph::SceneGraphNode &node, io::SeekableWriteStream &stream, int &subResourceId,
						 bool textured) {
	const palette::Palette &palette = node.palette();
	for (size_t i = 0; i < palette.size(); ++i) {
		wrapBool(stream.writeStringFormat(false, "[sub_resource type=\"StandardMaterial3D\" id=\"mat_%i\"]\n",
										  subResourceId))
		const color::RGBA &color = palette.color(i);
		const core::String &colorStr = createColor(textured ? color::RGBA(255, 255, 255, 255) : color);
		wrapBool(stream.writeStringFormat(false, "albedo_color = %s\n", colorStr.c_str()))
		if (textured) {
			wrapBool(stream.writeStringFormat(
				false, "albedo_texture = ExtResource(\"texture_%i\")\ntexture_filter = 0\ntexture_repeat = false\n",
				node.id()))
		}
		if (color.a < 255 || textured) {
			wrapBool(stream.writeString("transparency = 1\n", false))
		}
		const palette::Material &material = palette.material(i);
		if (material.has(palette::MaterialMetal)) {
			wrapBool(stream.writeStringFormat(false, "metallic = %f\n", material.value(palette::MaterialMetal)))
		}
		if (material.has(palette::MaterialSpecular)) {
			wrapBool(
				stream.writeStringFormat(false, "metallic_specular = %f\n", material.value(palette::MaterialSpecular)))
		}
		if (material.has(palette::MaterialRoughness)) {
			wrapBool(stream.writeStringFormat(false, "roughness = %f\n", material.value(palette::MaterialRoughness)))
		}
		if (material.has(palette::MaterialEmit)) {
			wrapBool(stream.writeString("emission_enabled = true\nemission = Color(1, 1, 1, 1)\n", false))
			wrapBool(stream.writeStringFormat(false, "emission_energy_multiplier = %f\n",
											  material.value(palette::MaterialEmit)))
		}
		// Refraction strength is not an index of refraction. Store unsupported
		// properties as metadata instead of assigning unrelated Godot parameters.
		wrapBool(stream.writeStringFormat(false, "metadata/vengi_type = %i\n", (int)material.type))
		for (int p = palette::MaterialMetal; p < (int)palette::MaterialMax; ++p) {
			const auto prop = (palette::MaterialProperty)p;
			if (material.has(prop)) {
				wrapBool(stream.writeStringFormat(false, "metadata/vengi_%s = %.9g\n",
												  palette::MaterialPropertyName(prop), material.value(prop)))
			}
		}
		wrapBool(stream.writeString("\n", false))
		++subResourceId;
	}
	return true;
}

static bool writePackedBytes(io::SeekableWriteStream &stream, const char *name,
							 const io::BufferedReadWriteStream &buffer) {
	wrapBool(stream.writeStringFormat(false, "\t\"%s\": PackedByteArray(", name))
	for (int64_t i = 0; i < buffer.size(); ++i)
		wrapBool(stream.writeStringFormat(false, "%s%u", i ? ", " : "", buffer.getBuffer()[i]))
	return stream.writeString("),\n", false);
}

bool GodotSceneFormat::saveNode(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
								const scenegraph::SceneGraphNode &node, io::SeekableWriteStream &stream,
								const ChunkMeshes &meshes, int &subResourceId, WriterStage stage,
								const glm::vec3 &scale, const core::Map<int, core::String> &paths) const {
	if (stopExecution()) {
		return false;
	}
	if (stage == WriterStage::SUB_RESOURCE) {
		if (node.isAnyModelNode()) {
			auto iter = meshIdxNodeMap.find(node.id());
			if (iter == meshIdxNodeMap.end()) {
				Log::error("Could not find mesh for node %s", node.name().c_str());
				return false;
			}

			const ChunkMeshExt &meshExt = meshes[iter->value];
			const bool textured = meshExt.texture && meshExt.texture->isLoaded();
			for (const voxel::Mesh &mesh : meshExt.mesh->mesh) {
				if (textured && !mesh.isEmpty() && mesh.getUVVector().size() != mesh.getNoOfVertices())
					return false;
			}
			int paletteStartOffset = subResourceId;
			if (!saveMaterial(node, stream, subResourceId, textured)) {
				Log::error("Could not save material for node %s", node.name().c_str());
				return false;
			}

			Log::debug("Exporting model %s (%i) (%i meshes total)", meshExt.name.c_str(), node.id(),
					   (int)meshIdxNodeMap.size());
			wrapBool(stream.writeStringFormat(false, "[sub_resource type=\"ArrayMesh\" id=\"%i\"]\n", node.id()))
			wrapBool(
				stream.writeStringFormat(false, "resource_name = \"%s\"\n", core::string::escape(node.name()).c_str()))

			int surfaceIdx = 0;
			const palette::Palette &palette = node.palette();
			wrapBool(stream.writeStringFormat(false, "_surfaces = ["))
			for (size_t c = 0; c < palette.size(); ++c) {
				glm::vec3 mins(FLT_MAX);
				glm::vec3 maxs(-FLT_MAX);

				if (stopExecution()) {
					return false;
				}

				int vertexCount = 0;
				io::BufferedReadWriteStream buffer(1024 * 3 * sizeof(float));
				io::BufferedReadWriteStream normals(1024);
				io::BufferedReadWriteStream uvs(1024);
				// Mesh vertices are relative to the volume origin. Bake the origin and pivot
				// into the mesh while keeping the node's transform intact.
				const glm::vec3 offset = sceneGraph.resolveRegion(node).getLowerCornerf() - meshExt.pivot * meshExt.size;
				meshExt.visitByMaterial(c, [&buffer, &normals, &uvs, textured, &mins, &maxs, &vertexCount, &scale,
											&offset](const voxel::Mesh &mesh, voxel::IndexType index0,
													voxel::IndexType index1, voxel::IndexType index2) {
					const glm::vec3 v[] = {(mesh.getVertex(index0).position + offset) * scale,
										   (mesh.getVertex(index1).position + offset) * scale,
										   (mesh.getVertex(index2).position + offset) * scale};
					glm::vec3 n = glm::cross(v[1] - v[0], v[2] - v[0]);
					const float len = glm::abs(n.x) + glm::abs(n.y) + glm::abs(n.z);
					if (len > 0.0f)
						n /= len;
					glm::vec2 oct(n);
					if (n.z < 0.0f)
						oct = (1.0f - glm::abs(glm::vec2(n.y, n.x))) * glm::vec2(n.x >= 0 ? 1 : -1, n.y >= 0 ? 1 : -1);
					const voxel::IndexType indices[] = {index0, index1, index2};
					for (int i = 2; i >= 0; --i) {
						if (textured) {
							const glm::vec2 &uv = mesh.getUVVector()[indices[i]];
							uvs.writeFloat(uv.x);
							// The greedy atlas UVs already use a top-left origin.
							uvs.writeFloat(uv.y);
						}
						mins = glm::min(mins, v[i]);
						maxs = glm::max(maxs, v[i]);
						buffer.writeFloat(v[i].x);
						buffer.writeFloat(v[i].y);
						buffer.writeFloat(v[i].z);
						normals.writeUInt16((uint16_t)glm::clamp((oct.x * 0.5f + 0.5f) * 65535.0f, 0.0f, 65535.0f));
						normals.writeUInt16((uint16_t)glm::clamp((oct.y * 0.5f + 0.5f) * 65535.0f, 0.0f, 65535.0f));
					}
					vertexCount += 3;
				});
				buffer.seek(0, SEEK_END);
				buffer.write(normals.getBuffer(), normals.size());
				if (vertexCount <= 0) {
					continue;
				}
				if (surfaceIdx > 0) {
					wrapBool(stream.writeString(",\n", false))
				}
				wrapBool(stream.writeStringFormat(false, "{\n"))
				wrapBool(stream.writeStringFormat(false, "\t\"material\":SubResource(\"mat_%i\"),\n",
												  paletteStartOffset + (int)c))
				wrapBool(stream.writeStringFormat(false, "\t\"primitive\":3,\n")) // triangles
				// ARRAY_FORMAT_VERTEX | ARRAY_FORMAT_NORMAL | ARRAY_FLAG_FORMAT_VERSION_2
				wrapBool(stream.writeStringFormat(false, "\t\"format\":%" PRIu64 ",\n",
												  (uint64_t)(34359738371ULL | (textured ? 16 : 0))))
				if (textured)
					wrapBool(writePackedBytes(stream, "attribute_data", uvs))
				wrapBool(writePackedBytes(stream, "vertex_data", buffer))
				wrapBool(stream.writeStringFormat(false, "\t\"vertex_count\": %i,\n", vertexCount))
				wrapBool(stream.writeStringFormat(false, "\t\"aabb\": AABB(%f, %f, %f, %f, %f, %f)\n", mins.x, mins.y,
												  mins.z, maxs.x - mins.x, maxs.y - mins.y, maxs.z - mins.z))
				wrapBool(stream.writeStringFormat(false, "}"))
				surfaceIdx++;
			}
			wrapBool(stream.writeStringFormat(false, "\n]\n"))

			wrapBool(stream.writeString("\n", false))

			++subResourceId;
		}
	} else if (stage == WriterStage::NODES) {
		const auto pathEntry = paths.find(node.id());
		if (pathEntry == paths.end())
			return false;
		const core::String &path = pathEntry->value;
		core::String name = node.isRootNode() ? node.name() : core::string::extractFilenameWithExtension(path);
		core::String parent = ".";
		if (!node.isRootNode())
			parent = paths.find(sceneGraph.parentId(node))->value;
		const char *type = "Node3D";
		if (node.isAnyModelNode())
			type = "MeshInstance3D";
		else if (node.isCameraNode())
			type = "Camera3D";
		else if (node.isPointNode())
			type = "Marker3D";
		if (node.isRootNode()) {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"%s\"]\n",
											  core::string::escape(name).c_str(), type))
		} else {
			wrapBool(stream.writeStringFormat(false, "[node name=\"%s\" type=\"%s\" parent=\"%s\"]\n",
											  core::string::escape(name).c_str(), type,
											  core::string::escape(parent).c_str()))
		}
		if (node.isAnyModelNode())
			wrapBool(stream.writeStringFormat(false, "mesh = SubResource(\"%i\")\n", node.id()))
		wrapBool(stream.writeStringFormat(false, "visible = %s\n", node.visible() ? "true" : "false"))
		wrapBool(stream.writeStringFormat(false, "transform = %s\n", createTransform(sceneGraph, node, scale).c_str()))
		if (node.isCameraNode()) {
			const auto &camera = scenegraph::toCameraNode(node);
			wrapBool(stream.writeStringFormat(false, "projection = %i\nfov = %i\nnear = %.9g\nfar = %.9g\n",
											  camera.isOrthographic() ? 1 : 0, camera.fieldOfView(), camera.nearPlane(),
											  camera.farPlane()))
		}
		for (const auto &property : node.properties()) {
			if (core::string::startsWith(property->key, GodotAnimationLength) ||
				core::string::startsWith(property->key, GodotAnimationLoop) ||
				property->key == "godot_animation_autoplay")
				continue;
			wrapBool(stream.writeStringFormat(false, "\"metadata/%s\" = \"%s\"\n",
											  core::string::escape(property->key).c_str(),
											  core::string::escape(property->value).c_str()))
		}
	}
	wrapBool(stream.writeString("\n", false))

	for (const core::UUID &childUUID : node.children()) {
		const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(childUUID);
		if (child == nullptr) {
			continue;
		}
		if (child->isRootNode()) {
			return false;
		}
		if (!saveNode(meshIdxNodeMap, sceneGraph, *child, stream, meshes, subResourceId, stage, scale, paths)) {
			Log::error("Failed to save node %s", child->name().c_str());
			return false;
		}
	}
	return true;
}

struct GodotAnimationClip {
	core::String name;
	core::String library;
	core::String playbackName;
	int lastFrame = 0;
	double length = 1.0;
	int loopMode = 0;
};

static core::DynamicArray<GodotAnimationClip> godotAnimationClips(const scenegraph::SceneGraph &graph) {
	core::DynamicArray<GodotAnimationClip> clips;
	core::StringMap<bool> usedNames;
	for (const core::String &name : graph.animations()) {
		GodotAnimationClip clip;
		clip.name = name;
		bool animated = false;
		for (const auto &entry : graph.nodes()) {
			const auto &node = entry->value;
			if (!node.allKeyFrames().hasKey(name))
				continue;
			const auto &keys = node.keyFrames(name);
			animated |= keys.size() > 1;
			if (!keys.empty())
				clip.lastFrame = glm::max(clip.lastFrame, keys.back().frameIdx);
		}
		if (name == "Default" && !animated)
			continue;
		const size_t slash = name.find("/");
		clip.library = slash == core::String::npos ? core::String("") : name.substr(0, slash);
		clip.playbackName = slash == core::String::npos ? name : name.substr(slash + 1);
		for (core::String *part : {&clip.library, &clip.playbackName}) {
			core::string::replaceAllChars(*part, "/:,[]", '_');
		}
		if (clip.playbackName.empty())
			clip.playbackName = "Animation";
		while (usedNames.hasKey(clip.library + "/" + clip.playbackName))
			clip.playbackName += "_2";
		usedNames.put(clip.library + "/" + clip.playbackName, true);
		clip.length = clip.lastFrame > 0 ? clip.lastFrame / GodotAnimationFPS : 1.0;
		const core::String length = graph.root().property(core::String(GodotAnimationLength) + name);
		double seconds = 0.0;
		core::string::parseDouble(length, seconds);
		// Imported clips can end between two frames. Preserve that final time until
		// the user edits the timeline beyond the original duration.
		if (glm::isfinite(seconds) && seconds > 0 &&
			(clip.lastFrame == 0 || (seconds > (clip.lastFrame - 1) / GodotAnimationFPS && seconds <= clip.length)))
			clip.length = seconds;
		const core::String loop = graph.root().property(core::String(GodotAnimationLoop) + name);
		clip.loopMode = glm::clamp(core::string::toInt(loop), 0, 2);
		clips.push_back(core::move(clip));
	}
	return clips;
}

static core::DynamicArray<core::String> godotAnimationLibraries(const core::DynamicArray<GodotAnimationClip> &clips) {
	core::DynamicArray<core::String> libraries;
	for (const auto &clip : clips) {
		bool found = false;
		for (const auto &library : libraries)
			found |= library == clip.library;
		if (!found)
			libraries.push_back(clip.library);
	}
	return libraries;
}

static bool saveGodotAnimations(const scenegraph::SceneGraph &graph, const core::Map<int, core::String> &paths,
								const core::DynamicArray<GodotAnimationClip> &clips, const glm::vec3 &scale,
								io::SeekableWriteStream &stream) {
	for (size_t i = 0; i < clips.size(); ++i) {
		const auto &clip = clips[i];
		if (clip.lastFrame > GodotMaxAnimationSamples ||
			(int64_t)(clip.lastFrame + 1) * paths.size() > GodotMaxAnimationSamples) {
			Log::error("Godot animation exceeds the export sample limit");
			return false;
		}
		wrapBool(stream.writeStringFormat(
			false,
			"[sub_resource type=\"Animation\" id=\"vengi_animation_%i\"]\nresource_name = \"%s\"\nmetadata/vengi_name "
			"= \"%s\"\nlength = %.9g\nloop_mode = %i\nstep = %.9g\n",
			(int)i, core::string::escape(clip.playbackName).c_str(), core::string::escape(clip.name).c_str(),
			clip.length, clip.loopMode, 1.0 / GodotAnimationFPS))
		int track = 0;
		for (const auto &entry : graph.nodes()) {
			const auto &node = entry->value;
			const auto path = paths.find(node.id());
			if (path == paths.end())
				continue;
			const auto &keys = node.allKeyFrames().hasKey(clip.name) ? node.keyFrames(clip.name) : node.keyFrames();
			if (keys.empty())
				return false;
			const int lastFrame = keys.size() > 1 ? clip.lastFrame : 0;
			core::Buffer<scenegraph::SceneGraphTransform> samples(lastFrame + 1);
			core::Buffer<bool> steps(lastFrame + 1);
			for (int frame = 0; frame <= lastFrame; ++frame)
				samples[frame] = scenegraph::sampleLocalTransform(keys, frame, &steps[frame]);
			static const char *types[] = {"position_3d", "rotation_3d", "scale_3d"};
			for (int channel = 0; channel < 3; ++channel, ++track) {
				wrapBool(stream.writeStringFormat(
					false,
					"tracks/%i/type = \"%s\"\ntracks/%i/path = NodePath(\"%s\")\ntracks/%i/interp = "
					"1\ntracks/%i/loop_wrap = false\ntracks/%i/keys = PackedFloat32Array(",
					track, types[channel], track, core::string::escape(path->value).c_str(), track, track, track))
				for (int frame = 0; frame <= lastFrame; ++frame) {
					const auto &transform = samples[frame];
					const double time = frame > 0 && frame == clip.lastFrame ? clip.length : frame / GodotAnimationFPS;
					wrapBool(
						stream.writeStringFormat(false, "%s%.9g, %i, ", frame ? ", " : "", time, steps[frame] ? 0 : 1))
					if (channel == 1) {
						glm::quat q = transform.localOrientation();
						// Float noise at 180 degrees can make Godot's escn optimizer
						// choose the opposite rotation arc after removing intermediate keys.
						for (int c = 0; c < 4; ++c) {
							if (glm::abs(q[c]) < 1e-7f)
								q[c] = 0.0f;
						}
						q = glm::normalize(q);
						wrapBool(stream.writeStringFormat(false, "%.9g, %.9g, %.9g, %.9g", q.x, q.y, q.z, q.w))
					} else {
						const glm::vec3 v =
							channel == 0 ? transform.localTranslation() * scale : transform.localScale();
						wrapBool(stream.writeStringFormat(false, "%.9g, %.9g, %.9g", v.x, v.y, v.z))
					}
				}
				wrapBool(stream.writeString(")\n", false))
			}
		}
		wrapBool(stream.writeString("\n", false))
	}
	const auto libraries = godotAnimationLibraries(clips);
	for (size_t lib = 0; lib < libraries.size(); ++lib) {
		wrapBool(stream.writeStringFormat(
			false, "[sub_resource type=\"AnimationLibrary\" id=\"vengi_animation_library_%i\"]\n_data = {", (int)lib))
		bool first = true;
		for (size_t i = 0; i < clips.size(); ++i) {
			if (clips[i].library != libraries[lib])
				continue;
			wrapBool(stream.writeStringFormat(false, "%s\"%s\": SubResource(\"vengi_animation_%i\")",
											  first ? "" : ",\n", core::string::escape(clips[i].playbackName).c_str(),
											  (int)i))
			first = false;
		}
		wrapBool(stream.writeString("}\n\n", false))
	}
	return true;
}

static bool saveGodotProject(const core::String &filename, const io::ArchivePtr &archive) {
	if (!core::getVar(cfg::VoxformatGodotProject)->boolVal()) {
		return true;
	}
	const core::String directory = core::string::extractDir(filename);
	// A scene exported into an existing project must not create a nested project
	// or replace the user's main scene and other project settings.
	if (!findGodotProject(directory, archive).empty())
		return true;
	const core::String projectFilename = directory + "project.godot";
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(projectFilename));
	if (!stream) {
		Log::error("Could not open Godot project file %s", projectFilename.c_str());
		return false;
	}
	return stream->writeStringFormat(
		false, "config_version=5\n\n[application]\nconfig/name = \"%s\"\nrun/main_scene = \"res://%s\"\n",
		core::string::escape(core::string::extractFilename(filename)).c_str(),
		core::string::escape(core::string::extractFilenameWithExtension(filename)).c_str());
}

bool GodotSceneFormat::saveMeshes(const core::Map<int, int> &meshIdxNodeMap, const scenegraph::SceneGraph &sceneGraph,
								  const ChunkMeshes &meshes, const core::String &filename,
								  const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad, bool withColor,
								  bool withTexCoords) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	Log::debug("Create godot scene file %s", filename.c_str());
	const auto clips = godotAnimationClips(sceneGraph);
	const auto libraries = godotAnimationLibraries(clips);
	int steps = 1 + (int)clips.size() + (int)libraries.size();
	core::Map<int, core::String> paths;
	core::StringMap<bool> usedPaths;
	// Visit parents before children, assigning valid, unique sibling names.
	core::DynamicArray<int> pending;
	pending.push_back(sceneGraph.root().id());
	paths.put(sceneGraph.root().id(), ".");
	for (size_t i = 0; i < pending.size(); ++i) {
		const scenegraph::SceneGraphNode &n = sceneGraph.node(pending[i]);
		if (n.isAnyModelNode())
			steps += (int)n.palette().size() + 1;
		for (const core::UUID &uuid : n.children()) {
			const scenegraph::SceneGraphNode *child = sceneGraph.findNodeByUUID(uuid);
			if (!child)
				return false;
			core::String name = child->name();
			if (name.empty())
				name = "Node";
			for (size_t j = 0; j < name.size(); ++j) {
				if (strchr(".:@/\\\"%", name[j]) || (uint8_t)name[j] < 32)
					name[j] = '_';
			}
			const core::String &parentPath = paths.find(n.id())->value;
			const core::String prefix = parentPath == "." ? core::String("") : parentPath + "/";
			core::String path = prefix + name;
			while (usedPaths.hasKey(path))
				path += core::String::format("_%i", child->id());
			usedPaths.put(path, true);
			paths.put(child->id(), path);
			pending.push_back(child->id());
		}
	}
	for (const ChunkMeshExt &mesh : meshes) {
		if (mesh.texture && mesh.texture->isLoaded())
			++steps;
	}
	wrapBool(stream->writeStringFormat(false, "[gd_scene load_steps=%i format=3]\n\n", steps))
	for (const ChunkMeshExt &mesh : meshes) {
		if (!mesh.texture || !mesh.texture->isLoaded())
			continue;
		const core::String textureName =
			core::string::extractFilename(filename) + core::String::format("-%i.png", mesh.nodeId);
		core::ScopedPtr<io::SeekableWriteStream> textureStream(
			archive->writeStream(core::string::extractDir(filename) + textureName));
		if (!textureStream || !mesh.texture->writePNG(*textureStream))
			return false;
		wrapBool(stream->writeStringFormat(false, "[ext_resource type=\"Texture2D\" path=\"%s\" id=\"texture_%i\"]\n\n",
										   core::string::escape(textureName).c_str(), mesh.nodeId))
	}

	int subResourceId = 0;
	const scenegraph::SceneGraphNode &node = sceneGraph.root();
	if (!saveNode(meshIdxNodeMap, sceneGraph, node, *stream, meshes, subResourceId, WriterStage::SUB_RESOURCE, scale,
				  paths)) {
		Log::error("Failed to save root node");
		return false;
	}
	if (!saveGodotAnimations(sceneGraph, paths, clips, scale, *stream))
		return false;
	if (!saveNode(meshIdxNodeMap, sceneGraph, node, *stream, meshes, subResourceId, WriterStage::NODES, scale, paths)) {
		return false;
	}
	if (!clips.empty()) {
		core::String playerName = "AnimationPlayer";
		while (usedPaths.hasKey(playerName))
			playerName += "_2";
		wrapBool(stream->writeStringFormat(
			false,
			"[node name=\"%s\" type=\"AnimationPlayer\" parent=\".\"]\nroot_node = NodePath(\"..\")\nlibraries = {",
			playerName.c_str()))
		for (size_t i = 0; i < libraries.size(); ++i) {
			wrapBool(stream->writeStringFormat(false, "%s\"%s\": SubResource(\"vengi_animation_library_%i\")",
											   i ? ", " : "", core::string::escape(libraries[i]).c_str(), (int)i))
		}
		wrapBool(stream->writeString("}\n", false))
		const core::String autoplay = sceneGraph.root().property("godot_animation_autoplay");
		for (const auto &clip : clips) {
			if (clip.name == autoplay)
				wrapBool(stream->writeStringFormat(false, "autoplay = \"%s\"\n",
												   core::string::escape(clip.library.empty()
																			? clip.playbackName
																			: clip.library + "/" + clip.playbackName)
													   .c_str()))
		}
	}
	return saveGodotProject(filename, archive);
}

#undef wrapBool

} // namespace voxelformat
