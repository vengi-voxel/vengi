/**
 * @file
 */

#include "tb_tempbuffer.h"
#include "core/Assert.h"
#include "tb_system.h"
#include <string.h>

namespace tb {

TBTempBuffer::TBTempBuffer() : m_data(nullptr), m_data_size(0), m_append_pos(0) {
}

TBTempBuffer::~TBTempBuffer() {
	SDL_free(m_data);
}

void TBTempBuffer::setAppendPos(int appendPos) {
	core_assert(appendPos >= 0 && appendPos <= m_data_size);
	m_append_pos = appendPos;
}

bool TBTempBuffer::reserve(int size) {
	if (size > m_data_size) {
		char *new_data = (char *)SDL_realloc(m_data, size);
		if (new_data == nullptr) {
			return false;
		}
		m_data = new_data;
		m_data_size = size;
	}
	return true;
}

int TBTempBuffer::getAppendReserveSize(int neededSize) const {
	// Reserve some extra memory to reduce the reserve calls.
	neededSize *= 2;
	return neededSize < 32 ? 32 : neededSize;
}

bool TBTempBuffer::append(const char *data, int size) {
	if (m_append_pos + size > m_data_size && !reserve(getAppendReserveSize(m_append_pos + size))) {
		return false;
	}
	SDL_memcpy(m_data + m_append_pos, data, size);
	m_append_pos += size;
	return true;
}

bool TBTempBuffer::appendSpace(int size) {
	if (m_append_pos + size > m_data_size && !reserve(getAppendReserveSize(m_append_pos + size))) {
		return false;
	}
	m_append_pos += size;
	return true;
}

bool TBTempBuffer::appendString(const char *str) {
	// Add 1 to include the null termination in the data.
	if (append(str, SDL_strlen(str) + 1)) {
		// Now remove the null termination from the append position
		// again, so another call will append to the same string (instead of
		// after the null termination of the first string)
		m_append_pos--;
		return true;
	}
	return false;
}

bool TBTempBuffer::appendPath(const char *fullPathAndFilename) {
	const char *str_start = fullPathAndFilename;
	while (const char *next = strpbrk(fullPathAndFilename, "\\/")) {
		fullPathAndFilename = next + 1;
	}

	if (str_start == fullPathAndFilename) // Filename contained no path
	{
		str_start = "./";
		fullPathAndFilename = str_start + 2;
	}

	const int len = fullPathAndFilename - str_start;
	if (reserve(m_append_pos + len + 1)) {
		// Add the string, and nulltermination.
		append(str_start, len);
		append("", 1);
		// Remove null termination from append pos again (see details in AppendString).
		m_append_pos--;
		return true;
	}
	return false;
}

bool TBTempBuffer::appendFile(const char *filename) {
	if (TBFile *file = TBFile::open(filename, TBFile::MODE_READ)) {
		const size_t file_size = file->size();
		if (reserve(m_append_pos + file_size + 1) && file->read(m_data + m_append_pos, 1, file_size) == file_size) {
			// Increase append position and null terminate
			m_append_pos += file_size;
			m_data[m_append_pos] = 0;
			delete file;
			return true;
		}
		delete file;
	}
	return false;
}

} // namespace tb
