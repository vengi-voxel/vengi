/**
 * @file
 */

#include "VENGIFormat.h"
#include "core/ArrayLength.h"
#include "core/FourCC.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/Var.h"
#include "core/collection/Array.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/ZipWriteStream.h"
#include "palette/Material.h"
#include "palette/NormalPalette.h"
#include "palette/PaletteView.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "palette/Palette.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxelutil/VolumeVisitor.h"

#include <glm/gtc/type_ptr.hpp>

#define wrapBool(action)                                                                                               \
	if ((action) != true) {                                                                                            \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__);                    \
		return false;                                                                                                  \
	}

#define wrap(action)                                                                                                   \
	if ((action) == -1) {                                                                                              \
		Log::error("Error: Failed to execute " CORE_STRINGIFY(action) " (line %i)", (int)__LINE__);                    \
		return false;                                                                                                  \
	}

namespace voxelformat {

static scenegraph::SceneGraphNodeType toNodeType(const core::String &type) {
	for (int i = 0; i < lengthof(scenegraph::SceneGraphNodeTypeStr); ++i) {
		if (type == scenegraph::SceneGraphNodeTypeStr[i]) {
			return (scenegraph::SceneGraphNodeType)i;
		}
	}
	return scenegraph::SceneGraphNodeType::Max;
}

static scenegraph::InterpolationType toInterpolationType(const core::String &type) {
	for (int i = 0; i < lengthof(scenegraph::InterpolationTypeStr); ++i) {
		if (type == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Max;
}

bool VENGIFormat::saveNodeProperties(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
									 io::WriteStream &stream) {
	const scenegraph::SceneGraphNodeProperties &properties = node.properties();
	if (properties.empty()) {
		return true;
	}
	wrapBool(stream.writeUInt32(FourCC('P', 'R', 'O', 'P')))
	wrapBool(stream.writeUInt32((uint32_t)properties.size()))
	for (const auto &e : properties) {
		wrapBool(stream.writePascalStringUInt16LE(e->key))
		wrapBool(stream.writePascalStringUInt16LE(e->value))
	}
	return true;
}

bool VENGIFormat::saveAnimation(const scenegraph::SceneGraphNode &node, const core::String &animation,
								io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('A', 'N', 'I', 'M')))
	wrapBool(stream.writePascalStringUInt16LE(animation))
	for (const scenegraph::SceneGraphKeyFrame &keyframe : node.keyFrames(animation)) {
		wrapBool(saveNodeKeyFrame(keyframe, stream))
	}
	wrapBool(stream.writeUInt32(FourCC('E', 'N', 'D', 'A')))
	return true;
}

bool VENGIFormat::saveNodeData(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
							   io::WriteStream &stream) {
	if (node.type() != scenegraph::SceneGraphNodeType::Model) {
		return true;
	}
	wrapBool(stream.writeUInt32(FourCC('D', 'A', 'T', 'A')))
	const voxel::RawVolume *v = node.volume();
	const voxel::Region &region = v->region();
	wrapBool(stream.writeInt32(region.getLowerX()))
	wrapBool(stream.writeInt32(region.getLowerY()))
	wrapBool(stream.writeInt32(region.getLowerZ()))
	wrapBool(stream.writeInt32(region.getUpperX()))
	wrapBool(stream.writeInt32(region.getUpperY()))
	wrapBool(stream.writeInt32(region.getUpperZ()))
	const int replaceIndex = core::Var::getSafe(cfg::VoxformatEmptyPaletteIndex)->intVal();
	int replacement = -1;
	if (replaceIndex != -1) {
		replacement = node.palette().findReplacement(replaceIndex);
		Log::debug("Looking for a similar color in the palette: %d", replacement);
	}
	const int64_t bufSize = region.getHeightInVoxels() * region.getDepthInVoxels() * sizeof(uint8_t) * 3;
	core::DynamicArray<io::BufferedReadWriteStream> streamBuffers;
	streamBuffers.reserve(region.getWidthInVoxels());
	for (int n = 0; n < region.getWidthInVoxels(); ++n) {
		streamBuffers.emplace_back(io::BufferedReadWriteStream(bufSize));
	}
	auto func = [&streamBuffers, replacement, replaceIndex, &region](int x, int, int, const voxel::Voxel &voxel) {
		const bool air = isAir(voxel.getMaterial());
		io::BufferedReadWriteStream &streamBuf = streamBuffers[x - region.getLowerX()];
		streamBuf.writeBool(air);
		if (!air) {
			if (voxel.getColor() == replaceIndex) {
				streamBuf.writeUInt8(replacement);
			} else {
				streamBuf.writeUInt8(voxel.getColor());
			}
			streamBuf.writeUInt8(voxel.getNormal());
		}
	};
	voxelutil::visitVolumeParallel(*v, func, voxelutil::VisitAll(), voxelutil::VisitorOrder::XYZ);
	for (auto &s : streamBuffers) {
		s.seek(0);
		wrapBool(stream.writeStream(s))
		s.trim();
	}
	return true;
}

bool VENGIFormat::saveNodeKeyFrame(const scenegraph::SceneGraphKeyFrame &keyframe, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('K', 'E', 'Y', 'F')))
	wrapBool(stream.writeUInt32(keyframe.frameIdx))
	wrapBool(stream.writeBool(keyframe.longRotation))
	wrapBool(stream.writePascalStringUInt16LE(scenegraph::InterpolationTypeStr[(int)keyframe.interpolation]))
	const scenegraph::SceneGraphTransform &transform = keyframe.transform();
	const float *localMatrix = glm::value_ptr(transform.localMatrix());
	for (int i = 0; i < 16; ++i) {
		wrapBool(stream.writeFloat(localMatrix[i]))
	}
	return true;
}

