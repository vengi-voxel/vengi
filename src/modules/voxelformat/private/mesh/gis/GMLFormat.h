/**
 * @file
 */

#pragma once

#include "voxelformat/private/mesh/MeshFormat.h"

namespace tinyxml2 {
class XMLElement;
}

namespace voxelformat {

/**
 * @brief Geography Markup Language (GML) / CityGML format loader
 *
 * This format loader handles GML files which are typically zip archives containing
 * XML files that represent geographic features like buildings. The loader parses
 * CityGML XML files and extracts polygon geometry from building surfaces (walls,
 * roofs, ground surfaces) and converts them to triangles for voxelization.
 *
 * The format is commonly used for 3D city models and uses GML (Geography Markup Language)
 * for encoding geographic information according to the ISO 19100 standards.
 *
 * CityGML/GML uses Z-up
 *
 * CityGML uses real-world coordinates in meters.
 *
 * Supported elements:
 * - bldg:Building (WallSurface, RoofSurface, GroundSurface)
 * - luse:LandUse
 * - dem:ReliefFeature / TINRelief
 * - veg:PlantCover / SolitaryVegetationObject
 * - wtr:WaterBody
 * - tran:Road / Railway
 * - brid:Bridge
 * - gen:GenericCityObject
 * - gml:Polygon with LinearRing coordinates
 *
 * @li https://www.ogc.org/standards/citygml
 * @li https://www.ogc.org/standards/gml
 * @li https://www.citygmlwiki.org/index.php?title=KIT_Sample_files_Energy_ADE
 * @li https://filipbiljecki.com/code/Random3Dcity
 * @li https://github.com/tudelft3d/Random3Dcity
 *
 * @ingroup Formats
 */
class GMLFormat : public MeshFormat {
private:
	// Surface types for color assignment
	enum class SurfaceType : uint8_t { Unknown, Wall, Roof, Ground, LandUse, Vegetation, Water, Terrain, Road, Bridge };

	struct GMLPolygon {
		core::DynamicArray<glm::vec3> vertices;
		core::String id;
		SurfaceType surfaceType = SurfaceType::Unknown;
	};

	// Metadata extracted from GML file
	struct GMLMetadata {
		core::String name;
		core::String description;
	};

	// Represents a single city object (building, bridge, etc.) with its own polygons
	struct CityObject {
		core::String name;
		core::String type;
		core::DynamicArray<GMLPolygon> polygons;
	};

	static color::RGBA surfaceTypeColor(SurfaceType type);
	static SurfaceType surfaceTypeFromName(const char *name);

	// Parse a space-separated list of coordinates (3 doubles per vertex by default)
	bool parsePosList(const char *posData, core::DynamicArray<glm::vec3> &vertices, const glm::dvec3 &offset,
					  int srsDimension = 3) const;
	// Parse the deprecated gml:coordinates element (cs/ts/decimal separators)
	bool parseCoordinatesElement(const tinyxml2::XMLElement *coordsElement,
								core::DynamicArray<glm::vec3> &vertices, const glm::dvec3 &offset) const;
	bool parseLinearRing(const tinyxml2::XMLElement *linearRing, GMLPolygon &polygon, const glm::dvec3 &offset) const;
	// Parse a gml:Polygon (exterior + optional interior rings)
	bool parsePolygon(const tinyxml2::XMLElement *polygonElement, core::DynamicArray<GMLPolygon> &polygons,
					  const glm::dvec3 &offset, SurfaceType surfaceType) const;
	bool parseMultiSurface(const tinyxml2::XMLElement *multiSurface, core::DynamicArray<GMLPolygon> &polygons,
						   const glm::dvec3 &offset, SurfaceType surfaceType) const;
	bool parseMultiGeometry(const tinyxml2::XMLElement *multiGeometry, core::DynamicArray<GMLPolygon> &polygons,
						   const glm::dvec3 &offset, SurfaceType surfaceType) const;
	bool parseAnyGeometry(const tinyxml2::XMLElement *parent, core::DynamicArray<GMLPolygon> &polygons,
						  const glm::dvec3 &offset, SurfaceType surfaceType) const;
	bool parseCityGMLBoundedBy(const tinyxml2::XMLElement *boundedBy, core::DynamicArray<GMLPolygon> &polygons,
							  const glm::dvec3 &offset) const;
	bool parseBuilding(const tinyxml2::XMLElement *building, core::DynamicArray<GMLPolygon> &polygons,
					   const glm::dvec3 &offset) const;
	bool parseLandUse(const tinyxml2::XMLElement *landUse, core::DynamicArray<GMLPolygon> &polygons,
					  const glm::dvec3 &offset) const;
	bool parseReliefFeature(const tinyxml2::XMLElement *relief, core::DynamicArray<GMLPolygon> &polygons,
							const glm::dvec3 &offset) const;
	bool parseVegetation(const tinyxml2::XMLElement *vegetation, core::DynamicArray<GMLPolygon> &polygons,
						 const glm::dvec3 &offset) const;
	bool parseWaterBody(const tinyxml2::XMLElement *waterBody, core::DynamicArray<GMLPolygon> &polygons,
						const glm::dvec3 &offset) const;
	bool parseTransportation(const tinyxml2::XMLElement *transportation, core::DynamicArray<GMLPolygon> &polygons,
							 const glm::dvec3 &offset) const;
	bool parseBridge(const tinyxml2::XMLElement *bridge, core::DynamicArray<GMLPolygon> &polygons,
					 const glm::dvec3 &offset) const;
	bool parseGenericCityObject(const tinyxml2::XMLElement *obj, core::DynamicArray<GMLPolygon> &polygons,
								const glm::dvec3 &offset) const;
	bool parseEnvelope(const tinyxml2::XMLElement *envelope, glm::dvec3 &lowerCorner) const;
	static core::String getObjectName(const tinyxml2::XMLElement *element, const char *typeName);
	bool polygonsToMesh(const core::DynamicArray<GMLPolygon> &polygons, Mesh &mesh) const;
	bool parseCityModel(const tinyxml2::XMLElement *cityModel, core::DynamicArray<CityObject> &objects,
						GMLMetadata &metadata) const;
	bool parseXMLFile(io::SeekableReadStream &stream, core::DynamicArray<CityObject> &objects,
					 GMLMetadata &metadata) const;

	static int getSrsDimension(const tinyxml2::XMLElement *element);

	// Helper to find child element with namespace prefix
	static const tinyxml2::XMLElement *findChildElement(const tinyxml2::XMLElement *parent, const char *localName);
	static const tinyxml2::XMLElement *findNextSiblingElement(const tinyxml2::XMLElement *element,
															  const char *localName);

protected:
	bool voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive, scenegraph::SceneGraph &sceneGraph,
						const LoadContext &ctx) override;

	bool saveMeshes(const core::Map<int, int> &, const scenegraph::SceneGraph &, const ChunkMeshes &meshes,
					const core::String &filename, const io::ArchivePtr &archive, const glm::vec3 &scale, bool quad,
					bool withColor, bool withTexCoords) override {
		return false; // Save not supported
	}

public:
	static const io::FormatDescription &format() {
		static io::FormatDescription f{"Geography Markup Language", "", {"gml", "xml"}, {}, VOX_FORMAT_FLAG_MESH};
		return f;
	}
};

} // namespace voxelformat
