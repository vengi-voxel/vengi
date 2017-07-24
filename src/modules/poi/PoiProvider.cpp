/**
 * @file
 */

#include "PoiProvider.h"
#include "core/TimeProvider.h"
#include "core/Random.h"
#include "voxel/World.h"
#include "core/Var.h"

namespace poi {

PoiProvider::PoiProvider(const voxel::WorldPtr& world, const core::TimeProviderPtr& timeProvider) :
		_world(world), _timeProvider(timeProvider), _lock("PoiProvider") {
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
		if (poi.time + seconds > _timeProvider->tickTime()) {
			break;
		}
		_pois.erase(i);
	}
}

void PoiProvider::addPointOfInterest(const glm::vec3& pos) {
	core::ScopedWriteLock scoped(_lock);
	_pois.emplace_back(Poi{pos, _timeProvider->tickTime()});

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
		return _world->randomPos();
	}
	return _world->random().randomElement(_pois.begin(), _pois.end())->pos;
}

}
