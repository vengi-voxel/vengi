// Copyright (c) 2011 The LevelDB Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file. See the AUTHORS file for names of contributors.

#include "table/format.h"

#include "leveldb/env.h"
#include "leveldb/compressor.h"
#include "port/port.h"
#include "table/block.h"
#include "util/coding.h"
#include "util/crc32c.h"
#include "leveldb/decompress_allocator.h"
#include <map>

namespace leveldb {

std::string DecompressAllocator::get() {
  std::string buffer;
  std::lock_guard<std::mutex> lock(mutex);

  if (!stack.empty()) {
    buffer = std::move(stack.back());
    buffer.clear();
    stack.pop_back();
  }
  return buffer;
}

void DecompressAllocator::release(std::string&& string) {
  std::lock_guard<std::mutex> lock(mutex);
  stack.push_back(std::move(string));
}

void DecompressAllocator::prune() {
  std::lock_guard<std::mutex> lock(mutex);
  stack.clear();
}

void BlockHandle::EncodeTo(std::string* dst) const {
  // Sanity check that all fields have been set
  assert(offset_ != ~static_cast<uint64_t>(0));
  assert(size_ != ~static_cast<uint64_t>(0));
  PutVarint64(dst, offset_);
  PutVarint64(dst, size_);
}

Status BlockHandle::DecodeFrom(Slice* input) {
  if (GetVarint64(input, &offset_) &&
      GetVarint64(input, &size_)) {
    return Status::OK();
  } else {
    return Status::Corruption("bad block handle");
  }
}

void Footer::EncodeTo(std::string* dst) const {
  const size_t original_size = dst->size();
  metaindex_handle_.EncodeTo(dst);
  index_handle_.EncodeTo(dst);
  dst->resize(2 * BlockHandle::kMaxEncodedLength);  // Padding
  PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber & 0xffffffffu));
  PutFixed32(dst, static_cast<uint32_t>(kTableMagicNumber >> 32));
  assert(dst->size() == original_size + kEncodedLength);
  (void)original_size;  // Disable unused variable warning.
}

Status Footer::DecodeFrom(Slice* input) {
  if (input->size() < kEncodedLength) {
    return Status::Corruption("not an sstable (footer too short)");
  }

  const char* magic_ptr = input->data() + kEncodedLength - 8;
  const uint32_t magic_lo = DecodeFixed32(magic_ptr);
  const uint32_t magic_hi = DecodeFixed32(magic_ptr + 4);
  const uint64_t magic = ((static_cast<uint64_t>(magic_hi) << 32) |
                          (static_cast<uint64_t>(magic_lo)));
  if (magic != kTableMagicNumber) {
    return Status::Corruption("not an sstable (bad magic number)");
  }

  Status result = metaindex_handle_.DecodeFrom(input);
  if (result.ok()) {
    result = index_handle_.DecodeFrom(input);
  }
  if (result.ok()) {
    // We skip over any leftover data (just padding for now) in "input"
    const char* end = magic_ptr + 8;
    *input = Slice(end, input->data() + input->size() - end);
  }
  return result;
}

Status ReadBlock(RandomAccessFile* file, const Options& dbOptions, const ReadOptions& options,
                 const BlockHandle& handle, BlockContents* result) {
  result->data = Slice();
  result->cachable = false;
  result->heap_allocated = false;

  // Read the block contents as well as the type/crc footer.
  // See table_builder.cc for the code that built this structure.
  size_t n = static_cast<size_t>(handle.size());
  char* buf = new char[n + kBlockTrailerSize];
  Slice contents;
  Status s = file->Read(handle.offset(), n + kBlockTrailerSize, &contents, buf);
  if (!s.ok()) {
    delete[] buf;
    return s;
  }
  if (contents.size() != n + kBlockTrailerSize) {
    delete[] buf;
    return Status::Corruption("truncated block read");
  }

  // Check the crc of the type and the block contents
  const char* data = contents.data();  // Pointer to where Read put the data
  if (options.verify_checksums) {
    const uint32_t crc = crc32c::Unmask(DecodeFixed32(data + n + 1));
    const uint32_t actual = crc32c::Value(data, n + 1);
    if (actual != crc) {
      delete[] buf;
      s = Status::Corruption("block checksum mismatch");
      return s;
    }
  }
		unsigned char compressionID = data[n];

		if (compressionID == 0) {
			if (data != buf) {
				// File implementation gave us pointer to some other data.
				// Use it directly under the assumption that it will be live
				// while the file is open.
				delete[] buf;
				result->data = Slice(data, n);
				result->heap_allocated = false;
				result->cachable = false;  // Do not double-cache
			}
			else {
				result->data = Slice(buf, n);
				result->heap_allocated = true;
				result->cachable = true;
			}
		}
		else {
			//find the required compressor
			Compressor* compressor = nullptr;
			for (auto& c : dbOptions.compressors) {
				if (!c || c->uniqueCompressionID == compressionID) {
					compressor = c;
					break;
				}
			}

			if (compressor == nullptr) {
				delete[] buf;
				return Status::NotSupported("encountered a block compressed with an unknown decompressor");
			}

			std::string buffer;
			if (options.decompress_allocator) {
				buffer = options.decompress_allocator->get();
			}

			bool success = compressor->decompress(data, n, buffer);

			if (success) {
				auto ubuf = new char[buffer.size()];
				memcpy(ubuf, buffer.data(), buffer.size());
				result->data = Slice(ubuf, buffer.size());
				result->heap_allocated = true;
				result->cachable = true;
			}

			delete[] buf;
			
			if (options.decompress_allocator) {
				options.decompress_allocator->release(std::move(buffer));
			}

			if (!success) {
				return Status::Corruption("corrupted compressed block contents");
			}
		}

		return Status::OK();
	}
}  // namespace leveldb