bool VENGIFormat::saveNodePaletteNormals(const scenegraph::SceneGraph &sceneGraph,
										const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	if (!node.hasNormalPalette()) {
		return true;
	}
	wrapBool(stream.writeUInt32(FourCC('P', 'A', 'L', 'N')))
	const palette::NormalPalette &palette = node.normalPalette();
	wrapBool(stream.writeUInt32(palette.size()))
	for (size_t i = 0; i < palette.size(); ++i) {
		wrapBool(stream.writeUInt32(palette.normal(i).rgba))
	}
	return true;
}

bool VENGIFormat::saveNodePaletteColors(const scenegraph::SceneGraph &sceneGraph,
										const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('P', 'A', 'L', 'C')))
	const palette::Palette &palette = node.palette();
	wrapBool(stream.writePascalStringUInt16LE(palette.name()))
	wrapBool(stream.writeUInt32(palette.colorCount()))
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt32(palette.color(i).rgba))
	}
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt32(0)) // emit colors are not written anymore
	}
	const palette::PaletteIndicesArray &indices = palette.view().uiIndices();
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writeUInt8(indices[i]))
	}
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrapBool(stream.writePascalStringUInt16LE(palette.colorName(i)))
	}

	wrapBool(stream.writeUInt32(palette.colorCount()))
	for (int i = 0; i < palette.colorCount(); ++i) {
		const palette::Material &material = palette.material(i);
		wrapBool(stream.writeUInt32((uint32_t)material.type))
		wrapBool(stream.writeUInt8(palette::MaterialProperty::MaterialMax - 1))
		for (uint32_t n = 0u; n < palette::MaterialProperty::MaterialMax - 1; ++n) {
			const char *materialName = palette::MaterialPropertyNames[n];
			wrapBool(stream.writePascalStringUInt16LE(materialName))
			const palette::MaterialProperty property = (palette::MaterialProperty)(n + 1);
			const float value = material.value(property);
			wrapBool(stream.writeFloat(value));
		}
	}
	return true;
}

