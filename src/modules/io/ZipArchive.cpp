/**
 * @file
 */

#include "ZipArchive.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/external/miniz.h"
#include "io/Stream.h"

namespace io {

static size_t ziparchive_read(void *userdata, mz_uint64 offset, void *targetBuf, size_t targetBufSize) {
	io::SeekableReadStream *stream = (io::SeekableReadStream *)userdata;
	mz_int64 currentPos = stream->pos();
	if ((mz_int64)offset < 0) {
		Log::debug("ziparchive_read: Invalid file offset: %i", (int)offset);
		return 0;
	}
	if (currentPos != (mz_int64)offset && stream->seek((mz_int64)offset, SEEK_SET) == -1) {
		Log::error("ziparchive_read: Failed to seek");
		return 0;
	}
	// TODO: read until we have read the expected size
	const int64_t read = stream->read(targetBuf, targetBufSize);
	if (read <= -1) {
		Log::error("Failed to read %i bytes from stream", (int)targetBufSize);
		return read;
	}
	return read;
}

static size_t ziparchive_write_callback(void *userdata, mz_uint64 offset, const void *targetBuf, size_t targetBufSize) {
	io::BufferedReadWriteStream *out = (io::BufferedReadWriteStream *)userdata;
	if (out->seek((int64_t)offset, SEEK_SET) == -1) {
		return 0u;
	}
	const int64_t written = out->write(targetBuf, targetBufSize);
	if (written <= -1) {
		Log::error("Failed to write %i bytes into stream", (int)targetBufSize);
		return written;
	}
	return written;
}

static size_t ziparchive_write(void *userdata, mz_uint64 offset, const void *targetBuf, size_t targetBufSize) {
	io::SeekableWriteStream *out = (io::SeekableWriteStream *)userdata;
	if (out->seek((int64_t)offset, SEEK_SET) == -1) {
		return 0u;
	}
	const int64_t written = out->write(targetBuf, targetBufSize);
	if (written <= -1) {
		Log::error("Failed to write %i bytes into stream", (int)targetBufSize);
		return written;
	}
	return written;
}

static void *ziparchive_malloc(void *opaque, size_t items, size_t size) {
	return core_malloc(items * size);
}

static void ziparchive_free(void *opaque, void *address) {
	core_free(address);
}

static void *ziparchive_realloc(void *opaque, void *address, size_t items, size_t size) {
	return core_realloc(address, items * size);
}

ZipArchive::ZipArchive() {
}

ZipArchive::~ZipArchive() {
	ZipArchive::shutdown();
}

void ZipArchive::shutdown() {
	reset();
}

void ZipArchive::reset() {
	if (isWrite()) {
		flush();
	}
	if (_zip != nullptr) {
		mz_zip_end((mz_zip_archive *)_zip);
		core_free(_zip);
		_zip = nullptr;
	}
}

bool ZipArchive::validStream(io::SeekableReadStream &stream) {
	const int64_t currentPos = stream.pos();
	mz_zip_archive zip;
	mz_zip_zero_struct(&zip);
	zip.m_pRead = ziparchive_read;
	zip.m_pAlloc = ziparchive_malloc;
	zip.m_pRealloc = ziparchive_realloc;
	zip.m_pFree = ziparchive_free;
	zip.m_pIO_opaque = &stream;
	int64_t size = stream.size();
	if (!mz_zip_reader_init(&zip, size, MZ_ZIP_FLAG_VALIDATE_HEADERS_ONLY)) {
		const mz_zip_error error = mz_zip_get_last_error(&zip);
		const char *err = mz_zip_get_error_string(error);
		Log::debug("Failed to initialize the zip reader with stream of size '%i': %s", (int)size, err);
		stream.seek(currentPos);
		mz_zip_end(&zip);
		return false;
	}
	stream.seek(currentPos);
	mz_zip_end(&zip);
	return true;
}

bool ZipArchive::init(const core::String &path, io::SeekableReadStream *stream) {
	if (stream == nullptr) {
		Log::error("No stream given");
		return false;
	}
	reset();
	mz_zip_archive *zip = (mz_zip_archive *)core_malloc(sizeof(mz_zip_archive));
	_zip = zip;
	mz_zip_zero_struct(zip);
	zip->m_pAlloc = ziparchive_malloc;
	zip->m_pRealloc = ziparchive_realloc;
	zip->m_pFree = ziparchive_free;
	zip->m_pRead = ziparchive_read;
	zip->m_pIO_opaque = stream;
	_files.clear();
	int64_t size = stream->size();
	if (!mz_zip_reader_init(zip, size, 0)) {
		const mz_zip_error error = mz_zip_get_last_error(zip);
		const char *err = mz_zip_get_error_string(error);
		Log::error("Failed to initialize the zip reader with stream of size '%i': %s", (int)size, err);
		reset();
		return false;
	}

	mz_uint numFiles = mz_zip_reader_get_num_files(zip);

	mz_zip_archive_file_stat zipStat;
	for (mz_uint i = 0; i < numFiles; ++i) {
		if (mz_zip_reader_is_file_a_directory(zip, i)) {
			continue;
		}
		if (mz_zip_reader_is_file_encrypted(zip, i)) {
			continue;
		}
		if (!mz_zip_reader_file_stat(zip, i, &zipStat)) {
			continue;
		}
		FilesystemEntry entry;
		entry.fullPath = zipStat.m_filename;
		entry.name = core::string::extractFilenameWithExtension(entry.fullPath);
		entry.type = FilesystemEntry::Type::file;
		entry.size = zipStat.m_uncomp_size;
		entry.mtime = zipStat.m_time;
		_files.emplace_back(core::move(entry));
	}
	_files.sort([](const io::FilesystemEntry &a, const io::FilesystemEntry &b) { return a.name < b.name; });

	return true;
}

bool ZipArchive::init(io::SeekableWriteStream *stream) {
	if (stream == nullptr) {
		Log::error("No stream given for writing");
		return false;
	}
	reset();

	mz_zip_archive *zip = (mz_zip_archive *)core_malloc(sizeof(mz_zip_archive));
	_zip = zip;

	mz_zip_zero_struct(zip);
	zip->m_pAlloc = ziparchive_malloc;
	zip->m_pRealloc = ziparchive_realloc;
	zip->m_pFree = ziparchive_free;
	zip->m_pWrite = ziparchive_write;
	zip->m_pIO_opaque = stream;

	if (!mz_zip_writer_init(zip, 0)) {
		const mz_zip_error error = mz_zip_get_last_error(zip);
		const char *err = mz_zip_get_error_string(error);
		Log::error("Failed to initialize the zip writer: %s", err);
		reset();
		return false;
	}

	return true;
}

bool ZipArchive::isWrite() const {
	if (_zip == nullptr) {
		return false;
	}

	mz_zip_archive *zip = (mz_zip_archive *)_zip;
	return zip->m_zip_mode == MZ_ZIP_MODE_WRITING;
}

bool ZipArchive::flush() {
	core_assert(isWrite());

	if (!mz_zip_writer_finalize_archive((mz_zip_archive *)_zip)) {
		const mz_zip_error error = mz_zip_get_last_error((mz_zip_archive *)_zip);
		const char *err = mz_zip_get_error_string(error);
		Log::error("Failed to finalize zip archive: %s", err);
		return false;
	}

	return true;
}

SeekableReadStream *ZipArchive::readStream(const core::String &filePath) {
	if ((mz_zip_archive *)_zip == nullptr) {
		Log::error("No zip archive loaded");
		return nullptr;
	}
	mz_uint32 fileIndex;
	if (!mz_zip_reader_locate_file_v2((mz_zip_archive *)_zip, filePath.c_str(), nullptr, 0, &fileIndex)) {
		Log::error("File '%s' not found in zip archive", filePath.c_str());
		return nullptr;
	}

	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat((mz_zip_archive *)_zip, fileIndex, &stat)) {
		Log::error("Failed to get file stat for file '%s' in zip archive", filePath.c_str());
		return nullptr;
	}

