/**
 * @file
 */

#include "core/GLM.h"
#include "voxel/Voxel.h"
#include "voxelutil/FloorTraceResult.h"
#include "Shared_generated.h"
#include <stdint.h>
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <functional>

namespace shared {

using WalkableFloorResolver = std::function<voxelutil::FloorTraceResult(const glm::ivec3& pos, int maxWalkableHeight)>;

class SharedMovement {
protected:
	network::MoveDirection _move = network::MoveDirection::NONE;
	bool _gliding = false;
	bool _jumping = false;
	bool _swimming = false;
	bool _inWater = false;

	double _fallingVelocity = 0.0;
	voxelutil::FloorTraceResult _floor;
	double _delay = 0.0;
	double _speed = 0.0;

	double gravity() const;

	glm::vec3 calculateDelta(const glm::quat& rot) const;
public:
	glm::vec3 update(double deltaFrameSeconds, float orientation, double speed, const glm::vec3& currentPos, const WalkableFloorResolver& heightResolver);

	void setMoveMask(network::MoveDirection moveMask);
	network::MoveDirection moveMask() const;

	bool left() const;
	bool right() const;
	bool forward() const;
	bool backward() const;
	bool jump() const;
	bool gliding() const;
	bool walking() const;

	bool moving() const;
	network::Animation animation() const;

	voxel::Voxel groundVoxel() const;
	int groundHeight() const;
};

inline void SharedMovement::setMoveMask(network::MoveDirection moveMask) {
	_move = moveMask;
}

inline network::MoveDirection SharedMovement::moveMask() const {
	return _move;
}

inline bool SharedMovement::moving() const {
	return left() || right() || forward() || backward();
}

inline bool SharedMovement::walking() const {
	if (!moving()) {
		return false;
	}
	return !_swimming && !_gliding && !_jumping;
}

inline bool SharedMovement::left() const {
	return (_move & network::MoveDirection::MOVELEFT) == network::MoveDirection::MOVELEFT;
}

inline bool SharedMovement::right() const {
	return (_move & network::MoveDirection::MOVERIGHT) == network::MoveDirection::MOVERIGHT;
}

inline bool SharedMovement::forward() const {
	return (_move & network::MoveDirection::MOVEFORWARD) == network::MoveDirection::MOVEFORWARD;
}

inline bool SharedMovement::backward() const {
	return (_move & network::MoveDirection::MOVEBACKWARD) == network::MoveDirection::MOVEBACKWARD;
}

inline bool SharedMovement::jump() const {
	return (_move & network::MoveDirection::JUMP) == network::MoveDirection::JUMP;
}

inline bool SharedMovement::gliding() const {
	return _gliding;
}

inline int SharedMovement::groundHeight() const {
	return _floor.heightLevel;
}

inline voxel::Voxel SharedMovement::groundVoxel() const {
	return _floor.voxel;
}

}
