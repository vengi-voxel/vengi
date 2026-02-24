/**
 * @file
 */

#include "OSMFormat.h"
#include "color/ColorUtil.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
#include "core/Var.h"
#include "io/Archive.h"
#include "io/Stream.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxel/RawVolume.h"
#include "voxelformat/private/mesh/Mesh.h"
#include "voxelformat/private/mesh/Polygon.h"
#include "json/JSON.h"
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>

namespace voxelformat {

color::RGBA OSMFormat::featureTypeColor(FeatureType type) {
	switch (type) {
	case FeatureType::Building:
		core_assert_msg(false, "Use featureSubtypeColor for buildings to allow custom colors");
		return color::RGBA(180, 120, 100, 255);
	case FeatureType::Highway:
		return color::RGBA(100, 100, 100, 255);
	case FeatureType::Natural:
		return color::RGBA(80, 160, 80, 255);
	case FeatureType::Water:
		return color::RGBA(70, 130, 200, 255);
	case FeatureType::LandUse:
		return color::RGBA(160, 190, 120, 255);
	case FeatureType::Railway:
		return color::RGBA(110, 110, 110, 255);
	case FeatureType::Leisure:
		return color::RGBA(140, 200, 100, 255);
	case FeatureType::Amenity:
		return color::RGBA(180, 150, 180, 255);
	case FeatureType::Aeroway:
		return color::RGBA(160, 160, 170, 255);
	case FeatureType::Aerialway:
		return color::RGBA(80, 80, 80, 255);
	case FeatureType::Boundary:
		return color::RGBA(200, 100, 150, 255);
	case FeatureType::Unknown:
	default:
		return color::RGBA(200, 200, 200, 255);
	}
}

color::RGBA OSMFormat::featureSubtypeColor(const OSMElement &elem) {
	switch (elem.featureType) {
	case FeatureType::Natural:
		if (elem.isForest())
			return color::RGBA(34, 139, 34, 255);
		if (elem.naturalType == "scrub")
			return color::RGBA(107, 142, 35, 255);
		if (elem.naturalType == "water")
			return color::RGBA(70, 130, 200, 255);
		if (elem.naturalType == "glacier")
			return color::RGBA(200, 230, 255, 255);
		if (elem.naturalType == "wetland")
			return color::RGBA(70, 170, 150, 255);
		if (elem.naturalType == "heath")
			return color::RGBA(170, 160, 80, 255);
		if (elem.naturalType == "grassland")
			return color::RGBA(140, 200, 100, 255);
		if (elem.naturalType == "bare_rock")
			return color::RGBA(160, 160, 160, 255);
		if (elem.naturalType == "sand" || elem.naturalType == "beach")
			return color::RGBA(210, 190, 140, 255);
		if (elem.naturalType == "reef")
			return color::RGBA(100, 180, 220, 255);
		if (elem.isTree())
			return color::RGBA(34, 139, 34, 255);
		if (elem.naturalType == "peak")
			return color::RGBA(139, 90, 43, 255);
		return featureTypeColor(elem.featureType);
	case FeatureType::LandUse:
		if (elem.landuseType == "residential")
			return color::RGBA(220, 200, 170, 255);
		if (elem.landuseType == "commercial" || elem.landuseType == "retail")
			return color::RGBA(230, 180, 180, 255);
		if (elem.landuseType == "industrial")
			return color::RGBA(200, 190, 210, 255);
		if (elem.landuseType == "farmland" || elem.landuseType == "farmyard")
			return color::RGBA(230, 220, 155, 255);
		if (elem.landuseType == "forest")
			return color::RGBA(34, 139, 34, 255);
		if (elem.isGrass())
			return color::RGBA(140, 200, 100, 255);
		if (elem.landuseType == "cemetery")
			return color::RGBA(100, 130, 100, 255);
		if (elem.landuseType == "military")
			return color::RGBA(200, 150, 150, 255);
		if (elem.landuseType == "orchard" || elem.landuseType == "vineyard")
			return color::RGBA(170, 200, 100, 255);
		if (elem.landuseType == "allotments")
			return color::RGBA(190, 200, 140, 255);
		if (elem.landuseType == "brownfield" || elem.landuseType == "greenfield")
			return color::RGBA(180, 160, 120, 255);
		if (elem.landuseType == "recreation_ground")
			return color::RGBA(140, 200, 140, 255);
		if (elem.landuseType == "flowerbed")
			return color::RGBA(200, 160, 180, 255);
		if (elem.landuseType == "construction")
			return color::RGBA(170, 160, 130, 255);
		return featureTypeColor(elem.featureType);
	case FeatureType::Railway:
		if (elem.railwayType == "subway")
			return color::RGBA(50, 50, 180, 255);
		if (elem.railwayType == "tram" || elem.railwayType == "light_rail")
			return color::RGBA(130, 50, 50, 255);
		return featureTypeColor(elem.featureType);
	case FeatureType::Leisure:
		if (elem.leisureType == "park" || elem.leisureType == "garden")
			return color::RGBA(140, 200, 100, 255);
		if (elem.leisureType == "golf_course")
			return color::RGBA(120, 200, 120, 255);
		if (elem.leisureType == "pitch" || elem.leisureType == "sports_centre")
			return color::RGBA(100, 180, 120, 255);
		if (elem.leisureType == "nature_reserve")
			return color::RGBA(50, 140, 50, 255);
		if (elem.leisureType == "playground")
			return color::RGBA(180, 200, 120, 255);
		if (elem.leisureType == "swimming_pool")
			return color::RGBA(100, 160, 220, 255);
		return featureTypeColor(elem.featureType);
	case FeatureType::Amenity:
		if (elem.amenityType == "school" || elem.amenityType == "university" || elem.amenityType == "college")
			return color::RGBA(200, 180, 140, 255);
		if (elem.amenityType == "hospital")
			return color::RGBA(220, 160, 160, 255);
		if (elem.amenityType == "place_of_worship")
			return color::RGBA(180, 170, 200, 255);
		return featureTypeColor(elem.featureType);
	case FeatureType::Aeroway:
		if (elem.aerowayType == "runway")
			return color::RGBA(140, 140, 150, 255);
		if (elem.aerowayType == "taxiway")
			return color::RGBA(160, 160, 170, 255);
		if (elem.aerowayType == "apron")
			return color::RGBA(180, 180, 185, 255);
		return featureTypeColor(elem.featureType);
	default:
		return featureTypeColor(elem.featureType);
	}
}

glm::vec3 OSMFormat::latLonToLocal(double lat, double lon, double elevation, const CoordSystem &cs) const {
	// Equirectangular projection centered on the data centroid
	const double R = 6371000.0; // Earth radius in meters
	const double centerLatRad = cs.centerLat * glm::pi<double>() / 180.0;
	const double deltaLat = (lat - cs.centerLat) * glm::pi<double>() / 180.0;
	const double deltaLon = (lon - cs.centerLon) * glm::pi<double>() / 180.0;

	const double x = R * deltaLon * glm::cos(centerLatRad) / cs.metersPerVoxel;
	const double z = R * deltaLat / cs.metersPerVoxel;
	const double y = elevation / cs.metersPerVoxel;

	return glm::vec3((float)x, (float)y, (float)z);
}

float OSMFormat::estimateBuildingHeight(const OSMElement &elem) {
	// height = total height from ground to top of roof (including roof)
	if (elem.height > 0.0f) {
		return elem.height;
	}
	// Estimate from levels: building:levels + roof:levels, each floor ~3m
	if (elem.levels > 0) {
		const float facadeLevels = (float)elem.levels;
		const float roofLevels = (float)elem.roofLevels;
		return (facadeLevels + roofLevels) * 3.0f;
	}
	// Fallback by building type
	if (elem.buildingType == "house" || elem.buildingType == "residential" || elem.buildingType == "detached") {
		return 6.0f;
	}
	if (elem.buildingType == "commercial" || elem.buildingType == "retail" || elem.buildingType == "office") {
		return 12.0f;
	}
	if (elem.buildingType == "industrial" || elem.buildingType == "warehouse") {
		return 8.0f;
	}
	if (elem.buildingType == "church" || elem.buildingType == "cathedral") {
		return 20.0f;
	}
	if (elem.buildingType == "apartments") {
		return 15.0f;
	}
	return 9.0f; // Default: 3 stories
}

float OSMFormat::estimateMinHeight(const OSMElement &elem) {
	// min_height takes precedence
	if (elem.minHeight > 0.0f) {
		return elem.minHeight;
	}
	// building:min_level: convert to height, each level ~3m
	if (elem.minLevel > 0) {
		return (float)elem.minLevel * 3.0f;
	}
	return 0.0f;
}

float OSMFormat::estimateRoofHeight(const OSMElement &elem, float totalHeight, float shortExtent) {
	// roof:height is explicit
	if (elem.roofHeight >= 0.0f) {
		return elem.roofHeight;
	}
	// roof:angle: compute roof height from angle and half the short extent
	// tan(angle) = roofHeight / (shortExtent / 2)
	if (elem.roofAngle > 0.0f) {
		const float angleRad = elem.roofAngle * glm::pi<float>() / 180.0f;
		return glm::tan(angleRad) * (shortExtent * 0.5f);
	}
	// roof:levels: each roof level ~3m
	if (elem.roofLevels > 0) {
		return (float)elem.roofLevels * 3.0f;
	}
	// For non-flat roof shapes, default to a reasonable proportion
	const core::String &shape = elem.roofShape;
	if (shape.empty() || shape == "flat") {
		return 0.0f;
	}
	// Default: roof is ~1/3 of total height, but at least 2m
	const float defaultRoof = glm::max(totalHeight * 0.33f, 2.0f);
	return glm::min(defaultRoof, totalHeight * 0.5f);
}

float OSMFormat::estimateRoadHalfWidth(const core::String &highwayType) {
	if (highwayType == "motorway" || highwayType == "trunk") {
		return 6.0f;
	}
	if (highwayType == "primary" || highwayType == "secondary") {
		return 4.0f;
	}
	if (highwayType == "tertiary" || highwayType == "residential" || highwayType == "unclassified") {
		return 3.0f;
	}
	if (highwayType == "service" || highwayType == "living_street") {
		return 2.0f;
	}
	if (highwayType == "footway" || highwayType == "path" || highwayType == "cycleway" || highwayType == "steps") {
		return 1.0f;
	}
	if (highwayType == "pedestrian") {
		return 3.0f;
	}
	return 2.5f;
}

float OSMFormat::estimateLinearHalfWidth(const OSMElement &elem) {
	if (elem.featureType == FeatureType::Highway) {
		return estimateRoadHalfWidth(elem.highwayType);
	}
	if (elem.featureType == FeatureType::Railway) {
		if (elem.railwayType == "rail")
			return 2.0f;
		if (elem.railwayType == "subway")
			return 2.0f;
		if (elem.railwayType == "light_rail")
			return 1.5f;
		if (elem.railwayType == "tram")
			return 1.5f;
		if (elem.railwayType == "narrow_gauge")
			return 1.0f;
		return 1.5f;
	}
	if (elem.featureType == FeatureType::Aeroway) {
		if (elem.aerowayType == "runway")
			return 22.5f;
		if (elem.aerowayType == "taxiway")
			return 11.0f;
		return 5.0f;
	}
	if (elem.featureType == FeatureType::Aerialway) {
		return 0.5f;
	}
	if (elem.featureType == FeatureType::Natural) {
		if (elem.naturalType == "tree_row")
			return 2.0f;
		return 1.5f;
	}
	return 2.5f;
}

bool OSMFormat::pointNode(scenegraph::SceneGraph &sceneGraph, const OSMElement &elem, const CoordSystem &cs) const {
	if (elem.geometry.empty()) {
		return false;
	}

	const GeomPoint &gp = elem.geometry[0];
	const glm::vec3 pos = latLonToLocal(gp.lat, gp.lon, gp.elevation, cs);
	const color::RGBA color = featureSubtypeColor(elem);

	scenegraph::SceneGraphNode groundNode(scenegraph::SceneGraphNodeType::Point);
	groundNode.setName(elem.name);
	groundNode.setTranslation(pos);
	groundNode.setColor(color);
	return sceneGraph.emplace(core::move(groundNode), sceneGraph.root().id()) != InvalidNodeId;
}

bool OSMFormat::treeToVoxels(scenegraph::SceneGraph &sceneGraph, const OSMElement &elem, const CoordSystem &cs,
							 int parentGroupId) const {
	if (elem.geometry.empty()) {
		return false;
	}

	const GeomPoint &gp = elem.geometry[0];
	const glm::vec3 pos = latLonToLocal(gp.lat, gp.lon, gp.elevation, cs);
	const int wx = (int)glm::round(pos.x);
	const int wy = (int)glm::round(pos.y) + 1; // start above ground plane
	const int wz = (int)glm::round(pos.z);

	// Small voxel tree: 3 wide, 7 tall, 3 deep
	const voxel::Region region(wx - 1, wy, wz - 1, wx + 1, wy + 6, wz + 1);
	voxel::RawVolume *volume = new voxel::RawVolume(region);

	palette::Palette palette;
	const color::RGBA trunkColor(139, 90, 43, 255);
	const color::RGBA leafColor(34, 139, 34, 255);
	palette.setColor(0, trunkColor);
	palette.setColor(1, leafColor);
	const voxel::Voxel trunkVoxel = voxel::createVoxel(palette, 0);
	const voxel::Voxel leafVoxel = voxel::createVoxel(palette, 1);

	// Trunk: 3 voxels tall at center column
	for (int y = wy; y < wy + 3; ++y) {
		volume->setVoxel(wx, y, wz, trunkVoxel);
	}

	// Canopy layer 1 (full 3x3)
	for (int x = wx - 1; x <= wx + 1; ++x) {
		for (int z = wz - 1; z <= wz + 1; ++z) {
			volume->setVoxel(x, wy + 3, z, leafVoxel);
		}
	}
	// Canopy layer 2 (full 3x3)
	for (int x = wx - 1; x <= wx + 1; ++x) {
		for (int z = wz - 1; z <= wz + 1; ++z) {
			volume->setVoxel(x, wy + 4, z, leafVoxel);
		}
	}
	// Canopy layer 3 (cross shape - no corners)
	volume->setVoxel(wx, wy + 5, wz, leafVoxel);
	volume->setVoxel(wx - 1, wy + 5, wz, leafVoxel);
	volume->setVoxel(wx + 1, wy + 5, wz, leafVoxel);
	volume->setVoxel(wx, wy + 5, wz - 1, leafVoxel);
	volume->setVoxel(wx, wy + 5, wz + 1, leafVoxel);
	// Canopy top (single voxel)
	volume->setVoxel(wx, wy + 6, wz, leafVoxel);

	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setName(elem.name);
	node.setVolume(volume, true);
	node.setPalette(palette);
	node.setProperty("osm:id", core::string::toString(elem.id));
	return sceneGraph.emplace(core::move(node), parentGroupId) != InvalidNodeId;
}

bool OSMFormat::parseOverpassJson(const core::String &json, core::DynamicArray<OSMElement> &elements) const {
	nlohmann::json doc = nlohmann::json::parse(json.c_str(), nullptr, false, true);
	if (doc.is_discarded()) {
		Log::error("OSM: Failed to parse JSON");
		return false;
	}
	if (!doc.contains("elements")) {
		Log::error("OSM: No 'elements' array in JSON response");
		return false;
	}

	const auto &jsonElements = doc["elements"];
	if (!jsonElements.is_array()) {
		Log::error("OSM: 'elements' is not an array");
		return false;
	}

	for (const auto &je : jsonElements) {
		const std::string type = je.value("type", "");

		OSMElement elem;
		elem.id = je.value("id", (int64_t)0);

		if (type == "node") {
			// Point feature - lat/lon at top level
			GeomPoint pt;
			pt.lat = je.value("lat", 0.0);
			pt.lon = je.value("lon", 0.0);
			elem.geometry.push_back(pt);
		} else if (type == "way" || type == "relation") {
			// Extract inline geometry from 'out geom'
			if (!je.contains("geometry") || !je["geometry"].is_array()) {
				continue;
			}
			for (const auto &gp : je["geometry"]) {
				GeomPoint pt;
				pt.lat = gp.value("lat", 0.0);
				pt.lon = gp.value("lon", 0.0);
				elem.geometry.push_back(pt);
			}
			if (elem.geometry.size() < 2) {
				continue;
			}
		} else {
			continue;
		}

		// Parse tags
		if (je.contains("tags") && je["tags"].is_object()) {
			const auto &tags = je["tags"];

			for (auto it = tags.begin(); it != tags.end(); ++it) {
				const std::string &key = it.key();
				const std::string &value = it.value().get<std::string>();
				elem.properties.put(key.c_str(), value.c_str());
			}

			elem.name = json::toStr(tags, "name");

			// Height
			if (tags.contains("height")) {
				const core::String hStr = tags.value("height", "").c_str();
				elem.height = (float)core::string::toFloat(hStr.c_str());
			}
			if (tags.contains("min_height")) {
				const core::String mhStr = tags.value("min_height", "").c_str();
				elem.minHeight = (float)core::string::toFloat(mhStr.c_str());
			}
			if (tags.contains("building:levels")) {
				const core::String levelsStr = json::toStr(tags, "building:levels");
				elem.levels = core::string::toInt(levelsStr);
			}
			if (tags.contains("building:min_level")) {
				const core::String mlStr = json::toStr(tags, "building:min_level");
				elem.minLevel = core::string::toInt(mlStr);
			}
			if (tags.contains("roof:levels")) {
				const core::String rlStr = json::toStr(tags, "roof:levels");
				elem.roofLevels = core::string::toInt(rlStr);
			}
			if (tags.contains("roof:height")) {
				const core::String rhStr = tags.value("roof:height", "").c_str();
				elem.roofHeight = (float)core::string::toFloat(rhStr.c_str());
			}
			if (tags.contains("roof:angle")) {
				const core::String raStr = tags.value("roof:angle", "").c_str();
				elem.roofAngle = (float)core::string::toFloat(raStr.c_str());
			}
			if (tags.contains("roof:shape")) {
				elem.roofShape = json::toStr(tags, "roof:shape");
			}
			if (tags.contains("roof:orientation")) {
				elem.roofOrientation = json::toStr(tags, "roof:orientation");
			}

			// Determine feature type
			if (tags.contains("building")) {
				elem.featureType = FeatureType::Building;
				elem.buildingType = json::toStr(tags, "building");
			} else if (tags.contains("highway")) {
				elem.featureType = FeatureType::Highway;
				elem.highwayType = json::toStr(tags, "highway");
			} else if (tags.contains("railway")) {
				elem.featureType = FeatureType::Railway;
				elem.railwayType = json::toStr(tags, "railway");
			} else if (tags.contains("aeroway")) {
				elem.featureType = FeatureType::Aeroway;
				elem.aerowayType = json::toStr(tags, "aeroway");
			} else if (tags.contains("aerialway")) {
				elem.featureType = FeatureType::Aerialway;
				elem.aerialwayType = json::toStr(tags, "aerialway");
			} else if (tags.contains("waterway") ||
					   (tags.contains("natural") && json::toStr(tags, "natural") == "water") ||
					   tags.contains("water")) {
				elem.featureType = FeatureType::Water;
				elem.waterwayType = json::toStr(tags, "waterway");
			} else if (tags.contains("natural")) {
				elem.featureType = FeatureType::Natural;
				elem.naturalType = json::toStr(tags, "natural");
			} else if (tags.contains("landuse")) {
				elem.featureType = FeatureType::LandUse;
				elem.landuseType = json::toStr(tags, "landuse");
			} else if (tags.contains("leisure")) {
				elem.featureType = FeatureType::Leisure;
				elem.leisureType = json::toStr(tags, "leisure");
			} else if (tags.contains("amenity")) {
				elem.featureType = FeatureType::Amenity;
				elem.amenityType = json::toStr(tags, "amenity");
			} else if (tags.contains("boundary")) {
				elem.featureType = FeatureType::Boundary;
				elem.boundaryType = json::toStr(tags, "boundary");
			} else if (tags.contains("building:part")) {
				elem.featureType = FeatureType::Building;
				elem.buildingType = json::toStr(tags, "building:part");
			}

			// Skip underground structures
			if (tags.contains("location") && tags.value("location", "") == "underground") {
				continue;
			}
			if (tags.contains("layer")) {
				const core::String layerStr = json::toStr(tags, "layer");
				const int layer = core::string::toInt(layerStr);
				if (layer < 0) {
					continue;
				}
			}
		}

		if (elem.featureType == FeatureType::Unknown) {
			continue; // Skip elements we can't classify
		}

		if (elem.name.empty()) {
			elem.name = core::String::format("%s_%lld",
											 elem.featureType == FeatureType::Building	  ? "building"
											 : elem.featureType == FeatureType::Highway	  ? "road"
											 : elem.featureType == FeatureType::Natural	  ? "natural"
											 : elem.featureType == FeatureType::Water	  ? "water"
											 : elem.featureType == FeatureType::LandUse	  ? "landuse"
											 : elem.featureType == FeatureType::Railway	  ? "railway"
											 : elem.featureType == FeatureType::Leisure	  ? "leisure"
											 : elem.featureType == FeatureType::Amenity	  ? "amenity"
											 : elem.featureType == FeatureType::Aeroway	  ? "aeroway"
											 : elem.featureType == FeatureType::Aerialway ? "aerialway"
											 : elem.featureType == FeatureType::Boundary  ? "boundary"
																						  : "unknown",
											 (long long)elem.id);
		}

		elements.push_back(core::move(elem));
	}

	Log::info("OSM: Parsed %d features from JSON", (int)elements.size());
	return true;
}

glm::vec3 OSMFormat::roofPt(const RoofParams &rp, float u, float v, float y) {
	return glm::vec3(rp.ocX + rp.ridgeDir2.x * u + rp.perpDir2.x * v, y,
					 rp.ocZ + rp.ridgeDir2.y * u + rp.perpDir2.y * v);
}

glm::vec2 OSMFormat::toUV(const RoofParams &rp, float wx, float wz) {
	const float dx = wx - rp.ocX;
	const float dz = wz - rp.ocZ;
	return glm::vec2(dx * rp.ridgeDir2.x + dz * rp.ridgeDir2.y, dx * rp.perpDir2.x + dz * rp.perpDir2.y);
}

void OSMFormat::edgeToRidgeRoof(const RoofParams &rp, const core::DynamicArray<glm::vec3> &verts, size_t numVerts,
								color::RGBA roofColor, Mesh &mesh, float ridgeHalfU, float ridgeV) {
	mesh.reserveAdditionalTris(2 * numVerts);
	for (size_t i = 0; i < numVerts; ++i) {
		const size_t j = (i + 1) % numVerts;
		const glm::vec3 ev0(verts[i].x, verts[i].y + rp.archBaseY + rp.wallHeight, verts[i].z);
		const glm::vec3 ev1(verts[j].x, verts[j].y + rp.archBaseY + rp.wallHeight, verts[j].z);
		const glm::vec2 uv0 = toUV(rp, verts[i].x, verts[i].z);
		const glm::vec2 uv1 = toUV(rp, verts[j].x, verts[j].z);
		// Nearest point on the ridge line for each vertex
		const float cu0 = glm::clamp(uv0.x, -ridgeHalfU, ridgeHalfU);
		const glm::vec3 rp0 = roofPt(rp, cu0, ridgeV, rp.pY);
		const float cu1 = glm::clamp(uv1.x, -ridgeHalfU, ridgeHalfU);
		const glm::vec3 rp1 = roofPt(rp, cu1, ridgeV, rp.pY);
		if (glm::distance(rp0, rp1) < 0.001f) {
			// Both vertices project to the same ridge point -> triangle (hip/gable end)
			Polygon p;
			p.addVertex(ev0, glm::vec2(0.0f), roofColor);
			p.addVertex(ev1, glm::vec2(0.0f), roofColor);
			p.addVertex(rp0, glm::vec2(0.0f), roofColor);
			p.toTris(mesh);
		} else {
			// Quad from footprint edge to ridge segment (slope face)
			Polygon p;
			p.addVertex(ev0, glm::vec2(0.0f), roofColor);
			p.addVertex(ev1, glm::vec2(0.0f), roofColor);
			p.addVertex(rp1, glm::vec2(0.0f), roofColor);
			p.addVertex(rp0, glm::vec2(0.0f), roofColor);
			p.toTris(mesh);
		}
	}
}

template<typename HeightFn>
void OSMFormat::perVertexHeightRoof(const RoofParams &rp, const core::DynamicArray<glm::vec3> &verts, size_t numVerts,
									color::RGBA roofColor, color::RGBA wallColor, Mesh &mesh, HeightFn heightFn) {
	// Roof surface polygon
	Polygon roofPoly;
	for (size_t i = 0; i < numVerts; ++i) {
		const glm::vec2 uv = toUV(rp, verts[i].x, verts[i].z);
		const float h = heightFn(uv.x, uv.y);
		const float roofY = verts[i].y + rp.archBaseY + rp.wallHeight + h;
		roofPoly.addVertex(glm::vec3(verts[i].x, roofY, verts[i].z), glm::vec2(0.0f), roofColor);
	}
	roofPoly.toTris(mesh);
	// Gable/end walls: fill vertical gaps between wall top and roof surface
	for (size_t i = 0; i < numVerts; ++i) {
		const size_t j = (i + 1) % numVerts;
		const glm::vec2 uv0 = toUV(rp, verts[i].x, verts[i].z);
		const glm::vec2 uv1 = toUV(rp, verts[j].x, verts[j].z);
		const float h0 = heightFn(uv0.x, uv0.y);
		const float h1 = heightFn(uv1.x, uv1.y);
		if (h0 < 0.01f && h1 < 0.01f) {
			continue;
		}
		const float eave0 = verts[i].y + rp.archBaseY + rp.wallHeight;
		const float eave1 = verts[j].y + rp.archBaseY + rp.wallHeight;
		Polygon gable;
		gable.addVertex(glm::vec3(verts[i].x, eave0, verts[i].z), glm::vec2(0.0f), wallColor);
		gable.addVertex(glm::vec3(verts[i].x, eave0 + h0, verts[i].z), glm::vec2(0.0f), wallColor);
		gable.addVertex(glm::vec3(verts[j].x, eave1 + h1, verts[j].z), glm::vec2(0.0f), wallColor);
		gable.addVertex(glm::vec3(verts[j].x, eave1, verts[j].z), glm::vec2(0.0f), wallColor);
		gable.toTris(mesh);
	}
}

bool OSMFormat::buildingToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const {
	if (elem.geometry.size() < 3) {
		return false;
	}

	// Convert geometry to local coordinates
	core::DynamicArray<glm::vec3> verts;
	verts.reserve(elem.geometry.size());
	for (const GeomPoint &gp : elem.geometry) {
		verts.push_back(latLonToLocal(gp.lat, gp.lon, gp.elevation, cs));
	}

	// Skip closing vertex if it duplicates the first
	size_t numVerts = verts.size();
	if (numVerts > 3 && glm::distance(verts[0], verts[numVerts - 1]) < 0.001f) {
		--numVerts;
	}
	if (numVerts < 3) {
		return false;
	}

	// Compute footprint AABB
	float roofMinX = FLT_MAX, roofMaxX = -FLT_MAX;
	float roofMinZ = FLT_MAX, roofMaxZ = -FLT_MAX;
	float avgTerrainY = 0.0f;
	for (size_t i = 0; i < numVerts; ++i) {
		roofMinX = glm::min(roofMinX, verts[i].x);
		roofMaxX = glm::max(roofMaxX, verts[i].x);
		roofMinZ = glm::min(roofMinZ, verts[i].z);
		roofMaxZ = glm::max(roofMaxZ, verts[i].z);
		avgTerrainY += verts[i].y;
	}
	avgTerrainY /= (float)numVerts;
	const float xExtent = roofMaxX - roofMinX;
	const float zExtent = roofMaxZ - roofMinZ;

	// height calculation:
	// totalHeight = ground to top of roof
	// roofH = height of the roof portion
	// wallHeight = totalHeight - roofH (facade height)
	// minH = min_height or building:min_level * 3m (height below building structure)
	const float metersPerVoxel = (float)cs.metersPerVoxel;
	const float totalHeightMeters = estimateBuildingHeight(elem);
	const float minHMeters = estimateMinHeight(elem);
	const float shortExtentMeters = glm::min(xExtent, zExtent) * metersPerVoxel;
	float roofHMeters = estimateRoofHeight(elem, totalHeightMeters, shortExtentMeters);
	// Clamp roof height so walls are at least 1m
	if (roofHMeters > totalHeightMeters - minHMeters - 1.0f) {
		roofHMeters = glm::max(0.0f, totalHeightMeters - minHMeters - 1.0f);
	}
	const float wallHeight = (totalHeightMeters - roofHMeters - minHMeters) / metersPerVoxel;
	const float roofPeakHeight = roofHMeters / metersPerVoxel;
	const float archBaseY = minHMeters / metersPerVoxel;

	// Colors
	core::String buildingColorHex = "0xb4a08cff";
	elem.properties.get("building:colour", buildingColorHex);
	const color::RGBA wallColor = color::fromHex(buildingColorHex.c_str());

	core::String roofColorHex = "0xc87864ff";
	elem.properties.get("roof:colour", roofColorHex);
	const color::RGBA roofColor = color::fromHex(roofColorHex.c_str());

	// Walls: one quad per edge
	mesh.reserveAdditionalTris(2 * numVerts);
	for (size_t i = 0; i < numVerts; ++i) {
		const glm::vec3 &v0 = verts[i];
		const glm::vec3 &v1 = verts[(i + 1) % numVerts];

		const float base0 = v0.y + archBaseY;
		const float base1 = v1.y + archBaseY;

		const glm::vec3 bl(v0.x, base0, v0.z);
		const glm::vec3 tl(v0.x, base0 + wallHeight, v0.z);
		const glm::vec3 tr(v1.x, base1 + wallHeight, v1.z);
		const glm::vec3 br(v1.x, base1, v1.z);

		const size_t base = mesh.vertices.size();
		for (int k = 0; k < 4; ++k) {
			MeshVertex mv;
			mv.color = wallColor;
			mesh.vertices.push_back(mv);
		}
		mesh.vertices[base + 0].pos = bl;
		mesh.vertices[base + 1].pos = tl;
		mesh.vertices[base + 2].pos = tr;
		mesh.vertices[base + 3].pos = br;

		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 1));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 3));
	}

	// Eave height = base of roof (top of walls)
	const float avgEaveY = avgTerrainY + archBaseY + wallHeight;

	// Determine roof shape
	core::String roofShape = elem.roofShape;
	if (roofShape.empty()) {
		roofShape = "flat";
	}
	// Normalize common aliases
	if (roofShape == "pitched") {
		roofShape = "gabled";
	} else if (roofShape == "pyramid") {
		roofShape = "pyramidal";
	} else if (roofShape == "lean_to") {
		roofShape = "skillion";
	}

	// Oriented Bounding Rectangle (OBR) for roof shapes
	// Find the longest polygon edge to determine the building's major axis.
	// The ridge runs parallel to this axis (unless roof:orientation=across).
	glm::vec2 ridgeDir2(1.0f, 0.0f);
	float maxEdgeLen2 = 0.0f;
	for (size_t i = 0; i < numVerts; ++i) {
		const size_t j = (i + 1) % numVerts;
		const glm::vec2 edge(verts[j].x - verts[i].x, verts[j].z - verts[i].z);
		const float len2 = glm::dot(edge, edge);
		if (len2 > maxEdgeLen2) {
			maxEdgeLen2 = len2;
			ridgeDir2 = edge;
		}
	}
	const float edgeLen = glm::sqrt(maxEdgeLen2);
	if (edgeLen > 0.001f) {
		ridgeDir2 /= edgeLen;
	}
	// roof:orientation=across rotates ridge 90 degree to be perpendicular to the longest side
	if (elem.roofOrientation == "across") {
		ridgeDir2 = glm::vec2(-ridgeDir2.y, ridgeDir2.x);
	}
	// Perpendicular direction (90 degree CCW rotation in XZ plane)
	const glm::vec2 perpDir2(-ridgeDir2.y, ridgeDir2.x);

	// Compute centroid of the polygon footprint in XZ
	float cX = 0.0f, cZ = 0.0f;
	for (size_t i = 0; i < numVerts; ++i) {
		cX += verts[i].x;
		cZ += verts[i].z;
	}
	cX /= (float)numVerts;
	cZ /= (float)numVerts;

	// Project all vertices onto the oriented axes to find extents
	float minU = FLT_MAX, maxU = -FLT_MAX;
	float minV = FLT_MAX, maxV = -FLT_MAX;
	for (size_t i = 0; i < numVerts; ++i) {
		const float dx = verts[i].x - cX;
		const float dz = verts[i].z - cZ;
		const float u = dx * ridgeDir2.x + dz * ridgeDir2.y;
		const float v = dx * perpDir2.x + dz * perpDir2.y;
		minU = glm::min(minU, u);
		maxU = glm::max(maxU, u);
		minV = glm::min(minV, v);
		maxV = glm::max(maxV, v);
	}
	const float halfU = (maxU - minU) * 0.5f; // half-extent along ridge
	const float halfV = (maxV - minV) * 0.5f; // half-extent across ridge
	// OBR center (may differ from centroid if polygon is asymmetric)
	const float oMidU = (minU + maxU) * 0.5f;
	const float oMidV = (minV + maxV) * 0.5f;
	const float ocX = cX + ridgeDir2.x * oMidU + perpDir2.x * oMidV;
	const float ocZ = cZ + ridgeDir2.y * oMidU + perpDir2.y * oMidV;

	RoofParams rp;
	rp.ridgeDir2 = ridgeDir2;
	rp.perpDir2 = perpDir2;
	rp.ocX = ocX;
	rp.ocZ = ocZ;
	rp.eY = avgEaveY;
	rp.pY = avgEaveY + roofPeakHeight;
	rp.archBaseY = archBaseY;
	rp.wallHeight = wallHeight;
	rp.roofPeakHeight = roofPeakHeight;
	rp.halfU = halfU;
	rp.halfV = halfV;

	const float eY = rp.eY;

	// Generate roof geometry
	if (roofPeakHeight <= 0.0f || roofShape == "flat") {
		// Flat roof: polygon at wall top
		Polygon roofPoly;
		for (size_t i = 0; i < numVerts; ++i) {
			const float roofY = verts[i].y + archBaseY + wallHeight;
			roofPoly.addVertex(glm::vec3(verts[i].x, roofY, verts[i].z), glm::vec2(0.0f), roofColor);
		}
		roofPoly.toTris(mesh);
	} else if (roofShape == "pyramidal" || roofShape == "cone") {
		// All slopes to a single apex (ridgeHalfU = 0)
		edgeToRidgeRoof(rp, verts, numVerts, roofColor, mesh, 0.0f, 0.0f);
	} else if (roofShape == "gabled") {
		// Ridge spans full length (ridgeHalfU = halfU); gable ends are vertical
		edgeToRidgeRoof(rp, verts, numVerts, roofColor, mesh, halfU, 0.0f);
	} else if (roofShape == "hipped") {
		// Ridge shortened by halfV from each end; hip slopes at ends
		const float ridgeHalfU = glm::max(0.0f, halfU - halfV);
		edgeToRidgeRoof(rp, verts, numVerts, roofColor, mesh, ridgeHalfU, 0.0f);
	} else if (roofShape == "half-hipped") {
		// Like hipped but with a much longer ridge (smaller hips)
		const float hipFraction = 0.33f;
		const float ridgeHalfU = glm::max(0.0f, halfU - halfV * hipFraction);
		edgeToRidgeRoof(rp, verts, numVerts, roofColor, mesh, ridgeHalfU, 0.0f);
	} else if (roofShape == "saltbox") {
		// Asymmetric gable: ridge offset toward -V side
		const float ridgeV = -halfV * 0.33f;
		edgeToRidgeRoof(rp, verts, numVerts, roofColor, mesh, halfU, ridgeV);
	} else if (roofShape == "skillion") {
		// Mono-pitch: height varies linearly across V
		perVertexHeightRoof(rp, verts, numVerts, roofColor, wallColor, mesh, [&](float /*u*/, float v) -> float {
			return roofPeakHeight * (halfV - v) / (2.0f * halfV);
		});
	} else if (roofShape == "gambrel") {
		// Double-slope gable: steep lower + gentle upper, per-vertex height
		const float breakFrac = 0.6f;
		const float breakV = halfV * breakFrac;
		const float breakH = roofPeakHeight * 0.5f;
		perVertexHeightRoof(rp, verts, numVerts, roofColor, wallColor, mesh, [&](float /*u*/, float v) -> float {
			const float absV = glm::abs(v);
			if (absV >= halfV) {
				return 0.0f;
			}
			if (absV >= breakV) {
				return breakH * (halfV - absV) / (halfV - breakV);
			}
			return breakH + (roofPeakHeight - breakH) * (breakV - absV) / breakV;
		});
	} else if (roofShape == "mansard") {
		// Hipped with break + flat top, per-vertex height
		const float breakH = roofPeakHeight * 0.7f;
		const float breakT = 0.65f; // normalized distance at which break occurs
		perVertexHeightRoof(rp, verts, numVerts, roofColor, wallColor, mesh, [&](float u, float v) -> float {
			const float tu = (halfU > 0.001f) ? glm::abs(u) / halfU : 0.0f;
			const float tv = (halfV > 0.001f) ? glm::abs(v) / halfV : 0.0f;
			const float t = glm::max(tu, tv); // 0 at center, 1 at edge
			if (t >= 1.0f) {
				return 0.0f;
			}
			if (t >= breakT) {
				return breakH * (1.0f - t) / (1.0f - breakT);
			}
			return breakH + (roofPeakHeight - breakH) * (breakT - t) / breakT;
		});
	} else if (roofShape == "dome" || roofShape == "onion") {
		// Approximate dome/onion with latitude/longitude segments centered on OBR
		const int segments = 8;
		const int rings = 6;
		mesh.reserveAdditionalTris(2 * segments * rings);
		for (int ring = 0; ring < rings; ++ring) {
			const float phi0 = ((float)ring / (float)rings) * glm::half_pi<float>();
			const float phi1 = ((float)(ring + 1) / (float)rings) * glm::half_pi<float>();
			float rU0, rV0, rU1, rV1;
			if (roofShape == "onion") {
				const float bulge0 = 1.0f + 0.3f * glm::sin(phi0 * 2.0f);
				const float bulge1 = 1.0f + 0.3f * glm::sin(phi1 * 2.0f);
				rU0 = halfU * glm::cos(phi0) * bulge0;
				rV0 = halfV * glm::cos(phi0) * bulge0;
				rU1 = halfU * glm::cos(phi1) * bulge1;
				rV1 = halfV * glm::cos(phi1) * bulge1;
			} else {
				rU0 = halfU * glm::cos(phi0);
				rV0 = halfV * glm::cos(phi0);
				rU1 = halfU * glm::cos(phi1);
				rV1 = halfV * glm::cos(phi1);
			}
			const float y0 = eY + roofPeakHeight * glm::sin(phi0);
			const float y1 = eY + roofPeakHeight * glm::sin(phi1);
			for (int seg = 0; seg < segments; ++seg) {
				const float theta0 = ((float)seg / (float)segments) * glm::two_pi<float>();
				const float theta1 = ((float)(seg + 1) / (float)segments) * glm::two_pi<float>();
				const glm::vec3 v00 = roofPt(rp, rU0 * glm::cos(theta0), rV0 * glm::sin(theta0), y0);
				const glm::vec3 v10 = roofPt(rp, rU1 * glm::cos(theta0), rV1 * glm::sin(theta0), y1);
				const glm::vec3 v11 = roofPt(rp, rU1 * glm::cos(theta1), rV1 * glm::sin(theta1), y1);
				const glm::vec3 v01 = roofPt(rp, rU0 * glm::cos(theta1), rV0 * glm::sin(theta1), y0);
				if (ring == rings - 1) {
					Polygon p;
					p.addVertex(v00, glm::vec2(0.0f), roofColor);
					p.addVertex(v10, glm::vec2(0.0f), roofColor);
					p.addVertex(v01, glm::vec2(0.0f), roofColor);
					p.toTris(mesh);
				} else {
					Polygon p;
					p.addVertex(v00, glm::vec2(0.0f), roofColor);
					p.addVertex(v10, glm::vec2(0.0f), roofColor);
					p.addVertex(v11, glm::vec2(0.0f), roofColor);
					p.addVertex(v01, glm::vec2(0.0f), roofColor);
					p.toTris(mesh);
				}
			}
		}
	} else if (roofShape == "round") {
		// Barrel vault: semicircular cross-section perpendicular to ridge
		const int arcSegments = 8;
		mesh.reserveAdditionalTris(2 * arcSegments + 2 * arcSegments);
		for (int seg = 0; seg < arcSegments; ++seg) {
			const float angle0 = ((float)seg / (float)arcSegments) * glm::pi<float>();
			const float angle1 = ((float)(seg + 1) / (float)arcSegments) * glm::pi<float>();
			const float sv0 = -halfV * glm::cos(angle0);
			const float sy0 = eY + roofPeakHeight * glm::sin(angle0);
			const float sv1 = -halfV * glm::cos(angle1);
			const float sy1 = eY + roofPeakHeight * glm::sin(angle1);
			{
				Polygon p;
				p.addVertex(roofPt(rp, -halfU, sv0, sy0), glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, halfU, sv0, sy0), glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, halfU, sv1, sy1), glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, -halfU, sv1, sy1), glm::vec2(0.0f), roofColor);
				p.toTris(mesh);
			}
		}
		// End caps (semicircular fan)
		for (int seg = 0; seg < arcSegments; ++seg) {
			const float angle0 = ((float)seg / (float)arcSegments) * glm::pi<float>();
			const float angle1 = ((float)(seg + 1) / (float)arcSegments) * glm::pi<float>();
			{
				Polygon p;
				p.addVertex(roofPt(rp, -halfU, 0.0f, eY), glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, -halfU, -halfV * glm::cos(angle0), eY + roofPeakHeight * glm::sin(angle0)),
							glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, -halfU, -halfV * glm::cos(angle1), eY + roofPeakHeight * glm::sin(angle1)),
							glm::vec2(0.0f), roofColor);
				p.toTris(mesh);
			}
			{
				Polygon p;
				p.addVertex(roofPt(rp, halfU, 0.0f, eY), glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, halfU, -halfV * glm::cos(angle1), eY + roofPeakHeight * glm::sin(angle1)),
							glm::vec2(0.0f), roofColor);
				p.addVertex(roofPt(rp, halfU, -halfV * glm::cos(angle0), eY + roofPeakHeight * glm::sin(angle0)),
							glm::vec2(0.0f), roofColor);
				p.toTris(mesh);
			}
		}
	} else {
		// Unknown roof shape: fall back to flat
		Log::debug("OSM: Unknown roof:shape '%s', falling back to flat", roofShape.c_str());
		Polygon roofPoly;
		for (size_t i = 0; i < numVerts; ++i) {
			const float roofY = verts[i].y + archBaseY + wallHeight;
			roofPoly.addVertex(glm::vec3(verts[i].x, roofY, verts[i].z), glm::vec2(0.0f), roofColor);
		}
		roofPoly.toTris(mesh);
	}

	// Bottom face (floor)
	{
		Polygon floorPoly;
		for (size_t i = numVerts; i > 0; --i) {
			const float floorY = verts[i - 1].y + archBaseY;
			floorPoly.addVertex(glm::vec3(verts[i - 1].x, floorY, verts[i - 1].z), glm::vec2(0.0f), wallColor);
		}
		floorPoly.toTris(mesh);
	}

	return true;
}