	BufferedReadWriteStream *stream = new BufferedReadWriteStream(stat.m_uncomp_size);
	if (!mz_zip_reader_extract_to_callback((mz_zip_archive *)_zip, fileIndex, ziparchive_write_callback, stream, 0)) {
		const mz_zip_error error = mz_zip_get_last_error((mz_zip_archive *)_zip);
		const char *err = mz_zip_get_error_string(error);
		Log::error("Failed to extract file '%s' from zip: %s", filePath.c_str(), err);
		delete stream;
		return nullptr;
	}
	Log::debug("Read stream for file '%s' from zip", filePath.c_str());
	stream->seek(0);
	return stream;
}

class ZipArchiveWriteStream : public io::BufferedReadWriteStream {
private:
	using Super = io::BufferedReadWriteStream;
	mz_zip_archive *_zip;
	core::String _filePath;

public:
	ZipArchiveWriteStream(mz_zip_archive *zip, const core::String &filePath)
		: io::BufferedReadWriteStream(), _zip(zip), _filePath(filePath) {
	}

	~ZipArchiveWriteStream() override {
		flush();
		_zip = nullptr;
	}

	bool flush() override {
		const uint8_t *data = (const uint8_t *)getBuffer();
		const size_t size = (size_t)this->size();

		if (!mz_zip_writer_add_mem((mz_zip_archive *)_zip, _filePath.c_str(), data, size, MZ_DEFAULT_COMPRESSION)) {
			const mz_zip_error error = mz_zip_get_last_error((mz_zip_archive *)_zip);
			const char *err = mz_zip_get_error_string(error);
			Log::error("Failed to add file '%s' to zip: %s", _filePath.c_str(), err);
			return false;
		}
		Log::debug("Added file '%s' to zip (%i bytes)", _filePath.c_str(), (int)size);
		return Super::flush();
	}
};

SeekableWriteStream *ZipArchive::writeStream(const core::String &filePath) {
	if (_zip == nullptr) {
		Log::error("No write zip archive initialized");
		return nullptr;
	}

	ZipArchiveWriteStream *stream = new ZipArchiveWriteStream((mz_zip_archive *)_zip, filePath);
	Log::debug("Created write stream for file '%s'", filePath.c_str());
	return stream;
}

ArchivePtr openZipArchive(io::SeekableReadStream *stream) {
	if (!stream || !ZipArchive::validStream(*stream)) {
		return ArchivePtr{};
	}
	core::SharedPtr<ZipArchive> za = core::make_shared<ZipArchive>();
	za->init("", stream);
	return za;
}

} // namespace io
