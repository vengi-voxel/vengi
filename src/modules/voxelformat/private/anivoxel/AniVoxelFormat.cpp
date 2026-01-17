/**
 * @file
 */

#include "AniVoxelFormat.h"
#include "core/Assert.h"
#include "core/FourCC.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "io/StreamUtil.h"
#include "palette/Material.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxel/SparseVolume.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelformat {

#define wrap(read)                                                                                                     \
	if ((read) != 0) {                                                                                                 \
		Log::error("Could not load voxa file: Not enough data in stream " CORE_STRINGIFY(read));                       \
		return false;                                                                                                  \
	}

#define wrapBool(read)                                                                                                 \
	if ((read) != true) {                                                                                              \
		Log::error("Could not load voxa file: Not enough data in stream " CORE_STRINGIFY(read) " (line %i)",           \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

void AniVoxelFormat::seek(const ChunkHeader &header, io::SeekableReadStream &stream) {
	const int64_t pos = header.position + header.offset + header.size;
	if (stream.pos() < pos) {
		stream.seek(pos, SEEK_SET);
	}
	Log::debug("End of chunk at position %d", (int)stream.pos());
}

bool AniVoxelFormat::loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
									   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
									   const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return false;
	}

	uint32_t magic;
	wrap(stream->readUInt32(magic));
	if (magic != FourCC('V', 'O', 'X', 'A')) {
		Log::error("Not a valid VOXA file: %s", filename.c_str());
		return false;
	}

	uint32_t version;
	wrap(stream->readUInt32(version));
	if (version < 100) {
		Log::error("Unsupported VOXA version %u in file: %s", version, filename.c_str());
		return false;
	}

	Log::debug("Loading VOXA version %u", version);

	ChunkHeader header = readChunk(*stream);
	if (header.id == 0) {
		return false;
	}
	if (header.id != FourCC('M', 'A', 'I', 'N')) {
		Log::error("Expected MAIN chunk in VOXA file: %s", filename.c_str());
		return false;
	}

	if (!readArmature(*stream, sceneGraph, ctx, version)) {
		return false;
	}

	if (version >= 102) {
		if (!readPalette(*stream, palette, ctx)) {
			return false;
		}
	}

	if (!readModel(*stream, sceneGraph, palette, ctx, version)) {
		return false;
	}
#if 0
	if (stream->remaining() > 0) {
		readBuffers(*stream, sceneGraph, ctx);
	}
#endif
	return true;
}

