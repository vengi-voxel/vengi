/**
 * @file
 */

#pragma once

namespace tb {

/** TBTempBuffer manages a buffer that will be deleted on destruction.

	The buffer size can grow by calling Reserve or Append, but it
	will never shrink during the lifetime of the object.
*/
class TBTempBuffer {
public:
	TBTempBuffer();
	~TBTempBuffer();

	/** Make sure the buffer has at least size bytes.
		Returns false on OOM. */
	bool reserve(int size);

	/** Get a pointer to the buffer data. */
	char *getData() const {
		return m_data;
	}

	/** Return the size of the buffer in bytes. */
	int getCapacity() const {
		return m_data_size;
	}

	/** Append data with size bytes at the end of the buffer and
		increase the append position with the same amount.
		Returns false on OOM. */
	bool append(const char *data, int size);

	/** Increase the append position with size bytes without
		writing any data. This is useful if you want to write
		the data later and want to make sure space is reserved.
		Returns false on OOM. */
	bool appendSpace(int size);

	/** Append a null terminated string (including the null termination)
		at the end of the buffer. The append position will be increased
		with the length of the text (excluding the null termination) so
		multiple calls will produce a concatenated null terminated string.
		Returns false on OOM. */
	bool appendString(const char *str);

	/** Append a path without the ending filename.
		The buffer will be null terminated and the append position will be
		increased with the length of the path (excluding the null termination). */
	bool appendPath(const char *full_path_and_filename);

	/** Append file content at the end of the buffer. The append position will
		be increased by the size of the file. It will always append null
		termination (not included in append position).
		Returns false of OOM or if loading failed.
	*/
	bool appendFile(const char *filename);

	/** Set the position (in bytes) in the buffer where Append should write. */
	void setAppendPos(int append_pos);

	/** Reset the append position to 0. */
	void resetAppendPos() {
		m_append_pos = 0;
	}

	/** Return the current append position in in bytes. */
	int getAppendPos() const {
		return m_append_pos;
	}

private:
	int getAppendReserveSize(int needed_size) const;
	char *m_data;
	int m_data_size;
	int m_append_pos;
};

} // namespace tb
