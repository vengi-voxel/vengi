/**
 * @file
 */

#include "VXBFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxelutil/ImageUtils.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load vxb file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load vxb file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",            \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

size_t VXBFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
							  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t magic[4];
	wrap(stream->readUInt8(magic[0]))
	wrap(stream->readUInt8(magic[1]))
	wrap(stream->readUInt8(magic[2]))
	wrap(stream->readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'B') {
		Log::error("Could not load vxb file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version = magic[3] - '0';
	if (version != 1) {
		Log::error("Could not load vxb file: Unsupported version found (%i)", version);
		return false;
	}

	float opaque;
	wrap(stream->readFloat(opaque))
	float emissive;
	wrap(stream->readFloat(emissive))
	uint32_t blockSize;
	wrap(stream->readUInt32(blockSize))
	uint32_t uniqueFaces;
	wrap(stream->readUInt32(uniqueFaces))
	uint32_t indices[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readUInt32(indices[i]))
	}
	float uSpeed[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readFloat(uSpeed[i]))
	}
	float vSpeed[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readFloat(vSpeed[i]))
	}
	uint32_t channelAmount;
	wrap(stream->readUInt32(channelAmount))
	if (channelAmount != 2) {
		Log::error("Could not load vxb file: Unsupported channel amount found (%i)", channelAmount);
		return false;
	}

	core::String diffuseId; // "Diffuse"
	wrapBool(stream->readString(sizeof(diffuseId), diffuseId, true))

	core::String emissiveId; // "Emissive"
	wrapBool(stream->readString(sizeof(emissiveId), emissiveId, true))

	image::ImagePtr diffuseImages[6];
	for (int i = 0; i < 6; ++i) {
		diffuseImages[i] =
			image::loadRGBAImageFromStream(diffuseId + core::string::toString(i), *stream, blockSize, blockSize);
	}
	image::ImagePtr emissiveImages[6];
	for (int i = 0; i < 6; ++i) {
		emissiveImages[i] =
			image::loadRGBAImageFromStream(emissiveId + core::string::toString(i), *stream, blockSize, blockSize);
	}

	uint32_t materialAmount;
	wrap(stream->readUInt32(materialAmount))

	for (int i = 0; i < (int)materialAmount; ++i) {
		uint8_t blue;
		wrap(stream->readUInt8(blue));
		uint8_t green;
		wrap(stream->readUInt8(green));
		uint8_t red;
		wrap(stream->readUInt8(red));
		uint8_t alpha;
		wrap(stream->readUInt8(alpha));
		uint8_t emissive;
		wrap(stream->readUInt8(emissive));
		palette.setColor(i, core::RGBA(red, green, blue, alpha));
		if (emissive) {
			palette.setEmit(i, emissive);
		}
		palette.setAlpha(i, opaque);
	}
	palette.setSize(materialAmount);
	return (size_t)materialAmount;
}

