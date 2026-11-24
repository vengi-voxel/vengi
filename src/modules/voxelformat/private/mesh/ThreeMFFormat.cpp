/**
 * @file
 */

#include "ThreeMFFormat.h"
#include "color/ColorUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "util/XMLUtil.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "image/Image.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipArchive.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "tinyxml2.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "math/CoordinateSystemUtil.h"
#include "math/Mat3x4f.h"
#include "math/Quatf.h"
#include <stdio.h>

namespace voxelformat {

namespace {

constexpr const char *CoreNs = "http://schemas.microsoft.com/3dmanufacturing/core/2015/02";
constexpr const char *MaterialNs = "http://schemas.microsoft.com/3dmanufacturing/material/2015/02";
constexpr const char *VolumetricNs = "http://schemas.3mf.io/3dmanufacturing/volumetric/2022/01";
constexpr const char *RelNs = "http://schemas.openxmlformats.org/package/2006/relationships";
constexpr const char *TypesNs = "http://schemas.openxmlformats.org/package/2006/content-types";
constexpr const char *ModelRel = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dmodel";
constexpr const char *TextureRel = "http://schemas.microsoft.com/3dmanufacturing/2013/01/3dtexture";
constexpr const char *ModelPart = "3D/3dmodel.model";

const char *localName(const char *name) {
	if (name == nullptr) {
		return "";
	}
	const char *colon = strrchr(name, ':');
	return colon != nullptr ? colon + 1 : name;
}

core::String partPath(const char *target) {
	core::String path(target != nullptr ? target : "");
	if (!path.empty() && path[0] == '/') {
		path = path.substr(1);
	}
	core::string::replaceAllChars(path, '\\', '/');
	return path;
}

bool writeAll(io::SeekableWriteStream &stream, const char *text) {
	return stream.writeString(text, false);
}

uint8_t linear8(uint8_t srgb) {
	return (uint8_t)(color::srgbToLinear(srgb) * 255.0 + 0.5);
}

uint8_t srgb8(uint8_t linear) {
	return (uint8_t)(color::linearToSrgb((float)linear / 255.0f) * 255.0f + 0.5f);
}

math::Vec3f convertPoint(math::CoordinateSystem from, math::CoordinateSystem to, const math::Vec3f &pos) {
	const math::Mat4f converted = math::convertCoordinateSystem(from, to, math::translate(pos));
	return math::Vec3f(converted[3][0], converted[3][1], converted[3][2]);
}

math::Vec3f to3mf(const math::Vec3f &pos, const math::Vec3f &scale) {
	return convertPoint(math::CoordinateSystem::Vengi, math::CoordinateSystem::ThreeMF, pos * scale);
}

math::Vec3f from3mf(const math::Vec3f &pos) {
	return convertPoint(math::CoordinateSystem::ThreeMF, math::CoordinateSystem::Vengi, pos);
}

bool parseFloats(const char *text, float *out, int n) {
	if (text == nullptr) {
		return false;
	}
	const char *cursor = text;
	for (int i = 0; i < n; ++i) {
		if (!core::string::parseReal(&cursor, &out[i])) {
			return false;
		}
	}
	return true;
}

// 3MF writes a row-major 3x4: m00 m01 m02 m10 m11 m12 m20 m21 m22 tx ty tz.
bool parseMatrix(const char *text, math::Mat3x4f &out) {
	float v[12];
	if (!parseFloats(text, v, 12)) {
		return false;
	}
	out = math::Mat3x4f(math::Vec4f(v[0], v[3], v[6], v[9]), math::Vec4f(v[1], v[4], v[7], v[10]),
						math::Vec4f(v[2], v[5], v[8], v[11]));
	return true;
}

bool rotIdentity(const math::Mat3x4f &m) {
	const math::Mat3x4f id = math::Mat3x4f::identity();
	for (int i = 0; i < 3; ++i) {
		if (math::abs(m[i].x - id[i].x) > 0.0001f || math::abs(m[i].y - id[i].y) > 0.0001f ||
			math::abs(m[i].z - id[i].z) > 0.0001f) {
			return false;
		}
	}
	return true;
}

bool diagonalVolume(const math::Mat3x4f &m) {
	return math::abs(m[1].x) < 0.0001f && math::abs(m[2].x) < 0.0001f && math::abs(m[0].y) < 0.0001f &&
		   math::abs(m[2].y) < 0.0001f && math::abs(m[0].z) < 0.0001f && math::abs(m[1].z) < 0.0001f &&
		   math::abs(m[0].x) > 0.0000001f && math::abs(m[1].y) > 0.0000001f && math::abs(m[2].z) > 0.0000001f;
}

math::Vec3f volumeOrigin(const math::Mat3x4f &m) {
	// shortcut: origin only, one texel is one voxel; a scaled field is not resampled
	return math::Vec3f((float)((double)-m[0].w / (double)m[0].x), (float)((double)-m[2].w / (double)m[2].z),
					   (float)((double)m[1].w / (double)m[1].y));
}

bool readText(io::SeekableReadStream &stream, core::String &out) {
	const int64_t size = stream.size();
	// shortcut: model XML is loaded whole, a streaming parser is the upgrade for multi-gigabyte meshes
	if (size <= 0 || size > 512 * 1024 * 1024) {
		Log::error("3MF part has unexpected size %i", (int)size);
		return false;
	}
	stream.seek(0);
	return stream.readString((int)size, out, false);
}

const tinyxml2::XMLElement *findChild(const tinyxml2::XMLElement *parent, const char *name) {
	if (parent == nullptr) {
		return nullptr;
	}
	for (const tinyxml2::XMLElement *child = parent->FirstChildElement(); child != nullptr;
		 child = child->NextSiblingElement()) {
		if (!strcmp(localName(child->Name()), name)) {
			return child;
		}
	}
	return nullptr;
}

struct ColorGroup {
	int id = 0;
	core::DynamicArray<color::RGBA> colors;
};

struct ImageStack {
	int id = 0;
	int columns = 0;
	int rows = 0;
	int sheets = 0;
	core::DynamicArray<core::String> paths;
};

struct VolumeColor {
	int id = 0;
	int functionId = 0;
	math::Mat3x4f transform = math::Mat3x4f::identity();
	bool hasTransform = false;
};

struct Tri {
	int v[3] = {0, 0, 0};
	int pid = 0;
	int p1 = 0;
	bool hasP1 = false;
};

struct ComponentRef {
	int objectId = 0;
	math::Mat3x4f transform = math::Mat3x4f::identity();
};

struct Object3MF {
	int id = 0;
	core::String name;
	int pid = 0;
	int pindex = 0;
	int volumeId = 0;
	bool hasMesh = false;
	core::DynamicArray<math::Vec3f> vertices;
	core::DynamicArray<Tri> tris;
	core::DynamicArray<ComponentRef> components;
};

struct BuildItem {
	int objectId = 0;
	math::Mat3x4f transform = math::Mat3x4f::identity();
};

struct Model3MF {
	core::DynamicArray<ColorGroup> colors;
	core::DynamicArray<ImageStack> images;
	core::DynamicArray<VolumeColor> volumes;
	core::DynamicArray<Object3MF> objects;
	core::DynamicArray<BuildItem> items;
	bool sawLevelSet = false;
	core::Map<int, int> functionImage;
};

const ColorGroup *findColors(const Model3MF &model, int id) {
	for (const ColorGroup &group : model.colors) {
		if (group.id == id) {
			return &group;
		}
	}
	return nullptr;
}

const ImageStack *findImage(const Model3MF &model, int id) {
	for (const ImageStack &image : model.images) {
		if (image.id == id) {
			return &image;
		}
	}
	return nullptr;
}

const VolumeColor *findVolume(const Model3MF &model, int id) {
	for (const VolumeColor &volume : model.volumes) {
		if (volume.id == id) {
			return &volume;
		}
	}
	return nullptr;
}

const Object3MF *findObject(const Model3MF &model, int id) {
	for (const Object3MF &object : model.objects) {
		if (object.id == id) {
			return &object;
		}
	}
	return nullptr;
}

color::RGBA propertyColor(const Model3MF &model, int pid, int pindex) {
	const ColorGroup *group = findColors(model, pid);
	if (group == nullptr || pindex < 0 || pindex >= (int)group->colors.size()) {
		return color::RGBA(255, 255, 255, 255);
	}
	return group->colors[pindex];
}

bool parseColorGroup(const tinyxml2::XMLElement *element, ColorGroup &group) {
	group.id = element->IntAttribute("id");
	const bool base = !strcmp(localName(element->Name()), "basematerials");
	for (const tinyxml2::XMLElement *child = element->FirstChildElement(); child != nullptr;
		 child = child->NextSiblingElement()) {
		const char *hex = base ? child->Attribute("displaycolor") : child->Attribute("color");
		if (hex == nullptr) {
			continue;
		}
		group.colors.push_back(color::fromHex(hex));
	}
	return group.id > 0 && !group.colors.empty();
}

bool parseImage(const tinyxml2::XMLElement *element, ImageStack &image) {
	image.id = element->IntAttribute("id");
	const tinyxml2::XMLElement *stack = findChild(element, "imagestack");
	if (stack == nullptr) {
		return false;
	}
	image.columns = stack->IntAttribute("columncount");
	image.rows = stack->IntAttribute("rowcount");
	image.sheets = stack->IntAttribute("sheetcount");
	for (const tinyxml2::XMLElement *sheet = stack->FirstChildElement(); sheet != nullptr;
		 sheet = sheet->NextSiblingElement()) {
		if (strcmp(localName(sheet->Name()), "imagesheet") != 0) {
			continue;
		}
		const char *path = sheet->Attribute("path");
		if (path == nullptr) {
			return false;
		}
		image.paths.push_back(partPath(path));
	}
	return image.id > 0 && image.columns > 0 && image.rows > 0 && (int)image.paths.size() == image.sheets;
}

bool parseObject(const tinyxml2::XMLElement *element, Object3MF &object) {
	object.id = element->IntAttribute("id");
	if (const char *name = element->Attribute("name")) {
		object.name = name;
	}
	object.pid = element->IntAttribute("pid");
	object.pindex = element->IntAttribute("pindex");
	if (const tinyxml2::XMLElement *mesh = findChild(element, "mesh")) {
		object.hasMesh = true;
		object.volumeId = mesh->IntAttribute("volumeid");
		if (const tinyxml2::XMLElement *vertices = findChild(mesh, "vertices")) {
			for (const tinyxml2::XMLElement *vertex = vertices->FirstChildElement(); vertex != nullptr;
				 vertex = vertex->NextSiblingElement()) {
				if (strcmp(localName(vertex->Name()), "vertex") != 0) {
					continue;
				}
				math::Vec3f pos;
				if (vertex->QueryFloatAttribute("x", &pos.x) != tinyxml2::XML_SUCCESS ||
					vertex->QueryFloatAttribute("y", &pos.y) != tinyxml2::XML_SUCCESS ||
					vertex->QueryFloatAttribute("z", &pos.z) != tinyxml2::XML_SUCCESS) {
					return false;
				}
				object.vertices.push_back(pos);
			}
		}
		if (const tinyxml2::XMLElement *tris = findChild(mesh, "triangles")) {
			for (const tinyxml2::XMLElement *triElem = tris->FirstChildElement(); triElem != nullptr;
				 triElem = triElem->NextSiblingElement()) {
				if (strcmp(localName(triElem->Name()), "triangle") != 0) {
					continue;
				}
				Tri tri;
				if (triElem->QueryIntAttribute("v1", &tri.v[0]) != tinyxml2::XML_SUCCESS ||
					triElem->QueryIntAttribute("v2", &tri.v[1]) != tinyxml2::XML_SUCCESS ||
					triElem->QueryIntAttribute("v3", &tri.v[2]) != tinyxml2::XML_SUCCESS) {
					return false;
				}
				tri.pid = triElem->IntAttribute("pid");
				tri.hasP1 = triElem->QueryIntAttribute("p1", &tri.p1) == tinyxml2::XML_SUCCESS;
				object.tris.push_back(tri);
			}
		}
	}
	if (const tinyxml2::XMLElement *components = findChild(element, "components")) {
		for (const tinyxml2::XMLElement *comp = components->FirstChildElement(); comp != nullptr;
			 comp = comp->NextSiblingElement()) {
			if (strcmp(localName(comp->Name()), "component") != 0) {
				continue;
			}
			if (comp->Attribute("path") != nullptr || comp->Attribute("p:path") != nullptr) {
				// shortcut: production-extension component paths are ignored, only objects in this model part are loaded
				Log::warn("3MF component path is not supported");
			}
			ComponentRef ref;
			ref.objectId = comp->IntAttribute("objectid");
			if (const char *transform = comp->Attribute("transform")) {
				if (!parseMatrix(transform, ref.transform)) {
					return false;
				}
			}
			object.components.push_back(ref);
		}
	}
	return object.id > 0;
}

bool parseModel(const core::String &xml, Model3MF &model) {
	tinyxml2::XMLDocument doc;
	if (doc.Parse(xml.c_str(), xml.size()) != tinyxml2::XML_SUCCESS) {
		Log::error("Failed to parse 3MF model: %s", doc.ErrorStr());
		return false;
	}
	const tinyxml2::XMLElement *root = doc.RootElement();
	if (root == nullptr || strcmp(localName(root->Name()), "model") != 0) {
		Log::error("3MF model part has no model root");
		return false;
	}
	const tinyxml2::XMLElement *resources = findChild(root, "resources");
	if (resources == nullptr) {
		Log::error("3MF model has no resources");
		return false;
	}
	for (const tinyxml2::XMLElement *child = resources->FirstChildElement(); child != nullptr;
		 child = child->NextSiblingElement()) {
		const char *name = localName(child->Name());
		if (!strcmp(name, "colorgroup") || !strcmp(name, "basematerials")) {
			ColorGroup group;
			if (parseColorGroup(child, group)) {
				model.colors.push_back(core::move(group));
			}
		} else if (!strcmp(name, "image3d")) {
			ImageStack image;
			if (!parseImage(child, image)) {
				Log::error("Invalid 3MF image3d");
				return false;
			}
			model.images.push_back(core::move(image));
		} else if (!strcmp(name, "functionfromimage3d")) {
			const int id = child->IntAttribute("id");
			const int imageId = child->IntAttribute("image3did");
			if (id > 0 && imageId > 0) {
				model.functionImage.put(id, imageId);
			}
		} else if (!strcmp(name, "volumedata")) {
			VolumeColor volume;
			volume.id = child->IntAttribute("id");
			if (const tinyxml2::XMLElement *color = findChild(child, "color")) {
				volume.functionId = color->IntAttribute("functionid");
				if (const char *transform = color->Attribute("transform")) {
					volume.hasTransform = parseMatrix(transform, volume.transform);
				}
			}
			if (volume.id > 0) {
				model.volumes.push_back(volume);
			}
		} else if (!strcmp(name, "object")) {
			Object3MF object;
			if (!parseObject(child, object)) {
				Log::error("Invalid 3MF object");
				return false;
			}
			model.objects.push_back(core::move(object));
		} else if (!strcmp(name, "levelset")) {
			model.sawLevelSet = true;
		}
	}
	if (model.sawLevelSet) {
		// shortcut: levelset shapes are not evaluated, the mesh (or image stack) is the imported geometry
		Log::warn("3MF levelset is not evaluated");
	}
	if (const tinyxml2::XMLElement *build = findChild(root, "build")) {
		for (const tinyxml2::XMLElement *item = build->FirstChildElement(); item != nullptr;
			 item = item->NextSiblingElement()) {
			if (strcmp(localName(item->Name()), "item") != 0) {
				continue;
			}
			BuildItem buildItem;
			buildItem.objectId = item->IntAttribute("objectid");
			if (const char *transform = item->Attribute("transform")) {
				if (!parseMatrix(transform, buildItem.transform)) {
					return false;
				}
			}
			if (buildItem.objectId > 0) {
				model.items.push_back(buildItem);
			}
		}
	}
	return !model.objects.empty();
}

core::String modelPartPath(const io::ArchivePtr &zip) {
	if (zip->exists(ModelPart)) {
		return ModelPart;
	}
	core::ScopedPtr<io::SeekableReadStream> rels(zip->readStream("_rels/.rels"));
	if (rels) {
		core::String xml;
		if (readText(*rels, xml)) {
			tinyxml2::XMLDocument doc;
			if (doc.Parse(xml.c_str(), xml.size()) == tinyxml2::XML_SUCCESS) {
				const tinyxml2::XMLElement *root = doc.RootElement();
				for (const tinyxml2::XMLElement *rel = root ? root->FirstChildElement() : nullptr; rel != nullptr;
					 rel = rel->NextSiblingElement()) {
					const char *type = rel->Attribute("Type");
					const char *target = rel->Attribute("Target");
					if (type != nullptr && target != nullptr && strstr(type, "3dmodel") != nullptr) {
						return partPath(target);
					}
				}
			}
		}
	}
	io::ArchiveFiles files;
	zip->list("*.model", files);
	if (!files.empty()) {
		return files[0].fullPath;
	}
	return "";
}

int colorIndexFor(palette::Palette &palette, core::Map<uint32_t, uint8_t> &known, color::RGBA rgba) {
	rgba.a = 255;
	auto iter = known.find(rgba.rgba);
	if (iter != known.end()) {
		return iter->value;
	}
	if (palette.colorCount() >= palette::PaletteMaxColors) {
		return 0;
	}
	const int idx = palette.colorCount();
	palette.setSize(idx + 1);
	palette.setColor((uint8_t)idx, rgba);
	known.put(rgba.rgba, (uint8_t)idx);
	return idx;
}

bool loadVolume(const io::ArchivePtr &zip, const Model3MF &model, const VolumeColor &volumeColor,
				scenegraph::SceneGraph &sceneGraph, const core::String &name, const math::Mat3x4f &itemTransform) {
	auto fn = model.functionImage.find(volumeColor.functionId);
	if (fn == model.functionImage.end()) {
		return false;
	}
	const ImageStack *image = findImage(model, fn->value);
	if (image == nullptr) {
		return false;
	}
	math::Vec3i lower(0);
	if (volumeColor.hasTransform && diagonalVolume(volumeColor.transform)) {
		const math::Vec3f origin = volumeOrigin(volumeColor.transform);
		lower.x = (int)math::round((double)origin.x);
		lower.y = (int)math::round((double)origin.y);
		lower.z = (int)math::round((double)origin.z);
	}
	const math::Vec3i upper(lower.x + image->columns - 1, lower.y + image->sheets - 1, lower.z + image->rows - 1);
	voxel::RawVolume *volume = new voxel::RawVolume(voxel::Region(lower, upper));
	palette::Palette palette;
	core::Map<uint32_t, uint8_t> known(512);
	bool any = false;
	for (int sheet = 0; sheet < image->sheets; ++sheet) {
		core::ScopedPtr<io::SeekableReadStream> stream(zip->readStream(image->paths[sheet]));
		if (!stream) {
			Log::error("Missing 3MF slice %s", image->paths[sheet].c_str());
			delete volume;
			return false;
		}
		const image::ImagePtr slice = image::loadImage(image->paths[sheet], *stream);
		if (!slice || !slice->isLoaded() || slice->width() != image->columns || slice->height() != image->rows) {
			Log::error("3MF slice %s does not match the image stack", image->paths[sheet].c_str());
			delete volume;
			return false;
		}
		for (int row = 0; row < image->rows; ++row) {
			for (int col = 0; col < image->columns; ++col) {
				const color::RGBA sample = slice->colorAt(col, row);
				if (sample.a == 0) {
					continue;
				}
				const color::RGBA rgba(srgb8(sample.r), srgb8(sample.g), srgb8(sample.b), 255);
				const int idx = colorIndexFor(palette, known, rgba);
				volume->setVoxel(lower.x + col, lower.y + sheet, lower.z + row,
								 voxel::createVoxel(palette, (uint8_t)idx));
				any = true;
			}
		}
	}
	if (!any) {
		delete volume;
		return false;
	}
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(name.empty() ? "3mf" : name);
	node.setPalette(palette);
	node.setVolume(volume);
	node.transform(0).setLocalTranslation(from3mf(math::Vec3f(itemTransform[0].w, itemTransform[1].w, itemTransform[2].w)));
	if (!rotIdentity(itemTransform)) {
		Log::warn("3MF volumetric import keeps the stack axis-aligned and drops item rotation");
	}
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool appendMesh(const Model3MF &model, const Object3MF &object, const math::Mat3x4f &transform, Mesh &mesh) {
	const int n = (int)object.vertices.size();
	for (const Tri &tri : object.tris) {
		if (tri.v[0] < 0 || tri.v[1] < 0 || tri.v[2] < 0 || tri.v[0] >= n || tri.v[1] >= n || tri.v[2] >= n) {
			Log::error("3MF triangle index out of range");
			return false;
		}
		const int pid = tri.pid != 0 ? tri.pid : object.pid;
		const int pindex = tri.hasP1 ? tri.p1 : object.pindex;
		const color::RGBA color = propertyColor(model, pid, pindex);
		const int base = (int)mesh.vertices.size();
		for (int i = 0; i < 3; ++i) {
			MeshVertex vertex;
			vertex.pos = from3mf(math::transform(transform, object.vertices[tri.v[i]]));
			vertex.color = color;
			mesh.vertices.push_back(vertex);
			mesh.indices.push_back(base + i);
		}
	}
	return true;
}

bool importObject(const ThreeMFFormat &format, const io::ArchivePtr &zip, const Model3MF &model, int objectId,
				  const math::Mat3x4f &transform, scenegraph::SceneGraph &sceneGraph, core::DynamicArray<int> &stack) {
	for (int seen : stack) {
		if (seen == objectId) {
			Log::error("3MF component cycle");
			return false;
		}
	}
	const Object3MF *object = findObject(model, objectId);
	if (object == nullptr) {
		Log::error("3MF build item references missing object %i", objectId);
		return false;
	}
	stack.push_back(objectId);
	bool ok = true;
	if (object->volumeId != 0 && !format.collectingBounds()) {
		if (const VolumeColor *volume = findVolume(model, object->volumeId)) {
			if (loadVolume(zip, model, *volume, sceneGraph, object->name, transform)) {
				stack.pop();
				return true;
			}
			Log::warn("3MF volume %i could not be read, using the mesh", object->volumeId);
		}
	}
	if (object->hasMesh && !object->tris.empty()) {
		Mesh mesh;
		if (format.collectingBounds() && object->volumeId != 0) {
			if (const VolumeColor *volume = findVolume(model, object->volumeId)) {
				auto fn = model.functionImage.find(volume->functionId);
				const ImageStack *image = fn != model.functionImage.end() ? findImage(model, fn->value) : nullptr;
				if (image != nullptr) {
					math::Vec3f corner(0.0f);
					math::Vec3f extent((float)image->columns, (float)image->sheets, (float)image->rows);
					if (volume->hasTransform && diagonalVolume(volume->transform)) {
						corner = volumeOrigin(volume->transform);
						extent += corner;
					}
					const float xs[2] = {corner.x, extent.x};
					const float ys[2] = {corner.y, extent.y};
					const float zs[2] = {corner.z, extent.z};
					for (int xi = 0; xi < 2; ++xi) {
						for (int yi = 0; yi < 2; ++yi) {
							for (int zi = 0; zi < 2; ++zi) {
								MeshVertex vertex;
								vertex.pos = math::Vec3f(xs[xi], ys[yi], zs[zi]);
								mesh.vertices.push_back(vertex);
							}
						}
					}
				}
			}
		}
		if (mesh.vertices.empty()) {
			ok = appendMesh(model, *object, transform, mesh);
		}
		if (ok && !mesh.vertices.empty()) {
			const core::String name = object->name.empty() ? "3mf" : object->name;
			ok = format.importLoadedMesh(name, sceneGraph, core::move(mesh)) != InvalidNodeId;
		}
	} else if (!object->components.empty()) {
		for (const ComponentRef &comp : object->components) {
			const math::Mat3x4f combined = transform * comp.transform;
			if (!importObject(format, zip, model, comp.objectId, combined, sceneGraph, stack)) {
				ok = false;
				break;
			}
		}
	}
	stack.pop();
	return ok;
}

bool rigidLocal(const scenegraph::SceneGraphTransform &transform, const math::Vec3f &pivot) {
	if (pivot.x != 0.0f || pivot.y != 0.0f || pivot.z != 0.0f) {
		return false;
	}
	const math::Vec3f &sc = transform.worldScale();
	if (math::abs(sc.x - 1.0f) > 0.0001f || math::abs(sc.y - 1.0f) > 0.0001f || math::abs(sc.z - 1.0f) > 0.0001f) {
		return false;
	}
	const math::Quatf q = transform.worldOrientation();
	return math::abs(q.x) < 0.0001f && math::abs(q.y) < 0.0001f && math::abs(q.z) < 0.0001f &&
		   math::abs(q.w - 1.0f) < 0.0001f;
}

struct ExportedObject {
	int meshIndex = 0;
	bool volumetric = false;
	int imageId = 0;
	int functionId = 0;
	int volumeId = 0;
	int colorId = 0;
	int objectId = 0;
	core::String folder;
	math::Vec3i lower{0};
	math::Vec3i dim{0};
	int remap[256];
	int colorCount = 0;
	bool useColor = false;
};

bool writeSlice(io::ZipArchive &zip, const core::String &zipPath, const voxel::RawVolume &volume,
				const palette::Palette &palette, const math::Vec3i &lower, int dimX, int dimZ, int y) {
	core::DynamicArray<uint8_t> pixels;
	pixels.resize((size_t)dimX * (size_t)dimZ * 4u);
	uint8_t *dst = pixels.data();
	for (int z = 0; z < dimZ; ++z) {
		for (int x = 0; x < dimX; ++x) {
			const voxel::Voxel voxel = volume.voxel(lower.x + x, lower.y + y, lower.z + z);
			if (voxel::isAir(voxel.getMaterial())) {
				dst[0] = dst[1] = dst[2] = dst[3] = 0;
			} else {
				const color::RGBA color = palette.color(voxel.getColor());
				dst[0] = linear8(color.r);
				dst[1] = linear8(color.g);
				dst[2] = linear8(color.b);
				dst[3] = 255;
			}
			dst += 4;
		}
	}
	core::ScopedPtr<io::SeekableWriteStream> stream(zip.writeStream(zipPath));
	return stream && image::Image::writePNG(*stream, pixels.data(), dimX, dimZ, 4);
}

} // namespace

bool ThreeMFFormat::writePackage(io::ZipArchive &zip, const scenegraph::SceneGraph &sceneGraph,
								 const ChunkMeshes &meshes, const math::Vec3f &scale, bool withColor, bool volumetric,
								 bool (*stop)()) {
	core::DynamicArray<ExportedObject> objects;
	objects.reserve(meshes.size());
	int nextId = 1;
	bool warnedPose = false;
	for (int meshIndex = 0; meshIndex < (int)meshes.size(); ++meshIndex) {
		if (stop != nullptr && stop()) {
			return false;
		}
		const ChunkMeshExt &meshExt = meshes[meshIndex];
		const scenegraph::SceneGraphNode &node = sceneGraph.node(meshExt.nodeId);
		ExportedObject object;
		object.meshIndex = meshIndex;
		const palette::Palette &palette = sceneGraph.resolvePalette(node);
		for (int i = 0; i < 256; ++i) {
			object.remap[i] = -1;
		}
		uint8_t used[256] = {};
		for (int part = 0; part < voxel::ChunkMesh::Meshes; ++part) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[part];
			if (mesh->isEmpty()) {
				continue;
			}
			const voxel::VertexArray &vertices = mesh->getVertexVector();
			for (const voxel::VoxelVertex &vertex : vertices) {
				if (vertex.colorIndex < palette.colorCount()) {
					used[vertex.colorIndex] = 1;
				}
			}
		}
		if (withColor) {
			for (int i = 0; i < palette.colorCount() && i < 256; ++i) {
				if (used[i] != 0) {
					object.remap[i] = object.colorCount++;
				}
			}
			object.useColor = object.colorCount > 0;
		}
		const voxel::RawVolume *volume = sceneGraph.resolveVolume(node);
		if (volumetric && volume != nullptr) {
			const voxel::Region region = sceneGraph.resolveRegion(node);
			object.dim = region.getDimensionsInVoxels();
			object.lower = region.getLowerCorner();
			if (object.dim.x > 0 && object.dim.y > 0 && object.dim.z > 0) {
				object.volumetric = true;
				object.imageId = nextId++;
				object.functionId = nextId++;
				object.volumeId = nextId++;
				object.folder = core::String::format("3D/volumetric/color%i", (int)objects.size());
				if (!rigidLocal(node.transform(0), meshExt.pivot) && !warnedPose) {
					Log::warn("3MF volumetric data stays in local voxel space; rotation, scale and pivot are not baked");
					warnedPose = true;
				}
				for (int y = 0; y < object.dim.y; ++y) {
					if (stop != nullptr && stop()) {
						return false;
					}
					const core::String path =
						core::String::format("%s/sheet%04i.png", object.folder.c_str(), y);
					if (!writeSlice(zip, path, *volume, palette, object.lower, object.dim.x, object.dim.z, y)) {
						Log::error("Failed to write 3MF slice %s", path.c_str());
						return false;
					}
				}
			}
		}
		if (object.useColor) {
			object.colorId = nextId++;
		}
		object.objectId = nextId++;
		objects.push_back(object);
	}
	if (objects.empty()) {
		Log::error("No 3MF objects to write");
		return false;
	}

