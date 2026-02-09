/**
 * @file
 */

#pragma once

#include "core/String.h"
#include "io/Archive.h"

namespace voxelformat {

class OSMDataLoader {
public:
	struct Options {
		double lat = 0.0;
		double lon = 0.0;
		double radiusKm = 0.5;
		bool includeBuildings = true;
		bool includeRoads = true;
		bool includeNatural = true;
		bool includeWater = true;
		bool includeLanduse = true;
	};

	/**
	 * @brief Build the Overpass API query string for the given options.
	 */
	static core::String buildOverpassQuery(const Options &options);

	/**
	 * @brief Generate a cache filename based on the options.
	 */
	static core::String buildCacheFilename(const Options &options);

	/**
	 * @brief Download OSM data from the Overpass API with caching.
	 * @param archive The archive to use for caching (typically filesystem archive)
	 * @param options The download options (coordinates, radius, feature flags)
	 * @param[out] cacheFilePath Output parameter for the cache file path (for use with infiles)
	 * @return The downloaded JSON data, or empty string on failure
	 */
	static core::String download(const io::ArchivePtr &archive, const Options &options,
								 core::String *cacheFilePath = nullptr);
};

} // namespace voxelformat
