/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/collection/Array.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/DynamicList.h"
#include <stdint.h>
#include <stddef.h>
#include <initializer_list>

namespace core {

namespace privdynamicmap {

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

struct SharedPtrHasher {
	template<typename T>
	inline size_t operator() (const T& o) const {
		return (size_t)(intptr_t)o.get();
	}
};

}

/**
 * @brief Dynamic growing hash map.
 * @sa Map
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmap::DefaultHasher,
		 typename COMPARE = privdynamicmap::EqualCompare, size_t BLOCK_SIZE = 256>
class DynamicMap {
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
		const VALUETYPE &second;
	};
protected:
	struct Block {
		KeyValue *_nodes;
		size_t _used = 0;

		Block(size_t count) : _nodes(static_cast<KeyValue *>(core_malloc(sizeof(KeyValue) * count))) {
		}

		Block(const Block&) = delete;
		Block &operator=(const Block&) = delete;
		Block(Block &&) = delete;
		Block &operator=(Block &&) = delete;

		~Block() {
			for (size_t i = 0; i < _used; ++i) {
				_nodes[i].~KeyValue();
			}
			core_free(_nodes);
			_nodes = nullptr;
			_used = 0;
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

		if (_blocks.empty() || _blocks.back()->_used >= BLOCK_SIZE) {
			_blocks.emplace(BLOCK_SIZE);
		}

		KeyValue *node = &_blocks.back()->_nodes[_blocks.back()->_used++];
		return ::new (node) KeyValue(core::forward<Args>(args)...);
	}

	void freeNode(KeyValue *node) {
		node->~KeyValue();
		_freeList.push_back(node);
	}

public:
	DynamicMap(std::initializer_list<KeyValue> other) {
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	DynamicMap() {
		_buckets.fill(nullptr);
	}
	DynamicMap(const DynamicMap& other) {
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	DynamicMap(DynamicMap &&other) noexcept
		: _buckets(other._buckets), _hasher(other._hasher), _size(other._size), _blocks(core::move(other._blocks)),
		  _freeList(core::move(other._freeList)) {
		other._buckets.fill(nullptr);
		other._size = 0;
	}
	~DynamicMap() {
		clear();
	}
	DynamicMap &operator=(DynamicMap &&other) noexcept {
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

	DynamicMap& operator=(const DynamicMap& other) {
		clear();
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
		return *this;
	}

	class iterator {
	private:
		const DynamicMap* _map;
		size_t _bucket;
		KeyValue* _ptr;
	public:
		constexpr iterator() :
			_map(nullptr), _bucket(0), _ptr(nullptr) {
		}

		iterator(const DynamicMap* map, size_t bucket, KeyValue *ptr) :
				_map(map), _bucket(bucket), _ptr(ptr) {
		}

		inline KeyValue* operator*() const {
			return _ptr;
		}

		iterator& operator++() {
			if (_ptr->next != nullptr) {
				_ptr = _ptr->next;
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

		inline KeyValue* operator->() const {
			return _ptr;
		}

		inline bool operator!=(const iterator& rhs) const {
			return _ptr != rhs._ptr;
		}

		inline bool operator==(const iterator& rhs) const {
			return _ptr == rhs._ptr;
		}
	};

	inline size_t size() const {
		return _size;
	}

	inline bool empty() const {
		return _size == 0u;
	}

	bool get(const KEYTYPE& key, VALUETYPE& value) const {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				value = entry->value;
				return true;
			}
			entry = entry->next;
		}
		return false;
	}

	bool hasKey(const KEYTYPE& key) const {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				return true;
			}
			entry = entry->next;
		}
		return false;
	}

	iterator find(const KEYTYPE& key) const {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				return iterator(this, hashValue % BUCKETSIZE, entry);
			}
			entry = entry->next;
		}
		return end();
	}

	void emplace(const KEYTYPE& key, VALUETYPE&& value) {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];

		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			entry = allocateNode(key, core::forward<VALUETYPE>(value));
			if (prev == nullptr) {
				_buckets[hashValue % BUCKETSIZE] = entry;
			} else {
				prev->next = entry;
			}
			++_size;
		} else {
			entry->value = core::forward<VALUETYPE>(value);
		}
	}

	void put(const KEYTYPE& key, const VALUETYPE& value) {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];

		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			entry = allocateNode(key, value);
			if (prev == nullptr) {
				_buckets[hashValue % BUCKETSIZE] = entry;
			} else {
				prev->next = entry;
			}
			++_size;
		} else {
			entry->value = value;
		}
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
		_buckets.fill(nullptr);
		_freeList.clear();
		_blocks.clear();
		_size = 0;
	}

	inline void erase(const iterator& iter) {
		remove(iter->key);
	}

	bool remove(const KEYTYPE& key) {
		const size_t hashValue = (size_t)_hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		KeyValue *prev = nullptr;

		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				break;
			}
			prev = entry;
			entry = entry->next;
		}
		if (entry == nullptr) {
			return false;
		}

		if (prev == nullptr) {
			_buckets[hashValue % BUCKETSIZE] = entry->next;
		} else {
			prev->next = entry->next;
		}

		freeNode(entry);
		--_size;
		return true;
	}
};
}
