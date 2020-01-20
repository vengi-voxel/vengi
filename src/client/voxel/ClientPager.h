/**
 * @file
 */

#pragma once

#include "voxelworld/FilePersister.h"
#include "voxel/PagedVolume.h"
#include "network/ClientMessageSender.h"
#include "http/HttpClient.h"

namespace client {

class ClientPager : public voxel::PagedVolume::Pager {
private:
	http::HttpClient _httpClient;
	unsigned int _seed = 0u;
	voxelworld::FilePersister _chunkPersister;
public:
	bool init(const std::string& baseUrl);

	bool pageIn(voxel::PagedVolume::PagerContext& ctx) override;
	void pageOut(voxel::PagedVolume::Chunk* chunk) override;
	void setSeed(unsigned int seed);
};

typedef std::shared_ptr<ClientPager> ClientPagerPtr;

}
