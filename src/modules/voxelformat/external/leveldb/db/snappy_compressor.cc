#include "port/port.h"

#if HAVE_SNAPPY

#include <snappy.h>

#include "leveldb/snappy_compressor.h"

namespace leveldb {
	void SnappyCompressor::compressImpl(const char* input, size_t length, ::std::string& output) const
	{
		output.resize(snappy::MaxCompressedLength(length));
		size_t outlen;
		snappy::RawCompress(input, length, &output[0], &outlen);
		output.resize(outlen);
	}

	bool SnappyCompressor::decompress(const char* input, size_t length, ::std::string& output) const
	{
		size_t ulength;
		if (!snappy::GetUncompressedLength(input, length, &ulength))
			return false; //could not decompress

		output.resize(ulength);

		return snappy::RawUncompress(input, length, (char*)output.data());
	}

}

#endif