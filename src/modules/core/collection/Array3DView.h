/**
 * @file
 * @note Similar to std::span
 */

#pragma once

#include <glm/vec3.hpp>
#include <stddef.h>

namespace core {

/**
 * @ingroup Collections
 */
template <class T>
class Array3DView {
private:
	T *_data;
	const int _width;
	const int _height;
	const int _depth;

	constexpr int index(int x, int y, int z) const {
		return x + _width * (y + _height * z);
	}

public:
	constexpr Array3DView(T *data, int w, int h, int d) : _data(data), _width(w), _height(h), _depth(d) {
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

	constexpr const T &get(int x, int y, int z) const {
		const int idx = index(x, y, z);
		return _data[idx];
	}

	constexpr T &get(int x, int y, int z) {
		const int idx = index(x, y, z);
		return _data[idx];
	}

	constexpr void set(int x, int y, int z, const T &t) {
		const int idx = index(x, y, z);
		_data[idx] = t;
	}

	constexpr const T &get(const glm::ivec3& v) const {
		const int idx = index(v.x, v.y, v.z);
		return _data[idx];
	}

	constexpr T &get(const glm::ivec3& v) {
		const int idx = index(v.x, v.y, v.z);
		return _data[idx];
	}

	constexpr void set(const glm::ivec3& v, const T &t) {
		const int idx = index(v.x, v.y, v.z);
		_data[idx] = t;
	}

	constexpr size_t size() const {
		return (size_t)(_width * _height * _depth);
	}

	constexpr bool empty() const {
		return !size();
	}
};

} // namespace core
