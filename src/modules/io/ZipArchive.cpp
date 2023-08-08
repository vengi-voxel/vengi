/**
 * @file
 */

#include "ZipArchive.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#define MINIZ_NO_STDIO
#include "io/external/miniz.h"
#include "io/Stream.h"

namespace io {

ZipArchive::ZipArchive() {
}

ZipArchive::~ZipArchive() {
	ZipArchive::shutdown();
}

void ZipArchive::shutdown() {
	reset();
}

void ZipArchive::reset() {
	if (_zip == nullptr) {
		return;
	}
	mz_zip_end((mz_zip_archive*)_zip);
	core_free(_zip);
	_zip = nullptr;
}

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

static size_t ziparchive_write(void *userdata, mz_uint64 offset, const void *targetBuf, size_t targetBufSize) {
	io::SeekableWriteStream *out = (io::SeekableWriteStream *)userdata;
	if (out->seek((int64_t)offset, SEEK_SET) == -1) {
		return 0u;
	}
	// TODO: write until we have written the expected size
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
		entry.name = zipStat.m_filename;
		entry.type = FilesystemEntry::Type::file;
		entry.size = zipStat.m_uncomp_size;
		entry.mtime = zipStat.m_time;
		_files.emplace_back(core::move(entry));
	}
	_files.sort([](const io::FilesystemEntry &a, const io::FilesystemEntry &b) { return a.name < b.name; });

	return true;
}

bool ZipArchive::load(const core::String &filePath, io::SeekableWriteStream &out) {
	if ((mz_zip_archive*)_zip == nullptr) {
		Log::error("No zip archive loaded");
		return false;
	}
	return mz_zip_reader_extract_file_to_callback((mz_zip_archive*)_zip, filePath.c_str(), ziparchive_write, (void *)&out, 0);
}

} // namespace io
