#ifndef LEVELDB_MCPE_INCLUDE_COMPRESSOR_H_
#define LEVELDB_MCPE_INCLUDE_COMPRESSOR_H_

#include <string>
#include <cassert>
#include <cstdint>
#include <unordered_map>
#include <memory>

#include "leveldb/export.h"

namespace leveldb {
	class Slice;

	class LEVELDB_EXPORT Compressor
	{
	public:

		uint64_t inputBytes = 0, compressedBytes = 0;
 
		//an ID that has to be unique across the whole system
		const char uniqueCompressionID;

        virtual ~Compressor() = default;
        
		Compressor(char uniqueCompressionID) :
			uniqueCompressionID(uniqueCompressionID) {

		}

		double getAverageCompression() const
		{
			return inputBytes ? ((double)compressedBytes / (double)inputBytes) : 0;
		}

		void resetAverageCompressionStats() {
			inputBytes = compressedBytes = 0;
		}

		void compress(const char* input, size_t length, ::std::string& output) {

			compressImpl(input, length, output);

			inputBytes += length;
			compressedBytes += output.length();
		}

		void compress(const std::string& in, std::string& out) {
			compress(in.data(), in.length(), out);
		}

		virtual void compressImpl(const char* input, size_t length, ::std::string& output) const = 0;

		virtual bool decompress(const char* input, size_t length, ::std::string &output) const = 0;

		bool decompress(const std::string& input, ::std::string& output) const {
			return decompress(input.data(), input.length(), output);
		}

	protected:
	private:
	};
}

#endif
