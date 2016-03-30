#include "PoiProvider.h"
#include "core/Random.h"
#include "voxel/World.h"
#include "core/Var.h"

namespace backend {

PoiProvider::PoiProvider(voxel::WorldPtr world, core::TimeProviderPtr timeProvider) :
		_world(world), _timeProvider(timeProvider), _lock("PoiProvider") {
}

void PoiProvider::update(long /*dt*/) {
	constexpr unsigned long seconds = 60L * 1000L;
	for (;;) {
		_lock.lockWrite();
		auto i = _pois.begin();
		if (i == _pois.end()) {
			_lock.unlockWrite();
			break;
		}
		Poi poi = *i;
		if (poi.time + seconds > _timeProvider->tickTime()) {
			_lock.unlockWrite();
			break;
		}
		_pois.erase(i);
		_lock.unlockWrite();
	}
}

void PoiProvider::addPointOfInterest(const glm::vec3& pos) {
	_lock.lockWrite();
	_pois.push_back(Poi { pos, _timeProvider->tickTime() });

	struct PoiComparatorLess: public std::binary_function<Poi, Poi, bool> {
		inline bool operator()(const Poi& x, const Poi& y) const {
			return std::less<unsigned long>()(x.time, y.time);
		}
	};

	std::sort(_pois.begin(), _pois.end(), PoiComparatorLess());
	_lock.unlockWrite();
}

size_t PoiProvider::getPoisCount() const {
	return _pois.size();
}

glm::vec3 PoiProvider::getPointOfInterest() const {
	if (_pois.empty())
		return _world->randomPos();
	return _world->random().randomElement(_pois.begin(), _pois.end())->pos;
}

}
