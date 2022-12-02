/**
 * @file
 */

#pragma once

#include "core/Common.h"
#include "core/collection/Array.h"
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
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = privdynamicmap::DefaultHasher, typename COMPARE = privdynamicmap::EqualCompare>
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
private:
	core::Array<KeyValue *, BUCKETSIZE> _buckets;
	HASHER _hasher;
	size_t _size = 0;
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
	DynamicMap(DynamicMap&& other) noexcept {
		_buckets = other._buckets;
		_hasher = other._hasher;
		_size = other._size;
		other._buckets.fill(nullptr);
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
			other._buckets.fill(nullptr);
		}
		return *this;
	}

	DynamicMap& operator=(const DynamicMap& other) {
		clear();
		_buckets.fill(nullptr);
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
		return find(key) != end();
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
			entry = new KeyValue(key, core::forward<VALUETYPE>(value));
			if (prev == nullptr) {
				_buckets[hashValue % BUCKETSIZE] = entry;
			} else {
				prev->next = entry;
			}
		} else {
			entry->value = core::forward<VALUETYPE>(value);
		}
		++_size;
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
			entry = new KeyValue(key, value);
			if (prev == nullptr) {
				_buckets[hashValue % BUCKETSIZE] = entry;
			} else {
				prev->next = entry;
			}
		} else {
			entry->value = value;
		}
		++_size;
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
				delete prev;
			}
			_buckets[i] = nullptr;
		}
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

		delete entry;
		--_size;
		return true;
	}
};

}
