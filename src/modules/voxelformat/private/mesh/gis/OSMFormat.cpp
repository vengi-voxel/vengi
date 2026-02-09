/**
 * @file
 */

#include "OSMFormat.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/StringUtil.h"
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
		return color::RGBA(180, 160, 140, 255);
	case FeatureType::Highway:
		return color::RGBA(100, 100, 100, 255);
	case FeatureType::Natural:
		return color::RGBA(80, 160, 80, 255);
	case FeatureType::Water:
		return color::RGBA(70, 130, 200, 255);
	case FeatureType::LandUse:
		return color::RGBA(160, 190, 120, 255);
	case FeatureType::Unknown:
	default:
		return color::RGBA(200, 200, 200, 255);
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
	if (elem.height > 0.0f) {
		return elem.height;
	}
	if (elem.levels > 0) {
		return (float)elem.levels * 3.0f;
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
		if (type != "way" && type != "relation") {
			continue; // We only process ways and relations for geometry
		}

		// Extract inline geometry from 'out geom'
		if (!je.contains("geometry") || !je["geometry"].is_array()) {
			continue;
		}

		OSMElement elem;
		elem.id = je.value("id", (int64_t)0);

		// Parse geometry
		for (const auto &gp : je["geometry"]) {
			GeomPoint pt;
			pt.lat = gp.value("lat", 0.0);
			pt.lon = gp.value("lon", 0.0);
			elem.geometry.push_back(pt);
		}
		if (elem.geometry.size() < 2) {
			continue;
		}

		// Parse tags
		if (je.contains("tags") && je["tags"].is_object()) {
			const auto &tags = je["tags"];

			elem.name = json::toStr(tags, "name");

			// Height
			if (tags.contains("height")) {
				const core::String hStr = tags.value("height", "").c_str();
				// Strip "m" suffix if present
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

			// Determine feature type
			if (tags.contains("building")) {
				elem.featureType = FeatureType::Building;
				elem.buildingType = json::toStr(tags, "building");
			} else if (tags.contains("highway")) {
				elem.featureType = FeatureType::Highway;
				elem.highwayType = json::toStr(tags, "highway");
			} else if (tags.contains("natural")) {
				elem.featureType = FeatureType::Natural;
				elem.naturalType = json::toStr(tags, "natural");
			} else if (tags.contains("waterway") ||
					   (tags.contains("natural") && tags.value("natural", "") == "water")) {
				elem.featureType = FeatureType::Water;
				elem.waterwayType = json::toStr(tags, "waterway");
			} else if (tags.contains("landuse")) {
				elem.featureType = FeatureType::LandUse;
				elem.landuseType = json::toStr(tags, "landuse");
			} else if (tags.contains("water")) {
				elem.featureType = FeatureType::Water;
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
											 elem.featureType == FeatureType::Building	? "building"
											 : elem.featureType == FeatureType::Highway ? "road"
											 : elem.featureType == FeatureType::Natural ? "natural"
											 : elem.featureType == FeatureType::Water	? "water"
											 : elem.featureType == FeatureType::LandUse ? "landuse"
																						: "unknown",
											 (long long)elem.id);
		}

		elements.push_back(core::move(elem));
	}

	Log::info("OSM: Parsed %d features from JSON", (int)elements.size());
	return true;
}

