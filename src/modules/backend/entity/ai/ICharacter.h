/**
 * @file
 */
#pragma once

#include "common/Math.h"
#include "ai-shared/common/CharacterId.h"
#include "ai-shared/common/CharacterAttributes.h"
#include "core/String.h"
#include "core/NonCopyable.h"
#include <atomic>
#include <memory>
#include <unordered_map>

namespace backend {

/**
 * @brief Class that should be extended by the @ai{AI} controlled entity class.
 *
 * It uses a @ai{CharacterId} to identify the character in the game. The @ai{AI} class
 * has a reference to its controlled @c ICharacter instance.
 *
 * @note Update the values of the @c ICharacter class only in in the @ai{ICharacter::update()}
 * method or from within the @ai{Zone} callbacks. Otherwise you will run into race conditions
 * if you run with multiple threads.
 */
class ICharacter : public core::NonCopyable, public std::enable_shared_from_this<ICharacter> {
protected:
	const ai::CharacterId _id;
	glm::vec3 _position;
	std::atomic<float> _orientation;
	// m/s
	std::atomic<float> _speed;
	ai::CharacterAttributes _attributes;

public:
	explicit ICharacter(ai::CharacterId id) :
			_id(id), _position(0.0f), _orientation(0.0f), _speed(0.0f) {
	}

	virtual ~ICharacter() {
	}

	bool operator ==(const ICharacter& character) const;
	bool operator !=(const ICharacter& character) const;

	ai::CharacterId getId() const;
	/**
	 * @note This is virtual because you might want to override this in your implementation to
	 * make sure that the new position is also forwarded to your AI controlled entity.
	 */
	virtual void setPosition(const glm::vec3& position);

	const glm::vec3& getPosition() const;
	/**
	 * @note This is virtual because you might want to override this in your implementation to
	 * make sure that the new orientation is also forwarded to your AI controlled entity.
	 *
	 * @see getOrientation()
	 */
	virtual void setOrientation(float orientation);
	/**
	 * @return the radians around the y (up) axis
	 *
	 * @see setOrientation()
	 */
	float getOrientation() const;
	/**
	 * @brief Sets the speed for the character in m/s
	 *
	 * @see getSpeed()
	 */
	virtual void setSpeed(float speed);
	/**
	 * @return The speed for the character in m/s
	 *
	 * @see setSpeed()
	 */
	float getSpeed() const;
	/**
	 * @brief Set an attribute that can be used for debugging
	 * @see AI::isDebuggingActive()
	 */
	virtual void setAttribute(const core::String& key, const core::String& value);
	/**
	 * @brief Get the debugger attributes.
	 */
	const ai::CharacterAttributes& getAttributes() const;
	/**
	 * @brief override this method to let your own @c ICharacter implementation
	 * tick with the @c Zone::update
	 *
	 * @param[in] dt the time delta in millis since the last update was executed
	 * @param[in] debuggingActive @c true if the debugging for this entity is activated. This
	 * can be used to determine whether it's useful to do setAttribute() calls.
	 */
	virtual void update(int64_t dt, bool debuggingActive) {
		(void)dt;
		(void)debuggingActive;
	}

	/**
	 * If the object is currently maintained by a shared_ptr, you can get a shared_ptr from a raw pointer
	 * instance that shares the state with the already existing shared_ptrs around.
	 */
	inline std::shared_ptr<ICharacter> ptr() {
		return shared_from_this();
	}
};

inline void ICharacter::setPosition(const glm::vec3& position) {
	_position = position;
}

inline void ICharacter::setOrientation (float orientation) {
	_orientation = orientation;
}

inline float ICharacter::getOrientation () const {
	return _orientation;
}

inline void ICharacter::setAttribute(const core::String& key, const core::String& value) {
	_attributes[key] = value;
}

inline const ai::CharacterAttributes& ICharacter::getAttributes() const {
	return _attributes;
}

inline bool ICharacter::operator ==(const ICharacter& character) const {
	return character._id == _id;
}

inline bool ICharacter::operator !=(const ICharacter& character) const {
	return character._id != _id;
}

inline ai::CharacterId ICharacter::getId() const {
	return _id;
}

inline const glm::vec3& ICharacter::getPosition() const {
	return _position;
}

inline void ICharacter::setSpeed(float speed) {
	_speed = speed;
}

inline float ICharacter::getSpeed() const {
	return _speed;
}

typedef std::shared_ptr<ICharacter> ICharacterPtr;

}
