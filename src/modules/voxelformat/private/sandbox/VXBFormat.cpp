/**
 * @file
 */

#include "VXBFormat.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "image/Image.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/VolumeVisitor.h"

#define VXB_PRINT_IMAGES 0
#if VXB_PRINT_IMAGES
#include "io/MemoryReadStream.h"
#endif

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

namespace priv {
static const voxel::FaceNames faceNames[] = {voxel::FaceNames::Left, voxel::FaceNames::Right, voxel::FaceNames::Down,
											 voxel::FaceNames::Up,	 voxel::FaceNames::Front, voxel::FaceNames::Back};
// TODO: VOXELFORMAT: document why this right/left flip is needed
static const voxel::FaceNames faceNamesSave[] = {voxel::FaceNames::Right, voxel::FaceNames::Left, voxel::FaceNames::Down,
											 voxel::FaceNames::Up,	 voxel::FaceNames::Front, voxel::FaceNames::Back};

// we have special needs for the visitor order here - to be independent from other use-cases for the
// face visitor, we define our own order here
static voxelutil::VisitorOrder visitorOrderForFace(voxel::FaceNames face) {
	voxelutil::VisitorOrder visitorOrder;
	switch (face) {
	case voxel::FaceNames::Front:
		visitorOrder = voxelutil::VisitorOrder::mYmXZ;
		break;
	case voxel::FaceNames::Back:
		visitorOrder = voxelutil::VisitorOrder::mYXmZ;
		break;
	case voxel::FaceNames::Right:
		visitorOrder = voxelutil::VisitorOrder::mYmZmX;
		break;
	case voxel::FaceNames::Left:
		visitorOrder = voxelutil::VisitorOrder::mYZX;
		break;
	case voxel::FaceNames::Up:
		visitorOrder = voxelutil::VisitorOrder::mZmXmY;
		break;
	case voxel::FaceNames::Down:
		visitorOrder = voxelutil::VisitorOrder::ZmXY;
		break;
	default:
		return voxelutil::VisitorOrder::Max;
	}
	return visitorOrder;
}

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

	float globalOpaque;
	wrap(stream->readFloat(globalOpaque))
	float globalEmissive;
	wrap(stream->readFloat(globalEmissive))
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

	core::DynamicArray<core::String> channels;
	for (uint32_t i = 0; i < channelAmount; ++i) {
		core::String channelName;
		wrapBool(stream->readString(64, channelName, true))
		channels.push_back(channelName);
	}

	// skip image data
	stream->skip(blockSize * blockSize * 4 * uniqueFaces * channelAmount);

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
		palette.setColor(i, color::RGBA(red, green, blue, alpha));
		if (emissive) {
			palette.setEmit(i, emissive);
		}
		palette.setAlpha(i, globalOpaque);
	}
	palette.setSize(materialAmount);
	return (size_t)materialAmount;
}