	{
		core::ScopedPtr<io::SeekableWriteStream> types(zip.writeStream("[Content_Types].xml"));
		if (!types) {
			return false;
		}
		if (!types->writeStringFormat(false,
									  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
									  "<Types xmlns=\"%s\">\n"
									  "<Default Extension=\"rels\" ContentType=\"application/vnd.openxmlformats-package.relationships+xml\"/>\n"
									  "<Default Extension=\"model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
									  "<Default Extension=\"png\" ContentType=\"image/png\"/>\n"
									  "<Override PartName=\"/3D/3dmodel.model\" ContentType=\"application/vnd.ms-package.3dmanufacturing-3dmodel+xml\"/>\n"
									  "</Types>\n",
									  TypesNs)) {
			return false;
		}
	}
	{
		core::ScopedPtr<io::SeekableWriteStream> rels(zip.writeStream("_rels/.rels"));
		if (!rels || !rels->writeStringFormat(false,
											  "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
											  "<Relationships xmlns=\"%s\">\n"
											  "<Relationship Target=\"/3D/3dmodel.model\" Id=\"rel-1\" Type=\"%s\"/>\n"
											  "</Relationships>\n",
											  RelNs, ModelRel)) {
			return false;
		}
	}

	bool anyVolume = false;
	bool anyColor = false;
	int textureRels = 0;
	for (const ExportedObject &object : objects) {
		anyVolume = anyVolume || object.volumetric;
		anyColor = anyColor || object.useColor;
		if (object.volumetric) {
			textureRels += object.dim.y;
		}
	}
	if (textureRels > 0) {
		core::ScopedPtr<io::SeekableWriteStream> rels(zip.writeStream("3D/_rels/3dmodel.model.rels"));
		if (!rels || !writeAll(*rels, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")) {
			return false;
		}
		if (!rels->writeStringFormat(false, "<Relationships xmlns=\"%s\">\n", RelNs)) {
			return false;
		}
		int rel = 0;
		for (const ExportedObject &object : objects) {
			if (!object.volumetric) {
				continue;
			}
			for (int y = 0; y < object.dim.y; ++y) {
				if (!rels->writeStringFormat(false, "<Relationship Target=\"/%s/sheet%04i.png\" Id=\"tex%i\" Type=\"%s\"/>\n",
											 object.folder.c_str(), y, rel++, TextureRel)) {
					return false;
				}
			}
		}
		if (!writeAll(*rels, "</Relationships>\n")) {
			return false;
		}
	}

	core::ScopedPtr<io::SeekableWriteStream> model(zip.writeStream(ModelPart));
	if (!model || !writeAll(*model, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n")) {
		return false;
	}
	if (!model->writeStringFormat(false, "<model unit=\"millimeter\" xml:lang=\"en-US\" xmlns=\"%s\"", CoreNs)) {
		return false;
	}
	if (anyColor && !model->writeStringFormat(false, " xmlns:m=\"%s\"", MaterialNs)) {
		return false;
	}
	if (anyVolume && !model->writeStringFormat(false, " xmlns:v=\"%s\"", VolumetricNs)) {
		return false;
	}
	if (!writeAll(*model, ">\n<metadata name=\"Application\">vengi</metadata>\n<resources>\n")) {
		return false;
	}

	for (const ExportedObject &object : objects) {
		if (stop != nullptr && stop()) {
			return false;
		}
		const ChunkMeshExt &meshExt = meshes[object.meshIndex];
		const scenegraph::SceneGraphNode &node = sceneGraph.node(meshExt.nodeId);
		const scenegraph::SceneGraphTransform &transform = node.transform(0);
		const palette::Palette &palette = sceneGraph.resolvePalette(node);
		if (object.volumetric) {
			const float sx = math::abs(scale.x) < 0.0000001f ? 1.0f : scale.x;
			const float sy = math::abs(scale.y) < 0.0000001f ? 1.0f : scale.y;
			const float sz = math::abs(scale.z) < 0.0000001f ? 1.0f : scale.z;
			const double m00 = 1.0 / ((double)object.dim.x * (double)sx);
			// 3MF +Y is -vengi Z, so the row axis scales the other way
			const double m11 = -1.0 / ((double)object.dim.z * (double)sz);
			const double m22 = 1.0 / ((double)object.dim.y * (double)sy);
			const double tx = -(double)object.lower.x / (double)object.dim.x;
			const double ty = -(double)object.lower.z / (double)object.dim.z;
			const double tz = -(double)object.lower.y / (double)object.dim.y;
			if (!model->writeStringFormat(false,
										  "<v:image3d id=\"%i\"><v:imagestack rowcount=\"%i\" columncount=\"%i\" sheetcount=\"%i\">\n",
										  object.imageId, object.dim.z, object.dim.x, object.dim.y)) {
				return false;
			}
			for (int y = 0; y < object.dim.y; ++y) {
				if (!model->writeStringFormat(false, "<v:imagesheet path=\"/%s/sheet%04i.png\"/>\n", object.folder.c_str(),
											  y)) {
					return false;
				}
			}
			if (!model->writeStringFormat(
					false,
					"</v:imagestack></v:image3d>\n"
					"<v:functionfromimage3d id=\"%i\" image3did=\"%i\" filter=\"nearest\" tilestyleu=\"clamp\" "
					"tilestylev=\"clamp\" tilestylew=\"clamp\"/>\n"
					"<v:volumedata id=\"%i\"><v:color functionid=\"%i\" channel=\"color\" minfeaturesize=\"1\" "
					"transform=\"%.9g 0 0 0 %.9g 0 0 0 %.9g %.9g %.9g %.9g\"/></v:volumedata>\n",
					object.functionId, object.imageId, object.volumeId, object.functionId, m00, m11, m22, tx, ty, tz)) {
				return false;
			}
		}
		if (object.useColor) {
			if (!model->writeStringFormat(false, "<m:colorgroup id=\"%i\">\n", object.colorId)) {
				return false;
			}
			for (int i = 0; i < palette.colorCount() && i < 256; ++i) {
				if (object.remap[i] < 0) {
					continue;
				}
				const core::String hex = color::toHex(palette.color((uint8_t)i));
				if (!model->writeStringFormat(false, "<m:color color=\"%s\"/>\n", hex.c_str())) {
					return false;
				}
			}
			if (!writeAll(*model, "</m:colorgroup>\n")) {
				return false;
			}
		}

		const core::String name = util::xmlEscape(meshExt.name.c_str());
		if (!model->writeStringFormat(false, "<object id=\"%i\" type=\"model\"", object.objectId)) {
			return false;
		}
		if (!name.empty() && !model->writeStringFormat(false, " name=\"%s\"", name.c_str())) {
			return false;
		}
		if (object.useColor && !model->writeStringFormat(false, " pid=\"%i\" pindex=\"0\"", object.colorId)) {
			return false;
		}
		if (!writeAll(*model, ">\n<mesh")) {
			return false;
		}
		if (object.volumetric && !model->writeStringFormat(false, " volumeid=\"%i\"", object.volumeId)) {
			return false;
		}
		if (!writeAll(*model, ">\n<vertices>\n")) {
			return false;
		}
		for (int part = 0; part < voxel::ChunkMesh::Meshes; ++part) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[part];
			if (mesh->isEmpty()) {
				continue;
			}
			const voxel::VertexArray &vertices = mesh->getVertexVector();
			for (const voxel::VoxelVertex &vertex : vertices) {
				math::Vec3f pos = vertex.position;
				if (!object.volumetric && meshExt.applyTransform) {
					pos = transform.apply(pos, meshExt.pivot * meshExt.size);
				}
				pos = to3mf(pos, scale);
				if (!model->writeStringFormat(false, "<vertex x=\"%.6f\" y=\"%.6f\" z=\"%.6f\"/>\n", pos.x, pos.y, pos.z)) {
					return false;
				}
			}
		}
		if (!writeAll(*model, "</vertices>\n<triangles>\n")) {
			return false;
		}
		int base = 0;
		for (int part = 0; part < voxel::ChunkMesh::Meshes; ++part) {
			const voxel::Mesh *mesh = &meshExt.mesh->mesh[part];
			if (mesh->isEmpty()) {
				continue;
			}
			const voxel::VertexArray &vertices = mesh->getVertexVector();
			const voxel::IndexArray &indices = mesh->getIndexVector();
			if (indices.size() % 3u != 0u) {
				Log::error("3MF export expected triangle indices");
				return false;
			}
			for (size_t i = 0; i < indices.size(); i += 3) {
				const int i0 = base + (int)indices[i];
				const int i1 = base + (int)indices[i + 1];
				const int i2 = base + (int)indices[i + 2];
				if (!model->writeStringFormat(false, "<triangle v1=\"%i\" v2=\"%i\" v3=\"%i\"", i0, i1, i2)) {
					return false;
				}
				if (object.useColor) {
					const int colorIndex = vertices[indices[i]].colorIndex;
					const int p1 = colorIndex < 256 && object.remap[colorIndex] >= 0 ? object.remap[colorIndex] : 0;
					if (!model->writeStringFormat(false, " pid=\"%i\" p1=\"%i\"", object.colorId, p1)) {
						return false;
					}
				}
				if (!writeAll(*model, "/>\n")) {
					return false;
				}
			}
			base += (int)vertices.size();
		}
		if (!writeAll(*model, "</triangles>\n</mesh>\n</object>\n")) {
			return false;
		}
	}
	if (!writeAll(*model, "</resources>\n<build>\n")) {
		return false;
	}
	for (const ExportedObject &object : objects) {
		const ChunkMeshExt &meshExt = meshes[object.meshIndex];
		const scenegraph::SceneGraphNode &node = sceneGraph.node(meshExt.nodeId);
		math::Vec3f translation(0.0f);
		// Mesh vertices are already baked with pivot and world transform. Only the volumetric
		// path keeps local voxels, so the build item has to carry the origin there.
		if (object.volumetric && meshExt.applyTransform) {
			const math::Vec3f origin = node.transform(0).apply(math::Vec3f(0.0f), meshExt.pivot * meshExt.size);
			translation = to3mf(origin, scale);
		}
		const bool moved = math::abs(translation.x) > 0.0001f || math::abs(translation.y) > 0.0001f ||
						   math::abs(translation.z) > 0.0001f;
		if (moved) {
			if (!model->writeStringFormat(false,
										  "<item objectid=\"%i\" transform=\"1 0 0 0 1 0 0 0 1 %.6f %.6f %.6f\"/>\n",
										  object.objectId, translation.x, translation.y, translation.z)) {
				return false;
			}
		} else if (!model->writeStringFormat(false, "<item objectid=\"%i\"/>\n", object.objectId)) {
			return false;
		}
	}
	return writeAll(*model, "</build>\n</model>\n");
}

bool ThreeMFFormat::saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &sceneGraph,
							   const ChunkMeshes &meshes, const core::String &filename, const io::ArchivePtr &archive,
							   const math::Vec3f &scale, bool quad, bool withColor, bool withTexCoords) {
	(void)quad;
	(void)withTexCoords;
	if (meshes.empty()) {
		Log::error("No meshes to write as 3MF");
		return false;
	}
	const bool volumetric = core::getVar(cfg::Voxformat3MFVolumetric)->boolVal();
	const auto stop = []() { return stopExecution(); };
	io::BufferedReadWriteStream blob;
	{
		io::ZipArchive zip;
		if (!zip.init(&blob)) {
			return false;
		}
		if (!writePackage(zip, sceneGraph, meshes, scale, withColor, volumetric, stop)) {
			zip.shutdown();
			return false;
		}
		zip.shutdown();
	}
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open %s", filename.c_str());
		return false;
	}
	return stream->write(blob.getBuffer(), blob.size()) == blob.size();
}

bool ThreeMFFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
								   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	(void)ctx;
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load %s", filename.c_str());
		return false;
	}
	const io::ArchivePtr zip = io::openZipArchive(stream);
	if (!zip) {
		Log::error("%s is not a 3MF package", filename.c_str());
		return false;
	}
	const core::String part = modelPartPath(zip);
	if (part.empty()) {
		Log::error("3MF package has no model part");
		return false;
	}
	core::ScopedPtr<io::SeekableReadStream> modelStream(zip->readStream(part));
	if (!modelStream) {
		Log::error("Could not read 3MF model %s", part.c_str());
		return false;
	}
	core::String xml;
	if (!readText(*modelStream, xml)) {
		return false;
	}
	Model3MF model;
	if (!parseModel(xml, model)) {
		return false;
	}
	if (model.items.empty()) {
		Log::error("3MF model has no build items");
		return false;
	}
	bool any = false;
	for (const BuildItem &item : model.items) {
		if (stopExecution()) {
			return false;
		}
		core::DynamicArray<int> stack;
		if (!importObject(*this, zip, model, item.objectId, item.transform, sceneGraph, stack)) {
			return false;
		}
		any = true;
	}
	return any;
}

} // namespace voxelformat
