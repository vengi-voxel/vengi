#pragma once

#include "CooldownType.h"
#include "core/EnumHash.h"
#include "core/Log.h"
#include <memory>

namespace cooldown {

static constexpr int DefaultDuration = 1000;

class CooldownDuration {
private:
	bool _initialized = false;
	long _durations[core::enumValue<Type>(Type::MAX) + 1];
	std::string _error;
public:

	CooldownDuration();

	long duration(Type type) const;

	bool init(const std::string& filename);

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
