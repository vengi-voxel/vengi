#pragma once

#include <functional>
#include <string>

namespace cooldown {

enum CooldownType {
	NONE,
	// the cooldown for increasing the population
	INCREASE,
	// defines the delay between hunts
	HUNT,
	// the cooldown for removing a disconnected user from the server
	LOGOUT,

	MAX
};

}

namespace std {
template<> struct hash<cooldown::CooldownType> {
	inline size_t operator()(const cooldown::CooldownType &c) const {
		return std::hash<size_t>()(static_cast<size_t>(c));
	}
};
}
