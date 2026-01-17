/**
 * @file
 */

#pragma once

#include <limits>
#include <glm/vec2.hpp>
#include <glm/common.hpp>

namespace math {

template<typename TYPE = int>
class Rect {
private:
	glm::tvec2<TYPE> _mins;
	glm::tvec2<TYPE> _maxs;
public:
	constexpr Rect() :
			_mins(glm::tvec2<TYPE>((TYPE)0)),
			_maxs(glm::tvec2<TYPE>((TYPE)0)) {
	}

	constexpr Rect(TYPE minX, TYPE minZ, TYPE maxX, TYPE maxZ) :
			_mins(minX, minZ), _maxs(maxX, maxZ) {
	}

	constexpr Rect(const glm::tvec2<TYPE>& mins, const glm::tvec2<TYPE>& maxs) :
			_mins(mins), _maxs(maxs) {
	}

	static constexpr Rect<TYPE> getMaxRect() {
		constexpr TYPE lowest = std::numeric_limits<TYPE>::lowest() / (TYPE)2;
		constexpr TYPE max = (std::numeric_limits<TYPE>::max)() / (TYPE)2;
		constexpr Rect<TYPE> maxRect(lowest, lowest, max, max);
		return maxRect;
	}

	inline constexpr TYPE getMaxZ() const {
		return _maxs.y;
	}

	constexpr void setMaxZ(TYPE v) {
		_maxs.y = v;
	}

	inline constexpr TYPE getMinX() const {
		return _mins.x;
	}

	constexpr void setMinX(TYPE v) {
		_mins.x = v;
	}

	inline constexpr TYPE getMaxX() const {
		return _maxs.x;
	}

	constexpr void setMaxX(TYPE v) {
		_maxs.x = v;
	}

	inline constexpr TYPE getMinZ() const {
		return _mins.y;
	}

	constexpr void setMinZ(TYPE v) {
		_mins.y = v;
	}

	inline constexpr bool operator==(const Rect& other) const {
		return _mins == other._mins && _maxs == other._maxs;
	}

	inline constexpr bool intersectsWith(const Rect& other) const {
		if (getMaxX() <= other.getMinX() || getMinX() >= other.getMaxX()) {
			return false;
		}
		if (getMaxZ() <= other.getMinZ() || getMinZ() >= other.getMaxZ()) {
			return false;
		}
		return true;
	}

	inline constexpr Rect<TYPE> offset(TYPE dx, TYPE dz) const {
		Rect<TYPE> rect(*this);
		rect.offset(dx, dz);
		return rect;
	}

	inline constexpr void offset(TYPE dx, TYPE dz) {
		if (getMaxRect() == *this) {
			return;
		}
		_mins.x += dx;
		_mins.y += dz;
		_maxs.x += dx;
		_maxs.y += dz;
	}

	inline constexpr bool contains(const Rect& rect) const {
		if (rect.getMaxX() > getMaxX()) {
			return false;
		}
		if (rect.getMaxZ() > getMaxZ()) {
			return false;
		}
		if (rect.getMinX() < getMinX()) {
			return false;
		}
		if (rect.getMinZ() < getMinZ()) {
			return false;
		}
		return true;
	}

	inline constexpr bool contains(const glm::tvec2<TYPE>& point) const {
		if (point.x > getMaxX()) {
			return false;
		}
		if (point.y > getMaxZ()) {
			return false;
		}
		if (point.x < getMinX()) {
			return false;
		}
		if (point.y < getMinZ()) {
			return false;
		}
		return true;
	}

	inline bool containsf(const glm::vec2& point) const {
		const glm::ivec2 p(glm::ceil(point));
		if (p.x > getMaxX()) {
			return false;
		}
		if (p.y > getMaxZ()) {
			return false;
		}
		if (p.x < getMinX()) {
			return false;
		}
		if (p.y < getMinZ()) {
			return false;
		}
		return true;
	}

	inline glm::tvec2<TYPE> maxs() const {
		return glm::tvec2<TYPE>(getMaxX(), getMaxZ());
	}

	inline glm::tvec2<TYPE> mins() const {
		return glm::tvec2<TYPE>(getMinX(), getMinZ());
	}

	inline glm::tvec2<TYPE> upperLeft() const {
		return mins();
	}

	inline glm::tvec2<TYPE> upperRight() const {
		return glm::tvec2<TYPE>(getMaxX(), getMinZ());
	}

	inline glm::tvec2<TYPE> lowerRight() const {
		return maxs();
	}

	inline glm::tvec2<TYPE> lowerLeft() const {
		return glm::tvec2<TYPE>(getMinX(), getMaxZ());
	}

	inline constexpr TYPE width() const {
		return getMaxX() - getMinX();
	}

	inline constexpr TYPE height() const {
		return getMaxZ() - getMinZ();
	}

	inline glm::tvec2<TYPE> size() const {
		return maxs() - mins();
	}

	inline glm::tvec2<TYPE> center() const {
		return (mins() + maxs()) / (TYPE)2;
	}

	inline glm::vec2 centerf() const {
		return glm::vec2(mins() + maxs()) / 2.0f;
	}
};

typedef Rect<uint32_t> RectuInt;
typedef Rect<float> RectFloat;

}
