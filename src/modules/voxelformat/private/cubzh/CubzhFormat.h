/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "io/Stream.h"
#include "voxelformat/Format.h"

namespace voxelformat {

/**
 * @brief Cubz (Particubes) 3zh format
 *
 * Cubzh uses a left-handed coordinates system. a positive value along its Z axis will move the object away from the
 * camera, a negative value will bring it closer or behind it if you go below zero
 *
 * 11 bytes |  char[6] | cubzh magic bytes
 *  4 bytes |   uint32 | file format version
 *
 *  4 bytes |   uint32 | preview byte count
 *  n bytes |  char[n] | preview png bytes
 *
 *  1 byte  |    uint8 | color encoding format ID (1 = 4 x uint8 (rgba))
 *  1 byte  |    uint8 | color palette row count
 *  1 byte  |    uint8 | color palette column count
 *  1 byte  |    uint8 | color count (max 255)
 *  n bytes |  char[n] | colors bytes
 *  1 byte  |    uint8 | index of default cube color
 *  1 byte  |    uint8 | index of default background color
 *  1 byte  |    uint8 | index of current cube color
 *  1 byte  |    uint8 | index of current background color
 *
 *  2 bytes |   uint16 | world width (X)
 *  2 bytes |   uint16 | world height (Y)
 *  2 bytes |   uint16 | world depth (Z)
 *  n bytes | uint8[n] | cubes (palette indexes)
 *
 *  4 bytes |  float32 | camera target position (X)
 *  4 bytes |  float32 | camera target position (Y)
 *  4 bytes |  float32 | camera target position (Z)
 *  4 bytes |  float32 | camera distance from target
 *  4 bytes |  float32 | camera rotation (left/right)
 *  4 bytes |  float32 | camera rotation (up/dowm)
 *  4 bytes |  float32 | camera rotation (roll)
 *
 * ================= Added in v2 =================
 *
 *  1 byte  |    uint8 | light enabled (0 or 1)
 *  1 byte  |    uint8 | light locked to creation (0 or 1)
 *  4 bytes |  float32 | light rotation (X)
 *  4 bytes |  float32 | light rotation (Y)
 *
 * ======= cubzh format version 6 ===============
 *
 * Cubzh .3zh File Format [10/20/2022]
 *
 * File
 * {
 *     Header
 *     // PNG Preview
 *     Chunk 'PREVIEW' : optional
 *     // Palette
 *     Chunk 'PALETTE' : optional
 *     // Objects
 *     Chunk 'SHAPE' / Chunk 'OBJECT' : optional
 *     ...
 *     Chunk 'SHAPE' / Chunk 'OBJECT' : optional
 * }
 * -------------------------------------------------------------------------------
 *
 * 1. Header
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 6        | char       | magic bytes 'CUBZH!' : 'C' 'U' 'B' 'Z' 'H' '!', 'C' is first
 * 4        | int        | version number : 6
 * 1        | uint8      | compression method : 0 (none), 1 (zip)
 * 4        | uint32     | total size of data (compressed or not)
 *
 * 2. Chunk Structure (not SubChunks)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | chunk id
 * 4        | uint32     | num bytes of chunk content (N)
 * 1        | uint8      | (0: not compressed - 1: compressed)
 * 4        | uint32     | num bytes of chunk content uncompressed (M)
 * N or M   |            | chunk content
 * -------------------------------------------------------------------------------
 *
 * 3. Chunk id 'PREVIEW' (1) : if it is absent, no PNG preview available
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * N        |            | PNG bytes
 * -------------------------------------------------------------------------------
 *
 * 4. Chunk id 'PALETTE' : optional, contains colors. Not related to any shapes
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | color count (N)
 * N x 4    | uint8      | (r, g, b, alpha) : 1 byte for each entry
 * N        | uint8      | emissive flag
 * -------------------------------------------------------------------------------
 *
 * 5. Chunk id 'OBJECT'
 * {
 *     SubChunk 'OBJECT_ID' : optional (default 1)
 *     SubChunk 'OBJECT_PARENT' : optional
 *     SubChunk 'OBJECT_TRANSFORM' : optional
 *     SubChunk 'OBJECT_PIVOT' : optional
 * }
 *
 * 6. Chunk id 'SHAPE' (3)
 * {
 *     SubChunk 'OBJECT_ID' : optional (default 1)
 *     SubChunk 'OBJECT_NAME' : optional
 *     SubChunk 'OBJECT_PARENT' : optional
 *     SubChunk 'OBJECT_TRANSFORM' : optional
 *     SubChunk 'OBJECT_PIVOT' : optional
 *     SubChunk 'OBJECT_COLLISION_BOX' : optional
 *     SubChunk 'OBJECT_IS_HIDDEN' : optional
 *     SubChunk 'SHAPE_PALETTE' : optional
 *     SubChunk 'SHAPE_SIZE'
 *     SubChunk 'SHAPE_BLOCKS'
 *     SubChunk 'SHAPE_POINT' : optional, multiple (named point)
 *     SubChunk 'SHAPE_POINT_ROTATION' : optional, multiple (named rotation)
 *     SubChunk 'SHAPE_BAKED_LIGHTING' : optional
 * }
 *
 * 7. Subchunk structure
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | byte       | chunk id
 * 4        | uint32     | chunk size
 * -------------------------------------------------------------------------------
 *
 * 8. SubChunk id 'OBJECT_ID' (17) : only for serialization in parent id
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 2        | uint16     | id
 * -------------------------------------------------------------------------------
 *
 * 9. SubChunk id 'OBJECT_NAME' (18) : only for serialization in parent id
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | lenName (N)
 * N        | string     | name
 * -------------------------------------------------------------------------------
 *
 * 10. SubChunk id 'OBJECT_PARENT' (19) : based on chunk 'OBJECT_ID'
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 2        | uint16     | id
 * -------------------------------------------------------------------------------
 *
 * 11. SubChunk id 'OBJECT_TRANSFORM' (20) : local transform, only if chunk 'OBJECT_PARENT' defined
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 4        | float      | position x
 * 4        | float      | position y
 * 4        | float      | position z
 * 4        | float      | rotation x
 * 4        | float      | rotation y
 * 4        | float      | rotation z
 * 4        | float      | scale x
 * 4        | float      | scale y
 * 4        | float      | scale z
 * -------------------------------------------------------------------------------
 *
 * 12. SubChunk id 'OBJECT_PIVOT' (21) : optional, default center of shape
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 4        | float      | position x
 * 4        | float      | position y
 * 4        | float      | position z
 * -------------------------------------------------------------------------------
 *
 * 13. SubChunk id 'OBJECT_COLLISION_BOX' (23) : optional, default is a box containing the whole shape
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 4        | float      | position min x
 * 4        | float      | position min y
 * 4        | float      | position min z
 * 4        | float      | position max x
 * 4        | float      | position max y
 * 4        | float      | position max z
 * -------------------------------------------------------------------------------
 *
 * 14. SubChunk id 'OBJECT_IS_HIDDEN' (24) : optional, visible by default
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | 0 if visible, 1 if hidden
 * -------------------------------------------------------------------------------
 *
 * 15. SubChunk id 'SHAPE_SIZE' (4)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 2        | short      | width (x)
 * 2        | short      | height (y)
 * 2        | short      | depth (z)
 * -------------------------------------------------------------------------------
 *
 * 16. SubChunk id 'SHAPE_BLOCKS' (5)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * C * 1    | uint8      | palette index (255 if air block) (C is blockCount)
 * -------------------------------------------------------------------------------
 *
 * 17. SubChunk id 'SHAPE_POINT' (6)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | name length (N)
 * N * 1    | string     | name
 * 4        | float      | position x
 * 4        | float      | position y
 * 4        | float      | position z
 * -------------------------------------------------------------------------------
 *
 * 18. SubChunk id 'SHAPE_POINT_ROTATION' (8)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | name length (N)
 * N * 1    | string     | name
 * 4        | float      | rotation x
 * 4        | float      | rotation y
 * 4        | float      | rotation z
 * -------------------------------------------------------------------------------
 *
 * 19. SubChunk id 'SHAPE_BAKED_LIGHTING' (7)
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * C * 2    | uint8      | light : 2 bytes per block (C is blockCount)
 * -------------------------------------------------------------------------------
 *
 * 20. SubChunk id 'SHAPE_PALETTE' (22) : optional, contains the colors of the shape
 * -------------------------------------------------------------------------------
 * # Bytes  | Type       | Value
 * -------------------------------------------------------------------------------
 * 1        | uint8      | color count (N)
 * N x 4    | uint8      | (r, g, b, alpha) : 1 byte for each entry
 * N        | uint8      | emissive flag
 * -------------------------------------------------------------------------------
 *
 * @ingroup Formats
 */
class CubzhFormat : public PaletteFormat {
protected:
	struct Header {
		uint32_t version = 0;
		uint32_t totalSize = 0;
		bool legacy = false;		 // .pcubes files
		uint8_t compressionType = 0; // 0 = none, 1 = zip
	};