bool AniVoxelFormat::readBuffers(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								 const LoadContext &ctx) {
	ChunkHeader header = readChunk(stream);
	if (header.id != FourCC('B', 'U', 'F', 'R')) {
		Log::error("Expected BUFR chunk in VOXA file");
		return false;
	}

	int32_t animCount;
	wrap(stream.readInt32(animCount));
	Log::debug("VOXA file has %d buffers", animCount);
	int32_t unknown1;
	wrap(stream.readInt32(unknown1));
	int32_t unknown2;
	wrap(stream.readInt32(unknown2));
	for (int32_t i = 0; i < animCount; ++i) {
		int32_t id;
		int32_t fps;
		int32_t frameLength;
		wrap(stream.readInt32(id));
		wrap(stream.readInt32(fps));
		wrap(stream.readInt32(frameLength));
		for (int32_t j = 0; j < frameLength; ++j) {
			int32_t indexCnt;
			wrap(stream.readInt32(indexCnt));
			for (int32_t k = 0; k < indexCnt; ++k) {
				int32_t index;
				wrap(stream.readInt32(index));
			}
			int32_t vertexCnt;
			wrap(stream.readInt32(vertexCnt));
			for (int32_t k = 0; k < vertexCnt; ++k) {
				glm::vec3 pos;
				wrapBool(io::readVec3(stream, pos));
				glm::vec3 normal;
				wrapBool(io::readVec3(stream, normal));
				color::RGBA color;
				wrapBool(io::readColor(stream, color));
			}
		}
	}

	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readModel(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
							   const palette::Palette &palette, const LoadContext &ctx, int version) {
	ChunkHeader header = readChunk(stream);
	if (header.id != FourCC('M', 'O', 'D', 'L')) {
		Log::error("Expected MODL chunk in VOXA file");
		return false;
	}
	if (version > 101) {
		int32_t cnt;
		wrap(stream.readInt32(cnt));
		Log::debug("VOXA model has %d sub-models", cnt);
		for (int32_t i = 0; i < cnt; ++i) {
			ChunkHeader subHeader = readChunk(stream);
			int32_t id;
			wrap(stream.readInt32(id));
			core::String name;
			wrapBool(stream.readPascalStringUInt32LE(name));
			int32_t w, h, d; // not empty bounds
			wrap(stream.readInt32(w));
			wrap(stream.readInt32(h));
			wrap(stream.readInt32(d));
			int32_t numVoxels;
			wrap(stream.readInt32(numVoxels));
			Log::debug("VOXA sub-model has %d voxels", numVoxels);
			const voxel::Region region(0, 0, 0, w - 1, h - 1, d - 1);
			if (!region.isValid()) {
				Log::error("Invalid region in VOXA model %s: %i:%i:%i", name.c_str(), w, h, d);
				return false;
			}
			voxel::SparseVolume v;
			for (int32_t j = 0; j < numVoxels; ++j) {
				int32_t x, y, z;
				wrap(stream.readInt32(x));
				wrap(stream.readInt32(y));
				wrap(stream.readInt32(z));
				int32_t boneId;
				wrap(stream.readInt32(boneId));
				int32_t palIdx;
				if (version >= 102) {
					wrap(stream.readInt32(palIdx));
				} else {
					color::RGBA color;
					wrapBool(io::readColor(stream, color));
					palIdx = palette.getClosestMatch(color);
				}
				const int boneIdx = boneId - 1;
				v.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx, 0u, 0u, boneIdx));
			}
			seek(subHeader, stream);

			const voxel::Region &sparseRegion = v.calculateRegion();
			if (!sparseRegion.isValid()) {
				Log::warn("Skipping empty VOXA sub-model: %s", name.c_str());
				continue;
			}
			scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
			node.setName(name);
			node.setPalette(palette);
			node.setProperty("id", core::string::toString(id));
			voxel::RawVolume *vol = new voxel::RawVolume(sparseRegion);
			v.copyTo(*vol);
			node.setVolume(vol, true);
			sceneGraph.emplace(core::move(node));
		}
	} else {
		int32_t w;
		wrap(stream.readInt32(w));
		int32_t h;
		wrap(stream.readInt32(h));
		int32_t d;
		wrap(stream.readInt32(d));
		int32_t numVoxels;
		wrap(stream.readInt32(numVoxels));
		const voxel::Region region(0, 0, 0, w - 1, h - 1, d - 1);
		if (!region.isValid()) {
			Log::error("Invalid region in VOXA model");
			return false;
		}
		voxel::SparseVolume v;
		for (int32_t i = 0; i < numVoxels; ++i) {
			int32_t x, y, z;
			wrap(stream.readInt32(x));
			wrap(stream.readInt32(y));
			wrap(stream.readInt32(z));
			int32_t boneId;
			wrap(stream.readInt32(boneId));
			color::RGBA color;
			wrapBool(io::readColor(stream, color));
			const int palIdx = palette.getClosestMatch(color);
			// the root bone has id 1
			const int boneIdx = boneId - 1;
			v.setVoxel(x, y, z, voxel::createVoxel(palette, palIdx, 0u, 0u, boneIdx));
		}
		scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
		voxel::RawVolume *vol = new voxel::RawVolume(v.calculateRegion());
		v.copyTo(*vol);
		node.setVolume(vol, true);
		node.setPalette(palette);
		sceneGraph.emplace(core::move(node));
	}
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readArmature(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								  const LoadContext &ctx, int version) {
	ChunkHeader header = readChunk(stream);
	if (header.id != FourCC('A', 'R', 'M', 'A')) {
		Log::error("Expected ARMA chunk in VOXA file");
		return false;
	}
	int32_t bonesCnt;
	int32_t animationsCnt;
	wrap(stream.readInt32(bonesCnt));
	wrap(stream.readInt32(animationsCnt));
	Log::debug("VOXA file has %d bones and %d animations", bonesCnt, animationsCnt);
	for (int32_t i = 0; i < bonesCnt; ++i) {
		if (!readBone(stream, sceneGraph, ctx)) {
			return false;
		}
	}
	for (int32_t i = 0; i < animationsCnt; ++i) {
		if (!readAnimation(stream, sceneGraph, ctx, version)) {
			return false;
		}
	}
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readBone(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
							  const LoadContext &ctx) {
	ChunkHeader header = readChunk(stream);
	int32_t boneId = -1;
	wrap(stream.readInt32(boneId));
	int32_t parentBoneId = -1;
	wrap(stream.readInt32(parentBoneId));
	Log::debug("Reading bone %i with parent %i", boneId, parentBoneId);
	core::String boneName;
	wrapBool(stream.readPascalStringUInt32LE(boneName));
	float length = 0.0f;
	wrap(stream.readFloat(length));
	glm::vec3 offset;
	wrapBool(io::readVec3(stream, offset));
	glm::vec3 rotation;
	wrapBool(io::readVec3(stream, rotation));
	color::RGBA color;
	wrapBool(io::readColor(stream, color));
	color::RGBA color2;
	wrapBool(io::readColor(stream, color2));
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readAnimation(io::SeekableReadStream &stream, scenegraph::SceneGraph &sceneGraph,
								   const LoadContext &ctx, int version) {
	ChunkHeader header = readChunk(stream);
	int32_t animationId = -1;
	wrap(stream.readInt32(animationId));
	core::String animationName;
	wrapBool(stream.readPascalStringUInt32LE(animationName));
	int32_t fps;
	wrap(stream.readInt32(fps));
	int32_t frameLength;
	wrap(stream.readInt32(frameLength));
	int cnt;
	wrap(stream.readInt32(cnt));
	int cnt2;
	if (version >= 101) {
		wrap(stream.readInt32(cnt2));
	}
	for (int i = 0; i < cnt; ++i) {
		ChunkHeader header2 = readChunk(stream);
		int num4;
		wrap(stream.readInt32(num4));
		int num5 = (version >= 103) ? 9 : 6;
		for (int j = 0; j < num5; ++j) {
			ChunkHeader header3 = readChunk(stream);
			core::String id;
			wrapBool(stream.readPascalStringUInt32LE(id));
			int num6;
			wrap(stream.readInt32(num6));
			for (int k = 0; k < num6; ++k) {
				float x, y;
				wrap(stream.readFloat(x));
				wrap(stream.readFloat(y));
				scenegraph::InterpolationType interpType = scenegraph::InterpolationType::Linear;
				if (version >= 103) {
					uint8_t inter;
					wrap(stream.readUInt8(inter));
					switch (inter) {
					case 0:
						interpType = scenegraph::InterpolationType::Linear;
						break;
					case 1:
						interpType = scenegraph::InterpolationType::QuadEaseIn;
						break;
					case 2:
						interpType = scenegraph::InterpolationType::QuadEaseOut;
						break;
					case 3:
						interpType = scenegraph::InterpolationType::QuadEaseInOut;
						break;
					default:
						break;
					}
				}
				Log::debug("Interpolation type: %s", scenegraph::InterpolationTypeStr[int(interpType)]);
			}
			seek(header3, stream);
		}
		seek(header2, stream);
	}
	for (int i = 0; i < cnt2; ++i) {
		ChunkHeader header2 = readChunk(stream);
		int num4;
		wrap(stream.readInt32(num4));
		for (int j = 0; j < 2; ++j) {
			core::String id;
			wrapBool(stream.readPascalStringUInt32LE(id));
			if (id == "vi") {
				int visibleCnt;
				wrap(stream.readInt32(visibleCnt));
				for (int k = 0; k < visibleCnt; ++k) {
					/* bool visible = */ stream.readBool();
				}
			} else if (id == "sm") {
				int smearFrameCnt;
				wrap(stream.readInt32(smearFrameCnt));
				for (int k = 0; k < smearFrameCnt; ++k) {
					uint8_t smearFrameLength;
					wrap(stream.readUInt8(smearFrameLength));
				}
			}
		}
		seek(header2, stream);
	}
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readPalette(io::SeekableReadStream &stream, palette::Palette &palette, const LoadContext &ctx) {
	ChunkHeader header = readChunk(stream);
	if (header.id != FourCC('M', 'A', 'T', 'P')) {
		return false;
	}

	uint32_t materialCnt;
	wrap(stream.readUInt32(materialCnt));

	for (uint32_t i = 0; i < materialCnt; ++i) {
		if (!readMaterial(stream, palette)) {
			return false;
		}
	}
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::readMaterial(io::SeekableReadStream &stream, palette::Palette &palette) {
	ChunkHeader header = readChunk(stream);
	uint32_t palIdx;
	wrap(stream.readUInt32(palIdx))
	color::RGBA color;
	wrapBool(io::readColor(stream, color))
	palette.setColor(palIdx, color);
	const bool hasMaterialProps = stream.readBool();
	if (hasMaterialProps) {
		uint32_t cnt;
		wrap(stream.readUInt32(cnt))
		palette.setMaterialType(palIdx, palette::MaterialType::Diffuse);
		for (uint32_t i = 0; i < cnt; ++i) {
			core::String propName;
			core::String propValue;
			wrapBool(stream.readPascalStringUInt32LE(propName))
			wrapBool(stream.readPascalStringUInt32LE(propValue))
			if (propName == "_type") {
				if (propValue == "_metal") {
					palette.setMaterialType(palIdx, palette::MaterialType::Metal);
				} else if (propValue == "_glass") {
					palette.setMaterialType(palIdx, palette::MaterialType::Glass);
				} else if (propValue == "_emit") {
					palette.setMaterialType(palIdx, palette::MaterialType::Emit);
				} else if (propValue == "_blend") {
					palette.setMaterialType(palIdx, palette::MaterialType::Blend);
				} else if (propValue == "_media") {
					palette.setMaterialType(palIdx, palette::MaterialType::Media);
				} else {
					Log::debug("VOXA: Unknown material type: %s", propValue.c_str());
				}
			} else {
				const float value = core::string::toFloat(propValue);
				if (propName == "_rough") {
					palette.setRoughness(palIdx, value);
				} else if (propName == "_ior") {
					palette.setIndexOfRefraction(palIdx, value);
				} else if (propName == "_spec") {
					palette.setSpecular(palIdx, value);
				} else if (propName == "_emit") {
					palette.setEmit(palIdx, value);
				} else if (propName == "_metal") {
					palette.setMetal(palIdx, value);
				} else if (propName == "_d") {
					palette.setDensity(palIdx, value);
				} else {
					// TODO: MATERIAL: _alpha, _flux, _ldr, _media, _g, _d, _ri
					Log::debug("VOXA: Material property is not supported yet: %s=%s", propName.c_str(), propValue.c_str());
				}
			}
		}
	}
	seek(header, stream);
	return true;
}

bool AniVoxelFormat::skipChunk(io::SeekableReadStream &stream) {
	ChunkHeader header = readChunk(stream);
	if (header.id == 0) {
		return false;
	}
	seek(header, stream);
	return true;
}

AniVoxelFormat::ChunkHeader AniVoxelFormat::readChunk(io::SeekableReadStream &stream) {
	uint32_t chunkId;
	if (stream.readUInt32(chunkId) != 0) {
		Log::error("Could not load voxa file: Failure to read chunk id");
		return ChunkHeader{0, 0, 0, 0};
	}
	uint8_t out[4];
	FourCCRev(out, chunkId);
	uint32_t chunkOffset;
	if (stream.readUInt32(chunkOffset) != 0) {
		Log::error("Could not load voxa file: Failure to read chunk offset for chunk %.4s", out);
		return ChunkHeader{0, 0, 0, 0};
	}
	uint32_t chunkSize;
	if (stream.readUInt32(chunkSize) != 0) {
		Log::error("Could not load voxa file: Failure to read chunk size for chunk %.4s", out);
		return ChunkHeader{0, 0, 0, 0};
	}

	Log::debug("Found chunk %.4s at offset %u with size %u", out, chunkOffset, chunkSize);
	return ChunkHeader{chunkId, chunkOffset, chunkSize, (uint32_t)stream.pos()};
}

size_t AniVoxelFormat::loadPalette(const core::String &filename, const io::ArchivePtr &archive,
								   palette::Palette &palette, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Failed to open stream for file: %s", filename.c_str());
		return 0u;
	}

	uint32_t magic;
	wrap(stream->readUInt32(magic));
	if (magic != FourCC('V', 'O', 'X', 'A')) {
		Log::error("Not a valid VOXA file: %s", filename.c_str());
		return 0u;
	}

	uint32_t version;
	wrap(stream->readUInt32(version));

	ChunkHeader header = readChunk(*stream);
	if (header.id == 0) {
		return false;
	}
	if (header.id != FourCC('M', 'A', 'I', 'N')) {
		Log::error("Expected MAIN chunk in VOXA file: %s", filename.c_str());
		return false;
	}
	if (!skipChunk(*stream)) {
		return false;
	}

	if (version >= 102) {
		if (!readPalette(*stream, palette, ctx)) {
			return false;
		}
	} else {
		Log::warn("VOXA file version %u does not contain a palette: %s", version, filename.c_str());
		// TODO: VOXELFORMAT: load all rgba colors from the voxel data
		return 0u;
	}
	return palette.colorCount();
}

#undef wrap
#undef wrapBool

#define wrapBool(write)                                                                                                \
	if ((write) != true) {                                                                                             \
		Log::error("Could not save voxa file: Failed to write to stream " CORE_STRINGIFY(write) " (line %i)",           \
				   (int)__LINE__);                                                                                     \
		return false;                                                                                                  \
	}

struct ScopedChunk {
	ScopedChunk(io::SeekableWriteStream &stream, uint32_t id, ScopedChunk *parent) : _stream(stream), _parent(parent) {
		_startPos = stream.pos();
		_stream.writeUInt32(id);
		_stream.writeUInt32(0); // chunksize
		_stream.writeUInt32(0); // childsize
	}
	~ScopedChunk() {
		const int64_t endPos = _stream.pos();
		const uint32_t chunkSize = endPos - _startPos - 12 - _childSize;
		_stream.seek(_startPos + 4, SEEK_SET);
		_stream.writeUInt32(_childSize);
		_stream.writeUInt32(chunkSize);
		_stream.seek(endPos, SEEK_SET);
		if (_parent) {
			_parent->addChildSize(endPos - _startPos);
		}
	}

private:
	io::SeekableWriteStream &_stream;
	int64_t _startPos;
	size_t _childSize = 0;
	ScopedChunk *_parent;
	inline void addChildSize(uint32_t size) {
		_childSize += size;
		if (_parent) {
			_parent->addChildSize(size);
		}
	}
};

bool AniVoxelFormat::saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
								const io::ArchivePtr &archive, const SaveContext &ctx) {
	core::ScopedPtr<io::SeekableWriteStream> stream(archive->writeStream(filename));
	if (!stream) {
		Log::error("Could not open file %s", filename.c_str());
		return false;
	}

	wrapBool(stream->writeUInt32(FourCC('V', 'O', 'X', 'A')))
	wrapBool(stream->writeUInt32(103)) // version

	ScopedChunk mainChunk(*stream, FourCC('M', 'A', 'I', 'N'), nullptr);
	{
		ScopedChunk armatureChunk(*stream, FourCC('A', 'R', 'M', 'A'), &mainChunk);
		int bonesCnt = 0;
		wrapBool(stream->writeInt32(bonesCnt))
		int animationsCnt = 0;
		wrapBool(stream->writeInt32(animationsCnt))
		for (int i = 0; i < bonesCnt; ++i) {
#if 0
			ScopedChunk boneChunk(*stream, FourCC('B', 'O', 'N', 'E'), &armatureChunk);
			wrapBool(stream->writeInt32(boneId))
			wrapBool(stream->writeInt32(boneParentId))
			wrapBool(stream->writePascalStringUInt32LE(boneName))
			wrapBool(stream->writeFloat(boneLength))
			wrapBool(io::writeVec3(stream, boneOffset))
			wrapBool(io::writeVec3(stream, boneRotation))
			wrapBool(io::writeColor(stream, boneColor))
			wrapBool(io::writeColor(stream, boneColor2))
#endif
		}
		for (int i = 0; i < animationsCnt; ++i) {
#if 0
			ScopedChunk animationChunk(*stream, FourCC('A', 'N', 'I', 'M'), &armatureChunk);
			wrapBool(stream->writeInt32(animId))
			wrapBool(stream->writeInt32(animParentId))
			wrapBool(stream->writePascalStringUInt32LE(animName))
			wrapBool(stream->writeInt32(fps))
			wrapBool(stream->writeInt32(frameLength))
			wrapBool(stream->writeInt32(boneGraphCnt))
			wrapBool(stream->writeInt32(meshGraphCnt))
			for (int i = 0; i < boneGraphCnt; ++i) {
				ScopedChunk boneGraphCollectionChunk(*stream, FourCC('A', 'B', 'G', 'C'), &animationChunk);
				wrapBool(stream->writeInt32(boneId))
				// write translation, rotation and scale values (GRS chunks)
				// TODO: VOXELFORMAT: finish me
			}
			for (int i = 0; i < meshGraphCnt; ++i) {
				ScopedChunk meshGraphCollectionChunk(*stream, FourCC('A', 'M', 'G', 'C'), &animationChunk);
				wrapBool(stream->writeInt32(meshId))
				wrapBool(stream->writePascalStringUInt32LE("vi"))
				wrapBool(stream->writeInt32(visibleCnt))
				for (int j = 0; j < visibleCnt; ++j) {
					wrapBool(stream->writeBool(true)) // visible
				}
				wrapBool(stream->writePascalStringUInt32LE("sm"))
				wrapBool(stream->writeInt32(smearFrameCnt))
				for (int j = 0; j < smearFrameCnt; ++j) {
					wrapBool(stream->writeUInt8(smearFrameStrength))
				}
			}
#endif
		}
	}
	const palette::Palette &pal = sceneGraph.mergePalettes(true);
	{
		ScopedChunk paletteChunk(*stream, FourCC('M', 'A', 'T', 'P'), &mainChunk);
		wrapBool(stream->writeInt32(pal.colorCount()))

		for (int i = 0; i < pal.colorCount(); ++i) {
			ScopedChunk palEntryChunk(*stream, FourCC('M', 'A', 'T', 'E'), &paletteChunk);
			wrapBool(stream->writeUInt32(i))
			const color::RGBA color = pal.color(i);
			wrapBool(stream->writeUInt8(color.r))
			wrapBool(stream->writeUInt8(color.g))
			wrapBool(stream->writeUInt8(color.b))
			wrapBool(stream->writeUInt8(color.a))
			const palette::Material &material = pal.material(i);
			const bool rough = material.has(palette::MaterialProperty::MaterialRoughness);
			const bool ior = material.has(palette::MaterialProperty::MaterialIndexOfRefraction);
			// TODO: MATERIAL: map me - in ogt_vox this is mapped to ior, too
			// const bool ri = material.has(palette::MaterialProperty::MaterialXXX);
			const bool d = material.has(palette::MaterialProperty::MaterialDensity);
			if (rough || ior || d) {
				stream->writeBool(true);
				uint32_t propCnt = 0;
				if (rough) {
					++propCnt;
				}
				if (ior) {
					++propCnt;
				}
				if (d) {
					++propCnt;
				}
				stream->writeUInt32(propCnt);
				if (rough) {
					core::String roughStr =
						core::string::toString(material.value(palette::MaterialProperty::MaterialRoughness));
					wrapBool(stream->writePascalStringUInt32LE("_rough"))
					wrapBool(stream->writePascalStringUInt32LE(roughStr))
				}
				if (ior) {
					core::String iorStr =
						core::string::toString(material.value(palette::MaterialProperty::MaterialIndexOfRefraction));
					wrapBool(stream->writePascalStringUInt32LE("_ior"))
					wrapBool(stream->writePascalStringUInt32LE(iorStr))
				}
				if (d) {
					core::String dStr =
						core::string::toString(material.value(palette::MaterialProperty::MaterialDensity));
					wrapBool(stream->writePascalStringUInt32LE("_d"))
					wrapBool(stream->writePascalStringUInt32LE(dStr))
				}
			} else {
				wrapBool(stream->writeBool(false))
			}
		}
	}

	{
		ScopedChunk header(*stream, FourCC('M', 'O', 'D', 'L'), &mainChunk);
		core::DynamicArray<const scenegraph::SceneGraphNode *> modelNodes;
		modelNodes.reserve(sceneGraph.nodes().size());
		for (const auto &e : sceneGraph.nodes()) {
			const scenegraph::SceneGraphNode &node = e->value;
			if (!node.isAnyModelNode()) {
				continue;
			}
			modelNodes.push_back(&node);
		}

		wrapBool(stream->writeInt32((int32_t)modelNodes.size()))
		palette::PaletteLookup palLookup(pal);
		for (const scenegraph::SceneGraphNode *node : modelNodes) {
			core_assert_always(node->isAnyModelNode());
			ScopedChunk headerMesh(*stream, FourCC('M', 'E', 'S', 'H'), &header);
			wrapBool(stream->writeUInt32(node->id()))
			wrapBool(stream->writePascalStringUInt32LE(node->name()))
			const voxel::RawVolume *v = sceneGraph.resolveVolume(*node);
			const voxel::Region &region = v->region();
			const palette::Palette &originalPalette = node->palette();
			// TODO: VOXELFORMAT: not empty bounds - but we aren't cropping here yet
			const glm::ivec3 &bounds = region.getDimensionsInVoxels();
			wrapBool(stream->writeInt32(bounds.x))
			wrapBool(stream->writeInt32(bounds.y))
			wrapBool(stream->writeInt32(bounds.z))
			core_assert_always(v != nullptr);
			const int numVoxels = voxelutil::countVoxels(*v);
			wrapBool(stream->writeInt32(numVoxels))
			auto func = [&](int x, int y, int z, const voxel::Voxel &voxel) {
				stream->writeInt32(x);
				stream->writeInt32(y);
				stream->writeInt32(z);
				const int boneId = voxel.getBoneIdx() + 1;
				stream->writeInt32(boneId);
				const int palIdx = voxel.getColor();
				const color::RGBA rgba = originalPalette.color(palIdx);
				const int mappedPalIdx = palLookup.findClosestIndex(rgba);
				stream->writeInt32(mappedPalIdx);
			};
			voxelutil::visitVolume(*v, func);
		}
	}

	return true;
}

#undef wrapBool

} // namespace voxelformat
