/**
 * @file
 */

#pragma once

#include "core/PoolAllocator.h"
#include <initializer_list>

namespace core {

/**
 * @brief Linked list with a max size given as constructor parameter.
 *
 * @note The allocations are performed with the @c PoolAllocator
 *
 * @ingroup Collections
 */
template<class TYPE>
class List {
private:
	struct Node {
		inline Node(const TYPE& _value) :
				value(_value), next(nullptr) {
		}
		TYPE value;
		Node *next;
	};
	core::PoolAllocator<Node> _allocator;
	Node *_first = nullptr;
	Node *_last = nullptr;
public:
	List(std::initializer_list<TYPE> other, int maxSize = 256) {
		_allocator.init(maxSize);
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(*i);
		}
	}
	List(int maxSize = 256) {
		_allocator.init(256);
	}
	List(const List& other) {
		_allocator.init((other._allocator.max)());
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->value);
		}
	}
	~List() {
		clear();
		_allocator.shutdown();
	}

	List& operator=(const List& other) {
		clear();
		_allocator.shutdown();
		_allocator.init((other._allocator.max)());
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(i->value);
		}
		return *this;
	}

	class iterator {
	private:
		const Node* _node;
	public:
		constexpr iterator() :
			_node(nullptr) {
		}

		iterator(const Node* node) : _node(node) {
		}

		inline const TYPE& operator*() const {
			return _node->value;
		}

		inline const TYPE& operator()() const {
			return _node->value;
		}

		inline TYPE& operator()() {
			return _node->value;
		}

		iterator& operator++() {
			if (_node->next != nullptr) {
				_node = _node->next;
				return *this;
			}
			_node = nullptr;
			return *this;
		}

		inline const Node* operator->() const {
			return _node;
		}

		inline bool operator!=(const iterator& rhs) const {
			return _node != rhs._node;
		}

		inline bool operator==(const iterator& rhs) const {
			return _node == rhs._node;
		}
	};

	inline size_t size() const {
		return _allocator.allocated();
	}

	inline bool empty() const {
		return size() == 0;
	}

	iterator begin() const {
		return iterator(_first);
	}

	constexpr iterator end() const {
		return iterator();
	}

	void clear() {
		Node *entry = _first;
		while (entry != nullptr) {
			Node *prev = entry;
			entry = entry->next;
			_allocator.free(prev);
		}
		_first = _last = nullptr;
	}

	bool insert(const TYPE& value) {
		Node* node = _allocator.alloc(value);
		if (node == nullptr) {
			return false;
		}
		if (_last == nullptr) {
			core_assert(_first == nullptr);
			_first = _last = node;
		} else {
			_last->next = node;
			_last = node;
		}
		return true;
	}

	const TYPE* back() const {
		if (_last == nullptr) {
			return nullptr;
		}
		return &_last->value;
	}

	TYPE* back() {
		if (_last == nullptr) {
			return nullptr;
		}
		return &_last->value;
	}

	/**
	 * @brief Only removed one element from the list. If there are more elements with the same value
	 * make sure to call this until false is returned.
	 */
	bool remove(const TYPE& value) {
		if (_first == nullptr) {
			return false;
		}
		if (_first->value == value) {
			if (_first->next == nullptr) {
				_allocator.free(_first);
				_first = _last = nullptr;
			} else {
				Node* next = _first->next;
				_allocator.free(_first);
				_first = next;
			}
			return true;
		}

		Node* prev = _first;
		Node* entry = _first->next;

		while (entry != nullptr) {
			if (entry->value == value) {
				prev->next = entry->next;
				_allocator.free(entry);
				return true;
			}
			entry = entry->next;
		}
		return false;
	}
};

}
