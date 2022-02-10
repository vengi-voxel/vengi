/**
 * @file
 * @note Similar to std::span
 */

#pragma once

#include <stddef.h>

namespace core {

/**
 * @ingroup Collections
 */
template <class T>
class Array2DView {
private:
	T *_data;
	const int _width;
	const int _height;

	constexpr int index(int x, int y) const {
		return y * _width + x;
	}

public:
	constexpr Array2DView(T *data, int w, int h) : _data(data), _width(w), _height(h) {
	}

	constexpr int width() const {
		return _width;
	}

	constexpr int height() const {
		return _height;
	}

	constexpr const T *data() const {
		return _data;
	}

	constexpr operator const T *() const {
		return _data;
	}

	constexpr const T &get(int x, int y) const {
		return _data[index(x, y)];
	}

	constexpr T &get(int x, int y) {
		return _data[index(x, y)];
	}

	constexpr void set(int x, int y, const T &t) {
		_data[index(x, y)] = t;
	}

	constexpr size_t size() const {
		return (size_t)(_width * _height);
	}

	constexpr bool empty() const {
		return !size();
	}
};

} // namespace core