bool VENGIFormat::saveNodePaletteIdentifier(const scenegraph::SceneGraph &sceneGraph,
											const scenegraph::SceneGraphNode &node, io::WriteStream &stream) {
	wrapBool(stream.writeUInt32(FourCC('P', 'A', 'L', 'I')))
	wrapBool(stream.writePascalStringUInt16LE(node.palette().name()))
	return true;
}

bool VENGIFormat::saveNode(const scenegraph::SceneGraph &sceneGraph, io::WriteStream &stream,
						   const scenegraph::SceneGraphNode &node) {
	wrapBool(stream.writeUInt32(FourCC('N', 'O', 'D', 'E')))
	wrapBool(stream.writePascalStringUInt16LE(node.name()))
	wrapBool(stream.writePascalStringUInt16LE(scenegraph::SceneGraphNodeTypeStr[(int)node.type()]))
	wrapBool(stream.writeUUID(node.uuid()))
	wrapBool(stream.writeInt32(node.id()))
	wrapBool(stream.writeInt32(node.reference()))
	wrapBool(stream.writeBool(node.visible()))
	wrapBool(stream.writeBool(node.locked()))
	wrapBool(stream.writeUInt32(node.color().rgba))
	wrapBool(stream.writeFloat(node.pivot().x))
	wrapBool(stream.writeFloat(node.pivot().y))
	wrapBool(stream.writeFloat(node.pivot().z))
	wrapBool(saveNodeProperties(sceneGraph, node, stream))
	if (node.palette().isBuiltIn()) {
		wrapBool(saveNodePaletteIdentifier(sceneGraph, node, stream))
	} else {
		wrapBool(saveNodePaletteColors(sceneGraph, node, stream))
	}
	wrapBool(saveNodePaletteNormals(sceneGraph, node, stream))
	wrapBool(saveNodeData(sceneGraph, node, stream))
	for (const core::String &animation : sceneGraph.animations()) {
		wrapBool(saveAnimation(node, animation, stream))
	}
	for (int childId : node.children()) {
		wrapBool(saveNode(sceneGraph, stream, sceneGraph.node(childId)))
	}
	wrapBool(stream.writeUInt32(FourCC('E', 'N', 'D', 'N')))
	return true;
}

bool VENGIFormat::loadNodeProperties(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
									 uint32_t version, io::ReadStream &stream) {
	uint32_t propertyCount;
	wrap(stream.readUInt32(propertyCount))
	Log::debug("Load %u properties", propertyCount);
	for (uint32_t i = 0; i < propertyCount; ++i) {
		core::String key, value;
		wrapBool(stream.readPascalStringUInt16LE(key))
		wrapBool(stream.readPascalStringUInt16LE(value))
		node.setProperty(key, value);
	}
	return true;
}

bool VENGIFormat::loadNodeData(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
							   io::ReadStream &stream) {
	glm::ivec3 mins, maxs;
	wrap(stream.readInt32(mins.x))
	wrap(stream.readInt32(mins.y))
	wrap(stream.readInt32(mins.z))
	wrap(stream.readInt32(maxs.x))
	wrap(stream.readInt32(maxs.y))
	wrap(stream.readInt32(maxs.z))
	Log::debug("Load region of %i:%i:%i %i:%i:%i", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	const voxel::Region region(mins, maxs);
	voxel::RawVolume *v = new voxel::RawVolume(region);
	node.setVolume(v, true);
	const palette::Palette &palette = node.palette();

	if (version >= 4u) {
		union Data {
			uint16_t data;
			struct {
				uint8_t color;
				uint8_t normal;
			};
		};
		voxel::RawVolume::Sampler sampler(*v);
		sampler.setPosition(region.getLowerCorner());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
					const bool air = stream.readBool();
					if (air) {
						sampler3.movePositiveZ();
						continue;
					}

					Data data;
					data.normal = NO_NORMAL;
					stream.readUInt16(data.data);
					sampler3.setVoxel(voxel::createVoxel(palette, data.color, data.normal));
					sampler3.movePositiveZ();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveX();
		}
	} else {
		voxel::RawVolume::Sampler sampler(*v);
		sampler.setPosition(region.getLowerCorner());
		for (int32_t x = region.getLowerX(); x <= region.getUpperX(); ++x) {
			voxel::RawVolume::Sampler sampler2 = sampler;
			for (int32_t y = region.getLowerY(); y <= region.getUpperY(); ++y) {
				voxel::RawVolume::Sampler sampler3 = sampler2;
				for (int32_t z = region.getLowerZ(); z <= region.getUpperZ(); ++z) {
					const bool air = stream.readBool();
					if (air) {
						sampler3.movePositiveZ();
						continue;
					}

					uint8_t color;
					stream.readUInt8(color);
					sampler3.setVoxel(voxel::createVoxel(palette, color));
					sampler3.movePositiveZ();
				}
				sampler2.movePositiveY();
			}
			sampler.movePositiveX();
		}
	}
	return true;
}

