/**
 * @file
 */

#include "ZipArchive.h"
#include "core/Log.h"
#include "core/StandardLib.h"
#include "core/StringUtil.h"
#include "io/BufferedReadWriteStream.h"
#include "io/ZipReadStream.h"
#include "io/external/miniz.h"
#include "io/Stream.h"

namespace io {

namespace priv {

constexpr int kZipLocalHeaderSize = 30;
constexpr int kZipEncryptHeaderSize = 12;

// PKWARE traditional zip encryption uses a different CRC-32 update than mz_crc32().
static uint32_t zipCryptoCrc32(uint32_t crc, uint8_t b) {
	static uint32_t table[256];
	static bool tableInit = false;
	if (!tableInit) {
		for (uint32_t i = 0; i < 256; ++i) {
			uint32_t r = i;
			for (int j = 0; j < 8; ++j) {
				r = (r & 1) ? (r >> 1) ^ 0xEDB88320u : (r >> 1);
			}
			table[i] = r;
		}
		tableInit = true;
	}
	return table[(crc ^ b) & 0xff] ^ (crc >> 8);
}

static void zipCryptoUpdateKeys(mz_uint32 keys[3], uint8_t c) {
	keys[0] = zipCryptoCrc32(keys[0], c);
	keys[1] = (keys[1] + (keys[0] & 0xff)) * 134775813 + 1;
	const uint8_t keyshift = (uint8_t)(keys[1] >> 24);
	keys[2] = zipCryptoCrc32(keys[2], keyshift);
}

static void zipCryptoInitKeys(const char *password, mz_uint32 keys[3]) {
	keys[0] = 305419896;
	keys[1] = 591751049;
	keys[2] = 878082192;
	if (password == nullptr) {
		return;
	}
	while (*password != '\0') {
		zipCryptoUpdateKeys(keys, (uint8_t)*password);
		++password;
	}
}

static uint8_t zipCryptoDecryptByte(const mz_uint32 keys[3]) {
	const mz_uint32 temp = ((keys[2] & 0xffff) | 2);
	return (uint8_t)(((temp * (temp ^ 1)) >> 8) & 0xff);
}

static uint8_t zipCryptoDecode(mz_uint32 keys[3], uint8_t c) {
	c ^= zipCryptoDecryptByte(keys);
	zipCryptoUpdateKeys(keys, c);
	return c;
}

} // namespace priv

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
	size_t remaining = targetBufSize;
	uint8_t *buf = (uint8_t *)targetBuf;
	while (remaining > 0) {
		const int64_t read = stream->read(buf, remaining);
		if (read <= -1) {
			Log::error("Failed to read %i bytes from stream", (int)remaining);
			return 0;
		}
		if (read == 0) {
			break;
		}
		buf += read;
		remaining -= (size_t)read;
	}
	return targetBufSize - remaining;
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
	_files.clear();
}

bool ZipArchive::exists(const core::String &file) const {
	const core::String normalized = core::string::sanitizePath(file);
	for (const auto &entry : _files) {
		if (entry.fullPath == normalized) {
			return true;
		}
	}
	return false;
}

