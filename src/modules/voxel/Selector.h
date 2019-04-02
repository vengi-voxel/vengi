/**
 * @file
 */

#include "voxel/polyvox/RawVolume.h"
#include "voxel/polyvox/Face.h"
#include "core/GLM.h"
#include <functional>
#include <unordered_set>

namespace voxel {

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
