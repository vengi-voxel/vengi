/**
 * @file
 */

#pragma once

#include "core/collection/Array.h"
#include "core/PoolAllocator.h"
#include <stddef.h>
#include <initializer_list>

namespace core {

namespace priv {

struct EqualCompare {
	template<typename T>
	CORE_FORCE_INLINE bool operator() (const T& lhs, const T& rhs) const {
		return lhs == rhs;
	}
};

struct DefaultHasher {
	template<typename T>
	CORE_FORCE_INLINE size_t operator() (const T& o) const {
		return (size_t)o;
	}
};

struct SharedPtrHasher {
	template<typename T>
	CORE_FORCE_INLINE size_t operator() (const T& o) const {
		return (size_t)(intptr_t)o.get();
	}
};

}

/**
 * @brief Hash map with a max size given as constructor parameter.
 *
 * @note The allocations are performed with the @c PoolAllocator
 *
 * @ingroup Collections
 */
template<typename KEYTYPE, typename VALUETYPE, size_t BUCKETSIZE = 11, typename HASHER = priv::DefaultHasher, typename COMPARE = priv::EqualCompare>
class Map {
public:
	using value_type = VALUETYPE;
	using key_type = KEYTYPE;

	struct KeyValue {
		CORE_FORCE_INLINE KeyValue(const KEYTYPE& _key, const VALUETYPE& _value) :
				key(_key), value(_value), next(nullptr), first(key), second(value) {
		}

		CORE_FORCE_INLINE KeyValue(const KEYTYPE& _key, VALUETYPE&& _value) :
				key(_key), value(core::forward<VALUETYPE>(_value)), next(nullptr), first(key), second(value) {
		}

		CORE_FORCE_INLINE KeyValue(KeyValue &&other) noexcept :
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
	core::PoolAllocator<KeyValue, uint32_t> _allocator;
	using Buckets = core::Array<KeyValue *, BUCKETSIZE>;
	Buckets _buckets;
	HASHER _hasher;
public:
	Map(std::initializer_list<KeyValue> other, int maxSize = 4096) {
		core_assert_msg(maxSize > 0, "Max size must be greater than 0 - but is %i", maxSize);
		core_assert_msg(maxSize >= (int)other.size(), "Max size must be greater than the initializer list: %i vs %i", maxSize, (int)other.size());
		core_assert_always(_allocator.init(core_max(2, maxSize)));
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	Map(int maxSize = 4096) {
		core_assert_msg(maxSize > 0, "Max size must be greater than 0 - but is %i", maxSize);
		core_assert_always(_allocator.init(core_max(2, maxSize)));
		_buckets.fill(nullptr);
	}
	Map(const Map& other) {
		core_assert_always(_allocator.init((other._allocator.max)()));
		_buckets.fill(nullptr);
		for (auto i = other.begin(); i != other.end(); ++i) {
			put(i->key, i->value);
		}
	}
	Map(Map&& other) noexcept : _allocator(core::move(other._allocator)), _buckets(other._buckets), _hasher(other._hasher) {
		other._buckets.fill(nullptr);
	}
	~Map() {
		clear();
		_allocator.shutdown();
	}
	Map &operator=(Map &&other) noexcept {
		if (this != &other) {
			clear();
			_allocator = core::move(other._allocator);
			_buckets = other._buckets;
			_hasher = other._hasher;
			other._buckets.fill(nullptr);
		}
		return *this;
	}

	Map& operator=(const Map& other) {
		clear();
		_allocator.shutdown(); // TODO: not needed init() will handle this
		core_assert_always(_allocator.init((other._allocator.max)()));
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
		KeyValue* _ptr;
	public:
		constexpr iterator() :
			_map(nullptr), _bucket(0), _ptr(nullptr) {
		}

		CORE_FORCE_INLINE iterator(const Map* map, size_t bucket, KeyValue *ptr) :
				_map(map), _bucket(bucket), _ptr(ptr) {
		}

		CORE_FORCE_INLINE KeyValue* operator*() const {
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

	CORE_FORCE_INLINE size_t size() const {
		return _allocator.allocated();
	}

	CORE_FORCE_INLINE bool empty() const {
		return size() == 0;
	}

	CORE_FORCE_INLINE size_t capacity() const {
		return (_allocator.max)();
	}

	bool get(const KEYTYPE& key, VALUETYPE& value) const {
		const size_t hashValue = (size_t)_hasher(key);
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
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
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
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
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
		while (entry != nullptr) {
			if (COMPARE()(entry->key, key)) {
				return iterator(this, bucketIdx, entry);
			}
			entry = entry->next;
		}
		return end();
	}

	void emplace(const KEYTYPE& key, VALUETYPE&& value) {
		const size_t hashValue = (size_t)_hasher(key);
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[bucketIdx];

		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			entry = _allocator.alloc(key, core::forward<VALUETYPE>(value));
			core_assert_msg(entry != nullptr, "Failed to allocate for hash: %i (size: %i/%i)", (int)hashValue, (int)size(), (int)capacity());
			if (prev == nullptr) {
				_buckets[bucketIdx] = entry;
			} else {
				prev->next = entry;
			}
		} else {
			entry->value = core::forward<VALUETYPE>(value);
		}
	}

	void put(const KEYTYPE& key, const VALUETYPE& value) {
		const size_t hashValue = (size_t)_hasher(key);
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *prev = nullptr;
		KeyValue *entry = _buckets[bucketIdx];

		while (entry != nullptr && !COMPARE()(entry->key, key)) {
			prev = entry;
			entry = entry->next;
		}

		if (entry == nullptr) {
			entry = _allocator.alloc(key, value);
			core_assert_msg(entry != nullptr, "Failed to allocate for hash: %i (size: %i/%i)", (int)hashValue, (int)size(), (int)capacity());
			if (prev == nullptr) {
				_buckets[bucketIdx] = entry;
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

	CORE_FORCE_INLINE bool erase(const iterator& iter) {
		return remove(iter->key);
	}

	bool remove(const KEYTYPE& key) {
		const size_t hashValue = (size_t)_hasher(key);
		const size_t bucketIdx = hashValue % BUCKETSIZE;
		KeyValue *entry = _buckets[bucketIdx];
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
			_buckets[bucketIdx] = entry->next;
		} else {
			prev->next = entry->next;
		}

		_allocator.free(entry);
		return true;
	}
};

struct hashCharPtr {
	size_t operator()(const char *p) const;
};
struct hashCharCompare {
	bool operator()(const char *lhs, const char *rhs) const;
};

using CharPointerMap = core::Map<const char*, const char*, 8, hashCharPtr, hashCharCompare>;

}