bool VXBFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
								  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
								  const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}
	uint8_t magic[4];
	wrap(stream->readUInt8(magic[0]))
	wrap(stream->readUInt8(magic[1]))
	wrap(stream->readUInt8(magic[2]))
	wrap(stream->readUInt8(magic[3]))
	if (magic[0] != 'V' || magic[1] != 'X' || magic[2] != 'B') {
		Log::error("Could not load vxb file: Invalid magic found (%c%c%c%c)", magic[0], magic[1], magic[2], magic[3]);
		return false;
	}
	int version = magic[3] - '0';
	if (version != 1) {
		Log::error("Could not load vxb file: Unsupported version found (%i)", version);
		return false;
	}

	float opaque;
	wrap(stream->readFloat(opaque))
	float emissive;
	wrap(stream->readFloat(emissive))
	uint32_t blockSize;
	wrap(stream->readUInt32(blockSize))
	uint32_t uniqueFaces;
	wrap(stream->readUInt32(uniqueFaces))
	uint32_t indices[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readUInt32(indices[i]))
	}
	float uSpeed[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readFloat(uSpeed[i]))
	}
	float vSpeed[6];
	for (int i = 0; i < 6; i++) {
		wrap(stream->readFloat(vSpeed[i]))
	}
	uint32_t channelAmount;
	wrap(stream->readUInt32(channelAmount))
	if (channelAmount != 2) {
		Log::error("Could not load vxb file: Unsupported channel amount found (%i)", channelAmount);
		return false;
	}

	core::String diffuseId; // "Diffuse"
	wrapBool(stream->readString(sizeof(diffuseId), diffuseId, true))

	core::String emissiveId; // "Emissive"
	wrapBool(stream->readString(sizeof(emissiveId), emissiveId, true))

	Log::debug("diffuseId: %s, emissiveId: %s", diffuseId.c_str(), emissiveId.c_str());

	core::DynamicArray<image::ImagePtr> diffuseImages;
	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		diffuseImages.emplace_back(image::loadRGBAImageFromStream(diffuseId + core::string::toString(i), *stream, blockSize, blockSize));
	}
	core::DynamicArray<image::ImagePtr> emissiveImages;
	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		emissiveImages.emplace_back(image::loadRGBAImageFromStream(emissiveId + core::string::toString(i), *stream, blockSize, blockSize));
	}

	uint8_t materialAmount;
	wrap(stream->readUInt8(materialAmount))
	Log::debug("materialAmount: %i", materialAmount);

	for (int i = 0; i < (int)materialAmount; ++i) {
		uint8_t blue;
		wrap(stream->readUInt8(blue));
		uint8_t green;
		wrap(stream->readUInt8(green));
		uint8_t red;
		wrap(stream->readUInt8(red));
		uint8_t alpha;
		wrap(stream->readUInt8(alpha));
		uint8_t hasEmissive;
		wrap(stream->readUInt8(hasEmissive));
		palette.setColor(i, core::RGBA(red, green, blue, alpha));
		if (hasEmissive) {
			palette.setEmit(i, emissive);
		}
		palette.setAlpha(i, opaque);
	}
	palette.setSize(materialAmount);

	const voxel::FaceNames faceNames[] = {voxel::FaceNames::Left, voxel::FaceNames::Right, voxel::FaceNames::Down,
										  voxel::FaceNames::Up,	  voxel::FaceNames::Front, voxel::FaceNames::Back};

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::Region region(0, 0, 0, blockSize - 1, blockSize - 1, blockSize - 1);
	if (!region.isValid()) {
		Log::error("Invalid region for block size %i", blockSize);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	node.setVolume(volume, true);
	node.setPalette(palette);
	const glm::vec2 uvs[2] = {glm::vec2(0.0f, 0.0f), glm::vec2(1.0f, 1.0f)};
	for (int i = 0; i < 6; ++i) {
		// TODO: emissive is wrong here...
		const int uniqueFace = indices[i];
		voxelutil::importFace(*volume, palette, faceNames[i], diffuseImages[uniqueFace], uvs[0], uvs[1]);
	}
	return sceneGraph.emplace(core::move(node)) != InvalidNodeId;
}

bool VXBFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
						   const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}
	wrapBool(stream->writeUInt32(FourCC('V', 'X', 'B', '1')))
	float opaque = 1.0f;
	wrapBool(stream->writeFloat(opaque))
	float emissive = 0.0f;
	wrapBool(stream->writeFloat(emissive))
	scenegraph::SceneGraphNode *model = sceneGraph.firstModelNode();
	if (model == nullptr) {
		Log::error("No model found in scene graph");
		return false;
	}
	const voxel::RawVolume *volume = model->volume();
	core_assert(volume != nullptr);
	const voxel::Region region = volume->region();
	// take the largest dimension as block size
	uint32_t blockSize =
		core_max(region.getWidthInVoxels(), core_max(region.getHeightInVoxels(), region.getDepthInVoxels()));
	wrapBool(stream->writeUInt32(blockSize))
	uint32_t uniqueFaces = 6; // TODO:
	wrapBool(stream->writeUInt32(uniqueFaces))
	uint32_t indices[6] = {0, 1, 2, 3, 4, 5}; // TODO:
	for (int i = 0; i < 6; i++) {
		wrapBool(stream->writeUInt32(indices[i]))
	}
	float uSpeed[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	for (int i = 0; i < 6; i++) {
		wrapBool(stream->writeFloat(uSpeed[i]))
	}
	float vSpeed[6] = {0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f};
	for (int i = 0; i < 6; i++) {
		wrapBool(stream->writeFloat(vSpeed[i]))
	}
	uint32_t channelAmount = 2;
	wrapBool(stream->writeUInt32(channelAmount))
	wrapBool(stream->writeString("Diffuse", true));
	wrapBool(stream->writeString("Emissive", true));

	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		// TODO: write images for each face
	}

	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		// TODO: write emissive images for each face
	}

	uint8_t materialAmount = model->palette().colorCount();
	wrapBool(stream->writeUInt8(materialAmount))
	for (int i = 0; i < (int)materialAmount; ++i) {
		const core::RGBA color = model->palette().color(i);
		wrapBool(stream->writeUInt8(color.b))
		wrapBool(stream->writeUInt8(color.g))
		wrapBool(stream->writeUInt8(color.r))
		wrapBool(stream->writeUInt8(color.a))
		wrapBool(stream->writeUInt8(model->palette().hasEmit(i)))
	}

	return false;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
