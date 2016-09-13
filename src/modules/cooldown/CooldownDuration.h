#pragma once

#include "CooldownType.h"
#include "core/EnumHash.h"
#include "core/Log.h"
#include <memory>

namespace cooldown {

/**
 * @brief Duration in millis that is taken to initialize all available cooldowns
 */
static constexpr int DefaultDuration = 1000;

/**
 * @brief Manages the cooldown durations
 * @ingroup Cooldowns
 */
class CooldownDuration {
private:
	bool _initialized = false;
	long _durations[core::enumValue<Type>(Type::MAX) + 1];
	std::string _error;
public:
	/**
	 * @brief Ctor to init all available cooldowns to the DefaultDuration
	 */
	CooldownDuration();

	/**
	 * @return The duration in millis for the given cooldown type
	 */
	long duration(Type type) const;

	/**
	 * @brief Allow to manually override the duration of a cooldown type
	 * @param[in] type The cooldown type to set the duration for
	 * @param[in] duration Duration is given in milliseconds.
	 * @return the previous duration set for the given type
	 */
	long setDuration(Type type, long duration);

	/**
	 * @brief Initializes the cooldown durations.
	 * @param[in] filename If this string is not empty, it is taken as a filename to the lua script
	 * that contains the cooldown initialization data.
	 * @return @c false in case of an error, @c true if the initialization was successful.
	 * @sa error()
	 */
	bool init(const std::string& filename);

	/**
	 * @brief Access to the last error that was reported in case the @c init() call failed.
	 * @sa init()
	 */
	const std::string& error() const;
};

inline const std::string& CooldownDuration::error() const {
	return _error;
}

inline long CooldownDuration::duration(Type type) const {
	if (!_initialized) {
		::Log::warn("Trying to get cooldown duration without CooldownDuration::init() being called");
	}
	return _durations[core::enumValue<Type>(type)];
}

typedef std::shared_ptr<CooldownDuration> CooldownDurationPtr;

}
