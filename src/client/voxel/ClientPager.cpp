/**
 * @file
 */

#include "ClientPager.h"
#include "core/App.h"
#include "core/io/Filesystem.h"
#include "http/ResponseParser.h"
#include "http/HttpMimeType.h"
#include "voxel/Region.h"

namespace client {

bool ClientPager::init(const core::String& baseUrl) {
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

void ClientPager::setMapId(int mapId) {
	_mapId = mapId;
	Log::info("set mapid: %u", _mapId);
}

bool ClientPager::pageIn(voxel::PagedVolume::PagerContext& pctx) {
	if (pctx.region.getLowerY() < 0) {
		return false;
	}
	if (!_chunkPersister.load(pctx.chunk.get(), _seed)) {
		const int x = pctx.region.getLowerX();
		const int y = pctx.region.getLowerY();
		const int z = pctx.region.getLowerZ();
		const http::ResponseParser& response = _httpClient.get("?x=%i&y=%i&z=%i&mapid=%i", x, y, z, _mapId);
		if (response.status != http::HttpStatus::Ok) {
			Log::error("Failed to download the chunk for position %i:%i:%i and seed %u on map %i", x, y, z, _seed, _mapId);
			if (response.isHeaderValue(http::header::CONTENT_TYPE, http::mimetype::TEXT_PLAIN)) {
				const core::String s(response.content, response.contentLength);
				Log::error("%s", s.c_str());
			}
			return false;
		}
		const size_t length = response.contentLength;
		const char* data = response.content;
		const char *contentType;
		if (!response.headers.get(http::header::CONTENT_TYPE, contentType)) {
			Log::error("No content type set in response");
			return false;
		}
		if (SDL_strcmp(contentType, "application/chunk")) {
			Log::error("Unexpected content type: %s", contentType);
			return false;
		}
		if (!_chunkPersister.loadCompressed(pctx.chunk.get(), (const uint8_t*)data, length)) {
			Log::error("Failed to uncompress the chunk for position %i:%i:%i and seed %u", x, y, z, _seed);
			return false;
		}
		if (!_chunkPersister.save(pctx.chunk.get(), _seed)) {
			Log::error("Failed to save the downloaded chunk");
			return false;
		}
	}
	if (!_chunkPersister.load(pctx.chunk.get(), _seed)) {
		Log::error("Failed to load the world");
	}
	return false;
}

void ClientPager::pageOut(voxel::PagedVolume::Chunk* chunk) {
}

}
