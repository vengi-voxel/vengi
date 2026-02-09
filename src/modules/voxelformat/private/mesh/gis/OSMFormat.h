/**
 * @file
 */

#pragma once

#include "core/StringUtil.h"
#include "core/collection/DynamicArray.h"
#include "core/collection/Map.h"
#include "scenegraph/SceneGraphNode.h"
#include "voxelformat/private/mesh/MeshFormat.h"

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
 * @li https://wiki.openstreetmap.org/wiki/JSON
 * @li https://wiki.openstreetmap.org/wiki/Overpass_API
 * @li https://overpass-api.de/
 * @li https://wiki.openstreetmap.org/wiki/Simple_3D_Buildings
 *
 * @ingroup Formats
 */
class OSMFormat : public MeshFormat {
public:
	/**
	 * @brief Feature category for grouping in the scene graph
	 */
	enum class FeatureType : uint8_t {
		Building,
		Highway,
		Natural,
		Water,
		LandUse,
		Railway,
		Leisure,
		Amenity,
		Aeroway,
		Aerialway,
		Boundary,
		Unknown
	};

private:
	/**
	 * @brief Geometry point from the Overpass 'out geom' response
	 */
	struct GeomPoint {
		double lat = 0.0;
		double lon = 0.0;
		double elevation = 0.0;
	};

	/**
	 * @brief An OSM element (node, way or relation) parsed from the JSON
	 */
	struct OSMElement {
		int64_t id = 0;
		FeatureType featureType = FeatureType::Unknown;
		core::String name;
		core::DynamicArray<GeomPoint> geometry;
		// Selected tags
		float height = 0.0f;		  ///< Total height from ground to top of roof (height=*)
		float minHeight = 0.0f;		  ///< Height below the building structure (min_height=*)
		int levels = 0;				  ///< Number of above-ground floors excluding roof (building:levels=*)
		int minLevel = 0;			  ///< Levels skipped below (building:min_level=*)
		int roofLevels = 0;			  ///< Number of floors within the roof (roof:levels=*)
		float roofHeight = -1.0f;	  ///< Height of the roof portion (roof:height=*), -1 = not set
		float roofAngle = 0.0f;		  ///< Roof inclination angle in degrees (roof:angle=*)
		core::String roofShape;		  ///< Roof shape type (roof:shape=*)
		core::String roofOrientation; ///< Ridge orientation: "along" or "across" (roof:orientation=*)
		core::String highwayType;
		core::String buildingType;
		core::String naturalType;
		core::String landuseType;
		core::String waterwayType;
		core::String railwayType;
		core::String leisureType;
		core::String amenityType;
		core::String aerowayType;
		core::String aerialwayType;
		core::String boundaryType;
		scenegraph::SceneGraphNodeProperties properties;

		bool isForest() const {
			return (featureType == FeatureType::Natural) && (naturalType == "wood" || naturalType == "forest");
		}

		bool isTree() const {
			return (featureType == FeatureType::Natural) && core::string::startsWith(naturalType, "tree");
		}

		bool isGrass() const {
			return (featureType == FeatureType::LandUse) &&
				   (landuseType == "grass" || landuseType == "meadow" || landuseType == "village_green");
		}
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
	static color::RGBA featureSubtypeColor(const OSMElement &elem);

	/**
	 * @brief Parameters for roof geometry generation, derived from the building footprint's
	 * Oriented Bounding Rectangle (OBR)
	 */
	struct RoofParams {
		glm::vec2 ridgeDir2;  ///< Direction along the ridge
		glm::vec2 perpDir2;	  ///< Direction perpendicular to the ridge
		float ocX;			  ///< OBR center X
		float ocZ;			  ///< OBR center Z
		float eY;			  ///< Eave Y (roof base)
		float pY;			  ///< Peak Y (roof top)
		float archBaseY;	  ///< Base Y offset (min_height)
		float wallHeight;	  ///< Wall height in voxels
		float roofPeakHeight; ///< Roof peak height in voxels
		float halfU;		  ///< Half-extent along ridge
		float halfV;		  ///< Half-extent across ridge
	};