bool VENGIFormat::loadNodePaletteNormals(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
										uint32_t version, io::ReadStream &stream) {
	palette::NormalPalette normalPalette;
	uint32_t normalCount;
	wrap(stream.readUInt32(normalCount))
	Log::debug("Load node normal palette with %u normals", normalCount);
	core::Array<color::RGBA, palette::NormalPaletteMaxNormals> normals;
	for (size_t i = 0; i < normalCount; ++i) {
		wrap(stream.readUInt32(normals[i].rgba))
	}
	normalPalette.loadNormalMap(normals.data(), normalCount);
	node.setNormalPalette(normalPalette);
	return true;
}

bool VENGIFormat::loadNodePaletteColors(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
										uint32_t version, io::ReadStream &stream) {
	palette::Palette palette;
	if (version >= 5) {
		core::String name;
		wrapBool(stream.readPascalStringUInt16LE(name))
		palette.setName(name);
	}
	uint32_t colorCount;
	wrap(stream.readUInt32(colorCount))
	Log::debug("Load node palette with %u color", colorCount);
	palette.setSize((int)colorCount);
	color::RGBA colors[palette::PaletteMaxColors];
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt32(colors[i].rgba))
	}
	color::RGBA emitColors[palette::PaletteMaxColors];
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt32(emitColors[i].rgba))
	}
	palette::PaletteIndicesArray &indices = palette.view().uiIndices();
	for (int i = 0; i < palette.colorCount(); ++i) {
		wrap(stream.readUInt8(indices[i]))
	}
	if (version >= 5) {
		for (int i = 0; i < palette.colorCount(); ++i) {
			core::String colorName;
			wrapBool(stream.readPascalStringUInt16LE(colorName))
			palette.setColorName(i, colorName);
		}
	}

	for (int i = 0; i < palette.colorCount(); ++i) {
		palette.setColor(i, colors[i]);
	}
	uint32_t materialCount;
	wrap(stream.readUInt32(materialCount))

	// old non-material save
	if (materialCount == 0u) {
		for (int i = 0; i < palette.colorCount(); ++i) {
			palette.setEmit(i, emitColors[i].a > 0 ? 1.0f : 0.0f);
		}
	}

	for (uint32_t i = 0; i < materialCount; ++i) {
		uint32_t type;
		wrap(stream.readUInt32(type))
		palette.setMaterialType(i, (palette::MaterialType)type);
		uint8_t propertyCount;
		wrap(stream.readUInt8(propertyCount))
		for (uint8_t n = 0; n < propertyCount; ++n) {
			core::String name;
			wrapBool(stream.readPascalStringUInt16LE(name))
			float value;
			wrap(stream.readFloat(value))
			if (name == "glossiness") {
				name = MaterialPropertyName(palette::MaterialProperty::MaterialPhase);
			}
			palette.setMaterialProperty(i, name, value);
		}
	}
	node.setPalette(palette);
	return true;
}

