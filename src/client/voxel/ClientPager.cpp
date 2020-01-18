/**
 * @file
 */

#include "ClientPager.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "http/ResponseParser.h"
#include "voxel/Region.h"

namespace client {

bool ClientPager::init(const std::string& baseUrl) {
	if (baseUrl.empty()) {
		return true;
	}
	if (!_httpClient.setBaseUrl(baseUrl)) {
		Log::warn("Invalid client pager url");
	} else {
		Log::info("Updated client pager url to '%s'", baseUrl.c_str());
	}

	return true;
}

void ClientPager::setSeed(unsigned int seed) {
	_seed = seed;
	Log::info("set seed: %u", _seed);
}

bool ClientPager::pageIn(voxel::PagedVolume::PagerContext& pctx) {
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	if (!_worldPersister.load(pctx.chunk.get(), _seed)) {
		const int x = pctx.region.getLowerX();
		const int y = pctx.region.getLowerY();
		const int z = pctx.region.getLowerZ();
		const int mapid = 1; // FIXME
		const http::ResponseParser& response = _httpClient.get("?x=%i&y=%i&z=%i&mapid=%i", x, y, z, mapid);
		if (response.status != http::HttpStatus::Ok) {
			Log::error("Failed to load the chunk for position %i:%i:%i and seed %u", x, y, z, _seed);
			return false;
		}
		const size_t length = response.contentLength;
		const char* data = response.content;
		const std::string& name = _worldPersister.getWorldName(pctx.region, _seed);
		io::filesystem()->write(name, (const uint8_t*)data, length);
	}
	if (!_worldPersister.load(pctx.chunk.get(), _seed)) {
		Log::error("Failed to load the world");
	}
	return false;
}

void ClientPager::pageOut(voxel::PagedVolume::Chunk* chunk) {
}

}
