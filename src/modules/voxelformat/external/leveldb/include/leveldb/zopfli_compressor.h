
#pragma once

#include "leveldb/compressor.h"

namespace leveldb {

	class DLLX ZopfliCompressor : public Compressor 
	{
	public:
		static const int SERIALIZE_ID = 2; //Same as ZLib since it is a replacement

		ZopfliCompressor() : Compressor(SERIALIZE_ID) {}

        virtual ~ZopfliCompressor() {}

		virtual void compressImpl(const char* input, size_t length, ::std::string& output) const override;

		virtual bool decompress(const char* input, size_t length, ::std::string &output) const override;

	private:
	};
}