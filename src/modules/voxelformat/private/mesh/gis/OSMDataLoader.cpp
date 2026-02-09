/**
 * @file
 */

#include "OSMDataLoader.h"
#include "core/Log.h"
#include "core/Var.h"
#include "http/HttpCacheStream.h"

#include <cstdlib>
#include <glm/trigonometric.hpp>

namespace voxelformat {

core::String OSMDataLoader::buildOverpassQuery(const Options &options) {
	// Convert radius in km to approximate lat/lon offsets
	const double latOffset = options.radiusKm / 111.32;
	const double lonOffset = options.radiusKm / (111.32 * cos(glm::radians(options.lat)));

	const double south = options.lat - latOffset;
	const double north = options.lat + latOffset;
	const double west = options.lon - lonOffset;
	const double east = options.lon + lonOffset;

	core::String query = "[out:json][timeout:180];\n(\n";
	if (options.includeBuildings) {
		query += core::String::format("  way[\"building\"](%f,%f,%f,%f);\n"
									  "  relation[\"building\"](%f,%f,%f,%f);\n",
									  south, west, north, east, south, west, north, east);
	}
	if (options.includeRoads) {
		query += core::String::format("  way[\"highway\"](%f,%f,%f,%f);\n", south, west, north, east);
	}
	if (options.includeNatural) {
		query += core::String::format("  way[\"natural\"](%f,%f,%f,%f);\n"
									  "  relation[\"natural\"](%f,%f,%f,%f);\n",
									  south, west, north, east, south, west, north, east);
	}
	if (options.includeWater) {
		query += core::String::format("  way[\"water\"](%f,%f,%f,%f);\n"
									  "  way[\"waterway\"](%f,%f,%f,%f);\n"
									  "  relation[\"water\"](%f,%f,%f,%f);\n",
									  south, west, north, east, south, west, north, east, south, west, north, east);
	}
	if (options.includeLanduse) {
		query += core::String::format("  way[\"landuse\"](%f,%f,%f,%f);\n"
									  "  relation[\"landuse\"](%f,%f,%f,%f);\n",
									  south, west, north, east, south, west, north, east);
	}
	query += ");out geom;\n";
	return query;
}

core::String OSMDataLoader::buildCacheFilename(const Options &options) {
	const int latInt = (int)options.lat;
	const int latDec = (int)((options.lat - latInt) * 1000000);
	const int lonInt = (int)options.lon;
	const int lonDec = (int)((options.lon - lonInt) * 1000000);
	const int radiusInt = (int)options.radiusKm;
	const int radiusDec = (int)((options.radiusKm - radiusInt) * 1000);

	return core::String::format("osm_%d_%06d_%d_%06d_%d_%03d_%d%d%d%d%d.osm.json", latInt, std::abs(latDec), lonInt,
								std::abs(lonDec), radiusInt, std::abs(radiusDec), options.includeBuildings ? 1 : 0,
								options.includeRoads ? 1 : 0, options.includeNatural ? 1 : 0,
								options.includeWater ? 1 : 0, options.includeLanduse ? 1 : 0);
}

core::String OSMDataLoader::download(const io::ArchivePtr &archive, const Options &options,
									 core::String *cacheFilePath) {
	Log::info("Downloading OSM data for lat=%.6f, lon=%.6f, radius=%.2fkm", options.lat, options.lon, options.radiusKm);

	const core::String &query = buildOverpassQuery(options);
	Log::debug("OSM Overpass query:\n%s", query.c_str());

	const core::String &cacheFilename = buildCacheFilename(options);
	if (cacheFilePath != nullptr) {
		*cacheFilePath = cacheFilename;
	}

	const core::VarPtr &urlVar = core::Var::get(cfg::VoxformatOSMURL, "https://overpass-api.de/api/interpreter");
	const core::String &url = urlVar->strVal();

	const core::String &postBody = core::String::format("data=%s", query.c_str());
	core::String downloadedJson = http::HttpCacheStream::stringPost(archive, cacheFilename, url, postBody);

	if (downloadedJson.empty()) {
		Log::error("Failed to download OSM data");
		return "";
	}

	Log::info("OSM data: %d bytes", (int)downloadedJson.size());
	return downloadedJson;
}

} // namespace voxelformat
