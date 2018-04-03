/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "core/ReadWriteLock.h"
#include "math/Random.h"
#include "Shared_generated.h"
#include <glm/vec3.hpp>
#include <stack>
#include <memory>

namespace poi {

struct PoiResult {
	glm::vec3 pos;
	bool valid;;
};

using Type = network::PointOfInterestType;

/**
 * @brief Maintains a list of points of interest that are only valid for a particular time.
 *
 * @note One can add new POIs by calling @c PoiProvider::addPointOfInterest() and get a random,
 * not yet expired POI by calling @c PoiProvider::getPointOfInterest().
 */
class PoiProvider {
private:
	struct Poi {
		glm::vec3 pos;
		Type type;
		unsigned long time;
	};

	typedef std::deque<Poi> PoiQueue;
	PoiQueue _pois;

	core::TimeProviderPtr _timeProvider;
	core::ReadWriteLock _lock;
	math::Random _random;
public:
	PoiProvider(const core::TimeProviderPtr& timeProvider);

	void update(long dt);

	void add(const glm::vec3& pos, Type type = Type::GENERIC);
	size_t count() const;
	/**
	 * @param[in] type If @c Type::NONE is given here we are just looking for any type of POI
	 */
	PoiResult query(Type type = Type::NONE) const;
};

typedef std::shared_ptr<PoiProvider> PoiProviderPtr;

}
