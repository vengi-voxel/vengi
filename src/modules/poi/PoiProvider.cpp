/**
 * @file
 */

#include "PoiProvider.h"
#include "core/TimeProvider.h"
#include "core/Random.h"
#include "core/Var.h"
#include "core/GLM.h"

namespace poi {

PoiProvider::PoiProvider(const core::TimeProviderPtr& timeProvider) :
		_timeProvider(timeProvider), _lock("PoiProvider") {
}

void PoiProvider::update(long /*dt*/) {
	constexpr unsigned long seconds = 60L * 1000L;
	for (;;) {
		core::ScopedWriteLock scoped(_lock);
		auto i = _pois.begin();
		if (i == _pois.end()) {
			break;
		}
		Poi poi = *i;
		if (poi.time + seconds > _timeProvider->tickMillis()) {
			break;
		}
		_pois.erase(i);
	}
}

void PoiProvider::addPointOfInterest(const glm::vec3& pos) {
	core::ScopedWriteLock scoped(_lock);
	_pois.emplace_back(Poi{pos, _timeProvider->tickMillis()});

	struct PoiComparatorLess: public std::binary_function<Poi, Poi, bool> {
		inline bool operator()(const Poi& x, const Poi& y) const {
			return std::less<unsigned long>()(x.time, y.time);
		}
	};

	std::sort(_pois.begin(), _pois.end(), PoiComparatorLess());
}

size_t PoiProvider::getPointOfInterestCount() const {
	return _pois.size();
}

glm::vec3 PoiProvider::getPointOfInterest() const {
	if (_pois.empty()) {
		return glm::zero<glm::vec3>();
	}
	return _random.randomElement(_pois.begin(), _pois.end())->pos;
}

}
