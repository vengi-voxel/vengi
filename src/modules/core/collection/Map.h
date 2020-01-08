/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "core/PoolAllocator.h"
#include <stddef.h>
#include <initializer_list>

namespace core {

/**
 * @brief Fixed element amount map
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE, typename HASHER>
class Map {
public:
	struct KeyValue {
		inline KeyValue(const KEYTYPE& _key, const VALUETYPE& _value) :
				key(_key), value(_value), next(nullptr), first(key), second(value) {
		}

		inline KeyValue(KeyValue &&other) :
				key(std::move(other.key)), value(std::move(other.value)), next(
						nullptr), first(key), second(value) {
		}

		KEYTYPE key;
		VALUETYPE value;
		KeyValue *next;
		const KEYTYPE &first;
		const VALUETYPE &second;
	};
private:
	core::PoolAllocator<KeyValue> _allocator;
	core::Array<KeyValue *, BUCKETSIZE> _buckets;
	HASHER _hasher;
public:
	Map(std::initializer_list<KeyValue> other, int maxSize = 4096) {
		_allocator.init(maxSize);
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	Map(int maxSize = 4096) {
		_allocator.init(maxSize);
		_buckets.fill(nullptr);
	}
	Map(const Map& other) {
		_allocator.init((other._allocator.max)());
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	~Map() {
		clear();
		_allocator.shutdown();
	}

	Map& operator=(const Map& other) {
		clear();
		_allocator.shutdown();
		_allocator.init((other._allocator.max)());
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
		return *this;
	}

	class iterator {
	private:
		const Map* _map;
		size_t _bucket;
		const KeyValue* _ptr;
	public:
		constexpr iterator() :
			_map(nullptr), _bucket(0), _ptr(nullptr) {
		}

		iterator(const Map* map, size_t bucket, KeyValue *ptr) :
				_map(map), _bucket(bucket), _ptr(ptr) {
		}

		inline const KeyValue* operator*() const {
			return _ptr;
		}

		iterator& operator++() {
			if (_ptr->next != nullptr) {
				_ptr = _ptr->next;
				return *this;
			}
			size_t bucket = _bucket;
			for (++bucket; bucket < BUCKETSIZE; ++bucket) {
				const KeyValue* ptr = _map->_buckets[bucket];
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

		inline const KeyValue* operator->() const {
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
		return _allocator.allocated();
	}

	inline bool empty() const {
		return size() == 0;
	}

	inline size_t capacity() const {
		return (_allocator.max)();
	}

	bool get(const KEYTYPE& key, VALUETYPE& value) const {
		unsigned long hashValue = _hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		while (entry != nullptr) {
			if (entry->key == key) {
				value = entry->value;
				return true;
			}
			entry = entry->next;
		}
		return false;
	}

	iterator find(const KEYTYPE& key) const {
		unsigned long hashValue = _hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		while (entry != nullptr) {
			if (entry->key == key) {
				return iterator(this, hashValue % BUCKETSIZE, entry);
			}
			entry = entry->next;
		}
		return end();
	}

	void put(const KEYTYPE& key, const VALUETYPE& value) {
		unsigned long hashValue = _hasher(key);
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];

		while (entry != nullptr && entry->key != key) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			entry = _allocator.alloc(key, value);
			core_assert_msg(entry != nullptr, "Failed to allocate for hash: %i", (int)hashValue);
			if (prev == nullptr) {
				_buckets[hashValue % BUCKETSIZE] = entry;
			} else {
				prev->next = entry;
			}
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
		for (size_t i = 0u; i < BUCKETSIZE; ++i) {
			KeyValue *entry = _buckets[i];
			while (entry != nullptr) {
				KeyValue *prev = entry;
				entry = entry->next;
				_allocator.free(prev);
			}
			_buckets[i] = nullptr;
		}
	}

	inline void erase(const iterator& iter) {
		remove(iter->key);
	}

	bool remove(const KEYTYPE& key) {
		unsigned long hashValue = _hasher(key);
		KeyValue *entry = _buckets[hashValue % BUCKETSIZE];
		KeyValue *prev = nullptr;

		while (entry != nullptr) {
			if (entry->key == key) {
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

		_allocator.free(entry);
		return true;
	}
};

}
