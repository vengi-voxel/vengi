/**
 * @file
 */

#pragma once

#include "Format.h"
#include "core/collection/DynamicArray.h"
#include "io/Stream.h"

namespace voxelformat {
/**
 * @brief Taken from gox
 *
 * File format, version 2:
 *
 * This is inspired by the png format, where the file consists of a list of
 * chunks with different types.
 *
 *  4 bytes magic string        : "GOX "
 *  4 bytes version             : 2
 *  List of chunks:
 *      4 bytes: type
 *      4 bytes: data length
 *      n bytes: data
 *      4 bytes: CRC
 *
 *  The layer can end with a DICT:
 *      for each entry:
 *          4 byte : key size (0 = end of dict)
 *          n bytes: key
 *          4 bytes: value size
 *          n bytes: value
 *
 *  chunks types:
 *
 *  IMG : a dict of info:
 *      - box: the image gox.
 *
 *  PREV: a png image for preview.
 *
 *  BL16: a 16^3 block saved as a 64x64 png image.
 *
 *  LAYR: a layer:
 *      4 bytes: number of blocks.
 *      for each block:
 *          4 bytes: block index
 *          4 bytes: x
 *          4 bytes: y
 *          4 bytes: z
 *          4 bytes: 0
 *      [DICT]
 *
 *  CAMR: a camera:
 *      [DICT] containing the following entries:
 *          name: string
 *          dist: float
 *          rot: quaternion
 *          ofs: offset
 *          ortho: bool
 *
 *   LIGH: the light:
 *      [DICT] containing the following entries:
 *          pitch: radian
 *          yaw: radian
 *          intensity: float
 */
class GoxFormat : public RGBAFormat {
private:
	static constexpr int BlockSize = 16;

	struct GoxChunk {
		uint32_t type = 0u;
		int64_t streamStartPos = 0u;
		int32_t length = 0u;
	};

	struct State {
		int32_t version = 0;
		core::DynamicArray<image::ImagePtr> images;
	};

	bool loadChunk_Header(GoxChunk &c, io::SeekableReadStream &stream);
	bool loadChunk_ReadData(io::SeekableReadStream &stream, char *buff, int size);
	void loadChunk_ValidateCRC(io::SeekableReadStream &stream);
	bool loadChunk_DictEntry(const GoxChunk &c, io::SeekableReadStream &stream, char *key, char *value);
	bool loadChunk_LAYR(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);
	bool loadChunk_BL16(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);
	bool loadChunk_MATE(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);
	bool loadChunk_CAMR(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);
	bool loadChunk_IMG(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);
	bool loadChunk_LIGH(State& state, const GoxChunk &c, io::SeekableReadStream &stream, SceneGraph &sceneGraph);

	bool saveChunk_DictEntry(io::SeekableWriteStream &stream, const char *key, const void *value, size_t valueSize);

	template<class T>
	bool saveChunk_DictEntry(io::SeekableWriteStream &stream, const char *key, const T &value) {
		return saveChunk_DictEntry(stream, key, &value, sizeof(T));
	}

	// Write image info and preview pic - not used.
	bool saveChunk_IMG(io::SeekableWriteStream &stream);
	bool saveChunk_PREV(io::SeekableWriteStream &stream);
	// Write all the cameras - not used.
	bool saveChunk_CAMR(io::SeekableWriteStream &stream, const SceneGraph &sceneGraph);
	// Write all the lights - not used.
	bool saveChunk_LIGH(io::SeekableWriteStream &stream);

	// Write all the blocks chunks.
	bool saveChunk_BL16(io::SeekableWriteStream &stream, const SceneGraph &sceneGraph, int &blocks);
	// Write all the materials.
	bool saveChunk_MATE(io::SeekableWriteStream &stream);
	// Write all the layers.
	bool saveChunk_LAYR(io::SeekableWriteStream &stream, const SceneGraph &sceneGraph, int numBlocks);
public:
	bool loadGroups(const core::String &filename, io::SeekableReadStream &stream, SceneGraph &sceneGraph) override;
	bool saveGroups(const SceneGraph &sceneGraph, const core::String &filename,
					io::SeekableWriteStream &stream) override;
	image::ImagePtr loadScreenshot(const core::String &filename, io::SeekableReadStream &stream) override;
};

} // namespace voxel