bool VENGIFormat::loadNodePaletteIdentifier(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
											uint32_t version, io::ReadStream &stream) {
	core::String name;
	wrapBool(stream.readPascalStringUInt16LE(name))
	palette::Palette palette;
	Log::debug("Load node palette %s", name.c_str());
	palette.load(name.c_str());
	if (palette.colorCount() == 0) {
		Log::error("Failed to load built-in palette %s", name.c_str());
		return false;
	}
	node.setPalette(palette);
	return true;
}

bool VENGIFormat::loadAnimation(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node, uint32_t version,
								io::ReadStream &stream) {
	core::String animation;
	wrapBool(stream.readPascalStringUInt16LE(animation))
	Log::debug("Load node animation %s", animation.c_str());
	sceneGraph.addAnimation(animation);
	node.setAnimation(animation);
	while (!stream.eos()) {
		uint32_t chunkMagic;
		wrap(stream.readUInt32(chunkMagic))
		if (chunkMagic == FourCC('K', 'E', 'Y', 'F')) {
			if (!loadNodeKeyFrame(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('E', 'N', 'D', 'A')) {
			return true;
		}
	}
	Log::error("ENDA magic is missing");
	return false;
}

bool VENGIFormat::loadNodeKeyFrame(scenegraph::SceneGraph &sceneGraph, scenegraph::SceneGraphNode &node,
								   uint32_t version, io::ReadStream &stream) {
	scenegraph::FrameIndex frameIdx = 0;
	wrap(stream.readInt32(frameIdx))
	scenegraph::KeyFrameIndex keyFrameIdx = node.addKeyFrame(frameIdx);
	if (keyFrameIdx == InvalidKeyFrame) {
		keyFrameIdx = node.keyFrameForFrame(frameIdx);
	}
	scenegraph::SceneGraphKeyFrame &keyframe = node.keyFrame(keyFrameIdx);
	keyframe.longRotation = stream.readBool();
	core::String interpolationType;
	wrapBool(stream.readPascalStringUInt16LE(interpolationType))
	keyframe.interpolation = toInterpolationType(interpolationType);
	Log::debug("Load animation keyframe %u: %s", frameIdx, interpolationType.c_str());
	scenegraph::SceneGraphTransform &transform = keyframe.transform();
	glm::mat4 localMatrix;
	float *localMatrixPtr = glm::value_ptr(localMatrix);
	for (int i = 0; i < 16; ++i) {
		wrap(stream.readFloat(localMatrixPtr[i]))
	}
	transform.setLocalMatrix(localMatrix);
	if (version <= 2) {
		glm::vec3 pivot;
		wrap(stream.readFloat(pivot.x))
		wrap(stream.readFloat(pivot.y))
		wrap(stream.readFloat(pivot.z))
		node.setPivot(pivot);
	}

	return true;
}

bool VENGIFormat::loadNode(scenegraph::SceneGraph &sceneGraph, int parent, uint32_t version, io::ReadStream &stream,
						   NodeMapping &nodeMapping) {
	core::String name;
	wrapBool(stream.readPascalStringUInt16LE(name))
	core::String type;
	wrapBool(stream.readPascalStringUInt16LE(type))
	scenegraph::SceneGraphNodeType nodeType = toNodeType(type);
	if (nodeType == scenegraph::SceneGraphNodeType::Max) {
		Log::error("Could not load node type %s", type.c_str());
		return false;
	}
	core::UUID uuid;
	if (version >= 6) {
		wrap(stream.readUUID(uuid))
	}
	Log::debug("Load node with name '%s' of type %s", name.c_str(), type.c_str());
	const bool isRootNode = nodeType == scenegraph::SceneGraphNodeType::Root;
	int nodeId = isRootNode ? sceneGraph.root().id() : InvalidNodeId;
	if (nodeId == InvalidNodeId) {
		scenegraph::SceneGraphNode node(nodeType, uuid);
		node.setName(name);
		if (nodeType == scenegraph::SceneGraphNodeType::Model) {
			// dummy volume - will be replaced later
			node.setVolume(new voxel::RawVolume(voxel::Region(0, 0)), true);
		}
		nodeId = sceneGraph.emplace(core::move(node), parent);
		if (nodeId == -1) {
			Log::error("Failed to add new node");
			return false;
		}
	}
	if (isRootNode) {
		sceneGraph.setRootUUID(uuid);
	}
	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);

	if (version >= 2) {
		int fileNodeId;
		wrap(stream.readInt32(fileNodeId))
		int referenceNodeId;
		wrap(stream.readInt32(referenceNodeId))
		// will get fixed up later once we know all node ids
		node.setReference(referenceNodeId);
		nodeMapping.put(fileNodeId, nodeId);
	}
	node.setVisible(stream.readBool());
	node.setLocked(stream.readBool());
	color::RGBA color;
	wrap(stream.readUInt32(color.rgba))
	node.setColor(color);
	glm::vec3 pivot(0.0f);
	if (version >= 3) {
		wrap(stream.readFloat(pivot.x))
		wrap(stream.readFloat(pivot.y))
		wrap(stream.readFloat(pivot.z))
	}

	while (!stream.eos()) {
		uint32_t chunkMagic;
		wrap(stream.readUInt32(chunkMagic))
		if (chunkMagic == FourCC('P', 'R', 'O', 'P')) {
			if (!loadNodeProperties(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('D', 'A', 'T', 'A')) {
			if (!loadNodeData(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('P', 'A', 'L', 'C')) {
			if (!loadNodePaletteColors(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('P', 'A', 'L', 'N')) {
			if (!loadNodePaletteNormals(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('P', 'A', 'L', 'I')) {
			if (!loadNodePaletteIdentifier(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('A', 'N', 'I', 'M')) {
			if (!loadAnimation(sceneGraph, node, version, stream)) {
				return false;
			}
		} else if (chunkMagic == FourCC('N', 'O', 'D', 'E')) {
			if (!loadNode(sceneGraph, node.id(), version, stream, nodeMapping)) {
				return false;
			}
		} else if (chunkMagic == FourCC('E', 'N', 'D', 'N')) {
			node.setPivot(pivot);
			return true;
		}
	}
	Log::error("ENDN magic is missing");
	return false;
}

bool VENGIFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
							 const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	Log::debug("Save scenegraph as vengi");
	wrapBool(stream->writeUInt32(FourCC('V', 'E', 'N', 'G')))
	io::ZipWriteStream zipStream(*stream, stream->size());
	wrapBool(zipStream.writeUInt32(6))
	if (!saveNode(sceneGraph, zipStream, sceneGraph.root())) {
		return false;
	}
	return true;
}

bool VENGIFormat::loadGroups(const core::String &filename, const io::ArchivePtr &archive,
							 scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint32_t magic;
	wrap(stream->readUInt32(magic))
	if (magic != FourCC('V', 'E', 'N', 'G')) {
		Log::error("Invalid vengi magic");
		return false;
	}
	io::ZipReadStream zipStream(*stream, stream->size());
	uint32_t version;
	wrap(zipStream.readUInt32(version))
	if (version > 6) {
		Log::error("Unsupported version %u", version);
		return false;
	}
	uint32_t chunkMagic;
	wrap(zipStream.readUInt32(chunkMagic))
	NodeMapping nodeMapping;
	if (chunkMagic == FourCC('N', 'O', 'D', 'E')) {
		if (!loadNode(sceneGraph, sceneGraph.root().id(), version, zipStream, nodeMapping)) {
			return false;
		}
		for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::ModelReference); iter != sceneGraph.end();
			 ++iter) {
			scenegraph::SceneGraphNode &node = *iter;
			int nodeId;
			if (!nodeMapping.get(node.reference(), nodeId)) {
				Log::error("Failed to perform node id mapping for references");
				return false;
			}
			Log::debug("Update node reference for node %i to: %i", node.id(), nodeId);
			node.setReference(nodeId);
		}
		sceneGraph.updateTransforms();
		return true;
	}
	Log::error("Unknown chunk magic");
	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
