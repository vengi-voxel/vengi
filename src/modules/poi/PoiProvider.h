/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "core/ReadWriteLock.h"
#include "core/Random.h"
#include <glm/vec3.hpp>
#include <vector>
#include <memory>

namespace poi {

/**
 * @brief Maintains a list of points of interest that are only valid for a particular time.
 *
 * @note One can add new POIs by calling @c PoiProvider::addPointOfInterest and get a random,
 * not yet expired POI by calling @c PoiProvider::getPointOfInterest(). If there are no POIs
 * left, you will get a random one from the @c voxel::World
 */
class PoiProvider {
private:
	struct Poi {
		glm::vec3 pos;
		unsigned long time;
	};

	typedef std::vector<Poi> PoiQueue;
	PoiQueue _pois;

	core::TimeProviderPtr _timeProvider;
	core::ReadWriteLock _lock;
	core::Random _random;
public:
	PoiProvider(const core::TimeProviderPtr& timeProvider);

	void update(long dt);

	void addPointOfInterest(const glm::vec3& pos);
	size_t getPointOfInterestCount() const;
	glm::vec3 getPointOfInterest() const;
};

typedef std::shared_ptr<PoiProvider> PoiProviderPtr;

}
