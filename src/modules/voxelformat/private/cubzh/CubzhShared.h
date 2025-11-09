/**
 * @file
 */

#pragma once

#include "core/Log.h"
#include "io/BufferedReadWriteStream.h"
#include "io/Stream.h"
#include "io/ZipWriteStream.h"

namespace voxelformat {

namespace priv {

enum ChunkId {
	CHUNK_ID_MIN = 1,

	CHUNK_ID_PREVIEW = 1,

	CHUNK_ID_PALETTE_V5 = 2,
	CHUNK_ID_SELECTED_COLOR_V5 = 3,			   // byte value of the selected color palette index
	CHUNK_ID_SELECTED_BACKGROUND_COLOR_V5 = 4, // byte value of the selected color palette index
	CHUNK_ID_SHAPE_V5 = 5,
	CHUNK_ID_SHAPE_SIZE_V5 = 6,
	CHUNK_ID_SHAPE_BLOCKS_V5 = 7,
	CHUNK_ID_SHAPE_POINT_V5 = 8,
	CHUNK_ID_SHAPE_CAMERA_V5 = 9,
	CHUNK_ID_DIRECTIONAL_LIGHT = 10,
	CHUNK_ID_SOURCE_METADATA = 11,
	CHUNK_ID_SHAPE_NAME_V5 = 12,
	CHUNK_ID_GENERAL_RENDERING_OPTIONS_V5 = 13,
	CHUNK_ID_SHAPE_BAKED_LIGHTING_V5 = 14,
	CHUNK_ID_MAX_V5 = 14,

	CHUNK_ID_PALETTE_LEGACY_V6 = CHUNK_ID_PALETTE_V5,
	CHUNK_ID_SHAPE_V6 = 3,
	CHUNK_ID_SHAPE_SIZE_V6 = 4,
	CHUNK_ID_SHAPE_BLOCKS_V6 = 5,
	CHUNK_ID_SHAPE_POINT_V6 = 6,
	CHUNK_ID_SHAPE_BAKED_LIGHTING_V6 = 7,
	CHUNK_ID_SHAPE_POINT_ROTATION_V6 = 8,
	// store the state of the camera (distance from item, angle)
	CHUNK_ID_CAMERA_V6 = 10,
	CHUNK_ID_DIRECTIONAL_LIGHT_V6 = 11,
	// store unsupported metadata when importing other formats (like .vox) to support writing them back if
	// exporting to that same original format at some point.
	CHUNK_ID_SOURCE_METADATA_V6 = 12,
	CHUNK_ID_GENERAL_RENDERING_OPTIONS_V6 = 14,
	CHUNK_ID_PALETTE_ID_V6 = 15,
	CHUNK_ID_PALETTE_V6 = 16,
	CHUNK_ID_SHAPE_ID_V6 = 17,
	CHUNK_ID_SHAPE_NAME_V6 = 18,
	CHUNK_ID_SHAPE_PARENT_ID_V6 = 19,
	CHUNK_ID_SHAPE_TRANSFORM_V6 = 20,
	CHUNK_ID_SHAPE_PIVOT_V6 = 21,
	CHUNK_ID_SHAPE_PALETTE_V6 = 22,
	CHUNK_ID_OBJECT_COLLISION_BOX_V6 = 23,
	CHUNK_ID_OBJECT_IS_HIDDEN_V6 = 24,

	CHUNK_ID_MAX_V6 = 24
};

inline bool supportsCompression(uint32_t chunkId) {
	return chunkId == priv::CHUNK_ID_PALETTE_V6 || chunkId == priv::CHUNK_ID_SHAPE_V6 ||
		   chunkId == priv::CHUNK_ID_PALETTE_LEGACY_V6 || chunkId == priv::CHUNK_ID_PALETTE_ID_V6;
}
} // namespace priv

class WriteChunkStream : public io::SeekableWriteStream {
private:
	uint32_t _chunkId;
	io::SeekableWriteStream &_stream;
	int64_t _chunkSizePos;
	int64_t _uncompressedSizePos = -1;
	int64_t _chunkHeaderEndPos;
	uint32_t _uncompressedChunkSize = 0;
	io::WriteStream *_stream2 = nullptr;

public:
	WriteChunkStream(uint32_t chunkId, io::SeekableWriteStream &stream) : _chunkId(chunkId), _stream(stream) {
		stream.writeUInt8(_chunkId);
		_chunkSizePos = stream.pos();
		stream.writeUInt32(0); // chunkSize
		if (priv::supportsCompression(_chunkId)) {
			stream.writeUInt8(1);
			_uncompressedSizePos = stream.pos();
			stream.writeUInt32(0); // uncompressedSize
			_stream2 = new io::ZipWriteStream(stream);
		}
		_chunkHeaderEndPos = stream.pos();
	}
	~WriteChunkStream() {
		delete _stream2;
		_stream2 = nullptr;
		const int64_t chunkSize = _stream.pos() - _chunkHeaderEndPos;
		if (_stream.seek(_chunkSizePos) == -1) {
			Log::error("Failed to seek to the chunk size position in the header");
			return;
		}
		_stream.writeUInt32((uint32_t)chunkSize);
		if (_uncompressedSizePos != -1) {
			if (_stream.seek(_uncompressedSizePos) == -1) {
				Log::error("Failed to seek to the uncompressed size position in the header");
				return;
			}
			_stream.writeUInt32(_uncompressedChunkSize);
		}
		_stream.seek(0, SEEK_END);
	}
	int write(const void *buf, size_t size) override {
		const int bytes = _stream2 ? _stream2->write(buf, size) : _stream.write(buf, size);
		if (bytes == -1) {
			return -1;
		}
		_uncompressedChunkSize += size;
		return bytes;
	}
	// don't seek in the middle of writing to the zip stream
	int64_t seek(int64_t position, int whence = SEEK_SET) override {
		return _stream.seek(position, whence);
	}
	int64_t size() const override {
		return _stream.size();
	}
	int64_t pos() const override {
		return _stream.pos();
	}
};

class WriteSubChunkStream : public io::SeekableWriteStream {
private:
	uint32_t _chunkId;
	io::SeekableWriteStream &_stream;
	io::BufferedReadWriteStream _buffer{4096};

public:
	WriteSubChunkStream(uint32_t chunkId, io::SeekableWriteStream &stream) : _chunkId(chunkId), _stream(stream) {
		stream.writeUInt8(_chunkId);
	}
	~WriteSubChunkStream() {
		_buffer.seek(0);
		_stream.writeUInt32((uint32_t)_buffer.size());
		_stream.write(_buffer.getBuffer(), _buffer.size());
	}
	int write(const void *buf, size_t size) override {
		return _buffer.write(buf, size);
	}
	int64_t seek(int64_t position, int whence = SEEK_SET) override {
		return _buffer.seek(position, whence);
	}
	int64_t size() const override {
		return _buffer.size();
	}
	int64_t pos() const override {
		return _buffer.pos();
	}
};

} // namespace voxelformat
