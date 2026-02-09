/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"

namespace voxelformat {

/**
 * @brief OpenStreetMap JSON format loader (from Overpass API)
 *
 * This format loads OSM JSON data (from Overpass API with @c out @c geom) and converts it into
 * voxel scenes with different layers for buildings, roads, natural features, etc.
 * Each feature type (buildings, roads, water, land use, natural) is placed in its own
 * scene graph group node for organized editing.
 *
 * Features:
 * - Buildings with height estimation from OSM tags (height, building:levels)
 * - Road networks with width estimation from highway type
 * - Natural features (water, forests, parks)
 * - Land use areas
 *
 * The expected input is the JSON response from the Overpass API with @c out @c geom
 * which inlines node coordinates directly into ways. This avoids the need for a
 * separate node lookup step.
 *
 * Coordinate transformation uses a simple equirectangular projection centered on
 * the data bounding box. The scale is configurable (default 1 meter per voxel).
 *
 * @sa GMLFormat for CityGML/GML support
 * @li https://wiki.openstreetmap.org/wiki/Overpass_API
 * @li https://overpass-api.de/
 *
 * @ingroup Formats
 */
class OSMFormat : public MeshFormat {
public:
	/**
	 * @brief Feature category for grouping in the scene graph
	 */
	enum class FeatureType : uint8_t { Building, Highway, Natural, Water, LandUse, Unknown };

private:
	/**
	 * @brief Geometry point from the Overpass 'out geom' response
	 */
	struct GeomPoint {
		double lat = 0.0;
		double lon = 0.0;
	};

	/**
	 * @brief An OSM element (way or relation) parsed from the JSON
	 */
	struct OSMElement {
		int64_t id = 0;
		FeatureType featureType = FeatureType::Unknown;
		core::String name;
		core::DynamicArray<GeomPoint> geometry;
		// Selected tags
		float height = 0.0f;
		float minHeight = 0.0f;
		int levels = 0;
		core::String highwayType;
		core::String buildingType;
		core::String naturalType;
		core::String landuseType;
		core::String waterwayType;
	};

	/**
	 * @brief Coordinate system for lat/lon to local voxel space conversion
	 */
	struct CoordSystem {
		double centerLat = 0.0;
		double centerLon = 0.0;
		double metersPerVoxel = 1.0;
	};

	static color::RGBA featureTypeColor(FeatureType type);

	/**
	 * @brief Convert lat/lon to local XZ coordinates (Y is up)
	 */
	glm::vec3 latLonToLocal(double lat, double lon, double elevation, const CoordSystem &cs) const;

	/**
	 * @brief Estimate building height from tags
	 */
	static float estimateBuildingHeight(const OSMElement &elem);

	/**
	 * @brief Estimate road half-width from highway type
	 */
	static float estimateRoadHalfWidth(const core::String &highwayType);

	/**
	 * @brief Parse the Overpass JSON response into elements
	 */
	bool parseOverpassJson(const core::String &json, core::DynamicArray<OSMElement> &elements) const;

	/**
	 * @brief Create a building mesh from a closed polygon + height
	 */
	bool buildingToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const;

	/**
	 * @brief Create a road mesh from a polyline + width
	 */
	bool roadToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const;

	/**
	 * @brief Create a flat area mesh from a closed polygon (for parks, water, landuse)
	 */
	bool areaToMesh(const OSMElement &elem, const CoordSystem &cs, FeatureType type, Mesh &mesh) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
						scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) override;

	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &,
					const core::String &, const io::ArchivePtr &, const glm::vec3 &, bool, bool, bool) override {
		return false;
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"OpenStreetMap JSON", "", {"osm.json"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
