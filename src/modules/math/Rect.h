/**
 * @file
 */

#pragma once

#include <limits>
#include "math/VecOf.h"

namespace math {

template<typename TYPE = int>
class Rect {
private:
	typename Vec2Of<TYPE>::Type _mins;
	typename Vec2Of<TYPE>::Type _maxs;
public:
	Rect() :
			_mins(typename Vec2Of<TYPE>::Type((TYPE)0)),
			_maxs(typename Vec2Of<TYPE>::Type((TYPE)0)) {
	}

	Rect(TYPE minX, TYPE minZ, TYPE maxX, TYPE maxZ) :
			_mins(minX, minZ), _maxs(maxX, maxZ) {
	}

	Rect(const typename Vec2Of<TYPE>::Type& mins, const typename Vec2Of<TYPE>::Type& maxs) :
			_mins(mins), _maxs(maxs) {
	}

	static Rect<TYPE> getMaxRect() {
		constexpr TYPE lowest = std::numeric_limits<TYPE>::lowest() / (TYPE)2;
		constexpr TYPE max = (std::numeric_limits<TYPE>::max)() / (TYPE)2;
		const Rect<TYPE> maxRect(lowest, lowest, max, max);
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

	inline constexpr bool contains(const typename Vec2Of<TYPE>::Type& point) const {
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

	inline bool containsf(const math::Vec2f& point) const {
		const math::Vec2i p(math::ceil(point));
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

	inline typename Vec2Of<TYPE>::Type maxs() const {
		return typename Vec2Of<TYPE>::Type(getMaxX(), getMaxZ());
	}

	inline typename Vec2Of<TYPE>::Type mins() const {
		return typename Vec2Of<TYPE>::Type(getMinX(), getMinZ());
	}

	inline typename Vec2Of<TYPE>::Type upperLeft() const {
		return mins();
	}

	inline typename Vec2Of<TYPE>::Type upperRight() const {
		return typename Vec2Of<TYPE>::Type(getMaxX(), getMinZ());
	}

	inline typename Vec2Of<TYPE>::Type lowerRight() const {
		return maxs();
	}

	inline typename Vec2Of<TYPE>::Type lowerLeft() const {
		return typename Vec2Of<TYPE>::Type(getMinX(), getMaxZ());
	}

	inline constexpr TYPE width() const {
		return getMaxX() - getMinX();
	}

	inline constexpr TYPE height() const {
		return getMaxZ() - getMinZ();
	}

	inline typename Vec2Of<TYPE>::Type size() const {
		return maxs() - mins();
	}

	inline typename Vec2Of<TYPE>::Type center() const {
		return (mins() + maxs()) / (TYPE)2;
	}

	inline math::Vec2f centerf() const {
		return math::Vec2f(mins() + maxs()) / 2.0f;
	}
};

typedef Rect<uint32_t> RectuInt;
typedef Rect<float> RectFloat;

}
