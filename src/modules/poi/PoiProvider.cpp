/**
 * @file
 */

#include "PoiProvider.h"
#include "core/TimeProvider.h"
#include "math/Random.h"
#include "core/Var.h"
#include "core/GLM.h"

namespace poi {

PoiProvider::PoiProvider(const core::TimeProviderPtr& timeProvider) :
		_timeProvider(timeProvider), _lock("PoiProvider") {
}

void PoiProvider::update(long /*dt*/) {
	constexpr unsigned long seconds = 60L * 1000L;
	core::ScopedWriteLock scoped(_lock);
	if (_pois.empty()) {
		return;
	}
	const uint64_t currentMillis = _timeProvider->tickMillis();
	for (;;) {
		auto i = _pois.end() - 1;
		// even if this is timed out - if we only have one, keep it.
		if (i == _pois.begin()) {
			break;
		}
		Poi poi = *i;
		if (poi.time + seconds > currentMillis) {
			break;
		}
		_pois.erase(i);
	}
}

void PoiProvider::add(const glm::vec3& pos, Type type) {
	core::ScopedWriteLock scoped(_lock);
	_pois.emplace_back(Poi{pos, type, _timeProvider->tickMillis()});
#if 0
	struct PoiComparatorLess: public std::binary_function<Poi, Poi, bool> {
		inline bool operator()(const Poi& x, const Poi& y) const {
			return std::less<unsigned long>()(x.time, y.time);
		}
	};

	std::sort(_pois.begin(), _pois.end(), PoiComparatorLess());
#endif
}

size_t PoiProvider::count() const {
	core::ScopedReadLock scoped(_lock);
	return _pois.size();
}

PoiResult PoiProvider::query(Type type) const {
	static constexpr PoiResult empty{glm::zero<glm::vec3>(), false};
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
