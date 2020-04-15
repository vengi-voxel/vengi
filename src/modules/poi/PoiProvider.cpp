/**
 * @file
 */

#include "PoiProvider.h"
#include "core/TimeProvider.h"
#include "math/Random.h"
#include "core/Var.h"
#include "core/GLM.h"
#include <glm/gtc/constants.hpp>

namespace poi {

PoiProvider::PoiProvider(const core::TimeProviderPtr& timeProvider) :
		_timeProvider(timeProvider), _lock("PoiProvider") {
}

void PoiProvider::update(long /*dt*/) {
	constexpr uint64_t seconds = 60L * 1000L;
	const uint64_t currentMillis = _timeProvider->tickNow();
	core::ScopedWriteLock scoped(_lock);
	// even if this is timed out - if we only have one, keep it.
	while (_pois.size() > 1) {
		Poi& poi = _pois.front();
		if (poi.time + seconds > currentMillis) {
			break;
		}
		_pois.pop_front();
	}
}

void PoiProvider::add(const glm::vec3& pos, Type type) {
	core::ScopedWriteLock scoped(_lock);
	_pois.emplace_back(Poi{pos, type, _timeProvider->tickNow()});
}

size_t PoiProvider::count() const {
	core::ScopedReadLock scoped(_lock);
	return _pois.size();
}

PoiResult PoiProvider::query(Type type) const {
	static PoiResult empty{glm::zero<glm::vec3>(), false};
	core::ScopedReadLock scoped(_lock);
	if (_pois.empty()) {
		return empty;
	}
	if (type == Type::NONE) {
		return PoiResult{_random.randomElement(_pois.begin(), _pois.end())->pos, true};
	}
	auto i = std::find_if(_pois.begin(), _pois.end(), [=] (const Poi& poi) {return poi.type == type;});
	if (i != _pois.end()) {
		return PoiResult{i->pos, true};
	}
	return empty;
}

}
