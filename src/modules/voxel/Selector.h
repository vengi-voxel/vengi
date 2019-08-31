/**
 * @file
 */

#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Face.h"
#include "core/GLM.h"
#include <functional>
#include <unordered_set>

namespace voxel {

/**
 * @brief Callback for walking the given volume.
 *
 * The sampler can be used to get the current position or the voxel, the face name indicates
 * the direction of the step.
 *
 * If this callback returns @c true, the step is continued into the given direction that
 * the face name indicates. Returning @c false indicates a stop.
 *
 * A coordinate in the given volume is not visited twice
 */
using SelectorCallback = std::function<bool(const voxel::RawVolume::Sampler&, voxel::FaceNames)>;

class Selector {
private:
	using Visited = std::unordered_set<glm::ivec3, std::hash<glm::ivec3> >;
	bool executeStep(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback, voxel::FaceNames face, Visited& set) const;
	void executeWalk(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback, Visited& set) const;
public:
	void walk(voxel::RawVolume::Sampler& sampler, SelectorCallback& callback) const;
};

}
