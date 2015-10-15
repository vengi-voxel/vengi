#pragma once

namespace voxel {

/**
 * @brief Two dimensional spiral
 *
 * The call to @c Spiral::next() will advance the coordinates that are then accessible via @c Spiral::x() and @c Spiral::y().
 */
class Spiral {
protected:
	int _layer = 1;
	int _leg = 0;
	int _x = 0;
	int _y = 0;
public:
	Spiral() :
			_layer(1), _leg(0), _x(0), _y(0) {
	}
	void next() {
		switch (_leg) {
		case 0:
			++_x;
			if (_x == _layer)
				++_leg;
			break;
		case 1:
			++_y;
			if (_y == _layer)
				++_leg;
			break;
		case 2:
			--_x;
			if (-_x == _layer)
				++_leg;
			break;
		case 3:
			--_y;
			if (-_y == _layer) {
				_leg = 0;
				++_layer;
			}
			break;
		}
	}

	inline int x() const {
		return _x;
	}
	inline int y() const {
		return _y;
	}
};

}