	/**
	 * @brief Convert oriented (u, v) coordinates to world-space vec3
	 */
	static glm::vec3 roofPt(const RoofParams &rp, float u, float v, float y);

	/**
	 * @brief Convert world (x, z) to oriented (u, v) coordinates
	 */
	static glm::vec2 toUV(const RoofParams &rp, float wx, float wz);

	/**
	 * @brief Generate edge-to-ridge roof geometry (pyramidal, gabled, hipped, etc.)
	 * For ridge-based shapes (pyramidal, gabled, hipped, half-hipped, saltbox),
	 * connect each footprint edge to the nearest point(s) on the ridge line.
	 * This naturally follows the actual polygon shape and handles non-convex footprints.
	 *
	 * ridgeHalfU controls the ridge length:
	 *   0 = pyramidal (single apex)
	 *   halfU = gabled (full ridge, open gable ends)
	 *   halfU - halfV = hipped (shortened ridge, hip slopes at ends)
	 * ridgeV controls ridge offset across V (0 = centered, nonzero = saltbox)
	 */
	static void edgeToRidgeRoof(const RoofParams &rp, const core::DynamicArray<glm::vec3> &verts, size_t numVerts,
								color::RGBA roofColor, Mesh &mesh, float ridgeHalfU, float ridgeV);

	/**
	 * @brief Per-vertex height roof generation
	 * For shapes defined by a height function h(u,v), compute the roof Y at each
	 * footprint vertex and generate a single roof polygon.
	 * Also fills gable wall gaps between wall tops and the roof surface.
	 */
	template<typename HeightFn>
	static void perVertexHeightRoof(const RoofParams &rp, const core::DynamicArray<glm::vec3> &verts, size_t numVerts,
									color::RGBA roofColor, color::RGBA wallColor, Mesh &mesh, HeightFn heightFn);

	/**
	 * @brief Convert lat/lon to local XZ coordinates (Y is up)
	 */
	glm::vec3 latLonToLocal(double lat, double lon, double elevation, const CoordSystem &cs) const;

	/**
	 * @brief Estimate building height from tags
	 */
	static float estimateBuildingHeight(const OSMElement &elem);

	/**
	 * @brief Estimate the min_height (bottom of building structure above ground)
	 * from min_height tag or building:min_level
	 */
	static float estimateMinHeight(const OSMElement &elem);

	/**
	 * @brief Estimate the roof height portion from roof:height, roof:angle, or roof:levels
	 * @param totalHeight The total building height
	 * @param shortExtent The shorter footprint dimension (in meters) for angle calculation
	 */
	static float estimateRoofHeight(const OSMElement &elem, float totalHeight, float shortExtent);

	/**
	 * @brief Estimate road half-width from highway type
	 */
	static float estimateRoadHalfWidth(const core::String &highwayType);

	/**
	 * @brief Estimate half-width for any linear feature (roads, railways, etc.)
	 */
	static float estimateLinearHalfWidth(const OSMElement &elem);

	/**
	 * @brief Parse the Overpass JSON response into elements
	 */
	bool parseOverpassJson(const core::String &json, core::DynamicArray<OSMElement> &elements) const;

	/**
	 * @brief Create a building mesh from a closed polygon + height
	 */
	bool buildingToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const;

	/**
	 * @brief Create a linear feature mesh from a polyline + width (roads, railways, etc.)
	 */
	bool roadToMesh(const OSMElement &elem, const CoordSystem &cs, Mesh &mesh) const;

	/**
	 * @brief Create a flat area mesh from a closed polygon (for parks, water, landuse, etc.)
	 */
	bool areaToMesh(const OSMElement &elem, const CoordSystem &cs, FeatureType type, Mesh &mesh) const;

	bool pointNode(scenegraph::SceneGraph &sceneGraph, const OSMElement &elem, const CoordSystem &cs) const;

	/**
	 * @brief Create a small voxel tree (trunk + leaf canopy) for a point tree node
	 */
	bool treeToVoxels(scenegraph::SceneGraph &sceneGraph, const OSMElement &elem, const CoordSystem &cs,
					  int parentGroupId) const;

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

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
