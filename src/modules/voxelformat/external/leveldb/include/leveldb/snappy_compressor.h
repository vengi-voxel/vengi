#ifndef LEVELDB_MCPE_INCLUDE_SNAPPY_COMPRESSOR_H_
#define LEVELDB_MCPE_INCLUDE_SNAPPY_COMPRESSOR_H_

#include "leveldb/compressor.h"

namespace leveldb {
	class LEVELDB_EXPORT SnappyCompressor : public Compressor 
	{
	public:
		static const char SERIALIZE_ID = 1;
        
        virtual ~SnappyCompressor() = default;

		SnappyCompressor() :
			Compressor(SERIALIZE_ID) {
		}

		virtual void compressImpl(const char* input, size_t length, ::std::string& output) const override;

		virtual bool decompress(const char* input, size_t length, ::std::string& output) const override;
	};
}

#endif