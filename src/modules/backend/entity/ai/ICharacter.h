/**
 * @file
 */
#pragma once

#include "ai-shared/common/CharacterId.h"
#include "ai-shared/common/CharacterMetaAttributes.h"
#include "attrib/ShadowAttributes.h"
#include "core/String.h"
#include "core/SharedPtr.h"
#include "core/NonCopyable.h"
#include "math/Random.h"
#include <glm/vec3.hpp>

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
class ICharacter : public core::NonCopyable {
protected:
	const ai::CharacterId _id;
	glm::vec3 _position { 0.0f };
	float _orientation = 0.0f;
	attrib::ShadowAttributes _shadowAttributes;
	ai::CharacterMetaAttributes _metaAttributes;
	math::Random _random;

public:
	explicit ICharacter(ai::CharacterId id) :
			_id(id) {
		_random.setSeed((unsigned int)_id);
	}

	virtual ~ICharacter() {
	}
	math::Random& random();

	bool operator==(const ICharacter &character) const;
	bool operator!=(const ICharacter &character) const;

	ai::CharacterId getId() const;
	/**
	 * @note This is virtual because you might want to override this in your implementation to
	 * make sure that the new position is also forwarded to your AI controlled entity.
	 */
	virtual void setPosition(const glm::vec3& position);

	const attrib::ShadowAttributes& shadowAttributes() const;
	double getCurrent(attrib::Type type) const;
	double getMax(attrib::Type type) const;
	void setCurrent(attrib::Type type, double value);
	void setMax(attrib::Type type, double value);

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
	 * @brief Set an meta attribute that can be used for debugging
	 * @see AI::isDebuggingActive()
	 */
	virtual void setMetaAttribute(const core::String& key, const core::String& value);
	/**
	 * @brief Get the debugger attributes.
	 */
	const ai::CharacterMetaAttributes& getMetaAttributes() const;
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
};

inline math::Random& ICharacter::random() {
	return _random;
}

inline void ICharacter::setOrientation(float orientation) {
	_orientation = orientation;
}

inline float ICharacter::getOrientation() const {
	return _orientation;
}

inline void ICharacter::setMetaAttribute(const core::String& key, const core::String& value) {
	_metaAttributes.put(key, value);
}

inline const ai::CharacterMetaAttributes& ICharacter::getMetaAttributes() const {
	return _metaAttributes;
}

inline bool ICharacter::operator==(const ICharacter &character) const {
	return character._id == _id;
}

inline bool ICharacter::operator!=(const ICharacter &character) const {
	return character._id != _id;
}

inline ai::CharacterId ICharacter::getId() const {
	return _id;
}

inline const glm::vec3& ICharacter::getPosition() const {
	return _position;
}

typedef core::SharedPtr<ICharacter> ICharacterPtr;

}