bool OSMFormat::roadToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const {
	if (elem.geometry.size() < 2) {
		return false;
	}

	const float halfWidth = estimateLinearHalfWidth(elem) / (float)cs.metersPerVoxel;
	const color::RGBA roadColor = featureSubtypeColor(elem);

	// Linear features are raised above ground-level areas (landuse, leisure) to avoid z-fighting
	float roadYOffset = 1.0f;
	float roadHeight = 1.0f; // 1 voxel height

	// Tree rows and natural linear features are taller
	if (elem.featureType == FeatureType::Natural && elem.naturalType == "tree_row") {
		roadHeight = 8.0f / (float)cs.metersPerVoxel;
		if (roadHeight < 3.0f) {
			roadHeight = 3.0f;
		}
		roadYOffset = 0.0f; // Trees grow from ground level
	}

	// Convert to local coords
	core::DynamicArray<glm::vec3> pts;
	pts.reserve(elem.geometry.size());
	for (const GeomPoint &gp : elem.geometry) {
		pts.push_back(latLonToLocal(gp.lat, gp.lon, gp.elevation, cs));
	}

	// Generate a box ribbon mesh along the polyline (closed volume for proper voxelization)
	mesh.reserveAdditionalTris(8 * pts.size());
	for (size_t i = 0; i < pts.size() - 1; ++i) {
		const glm::vec3 &p0 = pts[i];
		const glm::vec3 &p1 = pts[i + 1];

		glm::vec3 dir = p1 - p0;
		const float len = glm::length(dir);
		if (len < 0.001f) {
			continue;
		}
		dir /= len;

		// Perpendicular in the XZ plane
		const glm::vec3 perp(-dir.z, 0.0f, dir.x);

		// Use terrain elevation from vertices - offset above ground features
		const float bottom0 = p0.y + roadYOffset;
		const float bottom1 = p1.y + roadYOffset;
		const float top0 = p0.y + roadYOffset + roadHeight;
		const float top1 = p1.y + roadYOffset + roadHeight;

		// 8 vertices for a box segment
		const glm::vec3 v0 = glm::vec3(p0.x, bottom0, p0.z) - perp * halfWidth; // bottom-left-start
		const glm::vec3 v1 = glm::vec3(p0.x, bottom0, p0.z) + perp * halfWidth; // bottom-right-start
		const glm::vec3 v2 = glm::vec3(p1.x, bottom1, p1.z) + perp * halfWidth; // bottom-right-end
		const glm::vec3 v3 = glm::vec3(p1.x, bottom1, p1.z) - perp * halfWidth; // bottom-left-end
		const glm::vec3 v4 = glm::vec3(p0.x, top0, p0.z) - perp * halfWidth;	// top-left-start
		const glm::vec3 v5 = glm::vec3(p0.x, top0, p0.z) + perp * halfWidth;	// top-right-start
		const glm::vec3 v6 = glm::vec3(p1.x, top1, p1.z) + perp * halfWidth;	// top-right-end
		const glm::vec3 v7 = glm::vec3(p1.x, top1, p1.z) - perp * halfWidth;	// top-left-end

		const size_t base = mesh.vertices.size();
		for (int k = 0; k < 8; ++k) {
			MeshVertex mv;
			mv.color = roadColor;
			mesh.vertices.push_back(mv);
		}
		mesh.vertices[base + 0].pos = v0;
		mesh.vertices[base + 1].pos = v1;
		mesh.vertices[base + 2].pos = v2;
		mesh.vertices[base + 3].pos = v3;
		mesh.vertices[base + 4].pos = v4;
		mesh.vertices[base + 5].pos = v5;
		mesh.vertices[base + 6].pos = v6;
		mesh.vertices[base + 7].pos = v7;

		// Top face
		mesh.indices.push_back((voxel::IndexType)(base + 4));
		mesh.indices.push_back((voxel::IndexType)(base + 5));
		mesh.indices.push_back((voxel::IndexType)(base + 6));
		mesh.indices.push_back((voxel::IndexType)(base + 4));
		mesh.indices.push_back((voxel::IndexType)(base + 6));
		mesh.indices.push_back((voxel::IndexType)(base + 7));

		// Bottom face
		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 3));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 1));

		// Left side
		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 4));
		mesh.indices.push_back((voxel::IndexType)(base + 7));
		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 7));
		mesh.indices.push_back((voxel::IndexType)(base + 3));

		// Right side
		mesh.indices.push_back((voxel::IndexType)(base + 1));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 6));
		mesh.indices.push_back((voxel::IndexType)(base + 1));
		mesh.indices.push_back((voxel::IndexType)(base + 6));
		mesh.indices.push_back((voxel::IndexType)(base + 5));
	}

	return !mesh.vertices.empty();
}

