#pragma once

#include "voxel/polyvox/PagedVolume.h"
#include "voxel/polyvox/PagedVolumeWrapper.h"
#include "voxel/WorldPersister.h"

namespace voxel {

class BiomeManager;
struct WorldContext;

class WorldPager: public PagedVolume::Pager {
private:
	WorldPersister _worldPersister;
	bool _persist = true;
	long _seed = 0l;
	int _createFlags = 0;
	glm::vec2 _noiseSeedOffset;

	PagedVolume *_volumeData = nullptr;
	BiomeManager* _biomeManager = nullptr;
	WorldContext* _ctx = nullptr;

	// don't access the volume in anything that is called here
	void create(PagedVolumeWrapper& ctx);

public:
	bool init(PagedVolume *volumeData, BiomeManager* biomeManager, WorldContext* ctx);
	void shutdown();

	void setPersist(bool persist);
	void setSeed(long seed);
	void setCreateFlags(int flags);
	void setNoiseOffset(const glm::vec2& noiseOffset);
	void erase(PagedVolume::PagerContext& ctx);
	bool pageIn(PagedVolume::PagerContext& ctx) override;
	void pageOut(PagedVolume::PagerContext& ctx) override;
};

}
