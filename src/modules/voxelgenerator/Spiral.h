/**
 * @file
 */

#pragma once

namespace voxelgenerator {

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
	int _z = 0;
public:
	Spiral() :
			_layer(1), _leg(0), _x(0), _z(0) {
	}
	void next(int amount = 1) {
		for (int i = 0; i < amount; ++i) {
			switch (_leg) {
			case 0:
				++_x;
				if (_x == _layer)
					++_leg;
				break;
			case 1:
				++_z;
				if (_z == _layer)
					++_leg;
				break;
			case 2:
				--_x;
				if (-_x == _layer)
					++_leg;
				break;
			case 3:
				--_z;
				if (-_z == _layer) {
					_leg = 0;
					++_layer;
				}
				break;
			}
		}
	}

	inline int x() const {
		return _x;
	}
	inline int z() const {
		return _z;
	}
};

}