bool OSMFormat::areaToMesh(const OSMElement &elem, const CoordSystem &cs, FeatureType type, Mesh &mesh) const {
	if (elem.geometry.size() < 3) {
		return false;
	}

	core::DynamicArray<glm::vec3> verts;
	verts.reserve(elem.geometry.size());
	for (const GeomPoint &gp : elem.geometry) {
		verts.push_back(latLonToLocal(gp.lat, gp.lon, gp.elevation, cs));
	}

	size_t numVerts = verts.size();
	if (numVerts > 3 && glm::distance(verts[0], verts[numVerts - 1]) < 0.001f) {
		--numVerts;
	}
	if (numVerts < 3) {
		return false;
	}

	const color::RGBA color = featureSubtypeColor(elem);

	// Determine height for the area - minimum 1 voxel for proper visibility
	float areaHeight = 1.0f; // Default: 1 voxel height for ground features
	if (type == FeatureType::Natural) {
		if (elem.naturalType == "wood" || elem.naturalType == "forest") {
			areaHeight = 10.0f;
		} else if (elem.naturalType == "scrub") {
			areaHeight = 3.0f;
		} else if (elem.naturalType == "glacier") {
			areaHeight = 5.0f;
		} else if (elem.naturalType == "wetland") {
			areaHeight = 1.0f;
		}
	} else if (type == FeatureType::Water) {
		areaHeight = 1.0f;
	} else if (type == FeatureType::LandUse) {
		if (elem.landuseType == "forest") {
			areaHeight = 10.0f;
		} else if (elem.landuseType == "orchard" || elem.landuseType == "vineyard") {
			areaHeight = 4.0f;
		}
	} else if (type == FeatureType::Leisure) {
		if (elem.leisureType == "nature_reserve") {
			areaHeight = 2.0f;
		}
	}
	areaHeight /= (float)cs.metersPerVoxel;
	if (areaHeight < 1.0f) {
		areaHeight = 1.0f; // Minimum 1 voxel
	}

	// Top face: Use proper ear-clipping triangulation
	{
		Polygon topPoly;
		for (size_t i = 0; i < numVerts; ++i) {
			const glm::vec3 topVert(verts[i].x, verts[i].y + areaHeight, verts[i].z);
			topPoly.addVertex(topVert, glm::vec2(0.0f), color);
		}
		topPoly.toTris(mesh);
	}

	// Bottom face: Use proper ear-clipping triangulation (reversed winding)
	{
		Polygon bottomPoly;
		for (size_t i = numVerts; i > 0; --i) {
			const glm::vec3 bottomVert(verts[i - 1].x, verts[i - 1].y, verts[i - 1].z);
			bottomPoly.addVertex(bottomVert, glm::vec2(0.0f), color);
		}
		bottomPoly.toTris(mesh);
	}

	// Add walls to create a closed volume for proper voxelization
	mesh.reserveAdditionalTris(2 * numVerts);
	for (size_t i = 0; i < numVerts; ++i) {
		const glm::vec3 &v0 = verts[i];
		const glm::vec3 &v1 = verts[(i + 1) % numVerts];

		const size_t base = mesh.vertices.size();
		for (int k = 0; k < 4; ++k) {
			MeshVertex mv;
			mv.color = color;
			mesh.vertices.push_back(mv);
		}
		mesh.vertices[base + 0].pos = glm::vec3(v0.x, v0.y, v0.z);
		mesh.vertices[base + 1].pos = glm::vec3(v0.x, v0.y + areaHeight, v0.z);
		mesh.vertices[base + 2].pos = glm::vec3(v1.x, v1.y + areaHeight, v1.z);
		mesh.vertices[base + 3].pos = glm::vec3(v1.x, v1.y, v1.z);

		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 1));
		mesh.indices.push_back((voxel::IndexType)(base + 2));

		mesh.indices.push_back((voxel::IndexType)(base + 0));
		mesh.indices.push_back((voxel::IndexType)(base + 2));
		mesh.indices.push_back((voxel::IndexType)(base + 3));
	}

	return true;
}