	struct Chunk {
		uint32_t chunkSize = 0;
		uint32_t uncompressedSize = 0;
		uint8_t chunkId = 0;
		uint8_t compressed = 0;

		bool supportsCompression() const;
	};

	struct ChunkChecker {
		io::SeekableReadStream *_stream;
		int64_t _pos;
		int64_t _size;
		uint8_t _chunkId;

		ChunkChecker(io::SeekableReadStream &stream, const Chunk &chunk) {
			_stream = &stream;
			_pos = stream.pos();
			_size = chunk.chunkSize;
			_chunkId = chunk.chunkId;
		}
		~ChunkChecker() {
			const int64_t expectedPos = _pos + _size;
			if (_stream->pos() != expectedPos) {
				Log::error("Unexpected stream position after reading chunk: %i => %d != %d", (int)_chunkId,
						   (int)_stream->pos(), (int)expectedPos);
			}
		}
	};

	class CubzhReadStream : public io::ReadStream {
	private:
		bool _forwarded;
		io::ReadStream *_stream;
		uint32_t _pos;
		uint32_t _size;
	public:
		CubzhReadStream(const Header &header, const Chunk &chunk, io::SeekableReadStream &forward);
		~CubzhReadStream() override;
		int read(void *dataPtr, size_t dataSize) override;
		bool eos() const override;
		int64_t size() const;
		int64_t pos() const;
		int64_t remaining() const;
		bool empty() const;
	};

