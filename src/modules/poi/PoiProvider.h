/**
 * @file
 */

#pragma once

#include "backend/ForwardDecl.h"
#include "core/ReadWriteLock.h"
#include "math/Random.h"
#include "Type.h"
#include <glm/fwd.hpp>
#include <glm/vec3.hpp>
#include <stack>
#include <memory>

namespace poi {

/**
 * @brief PoiProvider::query() result.
 */
struct PoiResult {
	glm::vec3 pos;
	bool valid; /**< If no valid POI was found, this is @c false, @c true otherwise */
};

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
		uint64_t time;
	};

	typedef std::deque<Poi> PoiQueue;
	PoiQueue _pois;

	core::TimeProviderPtr _timeProvider;
	core::ReadWriteLock _lock;
	math::Random _random;
public:
	PoiProvider(const core::TimeProviderPtr& timeProvider);

	/**
	 * @brief This will deleted outdated POIs. But tries to keep at least one in the list.
	 */
	void update(long dt);

	/**
	 * @brief Adds a POI for the given position
	 */
	void add(const glm::vec3& pos, Type type = Type::GENERIC);
	/**
	 * @brief The overall amount of POIs
	 */
	size_t count() const;
	/**
	 * @brief Get a POI either randomly or by specifying a type.
	 * @param[in] type If @c Type::NONE is given here we are just looking for any type of POI
	 */
	PoiResult query(Type type = Type::NONE) const;
};

typedef std::shared_ptr<PoiProvider> PoiProviderPtr;

}