bool OSMFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("OSM: Could not open file %s", filename.c_str());
		return false;
	}

	core::String jsonContent;
	if (!stream->readString((int)stream->size(), jsonContent, false)) {
		Log::error("OSM: Failed to read JSON file");
		return false;
	}

	core::DynamicArray<OSMElement> elements;
	if (!parseOverpassJson(jsonContent, elements)) {
		return false;
	}

	if (elements.empty()) {
		Log::warn("OSM: No valid features found in JSON data");
		return false;
	}

	// Compute bounding box of all geometry to establish coordinate system center
	double minLat = 90.0, maxLat = -90.0;
	double minLon = 180.0, maxLon = -180.0;
	for (const OSMElement &elem : elements) {
		for (const GeomPoint &gp : elem.geometry) {
			minLat = glm::min(minLat, gp.lat);
			maxLat = glm::max(maxLat, gp.lat);
			minLon = glm::min(minLon, gp.lon);
			maxLon = glm::max(maxLon, gp.lon);
		}
	}

	CoordSystem cs;
	cs.centerLat = (minLat + maxLat) * 0.5;
	cs.centerLon = (minLon + maxLon) * 0.5;
	cs.metersPerVoxel = core::Var::getVar(cfg::VoxformatOSMMetersPerVoxel)->floatVal();
	if (cs.metersPerVoxel <= 0.0) {
		cs.metersPerVoxel = 1.0;
	}

	Log::debug("OSM: Data center: lat=%.6f lon=%.6f", cs.centerLat, cs.centerLon);

	{
		// Compute local XZ bounds for ground plane
		glm::vec2 localMinXZ(FLT_MAX, FLT_MAX);
		glm::vec2 localMaxXZ(-FLT_MAX, -FLT_MAX);
		for (const OSMElement &elem : elements) {
			for (const GeomPoint &gp : elem.geometry) {
				const glm::vec3 localPos = latLonToLocal(gp.lat, gp.lon, gp.elevation, cs);
				localMinXZ.x = glm::min(localMinXZ.x, localPos.x);
				localMinXZ.y = glm::min(localMinXZ.y, localPos.z);
				localMaxXZ.x = glm::max(localMaxXZ.x, localPos.x);
				localMaxXZ.y = glm::max(localMaxXZ.y, localPos.z);
			}
		}
		const float padding = 2.0f;
		const int x0 = (int)glm::floor(localMinXZ.x - padding);
		const int x1 = (int)glm::ceil(localMaxXZ.x + padding);
		const int z0 = (int)glm::floor(localMinXZ.y - padding);
		const int z1 = (int)glm::ceil(localMaxXZ.y + padding);

		const voxel::Region groundRegion(x0, -1, z0, x1, 0, z1);
		voxel::RawVolume *groundVolume = new voxel::RawVolume(groundRegion);

		const color::RGBA groundColor(180, 170, 150, 255);
		palette::Palette groundPalette;
		groundPalette.setColor(0, groundColor);
		const voxel::Voxel groundVoxel = voxel::createVoxel(groundPalette, 0);

		// TODO: PERF: use a sampler
		for (int x = x0; x <= x1; ++x) {
			for (int z = z0; z <= z1; ++z) {
				groundVolume->setVoxel(x, -1, z, groundVoxel);
				groundVolume->setVoxel(x, 0, z, groundVoxel);
			}
		}

		scenegraph::SceneGraphNode groundNode(scenegraph::SceneGraphNodeType::Model);
		groundNode.setName("Ground");
		groundNode.setVolume(groundVolume, true);
		groundNode.setPalette(groundPalette);
		sceneGraph.emplace(core::move(groundNode), sceneGraph.root().id());
	}

	// Create group nodes for each feature type
	scenegraph::SceneGraphNode buildingsGroup(scenegraph::SceneGraphNodeType::Group);
	buildingsGroup.setName("Buildings");
	const int buildingsGroupId = sceneGraph.emplace(core::move(buildingsGroup));

	scenegraph::SceneGraphNode roadsGroup(scenegraph::SceneGraphNodeType::Group);
	roadsGroup.setName("Roads");
	const int roadsGroupId = sceneGraph.emplace(core::move(roadsGroup));

	scenegraph::SceneGraphNode naturalGroup(scenegraph::SceneGraphNodeType::Group);
	naturalGroup.setName("Natural");
	const int naturalGroupId = sceneGraph.emplace(core::move(naturalGroup));

	scenegraph::SceneGraphNode waterGroup(scenegraph::SceneGraphNodeType::Group);
	waterGroup.setName("Water");
	const int waterGroupId = sceneGraph.emplace(core::move(waterGroup));

	scenegraph::SceneGraphNode landuseGroup(scenegraph::SceneGraphNodeType::Group);
	landuseGroup.setName("LandUse");
	const int landuseGroupId = sceneGraph.emplace(core::move(landuseGroup));

	scenegraph::SceneGraphNode railwayGroup(scenegraph::SceneGraphNodeType::Group);
	railwayGroup.setName("Railway");
	const int railwayGroupId = sceneGraph.emplace(core::move(railwayGroup));

	scenegraph::SceneGraphNode leisureGroup(scenegraph::SceneGraphNodeType::Group);
	leisureGroup.setName("Leisure");
	const int leisureGroupId = sceneGraph.emplace(core::move(leisureGroup));

	scenegraph::SceneGraphNode amenityGroup(scenegraph::SceneGraphNodeType::Group);
	amenityGroup.setName("Amenity");
	const int amenityGroupId = sceneGraph.emplace(core::move(amenityGroup));

	scenegraph::SceneGraphNode aerowayGroup(scenegraph::SceneGraphNodeType::Group);
	aerowayGroup.setName("Aeroway");
	const int aerowayGroupId = sceneGraph.emplace(core::move(aerowayGroup));

	scenegraph::SceneGraphNode aerialwayGroup(scenegraph::SceneGraphNodeType::Group);
	aerialwayGroup.setName("Aerialway");
	const int aerialwayGroupId = sceneGraph.emplace(core::move(aerialwayGroup));

	scenegraph::SceneGraphNode boundaryGroup(scenegraph::SceneGraphNodeType::Group);
	boundaryGroup.setName("Boundary");
	const int boundaryGroupId = sceneGraph.emplace(core::move(boundaryGroup));

	int nodesCreated = 0;
	int elementIdx = 0;
	for (const OSMElement &elem : elements) {
		Mesh mesh;
		int parentGroupId = 0;
		bool ok = false;

		if (elem.geometry.size() == 1) {
			if (elem.featureType == FeatureType::Natural && elem.naturalType == "tree") {
				if (treeToVoxels(sceneGraph, elem, cs, naturalGroupId)) {
					++nodesCreated;
				}
				++elementIdx;
				continue;
			}
			ok = pointNode(sceneGraph, elem, cs);
		} else {
			switch (elem.featureType) {
			case FeatureType::Building:
				ok = buildingToMesh(elem, cs, mesh);
				break;
			case FeatureType::Highway:
			case FeatureType::Railway:
			case FeatureType::Aerialway:
				ok = roadToMesh(elem, cs, mesh);
				break;
			case FeatureType::Aeroway:
				if (elem.aerowayType == "runway" || elem.aerowayType == "taxiway") {
					ok = roadToMesh(elem, cs, mesh);
				} else {
					ok = areaToMesh(elem, cs, FeatureType::Aeroway, mesh);
				}
				break;
			case FeatureType::Natural:
				if (elem.naturalType == "tree_row") {
					ok = roadToMesh(elem, cs, mesh);
				} else {
					ok = areaToMesh(elem, cs, FeatureType::Natural, mesh);
				}
				break;
			case FeatureType::Water:
			case FeatureType::LandUse:
			case FeatureType::Leisure:
			case FeatureType::Amenity:
			case FeatureType::Boundary:
				ok = areaToMesh(elem, cs, elem.featureType, mesh);
				break;
			default:
				break;
			}
		}

		if (!ok) {
			++elementIdx;
			continue;
		}

		switch (elem.featureType) {
		case FeatureType::Building:
			parentGroupId = buildingsGroupId;
			break;
		case FeatureType::Highway:
			parentGroupId = roadsGroupId;
			break;
		case FeatureType::Railway:
			parentGroupId = railwayGroupId;
			break;
		case FeatureType::Natural:
			parentGroupId = naturalGroupId;
			break;
		case FeatureType::Water:
			parentGroupId = waterGroupId;
			break;
		case FeatureType::LandUse:
			parentGroupId = landuseGroupId;
			break;
		case FeatureType::Leisure:
			parentGroupId = leisureGroupId;
			break;
		case FeatureType::Amenity:
			parentGroupId = amenityGroupId;
			break;
		case FeatureType::Aeroway:
			parentGroupId = aerowayGroupId;
			break;
		case FeatureType::Aerialway:
			parentGroupId = aerialwayGroupId;
			break;
		case FeatureType::Boundary:
			parentGroupId = boundaryGroupId;
			break;
		default:
			++elementIdx;
			continue;
		}

		const int nodeId = voxelizeMesh(elem.name, sceneGraph, core::move(mesh), parentGroupId);
		if (nodeId != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			node.setProperty("osm:id", core::string::toString(elem.id));
			for (const auto &e : elem.properties) {
				node.setProperty(e->first.c_str(), e->second.c_str());
			}
			++nodesCreated;
		}

		++elementIdx;
		ctx.progress("voxelizing OSM features", elementIdx, (int)elements.size());

		if (stopExecution()) {
			break;
		}
	}

	// Remove empty groups
	const int groupIds[] = {buildingsGroupId, roadsGroupId,		naturalGroupId, waterGroupId,
							landuseGroupId,	  railwayGroupId,	leisureGroupId, amenityGroupId,
							aerowayGroupId,	  aerialwayGroupId, boundaryGroupId};
	for (int groupId : groupIds) {
		if (sceneGraph.node(groupId).children().empty()) {
			sceneGraph.removeNode(groupId, false);
		}
	}

	if (nodesCreated == 0) {
		Log::error("OSM: No voxel nodes created from %d features", (int)elements.size());
		return false;
	}

	Log::info("OSM: Created %d voxel nodes from %d features", nodesCreated, (int)elements.size());
	return true;
}

} // namespace voxelformat
