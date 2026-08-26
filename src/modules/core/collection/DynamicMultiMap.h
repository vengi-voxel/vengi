/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/Pair.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicList.h"
#include <stdint.h>
#include <stddef.h>

namespace core {

namespace privdynamicmultimap {

struct EqualCompare {
	template<typename T>
	inline bool operator() (const T& lhs, const T& rhs) const {
		return lhs == rhs;
	}
};

struct DefaultHasher {
	template<typename T>
	inline size_t operator() (const T& o) const {
		return (size_t)o;
	}
};

}

/**
 * @brief Dynamic growing hash multimap. Equal keys are stored adjacent in the bucket chain
 * so @c equal_range() can walk them as a half-open iterator range.
 * @sa DynamicMap
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmultimap::DefaultHasher,
		 typename COMPARE = privdynamicmultimap::EqualCompare, size_t BLOCK_SIZE = 256>
class DynamicMultiMap {
public:
	using value_type = VALUETYPE;
	using key_type = KEYTYPE;

	struct KeyValue {
		inline KeyValue(const KEYTYPE& _key, const VALUETYPE& _value) :
				key(_key), value(_value), next(nullptr), first(key), second(value) {
		}

		inline KeyValue(const KEYTYPE& _key, VALUETYPE&& _value) :
				key(_key), value(core::forward<VALUETYPE>(_value)), next(nullptr), first(key), second(value) {
		}

		inline KeyValue(KeyValue &&other) noexcept :
				key(core::move(other.key)), value(core::move(other.value)), next(
						nullptr), first(key), second(value) {
		}

		KEYTYPE key;
		VALUETYPE value;
		KeyValue *next;
		const KEYTYPE &first;
		VALUETYPE &second;
	};
protected:
	struct Block {
		KeyValue *_nodes;
		size_t _used = 0;
		const size_t _size;

		Block(size_t count) : _nodes(static_cast<KeyValue *>(core_malloc(sizeof(KeyValue) * count))), _size(count) {
		}

		Block(const Block&) = delete;
		Block &operator=(const Block&) = delete;
		Block(Block &&) = delete;
		Block &operator=(Block &&) = delete;

		~Block() {
			core_free(_nodes);
			_nodes = nullptr;
			_used = 0;
		}

		bool full() const {
			return _used >= _size;
		}
	};

	core::Array<KeyValue *, BUCKETSIZE> _buckets;
	HASHER _hasher;
	size_t _size = 0;

	core::DynamicList<Block> _blocks;
	core::DynamicArray<KeyValue *> _freeList;

	template<typename... Args>
	KeyValue *allocateNode(Args &&...args) {
		if (!_freeList.empty()) {
			KeyValue *node = _freeList.back();
			_freeList.pop();
			return ::new (node) KeyValue(core::forward<Args>(args)...);
		}

		if (_blocks.empty() || _blocks.back()->full()) {
			_blocks.emplace(BLOCK_SIZE);
		}

		KeyValue *node = &_blocks.back()->_nodes[_blocks.back()->_used++];
		return ::new (node) KeyValue(core::forward<Args>(args)...);
	}

	void freeNode(KeyValue *node) {
		node->~KeyValue();
		_freeList.push_back(node);
	}

	void linkNode(const KEYTYPE& key, KeyValue *node) {
		const size_t bucketIdx = (size_t)_hasher(key) % BUCKETSIZE;
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[bucketIdx];
		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			node->next = nullptr;
			if (prev == nullptr) {
				_buckets[bucketIdx] = node;
			} else {
				prev->next = node;
			}
		} else {
			KeyValue *lastMatch = entry;
			while (lastMatch->next != nullptr && COMPARE()(lastMatch->next->key, key)) {
				lastMatch = lastMatch->next;
			}
			node->next = lastMatch->next;
			lastMatch->next = node;
		}
		++_size;
	}

public:
	DynamicMultiMap() {
		_buckets.fill(nullptr);
	}
	DynamicMultiMap(const DynamicMultiMap& other) {
		_buckets.fill(nullptr);
		reserve(other.size());
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->key, i->value);
		}
	}
	DynamicMultiMap(DynamicMultiMap &&other) noexcept
		: _buckets(other._buckets), _hasher(other._hasher), _size(other._size), _blocks(core::move(other._blocks)),
		  _freeList(core::move(other._freeList)) {
		other._buckets.fill(nullptr);
		other._size = 0;
	}
	~DynamicMultiMap() {
		clear();
	}
	DynamicMultiMap &operator=(DynamicMultiMap &&other) noexcept {
		if (this != &other) {
			clear();
			_buckets = other._buckets;
			_hasher = other._hasher;
			_size = other._size;
			other._size = 0;
			_blocks = core::move(other._blocks);
			_freeList = core::move(other._freeList);
			other._buckets.fill(nullptr);
		}
		return *this;
	}

	DynamicMultiMap& operator=(const DynamicMultiMap& other) {
		if (this == &other) {
			return *this;
		}
		clear();
		reserve(other.size());
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->key, i->value);
		}
		return *this;
	}

	void reserve(size_t n) {
		if (n == 0u) {
			return;
		}
		if (!_blocks.empty()) {
			return;
		}
		_blocks.emplace(n);
	}

	class iterator {
	private:
		const DynamicMultiMap* _map;
		size_t _bucket;
		KeyValue* _ptr;
		bool _chainOnly;
		friend DynamicMultiMap;
	public:
		constexpr iterator() :
			_map(nullptr), _bucket(0), _ptr(nullptr), _chainOnly(false) {
		}

		iterator(const DynamicMultiMap* map, size_t bucket, KeyValue *ptr, bool chainOnly = false) :
				_map(map), _bucket(bucket), _ptr(ptr), _chainOnly(chainOnly) {
		}

		CORE_FORCE_INLINE KeyValue& operator*() const {
			return *_ptr;
		}

		CORE_FORCE_INLINE iterator& operator++() {
			if (_ptr->next != nullptr) {
				_ptr = _ptr->next;
				return *this;
			}
			if (_chainOnly) {
				_ptr = nullptr;
				_bucket = 0;
				return *this;
			}
			size_t bucket = _bucket;
			for (++bucket; bucket < BUCKETSIZE; ++bucket) {
				KeyValue* ptr = _map->_buckets[bucket];
				if (ptr != nullptr) {
					_ptr = ptr;
					_bucket = bucket;
					return *this;
				}
			}
			_ptr = nullptr;
			_bucket = 0;
			return *this;
		}

		CORE_FORCE_INLINE KeyValue* operator->() const {
			return _ptr;
		}

		CORE_FORCE_INLINE bool operator!=(const iterator& rhs) const {
			return _ptr != rhs._ptr;
		}

		CORE_FORCE_INLINE bool operator==(const iterator& rhs) const {
			return _ptr == rhs._ptr;
		}
	};

	using const_iterator = iterator;

	inline size_t size() const {
		return _size;
	}

	inline bool empty() const {
		return _size == 0u;
	}

	bool hasKey(const KEYTYPE& key) const {
		return find(key) != end();
	}

	size_t count(const KEYTYPE& key) const {
		size_t n = 0;
		const core::Pair<iterator, iterator> range = equal_range(key);
		for (iterator i = range.first; i != range.second; ++i) {
			++n;
		}
		return n;
	}

	iterator find(const KEYTYPE& key) const {
		const size_t bucketIdx = (size_t)_hasher(key) % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				return iterator(this, bucketIdx, entry);
			}
			entry = entry->next;
		}
		return end();
	}

	core::Pair<iterator, iterator> equal_range(const KEYTYPE& key) const {
		const size_t bucketIdx = (size_t)_hasher(key) % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			entry = entry->next;
		}
		if (entry == nullptr) {
			return core::Pair<iterator, iterator>(end(), end());
		}

		KeyValue *endEntry = entry;
		while (endEntry->next != nullptr && COMPARE()(endEntry->next->key, key)) {
			endEntry = endEntry->next;
		}
		return core::Pair<iterator, iterator>(iterator(this, bucketIdx, entry, true),
											  iterator(this, bucketIdx, endEntry->next, true));
	}

	void emplace(const KEYTYPE& key, VALUETYPE&& value) {
		linkNode(key, allocateNode(key, core::forward<VALUETYPE>(value)));
	}

	void insert(const KEYTYPE& key, const VALUETYPE& value) {
		linkNode(key, allocateNode(key, value));
	}

	void insert(const KEYTYPE& key, VALUETYPE&& value) {
		linkNode(key, allocateNode(key, core::forward<VALUETYPE>(value)));
	}

	iterator begin() const {
		for (size_t i = 0u; i < BUCKETSIZE; ++i) {
			KeyValue *entry = _buckets[i];
			if (entry != nullptr) {
				return iterator(this, i, entry);
			}
		}
		return end();
	}

	constexpr iterator end() const {
		return iterator();
	}

	void clear() {
		for (size_t i = 0; i < BUCKETSIZE; ++i) {
			KeyValue *entry = _buckets[i];
			while (entry != nullptr) {
				KeyValue *next = entry->next;
				entry->~KeyValue();
				entry = next;
			}
		}
		_buckets.fill(nullptr);
		_freeList.clear();
		_blocks.clear();
		_size = 0;
	}

	bool erase(const iterator& iter) {
		if (iter._ptr == nullptr) {
			return false;
		}
		const size_t bucketIdx = iter._bucket;
		KeyValue *entry = _buckets[bucketIdx];
		KeyValue *prev = nullptr;
		while (entry != nullptr) {
			if (entry == iter._ptr) {
				break;
			}
			prev = entry;
			entry = entry->next;
		}
		if (entry == nullptr) {
			return false;
		}

		if (prev == nullptr) {
			_buckets[bucketIdx] = entry->next;
		} else {
			prev->next = entry->next;
		}

		freeNode(entry);
		--_size;
		return true;
	}

	size_t remove(const KEYTYPE& key) {
		const size_t bucketIdx = (size_t)_hasher(key) % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
		KeyValue *prev = nullptr;
		size_t removed = 0;
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				KeyValue *next = entry->next;
				if (prev == nullptr) {
					_buckets[bucketIdx] = next;
				} else {
					prev->next = next;
				}
				freeNode(entry);
				entry = next;
				++removed;
				--_size;
				continue;
			}
			prev = entry;
			entry = entry->next;
		}
		return removed;
	}
};

}