void ZipArchive::list(const core::String &basePath, ArchiveFiles &out, const core::String &filter) const {
	const core::String normalizedBase = core::string::sanitizePath(basePath);
	for (const auto &entry : _files) {
		if (!normalizedBase.empty() && !core::string::startsWith(entry.fullPath, normalizedBase)) {
			continue;
		}
		if (core::string::fileMatchesMultiple(entry.name.c_str(), filter.c_str())) {
			out.push_back(entry);
		}
	}
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
		if (mz_zip_reader_is_file_encrypted(zip, i) && _password.empty()) {
			continue;
		}
		if (!mz_zip_reader_file_stat(zip, i, &zipStat)) {
			continue;
		}
		FilesystemEntry entry;
		entry.fullPath = core::string::sanitizePath(zipStat.m_filename);
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

bool ZipArchive::readEncryptedStream(uint32_t fileIndex, BufferedReadWriteStream &out) {
	mz_zip_archive *zip = (mz_zip_archive *)_zip;
	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat(zip, fileIndex, &stat)) {
		return false;
	}
	uint8_t localHeader[priv::kZipLocalHeaderSize];
	if (zip->m_pRead(zip->m_pIO_opaque, stat.m_local_header_ofs, localHeader, priv::kZipLocalHeaderSize) !=
		priv::kZipLocalHeaderSize) {
		Log::error("Failed to read zip local header");
		return false;
	}
	const uint16_t filenameLen = (uint16_t)localHeader[26] | ((uint16_t)localHeader[27] << 8);
	const uint16_t extraLen = (uint16_t)localHeader[28] | ((uint16_t)localHeader[29] << 8);
	const mz_uint32 compSize =
		(mz_uint32)localHeader[18] | ((mz_uint32)localHeader[19] << 8) | ((mz_uint32)localHeader[20] << 16) |
		((mz_uint32)localHeader[21] << 24);
	const mz_uint64 dataOffset = stat.m_local_header_ofs + priv::kZipLocalHeaderSize + filenameLen + extraLen;

	if (compSize < priv::kZipEncryptHeaderSize) {
		Log::error("Encrypted zip entry is too small");
		return false;
	}

	uint8_t *encrypted = (uint8_t *)core_malloc((size_t)compSize);
	if (encrypted == nullptr) {
		return false;
	}
	if (zip->m_pRead(zip->m_pIO_opaque, dataOffset, encrypted, (size_t)compSize) != compSize) {
		Log::error("Failed to read encrypted zip data");
		core_free(encrypted);
		return false;
	}

	mz_uint32 keys[3];
	priv::zipCryptoInitKeys(_password.c_str(), keys);
	uint8_t *const encryptedData = encrypted;
	for (int i = 0; i < priv::kZipEncryptHeaderSize; ++i) {
		priv::zipCryptoDecode(keys, encryptedData[i]);
	}

	BufferedReadWriteStream decryptedCompressed;
	const size_t encryptedPayloadSize = (size_t)stat.m_comp_size - priv::kZipEncryptHeaderSize;
	decryptedCompressed.reserve((int)encryptedPayloadSize);
	for (size_t i = priv::kZipEncryptHeaderSize; i < (size_t)compSize; ++i) {
		const uint8_t b = priv::zipCryptoDecode(keys, encryptedData[i]);
		if (decryptedCompressed.write(&b, 1) != 1) {
			return false;
		}
	}
	decryptedCompressed.seek(0);

	bool success = false;
	if (stat.m_method == 0) {
		success = out.write(decryptedCompressed.getBuffer(), decryptedCompressed.size()) == decryptedCompressed.size();
	} else if (stat.m_method != MZ_DEFLATED) {
		Log::error("Unsupported compression method %i in encrypted zip entry", stat.m_method);
	} else {
		ZipReadStream inflateStream(decryptedCompressed, (int)decryptedCompressed.size());
		uint8_t buf[4096];
		success = true;
		while (!inflateStream.eos() && !inflateStream.err()) {
			const int read = inflateStream.read(buf, sizeof(buf));
			if (read <= 0) {
				break;
			}
			if (out.write(buf, (size_t)read) != read) {
				success = false;
				break;
			}
		}
		success = success && !inflateStream.err();
	}
	core_free(encrypted);
	return success;
}

SeekableReadStream *ZipArchive::readStream(const core::String &filePath) {
	if ((mz_zip_archive *)_zip == nullptr) {
		Log::error("No zip archive loaded");
		return nullptr;
	}
	const core::String normalized = core::string::sanitizePath(filePath);
	mz_uint32 fileIndex;
	if (!mz_zip_reader_locate_file_v2((mz_zip_archive *)_zip, normalized.c_str(), nullptr, 0, &fileIndex)) {
		Log::error("File '%s' not found in zip archive", normalized.c_str());
		return nullptr;
	}

	mz_zip_archive_file_stat stat;
	if (!mz_zip_reader_file_stat((mz_zip_archive *)_zip, fileIndex, &stat)) {
		Log::error("Failed to get file stat for file '%s' in zip archive", normalized.c_str());
		return nullptr;
	}

	BufferedReadWriteStream *stream = new BufferedReadWriteStream((int)stat.m_uncomp_size);
	if (stat.m_is_encrypted) {
		if (_password.empty()) {
			Log::error("File '%s' is encrypted but no zip password was given", normalized.c_str());
			delete stream;
			return nullptr;
		}
		if (!readEncryptedStream(fileIndex, *stream)) {
			Log::error("Failed to decrypt file '%s' from zip", normalized.c_str());
			delete stream;
			return nullptr;
		}
	} else if (!mz_zip_reader_extract_to_callback((mz_zip_archive *)_zip, fileIndex, ziparchive_write_callback, stream,
												  0)) {
		const mz_zip_error error = mz_zip_get_last_error((mz_zip_archive *)_zip);
		const char *err = mz_zip_get_error_string(error);
		Log::error("Failed to extract file '%s' from zip: %s", normalized.c_str(), err);
		delete stream;
		return nullptr;
	}
	Log::debug("Read stream for file '%s' from zip", normalized.c_str());
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

ArchivePtr openZipArchive(io::SeekableReadStream *stream, const core::String &password) {
	if (!stream || !ZipArchive::validStream(*stream)) {
		return ArchivePtr{};
	}
	core::SharedPtr<ZipArchive> za = core::make_shared<ZipArchive>();
	za->setPassword(password);
	if (!za->init("", stream)) {
		return ArchivePtr{};
	}
	if (!password.empty()) {
		ArchiveFiles files;
		za->list("", files, "");
		if (files.empty()) {
			return ArchivePtr{};
		}
	}
	return za;
}

} // namespace io
