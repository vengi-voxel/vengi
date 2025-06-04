#ifndef LEVELDB_DECOMPRESS_ALLOCATOR_H_
#define LEVELDB_DECOMPRESS_ALLOCATOR_H_

#include <mutex>
#include <vector>
#include <string>

#include "leveldb/export.h"

namespace leveldb {
	class LEVELDB_EXPORT DecompressAllocator {
	public:
		virtual ~DecompressAllocator() = default;

		virtual std::string get();
		virtual void release(std::string&& string);

		virtual void prune();

	protected:
		std::mutex mutex;
		std::vector<std::string> stack;
	};
}

#endif