void VXBFormat::faceTexture(voxel::RawVolume &volume, const palette::Palette &palette, voxel::FaceNames face,
							const image::ImagePtr &diffuse, const image::ImagePtr &emissive) const {
	const int width = volume.region().getWidthInVoxels(); // matches the texture dimentions
	const int area = width * width;

	for (int y = 0; y < width; ++y) {
		for (int x = 0; x < width; ++x) {
			int voxelIdx;
			switch (face) {
			case voxel::FaceNames::Left:
				voxelIdx = (width - y - 1) * width + (width - x - 1) * area;
				break;
			case voxel::FaceNames::Right:
				voxelIdx = width - 1 + (width - y - 1) * width + x * area;
				break;
			case voxel::FaceNames::Down:
				voxelIdx = x + y * area;
				break;
			case voxel::FaceNames::Up:
				voxelIdx = x + (width - 1) * width + (width - y - 1) * area;
				break;
			case voxel::FaceNames::Front:
				voxelIdx = x + (width - y - 1) * width;
				break;
			case voxel::FaceNames::Back:
				voxelIdx = width - x - 1 + (width - y - 1) * width + (width - 1) * area;
				break;
			default:
				return; // invalid face, do nothing
			}
			// we are running in a different x-direction compared to the original
			const glm::ivec3 posFromIndex(width - (voxelIdx % width) - 1, (voxelIdx / width) % width, voxelIdx / area);
			const color::RGBA color = diffuse->colorAt(x, y);
			const color::RGBA emit = emissive->colorAt(x, y);

			if (color.a == 0) {
				// no voxel at all
			} else if (emit.a == 0 || emit == color::RGBA(0, 0, 0, 255)) {
				int mat = palette.getClosestMatch(color);
				if (mat == palette::PaletteColorNotFound) {
					mat = 0;
				}
				volume.setVoxel(posFromIndex, voxel::createVoxel(palette, mat));
			} else {
				int mat = palette.getClosestMatch(emit);
				if (mat == palette::PaletteColorNotFound) {
					mat = 0;
				}
				volume.setVoxel(posFromIndex, voxel::createVoxel(palette, mat));
			}
		}
	}
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
		Log::debug("index for %i is %i", i, indices[i]);
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

	core::DynamicArray<core::String> channels;
	for (uint32_t i = 0; i < channelAmount; ++i) {
		core::String channelName;
		wrapBool(stream->readString(64, channelName, true))
		channels.push_back(channelName);
	}

	core::DynamicArray<image::ImagePtr> diffuseImages;
	diffuseImages.reserve(uniqueFaces);
	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		const core::String name = channels[0] + core::string::toString(i);
		diffuseImages.emplace_back(image::loadRGBAImageFromStream(name, *stream, blockSize, blockSize));

#if VXB_PRINT_IMAGES
		const core::String imgPrint = image::print(diffuseImages[i], false);
		Log::printf("%s\n", imgPrint.c_str());
#endif
	}

	core::DynamicArray<image::ImagePtr> emissiveImages;
	emissiveImages.reserve(uniqueFaces);
	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		const core::String name = channels[1] + core::string::toString(i);
		emissiveImages.emplace_back(image::loadRGBAImageFromStream(name, *stream, blockSize, blockSize));
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
		palette.setColor(i, color::RGBA(red, green, blue, alpha));
		if (hasEmissive) {
			palette.setEmit(i, emissive);
		}
		palette.setAlpha(i, opaque);
	}
	palette.setSize(materialAmount);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	voxel::Region region(0, 0, 0, blockSize - 1, blockSize - 1, blockSize - 1);
	if (!region.isValid()) {
		Log::error("Invalid region for block size %i", blockSize);
		return false;
	}
	voxel::RawVolume *volume = new voxel::RawVolume(region);
	node.setVolume(volume, true);
	node.setPalette(palette);
	for (uint32_t i = 0; i < 6; ++i) {
		const int uniqueFace = indices[i];
		const voxel::FaceNames faceName = priv::faceNames[indices[i]];
		Log::debug("Load face %s for index %i (uniqueFace: %i)", voxel::faceNameString(faceName), (int)i, uniqueFace);
		faceTexture(*volume, palette, faceName, diffuseImages[uniqueFace], emissiveImages[uniqueFace]);
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
	if (region.getWidthInVoxels() != region.getHeightInVoxels() ||
		region.getWidthInVoxels() != region.getDepthInVoxels()) {
		Log::error("Block size must be equal in all dimensions");
		return false;
	}
	uint32_t blockSize = region.getWidthInVoxels();
	wrapBool(stream->writeUInt32(blockSize))
	uint32_t uniqueFaces = 6; // TODO: VOXELFORMAT: calculate unique faces and write the indices - this can reduce the file size
	wrapBool(stream->writeUInt32(uniqueFaces))
	uint32_t indices[6] = {0, 1, 2, 3, 4, 5};
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

	const palette::Palette &palette = model->palette();
	for (uint32_t i = 0; i < uniqueFaces; ++i) {
#if VXB_PRINT_IMAGES
		core::Buffer<color::RGBA> colors;
		colors.reserve(blockSize * blockSize);
#endif
		const int uniqueFace = indices[i];
		const voxel::FaceNames faceName = priv::faceNamesSave[uniqueFace];
		voxelutil::VisitorOrder visitorOrder = priv::visitorOrderForFace(faceName);
		Log::debug("Save face %s for index %i (uniqueFace: %i)", voxel::faceNameString(faceName), (int)i, uniqueFace);
		voxelutil::visitFace(*volume, faceName, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const color::RGBA color = palette.color(voxel.getColor());
			stream->writeUInt8(color.r);
			stream->writeUInt8(color.g);
			stream->writeUInt8(color.b);
			stream->writeUInt8(color.a);
#if VXB_PRINT_IMAGES
			colors.push_back(color);
#endif
		}, visitorOrder);
#if VXB_PRINT_IMAGES
		io::MemoryReadStream memStream(colors.data(), colors.size() * sizeof(color::RGBA));
		image::ImagePtr img = image::loadRGBAImageFromStream("diffuse", memStream, blockSize, blockSize);
		const core::String imgPrint = image::print(img, false);
		Log::printf("%s\n", imgPrint.c_str());
#endif
	}

	for (uint32_t i = 0; i < uniqueFaces; ++i) {
		const voxel::FaceNames faceName = priv::faceNamesSave[indices[i]];
		voxelutil::VisitorOrder visitorOrder = priv::visitorOrderForFace(faceName);
		voxelutil::visitFace(*volume, faceName, [&](int x, int y, int z, const voxel::Voxel &voxel) {
			const color::RGBA color = palette.emitColor(voxel.getColor());
			stream->writeUInt8(color.r);
			stream->writeUInt8(color.g);
			stream->writeUInt8(color.b);
			stream->writeUInt8(color.a);
		}, visitorOrder);
	}

	uint8_t materialAmount = palette.colorCount();
	wrapBool(stream->writeUInt8(materialAmount))
	for (int i = 0; i < (int)materialAmount; ++i) {
		const color::RGBA color = palette.color(i);
		wrapBool(stream->writeUInt8(color.b))
		wrapBool(stream->writeUInt8(color.g))
		wrapBool(stream->writeUInt8(color.r))
		wrapBool(stream->writeUInt8(color.a))
		wrapBool(stream->writeUInt8(palette.hasEmit(i)))
	}

	return true;
}

#undef wrap
#undef wrapBool

} // namespace voxelformat