bool OSMFormat::buildingToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const {
	if (elem.geometry.size() < 3) {
		return false;
	}

	// Convert geometry to local coordinates
	core::DynamicArray<glm::vec3> verts;
	verts.reserve(elem.geometry.size());
	for (const GeomPoint &gp : elem.geometry) {
		verts.push_back(latLonToLocal(gp.lat, gp.lon, 0.0, cs));
	}

	// Skip closing vertex if it duplicates the first
	size_t numVerts = verts.size();
	if (numVerts > 3 && glm::distance(verts[0], verts[numVerts - 1]) < 0.001f) {
		--numVerts;
	}
	if (numVerts < 3) {
		return false;
	}

	const float height = estimateBuildingHeight(elem) / (float)cs.metersPerVoxel;
	const float baseY = elem.minHeight / (float)cs.metersPerVoxel;
	const color::RGBA wallColor = featureTypeColor(FeatureType::Building);
	const color::RGBA roofColor(200, 120, 100, 255);
	// TODO: reserve the vertices and indices to avoid multiple reallocations

	// Walls: one quad per edge
	for (size_t i = 0; i < numVerts; ++i) {
		const glm::vec3 &v0 = verts[i];
		const glm::vec3 &v1 = verts[(i + 1) % numVerts];

		const glm::vec3 bl(v0.x, baseY, v0.z);
		const glm::vec3 tl(v0.x, baseY + height, v0.z);
		const glm::vec3 tr(v1.x, baseY + height, v1.z);
		const glm::vec3 br(v1.x, baseY, v1.z);

		// Two triangles per wall quad
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

	// Roof: Use proper ear-clipping triangulation via Polygon class
	{
		Polygon roofPoly;
		for (size_t i = 0; i < numVerts; ++i) {
			const glm::vec3 roofVert(verts[i].x, baseY + height, verts[i].z);
			roofPoly.addVertex(roofVert, glm::vec2(0.0f), roofColor);
		}
		roofPoly.toTris(mesh);
	}

	// Bottom face (floor): Use proper ear-clipping triangulation
	{
		Polygon floorPoly;
		// Add vertices in reverse order for correct winding
		for (size_t i = numVerts; i > 0; --i) {
			const glm::vec3 floorVert(verts[i - 1].x, baseY, verts[i - 1].z);
			floorPoly.addVertex(floorVert, glm::vec2(0.0f), wallColor);
		}
		floorPoly.toTris(mesh);
	}

	return true;
}

bool OSMFormat::roadToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const {
	if (elem.geometry.size() < 2) {
		return false;
	}

	const float halfWidth = estimateRoadHalfWidth(elem.highwayType) / (float)cs.metersPerVoxel;
	const color::RGBA roadColor = featureTypeColor(FeatureType::Highway);

	// Roads need thickness to be voxelized properly - make them 1 voxel tall
	const float roadBottom = 0.0f;
	const float roadTop = 1.0f; // 1 voxel height

	// Convert to local coords
	core::DynamicArray<glm::vec3> pts;
	pts.reserve(elem.geometry.size());
	for (const GeomPoint &gp : elem.geometry) {
		pts.push_back(latLonToLocal(gp.lat, gp.lon, 0.0, cs));
	}

	// Generate a box ribbon mesh along the polyline (closed volume for proper voxelization)
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

		// 8 vertices for a box segment
		const glm::vec3 v0 = glm::vec3(p0.x, roadBottom, p0.z) - perp * halfWidth; // bottom-left-start
		const glm::vec3 v1 = glm::vec3(p0.x, roadBottom, p0.z) + perp * halfWidth; // bottom-right-start
		const glm::vec3 v2 = glm::vec3(p1.x, roadBottom, p1.z) + perp * halfWidth; // bottom-right-end
		const glm::vec3 v3 = glm::vec3(p1.x, roadBottom, p1.z) - perp * halfWidth; // bottom-left-end
		const glm::vec3 v4 = glm::vec3(p0.x, roadTop, p0.z) - perp * halfWidth;	   // top-left-start
		const glm::vec3 v5 = glm::vec3(p0.x, roadTop, p0.z) + perp * halfWidth;	   // top-right-start
		const glm::vec3 v6 = glm::vec3(p1.x, roadTop, p1.z) + perp * halfWidth;	   // top-right-end
		const glm::vec3 v7 = glm::vec3(p1.x, roadTop, p1.z) - perp * halfWidth;	   // top-left-end

		// TODO: reserve the vertices and indices to avoid multiple reallocations
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
		verts.push_back(latLonToLocal(gp.lat, gp.lon, 0.0, cs));
	}

	size_t numVerts = verts.size();
	if (numVerts > 3 && glm::distance(verts[0], verts[numVerts - 1]) < 0.001f) {
		--numVerts;
	}
	if (numVerts < 3) {
		return false;
	}

	const color::RGBA color = featureTypeColor(type);

	// Determine height for the area - minimum 1 voxel for proper visibility
	float areaHeight = 1.0f; // Default: 1 voxel height for ground features
	if (type == FeatureType::Natural) {
		if (elem.naturalType == "wood" || elem.naturalType == "forest") {
			areaHeight = 10.0f;
		} else if (elem.naturalType == "scrub") {
			areaHeight = 3.0f;
		} else if (elem.naturalType == "water") {
			areaHeight = 1.0f;
		}
	} else if (type == FeatureType::Water) {
		areaHeight = 1.0f;
	}
	areaHeight /= (float)cs.metersPerVoxel;
	if (areaHeight < 1.0f) {
		areaHeight = 1.0f; // Minimum 1 voxel
	}

	const float baseY = 0.0f;

	// Top face: Use proper ear-clipping triangulation
	{
		Polygon topPoly;
		for (size_t i = 0; i < numVerts; ++i) {
			const glm::vec3 topVert(verts[i].x, baseY + areaHeight, verts[i].z);
			topPoly.addVertex(topVert, glm::vec2(0.0f), color);
		}
		topPoly.toTris(mesh);
	}

	// Bottom face: Use proper ear-clipping triangulation (reversed winding)
	{
		Polygon bottomPoly;
		for (size_t i = numVerts; i > 0; --i) {
			const glm::vec3 bottomVert(verts[i - 1].x, baseY, verts[i - 1].z);
			bottomPoly.addVertex(bottomVert, glm::vec2(0.0f), color);
		}
		bottomPoly.toTris(mesh);
	}

	// Add walls to create a closed volume for proper voxelization
	for (size_t i = 0; i < numVerts; ++i) {
		const glm::vec3 &v0 = verts[i];
		const glm::vec3 &v1 = verts[(i + 1) % numVerts];

		const size_t base = mesh.vertices.size();
		for (int k = 0; k < 4; ++k) {
			MeshVertex mv;
			mv.color = color;
			mesh.vertices.push_back(mv);
		}
		mesh.vertices[base + 0].pos = glm::vec3(v0.x, baseY, v0.z);
		mesh.vertices[base + 1].pos = glm::vec3(v0.x, baseY + areaHeight, v0.z);
		mesh.vertices[base + 2].pos = glm::vec3(v1.x, baseY + areaHeight, v1.z);
		mesh.vertices[base + 3].pos = glm::vec3(v1.x, baseY, v1.z);

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
	cs.metersPerVoxel = 1.0;

	Log::debug("OSM: Data center: lat=%.6f lon=%.6f", cs.centerLat, cs.centerLon);

	{
		// Compute local XZ bounds for ground plane
		glm::vec2 localMinXZ(FLT_MAX, FLT_MAX);
		glm::vec2 localMaxXZ(-FLT_MAX, -FLT_MAX);
		for (const OSMElement &elem : elements) {
			for (const GeomPoint &gp : elem.geometry) {
				const glm::vec3 localPos = latLonToLocal(gp.lat, gp.lon, 0.0, cs);
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

	int nodesCreated = 0;
	int elementIdx = 0;
	for (const OSMElement &elem : elements) {
		Mesh mesh;
		int parentGroupId = 0;

		switch (elem.featureType) {
		case FeatureType::Building:
			if (!buildingToMesh(elem, cs, mesh)) {
				continue;
			}
			parentGroupId = buildingsGroupId;
			break;
		case FeatureType::Highway:
			if (!roadToMesh(elem, cs, mesh)) {
				continue;
			}
			parentGroupId = roadsGroupId;
			break;
		case FeatureType::Natural:
			if (!areaToMesh(elem, cs, FeatureType::Natural, mesh)) {
				continue;
			}
			parentGroupId = naturalGroupId;
			break;
		case FeatureType::Water:
			if (!areaToMesh(elem, cs, FeatureType::Water, mesh)) {
				continue;
			}
			parentGroupId = waterGroupId;
			break;
		case FeatureType::LandUse:
			if (!areaToMesh(elem, cs, FeatureType::LandUse, mesh)) {
				continue;
			}
			parentGroupId = landuseGroupId;
			break;
		default:
			continue;
		}

		const int nodeId = voxelizeMesh(elem.name, sceneGraph, core::move(mesh), parentGroupId);
		if (nodeId != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			node.setProperty("osm:id", core::string::toString(elem.id));
			++nodesCreated;
		}

		++elementIdx;
		ctx.progress("voxelizing OSM features", elementIdx, (int)elements.size());

		if (stopExecution()) {
			break;
		}
	}

	// Remove empty groups
	if (sceneGraph.node(buildingsGroupId).children().empty()) {
		sceneGraph.removeNode(buildingsGroupId, false);
	}
	if (sceneGraph.node(roadsGroupId).children().empty()) {
		sceneGraph.removeNode(roadsGroupId, false);
	}
	if (sceneGraph.node(naturalGroupId).children().empty()) {
		sceneGraph.removeNode(naturalGroupId, false);
	}
	if (sceneGraph.node(waterGroupId).children().empty()) {
		sceneGraph.removeNode(waterGroupId, false);
	}
	if (sceneGraph.node(landuseGroupId).children().empty()) {
		sceneGraph.removeNode(landuseGroupId, false);
	}

	if (nodesCreated == 0) {
		Log::error("OSM: No voxel nodes created from %d features", (int)elements.size());
		return false;
	}

	Log::info("OSM: Created %d voxel nodes from %d features", nodesCreated, (int)elements.size());
	return true;
}

} // namespace voxelformat
