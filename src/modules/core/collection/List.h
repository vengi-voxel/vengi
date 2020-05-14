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
	using value_type = TYPE;

	List(std::initializer_list<TYPE> other, int maxSize = 256) {
		_allocator.init(maxSize);
		for (auto i = other.begin(); i != other.end(); ++i) {
			insert(*i);
		}
	}
	List(int maxSize = 256) {
		_allocator.init(maxSize);
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
	friend class List;
	private:
		Node* _node;
		Node* _prev;
	public:
		constexpr iterator() :
			_node(nullptr), _prev(nullptr) {
		}

		iterator(Node* node, Node* prev) :
			_node(node), _prev(prev) {
		}

		inline const TYPE& operator*() const {
			return _node->value;
		}

		inline TYPE& operator*() {
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
				_prev = _node;
				_node = _node->next;
				return *this;
			}
			_prev = _node;
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
		return iterator(_first, nullptr);
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
	 * @brief Only remove one element from the list. If there are more elements with the same value
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

	iterator erase(iterator iter) {
		if (iter == end()) {
			return iter;
		}
		Node* node = iter._node;
		Node* next = node->next;
		Node* prev = iter._prev;
		if (node == _first) {
			_first = next;
		}
		if (node == _last) {
			_last = prev;
		}
		if (prev != nullptr) {
			prev->next = next;
		}
		_allocator.free(node);
		return iterator(next, prev);
	}
};

}