	bool loadGroupsPalette(const core::String &filename, const io::ArchivePtr &archive,
						   scenegraph::SceneGraph &sceneGraph, palette::Palette &palette,
						   const LoadContext &ctx) override;
	bool loadAnimations(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) const;
	bool loadChunkHeader(const Header &header, io::ReadStream &stream, Chunk &chunk) const;
	bool loadSubChunkHeader(const Header &header, io::ReadStream &stream, Chunk &chunk) const;
	bool loadHeader(io::SeekableReadStream &stream, Header &header) const;
	bool loadSkipChunk(const Header &header, const Chunk &chunk, io::ReadStream &stream) const;
	bool loadSkipSubChunk(const Chunk &chunk, io::ReadStream &stream) const;

	bool saveModelNode(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
					   io::SeekableWriteStream *stream) const;
	bool savePointNodes(const scenegraph::SceneGraph &sceneGraph, const scenegraph::SceneGraphNode &node,
						io::SeekableWriteStream &stream) const;
	bool saveGroups(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) override;

	bool saveAnimations(const scenegraph::SceneGraph &sceneGraph, const core::String &filename,
					const io::ArchivePtr &archive, const SaveContext &ctx) const;

	bool loadVersion6(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) const;
	bool loadShape6(const core::String &filename, const Header &header, const Chunk &chunk, CubzhReadStream &stream,
					scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx) const;

	bool loadVersion5(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
					  scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) const;

	bool loadPalette5(io::ReadStream &stream, palette::Palette &palette, int version) const;
	bool loadShape5(const core::String &filename, const Header &header, const Chunk &chunk, io::SeekableReadStream &stream,
					scenegraph::SceneGraph &sceneGraph, const palette::Palette &palette, const LoadContext &ctx) const;
	bool loadPalette6(io::ReadStream &stream, palette::Palette &palette) const;
	bool loadPalettePCubes(io::ReadStream &stream, palette::Palette &palette) const;

	bool loadPCubes(const core::String &filename, const Header &header, io::SeekableReadStream &stream,
					scenegraph::SceneGraph &sceneGraph, palette::Palette &palette, const LoadContext &ctx) const;

public:
	size_t loadPalette(const core::String &filename, const io::ArchivePtr &archive, palette::Palette &palette,
					   const LoadContext &ctx) override;

	image::ImagePtr loadScreenshot(const core::String &filename, const io::ArchivePtr &archive,
								   const LoadContext &ctx) override;

	int emptyPaletteIndex() const override {
		return 255;
	}

	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Cubzh",
									"",
									{"3zh"},
									{"CUBZH!"},
									VOX_FORMAT_FLAG_PALETTE_EMBEDDED | VOX_FORMAT_FLAG_SCREENSHOT_EMBEDDED |
										FORMAT_FLAG_SAVE};
		return f;
	}
};

} // namespace voxelformat
