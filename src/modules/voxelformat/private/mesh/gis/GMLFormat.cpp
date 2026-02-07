/**
 * @file
 */

#include "GMLFormat.h"
#include "app/Async.h"
#include "core/Common.h"
#include "core/ConfigVar.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "core/StringUtil.h"
#include "core/Trace.h"
#include "core/Var.h"
#include "core/collection/DynamicArray.h"
#include "io/Archive.h"
#include "io/ZipArchive.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeProperties.h"
#include "tinyxml2.h"
#include <SDL_stdinc.h>
#include <float.h>

namespace voxelformat {

color::RGBA GMLFormat::surfaceTypeColor(SurfaceType type) {
	switch (type) {
	case SurfaceType::Roof:
		return color::RGBA(180, 80, 60, 255); // Brownish-red for roofs
	case SurfaceType::Wall:
		return color::RGBA(200, 190, 170, 255); // Light beige for walls
	case SurfaceType::Ground:
		return color::RGBA(100, 130, 90, 255); // Greenish-gray for ground
	case SurfaceType::LandUse:
		return color::RGBA(180, 160, 100, 255); // Sandy/tan for land use
	case SurfaceType::Vegetation:
		return color::RGBA(60, 140, 60, 255); // Green for vegetation
	case SurfaceType::Water:
		return color::RGBA(70, 130, 180, 255); // Steel blue for water
	case SurfaceType::Terrain:
		return color::RGBA(139, 119, 101, 255); // Brown for terrain
	case SurfaceType::Road:
		return color::RGBA(90, 90, 90, 255); // Dark gray for roads
	case SurfaceType::Bridge:
		return color::RGBA(160, 160, 160, 255); // Light gray for bridges
	case SurfaceType::Unknown:
	default:
		return color::RGBA(180, 180, 180, 255); // Gray for unknown
	}
}

// Helper function to match element names with optional namespace prefix
static bool matchElementName(const char *fullName, const char *localName) {
	if (fullName == nullptr || localName == nullptr) {
		return false;
	}
	// Check for exact match
	if (SDL_strcmp(fullName, localName) == 0) {
		return true;
	}
	// Check for namespace:localName pattern
	const char *colon = SDL_strchr(fullName, ':');
	if (colon != nullptr) {
		return SDL_strcmp(colon + 1, localName) == 0;
	}
	return false;
}

const tinyxml2::XMLElement *GMLFormat::findChildElement(const tinyxml2::XMLElement *parent, const char *localName) {
	if (parent == nullptr) {
		return nullptr;
	}
	for (const tinyxml2::XMLElement *child = parent->FirstChildElement(); child != nullptr;
		 child = child->NextSiblingElement()) {
		if (matchElementName(child->Name(), localName)) {
			return child;
		}
	}
	return nullptr;
}

const tinyxml2::XMLElement *GMLFormat::findNextSiblingElement(const tinyxml2::XMLElement *element,
															  const char *localName) {
	if (element == nullptr) {
		return nullptr;
	}
	for (const tinyxml2::XMLElement *sibling = element->NextSiblingElement(); sibling != nullptr;
		 sibling = sibling->NextSiblingElement()) {
		if (matchElementName(sibling->Name(), localName)) {
			return sibling;
		}
	}
	return nullptr;
}

int GMLFormat::getSrsDimension(const tinyxml2::XMLElement *element) {
	if (element == nullptr) {
		return 3;
	}
	int dim = element->IntAttribute("srsDimension", 0);
	if (dim >= 2 && dim <= 3) {
		return dim;
	}
	// Walk up to find srsDimension on a parent element
	const tinyxml2::XMLElement *parent = element->Parent() ? element->Parent()->ToElement() : nullptr;
	while (parent != nullptr) {
		dim = parent->IntAttribute("srsDimension", 0);
		if (dim >= 2 && dim <= 3) {
			return dim;
		}
		parent = parent->Parent() ? parent->Parent()->ToElement() : nullptr;
	}
	return 3; // default to 3D
}

bool GMLFormat::parsePosList(const char *posData, core::DynamicArray<glm::vec3> &vertices, const glm::dvec3 &offset,
							 int srsDimension) const {
	if (posData == nullptr) {
		return false;
	}

	const char *ptr = posData;
	while (*ptr) {
		// Skip whitespace
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
			++ptr;
		}
		if (*ptr == '\0') {
			break;
		}

		char *endPtr = nullptr;
		double x = SDL_strtod(ptr, &endPtr);
		if (endPtr == ptr) {
			break;
		}
		ptr = endPtr;

		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
			++ptr;
		}
		double y = SDL_strtod(ptr, &endPtr);
		if (endPtr == ptr) {
			Log::error("Failed to parse y coordinate");
			return false;
		}
		ptr = endPtr;

		double z = 0.0;
		if (srsDimension >= 3) {
			while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r') {
				++ptr;
			}
			z = SDL_strtod(ptr, &endPtr);
			if (endPtr == ptr) {
				Log::error("Failed to parse z coordinate");
				return false;
			}
			ptr = endPtr;
		}

		// GML uses geographic coordinates, apply offset to normalize
		// Swap Y and Z for proper orientation (GML: X=east, Y=north, Z=up -> our: X, Z, Y)
		const glm::vec3 vertex((float)(x - offset.x), (float)(z - offset.z), (float)(y - offset.y));
		vertices.push_back(vertex);
	}
	return true;
}

bool GMLFormat::parseCoordinatesElement(const tinyxml2::XMLElement *coordsElement,
										core::DynamicArray<glm::vec3> &vertices, const glm::dvec3 &offset) const {
	if (coordsElement == nullptr) {
		return false;
	}
	const char *text = coordsElement->GetText();
	if (text == nullptr) {
		return false;
	}

	// Per GML XSD CoordinatesType: cs (coordinate separator) defaults to ",",
	// ts (tuple separator) defaults to " ", decimal defaults to "."
	const char *csAttr = coordsElement->Attribute("cs");
	const char *tsAttr = coordsElement->Attribute("ts");
	const char cs = (csAttr && csAttr[0]) ? csAttr[0] : ',';
	const char ts = (tsAttr && tsAttr[0]) ? tsAttr[0] : ' ';

	const char *ptr = text;
	while (*ptr) {
		// Skip whitespace/tuple separators
		while (*ptr == ' ' || *ptr == '\t' || *ptr == '\n' || *ptr == '\r' || *ptr == ts) {
			++ptr;
		}
		if (*ptr == '\0') {
			break;
		}

		char *endPtr = nullptr;
		double x = SDL_strtod(ptr, &endPtr);
		if (endPtr == ptr) {
			break;
		}
		ptr = endPtr;
		if (*ptr == cs) {
			++ptr;
		}

		double y = SDL_strtod(ptr, &endPtr);
		if (endPtr == ptr) {
			return false;
		}
		ptr = endPtr;

		double z = 0.0;
		if (*ptr == cs) {
			++ptr;
			z = SDL_strtod(ptr, &endPtr);
			if (endPtr == ptr) {
				return false;
			}
			ptr = endPtr;
		}

		const glm::vec3 vertex((float)(x - offset.x), (float)(z - offset.z), (float)(y - offset.y));
		vertices.push_back(vertex);
	}
	return true;
}

bool GMLFormat::parseLinearRing(const tinyxml2::XMLElement *linearRing, GMLPolygon &polygon,
								const glm::dvec3 &offset) const {
	if (linearRing == nullptr) {
		return false;
	}

	// Per GML XSD, LinearRing content is a choice of:
	// 1. A sequence of pos or pointProperty elements (minOccurs=4)
	// 2. A single posList element
	// 3. A single coordinates element (deprecated)
	// 4. A sequence of coord elements (deprecated GML2)

	// Try posList first (most common in modern GML)
	const tinyxml2::XMLElement *posList = findChildElement(linearRing, "posList");
	if (posList != nullptr) {
		const char *text = posList->GetText();
		if (text != nullptr) {
			const int dim = getSrsDimension(posList);
			parsePosList(text, polygon.vertices, offset, dim);
		}
	}

	// Try individual pos elements
	for (const tinyxml2::XMLElement *pos = findChildElement(linearRing, "pos"); pos != nullptr;
		 pos = findNextSiblingElement(pos, "pos")) {
		const char *text = pos->GetText();
		if (text != nullptr) {
			const int dim = getSrsDimension(pos);
			parsePosList(text, polygon.vertices, offset, dim);
		}
	}

	// Try deprecated coordinates element
	const tinyxml2::XMLElement *coords = findChildElement(linearRing, "coordinates");
	if (coords != nullptr) {
		parseCoordinatesElement(coords, polygon.vertices, offset);
	}

	return !polygon.vertices.empty();
}

bool GMLFormat::parsePolygon(const tinyxml2::XMLElement *polygonElement, core::DynamicArray<GMLPolygon> &polygons,
							 const glm::dvec3 &offset, SurfaceType surfaceType) const {
	core_trace_scoped(ParsePolygon);
	if (polygonElement == nullptr) {
		return false;
	}

	// Get polygon ID if available (per XSD: gml:id attribute from AbstractGMLType)
	const char *id = polygonElement->Attribute("gml:id");
	if (id == nullptr) {
		id = polygonElement->Attribute("id");
	}

	// Per GML XSD PolygonType: exterior (0..1) + interior (0..*)
	const tinyxml2::XMLElement *exterior = findChildElement(polygonElement, "exterior");
	if (exterior == nullptr) {
		// Also check deprecated outerBoundaryIs (GML2 backward compat)
		exterior = findChildElement(polygonElement, "outerBoundaryIs");
	}
	if (exterior == nullptr) {
		return false;
	}

	const tinyxml2::XMLElement *linearRing = findChildElement(exterior, "LinearRing");
	if (linearRing == nullptr) {
		return false;
	}

	GMLPolygon polygon;
	polygon.surfaceType = surfaceType;
	if (id != nullptr) {
		polygon.id = id;
	}
	if (!parseLinearRing(linearRing, polygon, offset)) {
		return false;
	}
	polygons.emplace_back(core::move(polygon));

	// Parse interior rings (holes) - each becomes a separate polygon for triangulation
	for (const tinyxml2::XMLElement *interior = findChildElement(polygonElement, "interior"); interior != nullptr;
		 interior = findNextSiblingElement(interior, "interior")) {
		const tinyxml2::XMLElement *innerRing = findChildElement(interior, "LinearRing");
		if (innerRing != nullptr) {
			GMLPolygon holePolygon;
			holePolygon.surfaceType = surfaceType;
			if (parseLinearRing(innerRing, holePolygon, offset)) {
				polygons.emplace_back(core::move(holePolygon));
			}
		}
	}

	// Also check deprecated innerBoundaryIs (GML2 backward compat)
	for (const tinyxml2::XMLElement *innerBoundary = findChildElement(polygonElement, "innerBoundaryIs");
		 innerBoundary != nullptr; innerBoundary = findNextSiblingElement(innerBoundary, "innerBoundaryIs")) {
		const tinyxml2::XMLElement *innerRing = findChildElement(innerBoundary, "LinearRing");
		if (innerRing != nullptr) {
			GMLPolygon holePolygon;
			holePolygon.surfaceType = surfaceType;
			if (parseLinearRing(innerRing, holePolygon, offset)) {
				polygons.emplace_back(core::move(holePolygon));
			}
		}
	}

	return true;
}

bool GMLFormat::parseMultiSurface(const tinyxml2::XMLElement *multiSurface, core::DynamicArray<GMLPolygon> &polygons,
								  const glm::dvec3 &offset, SurfaceType surfaceType) const {
	core_trace_scoped(ParseMultiSurface);
	if (multiSurface == nullptr) {
		return false;
	}

	// Iterate through surfaceMember elements (per XSD: MultiSurfaceType)
	for (const tinyxml2::XMLElement *surfaceMember = findChildElement(multiSurface, "surfaceMember");
		 surfaceMember != nullptr; surfaceMember = findNextSiblingElement(surfaceMember, "surfaceMember")) {

		const tinyxml2::XMLElement *polygonElement = findChildElement(surfaceMember, "Polygon");
		if (polygonElement != nullptr) {
			parsePolygon(polygonElement, polygons, offset, surfaceType);
		}

		// Also check for CompositeSurface inside surfaceMember
		const tinyxml2::XMLElement *compSurface = findChildElement(surfaceMember, "CompositeSurface");
		if (compSurface != nullptr) {
			for (const tinyxml2::XMLElement *innerMember = findChildElement(compSurface, "surfaceMember");
				 innerMember != nullptr; innerMember = findNextSiblingElement(innerMember, "surfaceMember")) {
				const tinyxml2::XMLElement *innerPoly = findChildElement(innerMember, "Polygon");
				if (innerPoly != nullptr) {
					parsePolygon(innerPoly, polygons, offset, surfaceType);
				}
			}
		}
	}

	// Per XSD: surfaceMembers is the array property (SurfaceArrayPropertyType)
	const tinyxml2::XMLElement *surfaceMembers = findChildElement(multiSurface, "surfaceMembers");
	if (surfaceMembers != nullptr) {
		for (const tinyxml2::XMLElement *polygonElement = findChildElement(surfaceMembers, "Polygon");
			 polygonElement != nullptr; polygonElement = findNextSiblingElement(polygonElement, "Polygon")) {
			parsePolygon(polygonElement, polygons, offset, surfaceType);
		}
	}

	return !polygons.empty();
}

GMLFormat::SurfaceType GMLFormat::surfaceTypeFromName(const char *name) {
	if (SDL_strcmp(name, "WallSurface") == 0) {
		return SurfaceType::Wall;
	} else if (SDL_strcmp(name, "RoofSurface") == 0) {
		return SurfaceType::Roof;
	} else if (SDL_strcmp(name, "GroundSurface") == 0) {
		return SurfaceType::Ground;
	}
	return SurfaceType::Unknown;
}

bool GMLFormat::parseCityGMLBoundedBy(const tinyxml2::XMLElement *boundedBy, core::DynamicArray<GMLPolygon> &polygons,
									  const glm::dvec3 &offset) const {
	if (boundedBy == nullptr) {
		return false;
	}

	// Look for different surface types: WallSurface, RoofSurface, GroundSurface
	const char *surfaceTypes[] = {"WallSurface", "RoofSurface", "GroundSurface"};

	for (const char *surfaceTypeName : surfaceTypes) {
		const tinyxml2::XMLElement *surface = findChildElement(boundedBy, surfaceTypeName);
		if (surface == nullptr) {
			continue;
		}

		const SurfaceType surfaceType = surfaceTypeFromName(surfaceTypeName);

		// Try lod2MultiSurface first
		const tinyxml2::XMLElement *lodMultiSurface = findChildElement(surface, "lod2MultiSurface");
		if (lodMultiSurface == nullptr) {
			// Try lod1MultiSurface
			lodMultiSurface = findChildElement(surface, "lod1MultiSurface");
		}

		if (lodMultiSurface != nullptr) {
			const tinyxml2::XMLElement *gmlMultiSurface = findChildElement(lodMultiSurface, "MultiSurface");
			if (gmlMultiSurface != nullptr) {
				parseMultiSurface(gmlMultiSurface, polygons, offset, surfaceType);
			}
		}
	}

	return !polygons.empty();
}

bool GMLFormat::parseBuilding(const tinyxml2::XMLElement *building, core::DynamicArray<GMLPolygon> &polygons,
							  const glm::dvec3 &offset) const {
	if (building == nullptr) {
		return false;
	}

	const char *buildingId = building->Attribute("gml:id");
	if (buildingId == nullptr) {
		buildingId = building->Attribute("id");
	}
	if (buildingId != nullptr) {
		Log::debug("Parsing building: %s", buildingId);
	}

	// Parse all CityGML boundedBy elements within this building
	for (const tinyxml2::XMLElement *boundedBy = findChildElement(building, "boundedBy"); boundedBy != nullptr;
		 boundedBy = findNextSiblingElement(boundedBy, "boundedBy")) {
		parseCityGMLBoundedBy(boundedBy, polygons, offset);
	}

	// Parse any direct geometry on the building (e.g. lod1Solid, lod2MultiSurface etc.)
	parseAnyGeometry(building, polygons, offset, SurfaceType::Unknown);

	// Also look for building parts
	for (const tinyxml2::XMLElement *buildingPart = findChildElement(building, "consistsOfBuildingPart");
		 buildingPart != nullptr; buildingPart = findNextSiblingElement(buildingPart, "consistsOfBuildingPart")) {

		const tinyxml2::XMLElement *innerPart = findChildElement(buildingPart, "BuildingPart");
		if (innerPart != nullptr) {
			parseBuilding(innerPart, polygons, offset);
		}
	}

	return !polygons.empty();
}

// Parse a gml:MultiGeometry element (per XSD: MultiGeometryType has geometryMember/geometryMembers)
bool GMLFormat::parseMultiGeometry(const tinyxml2::XMLElement *multiGeometry, core::DynamicArray<GMLPolygon> &polygons,
								   const glm::dvec3 &offset, SurfaceType surfaceType) const {
	core_trace_scoped(ParseMultiGeometry);
	if (multiGeometry == nullptr) {
		return false;
	}

	const size_t initialCount = polygons.size();

	// Per XSD: geometryMember elements (GeometryPropertyType)
	for (const tinyxml2::XMLElement *member = findChildElement(multiGeometry, "geometryMember"); member != nullptr;
		 member = findNextSiblingElement(member, "geometryMember")) {

		const tinyxml2::XMLElement *polygon = findChildElement(member, "Polygon");
		if (polygon != nullptr) {
			parsePolygon(polygon, polygons, offset, surfaceType);
			continue;
		}

		const tinyxml2::XMLElement *multiSurface = findChildElement(member, "MultiSurface");
		if (multiSurface != nullptr) {
			parseMultiSurface(multiSurface, polygons, offset, surfaceType);
			continue;
		}

		const tinyxml2::XMLElement *compSurface = findChildElement(member, "CompositeSurface");
		if (compSurface != nullptr) {
			for (const tinyxml2::XMLElement *surfaceMember = findChildElement(compSurface, "surfaceMember");
				 surfaceMember != nullptr; surfaceMember = findNextSiblingElement(surfaceMember, "surfaceMember")) {
				const tinyxml2::XMLElement *poly = findChildElement(surfaceMember, "Polygon");
				if (poly != nullptr) {
					parsePolygon(poly, polygons, offset, surfaceType);
				}
			}
			continue;
		}

		// Nested MultiGeometry
		const tinyxml2::XMLElement *nested = findChildElement(member, "MultiGeometry");
		if (nested != nullptr) {
			parseMultiGeometry(nested, polygons, offset, surfaceType);
			continue;
		}
	}

	// Per XSD: geometryMembers array property (GeometryArrayPropertyType)
	const tinyxml2::XMLElement *members = findChildElement(multiGeometry, "geometryMembers");
	if (members != nullptr) {
		for (const tinyxml2::XMLElement *child = members->FirstChildElement(); child != nullptr;
			 child = child->NextSiblingElement()) {
			if (matchElementName(child->Name(), "Polygon")) {
				parsePolygon(child, polygons, offset, surfaceType);
			} else if (matchElementName(child->Name(), "MultiSurface")) {
				parseMultiSurface(child, polygons, offset, surfaceType);
			}
		}
	}

	return polygons.size() > initialCount;
}

// Helper to find and parse any geometry element (MultiSurface, MultiGeometry, CompositeSurface, Solid, etc.)
bool GMLFormat::parseAnyGeometry(const tinyxml2::XMLElement *parent, core::DynamicArray<GMLPolygon> &polygons,
								 const glm::dvec3 &offset, SurfaceType surfaceType) const {
	core_trace_scoped(ParseAnyGeometry);
	if (parent == nullptr) {
		return false;
	}

	const size_t initialCount = polygons.size();

	// Try lod0 through lod4 in order of preference (highest detail first)
	const char *lodPrefixes[] = {"lod4", "lod3", "lod2", "lod1", "lod0"};
	const char *geometryTypes[] = {"MultiSurface", "Solid", "Geometry", "MultiCurve", "Surface"};

	for (const char *prefix : lodPrefixes) {
		for (const char *geomType : geometryTypes) {
			core::String lodName = core::String(prefix) + geomType;
			const tinyxml2::XMLElement *lodElement = findChildElement(parent, lodName.c_str());
			if (lodElement == nullptr) {
				continue;
			}

			Log::debug("parseAnyGeometry: found lod element '%s'", lodName.c_str());

			// MultiSurface inside LOD element
			const tinyxml2::XMLElement *multiSurface = findChildElement(lodElement, "MultiSurface");
			if (multiSurface != nullptr) {
				parseMultiSurface(multiSurface, polygons, offset, surfaceType);
			}

			// MultiGeometry inside LOD element (per XSD MultiGeometryType)
			const tinyxml2::XMLElement *multiGeometry = findChildElement(lodElement, "MultiGeometry");
			if (multiGeometry != nullptr) {
				parseMultiGeometry(multiGeometry, polygons, offset, surfaceType);
			}

			// Solid: per XSD, exterior contains Shell/CompositeSurface with surfaceMembers
			const tinyxml2::XMLElement *solid = findChildElement(lodElement, "Solid");
			if (solid != nullptr) {
				const tinyxml2::XMLElement *exterior = findChildElement(solid, "exterior");
				if (exterior != nullptr) {
					const tinyxml2::XMLElement *shell = findChildElement(exterior, "Shell");
					if (shell == nullptr) {
						shell = findChildElement(exterior, "CompositeSurface");
					}
					if (shell != nullptr) {
						for (const tinyxml2::XMLElement *surfaceMember = findChildElement(shell, "surfaceMember");
							 surfaceMember != nullptr;
							 surfaceMember = findNextSiblingElement(surfaceMember, "surfaceMember")) {
							const tinyxml2::XMLElement *polygonEl = findChildElement(surfaceMember, "Polygon");
							if (polygonEl != nullptr) {
								parsePolygon(polygonEl, polygons, offset, surfaceType);
							}
						}
					}
				}
			}

			// Direct CompositeSurface
			const tinyxml2::XMLElement *compSurface = findChildElement(lodElement, "CompositeSurface");
			if (compSurface != nullptr) {
				for (const tinyxml2::XMLElement *surfaceMember = findChildElement(compSurface, "surfaceMember");
					 surfaceMember != nullptr; surfaceMember = findNextSiblingElement(surfaceMember, "surfaceMember")) {
					const tinyxml2::XMLElement *polygonEl = findChildElement(surfaceMember, "Polygon");
					if (polygonEl != nullptr) {
						parsePolygon(polygonEl, polygons, offset, surfaceType);
					}
				}
			}

			// Direct Polygon inside the lod element
			const tinyxml2::XMLElement *directPolygon = findChildElement(lodElement, "Polygon");
			if (directPolygon != nullptr) {
				parsePolygon(directPolygon, polygons, offset, surfaceType);
			}
		}
	}

	return polygons.size() > initialCount;
}

bool GMLFormat::parseLandUse(const tinyxml2::XMLElement *landUse, core::DynamicArray<GMLPolygon> &polygons,
							 const glm::dvec3 &offset) const {
	if (landUse == nullptr) {
		return false;
	}

	const char *id = landUse->Attribute("gml:id");
	if (id == nullptr) {
		id = landUse->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing land use: %s", id);
	}

	parseAnyGeometry(landUse, polygons, offset, SurfaceType::LandUse);

	return !polygons.empty();
}

bool GMLFormat::parseReliefFeature(const tinyxml2::XMLElement *relief, core::DynamicArray<GMLPolygon> &polygons,
								   const glm::dvec3 &offset) const {
	if (relief == nullptr) {
		return false;
	}

	const char *id = relief->Attribute("gml:id");
	if (id == nullptr) {
		id = relief->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing relief feature: %s", id);
	}

	// Look for reliefComponent containing TINRelief
	for (const tinyxml2::XMLElement *component = findChildElement(relief, "reliefComponent"); component != nullptr;
		 component = findNextSiblingElement(component, "reliefComponent")) {

		const tinyxml2::XMLElement *tinRelief = findChildElement(component, "TINRelief");
		if (tinRelief != nullptr) {
			// TINRelief contains tin element with TriangulatedSurface
			const tinyxml2::XMLElement *tin = findChildElement(tinRelief, "tin");
			if (tin != nullptr) {
				const tinyxml2::XMLElement *triSurface = findChildElement(tin, "TriangulatedSurface");
				if (triSurface != nullptr) {
					// TriangulatedSurface contains trianglePatches with Triangle elements
					const tinyxml2::XMLElement *patches = findChildElement(triSurface, "trianglePatches");
					if (patches == nullptr) {
						patches = findChildElement(triSurface, "patches");
					}
					if (patches != nullptr) {
						for (const tinyxml2::XMLElement *triangle = findChildElement(patches, "Triangle");
							 triangle != nullptr; triangle = findNextSiblingElement(triangle, "Triangle")) {
							// Each Triangle has an exterior with LinearRing
							GMLPolygon polygon;
							polygon.surfaceType = SurfaceType::Terrain;
							const tinyxml2::XMLElement *exterior = findChildElement(triangle, "exterior");
							if (exterior != nullptr) {
								const tinyxml2::XMLElement *ring = findChildElement(exterior, "LinearRing");
								if (ring != nullptr && parseLinearRing(ring, polygon, offset)) {
									polygons.emplace_back(core::move(polygon));
								}
							}
						}
					}
				}
			}
		}
	}

	return !polygons.empty();
}

bool GMLFormat::parseVegetation(const tinyxml2::XMLElement *vegetation, core::DynamicArray<GMLPolygon> &polygons,
								const glm::dvec3 &offset) const {
	if (vegetation == nullptr) {
		return false;
	}

	const char *id = vegetation->Attribute("gml:id");
	if (id == nullptr) {
		id = vegetation->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing vegetation: %s", id);
	}

	parseAnyGeometry(vegetation, polygons, offset, SurfaceType::Vegetation);

	return !polygons.empty();
}

bool GMLFormat::parseWaterBody(const tinyxml2::XMLElement *waterBody, core::DynamicArray<GMLPolygon> &polygons,
							   const glm::dvec3 &offset) const {
	if (waterBody == nullptr) {
		return false;
	}

	const char *id = waterBody->Attribute("gml:id");
	if (id == nullptr) {
		id = waterBody->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing water body: %s", id);
	}

	parseAnyGeometry(waterBody, polygons, offset, SurfaceType::Water);

	// WaterBody also has boundedBy with WaterSurface, WaterGroundSurface, etc.
	for (const tinyxml2::XMLElement *boundedBy = findChildElement(waterBody, "boundedBy"); boundedBy != nullptr;
		 boundedBy = findNextSiblingElement(boundedBy, "boundedBy")) {

		const char *surfaceTypes[] = {"WaterSurface", "WaterGroundSurface", "WaterClosureSurface"};
		for (const char *surfaceTypeName : surfaceTypes) {
			const tinyxml2::XMLElement *surface = findChildElement(boundedBy, surfaceTypeName);
			if (surface != nullptr) {
				parseAnyGeometry(surface, polygons, offset, SurfaceType::Water);
			}
		}
	}

	return !polygons.empty();
}

bool GMLFormat::parseTransportation(const tinyxml2::XMLElement *transportation,
									core::DynamicArray<GMLPolygon> &polygons, const glm::dvec3 &offset) const {
	if (transportation == nullptr) {
		return false;
	}

	const char *id = transportation->Attribute("gml:id");
	if (id == nullptr) {
		id = transportation->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing transportation: %s", id);
	}

	parseAnyGeometry(transportation, polygons, offset, SurfaceType::Road);

	// Transportation also uses TrafficArea and AuxiliaryTrafficArea
	const char *areaTypes[] = {"trafficArea", "auxiliaryTrafficArea"};
	for (const char *areaType : areaTypes) {
		for (const tinyxml2::XMLElement *area = findChildElement(transportation, areaType); area != nullptr;
			 area = findNextSiblingElement(area, areaType)) {
			const tinyxml2::XMLElement *trafficArea = findChildElement(area, "TrafficArea");
			if (trafficArea == nullptr) {
				trafficArea = findChildElement(area, "AuxiliaryTrafficArea");
			}
			if (trafficArea != nullptr) {
				parseAnyGeometry(trafficArea, polygons, offset, SurfaceType::Road);
			}
		}
	}

	return !polygons.empty();
}

bool GMLFormat::parseBridge(const tinyxml2::XMLElement *bridge, core::DynamicArray<GMLPolygon> &polygons,
							const glm::dvec3 &offset) const {
	if (bridge == nullptr) {
		return false;
	}

	const char *id = bridge->Attribute("gml:id");
	if (id == nullptr) {
		id = bridge->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing bridge: %s", id);
	}

	// Direct geometry on the bridge itself
	parseAnyGeometry(bridge, polygons, offset, SurfaceType::Bridge);

	// Parse outerBridgeConstruction elements (CityGML bridge module)
	for (const tinyxml2::XMLElement *construction = findChildElement(bridge, "outerBridgeConstruction");
		 construction != nullptr; construction = findNextSiblingElement(construction, "outerBridgeConstruction")) {
		const tinyxml2::XMLElement *element = findChildElement(construction, "BridgeConstructionElement");
		if (element != nullptr) {
			parseAnyGeometry(element, polygons, offset, SurfaceType::Bridge);
		}
	}

	// Parse outerBridgeInstallation elements
	for (const tinyxml2::XMLElement *installation = findChildElement(bridge, "outerBridgeInstallation");
		 installation != nullptr; installation = findNextSiblingElement(installation, "outerBridgeInstallation")) {
		const tinyxml2::XMLElement *element = findChildElement(installation, "BridgeInstallation");
		if (element != nullptr) {
			parseAnyGeometry(element, polygons, offset, SurfaceType::Bridge);
		}
	}

	// Bridges have CityGML boundedBy with surface types
	for (const tinyxml2::XMLElement *boundedBy = findChildElement(bridge, "boundedBy"); boundedBy != nullptr;
		 boundedBy = findNextSiblingElement(boundedBy, "boundedBy")) {
		const char *surfaceTypes[] = {"WallSurface",	   "RoofSurface",		  "GroundSurface",
									  "OuterFloorSurface", "OuterCeilingSurface", "ClosureSurface"};
		for (const char *surfaceTypeName : surfaceTypes) {
			const tinyxml2::XMLElement *surface = findChildElement(boundedBy, surfaceTypeName);
			if (surface != nullptr) {
				parseAnyGeometry(surface, polygons, offset, SurfaceType::Bridge);
			}
		}
	}

	// BridgePart handling (similar to BuildingPart)
	for (const tinyxml2::XMLElement *bridgePart = findChildElement(bridge, "consistsOfBridgePart");
		 bridgePart != nullptr; bridgePart = findNextSiblingElement(bridgePart, "consistsOfBridgePart")) {
		const tinyxml2::XMLElement *innerPart = findChildElement(bridgePart, "BridgePart");
		if (innerPart != nullptr) {
			parseBridge(innerPart, polygons, offset);
		}
	}

	return !polygons.empty();
}

bool GMLFormat::parseGenericCityObject(const tinyxml2::XMLElement *obj, core::DynamicArray<GMLPolygon> &polygons,
									   const glm::dvec3 &offset) const {
	if (obj == nullptr) {
		return false;
	}

	const char *id = obj->Attribute("gml:id");
	if (id == nullptr) {
		id = obj->Attribute("id");
	}
	if (id != nullptr) {
		Log::debug("Parsing generic city object: %s", id);
	}

	parseAnyGeometry(obj, polygons, offset, SurfaceType::Unknown);

	return !polygons.empty();
}

bool GMLFormat::parseEnvelope(const tinyxml2::XMLElement *envelope, glm::dvec3 &lowerCorner, glm::dvec3 &upperCorner) const {
	if (envelope == nullptr) {
		return false;
	}

	const tinyxml2::XMLElement *lower = findChildElement(envelope, "lowerCorner");
	if (lower == nullptr) {
		return false;
	}
	if (lower->IntAttribute("srcDimension", 3) != 3) {
		Log::error("Unsupported srcDimension on lowerCorner, expected 3");
		return false;
	}
	const char *text = lower->GetText();
	if (text == nullptr) {
		return false;
	}
	SDL_sscanf(text, "%lf %lf %lf", &lowerCorner.x, &lowerCorner.y, &lowerCorner.z);

	const tinyxml2::XMLElement *upper = findChildElement(envelope, "upperCorner");
	if (upper == nullptr) {
		return false;
	}
	if (upper->IntAttribute("srcDimension", 3) != 3) {
		Log::error("Unsupported srcDimension on upperCorner, expected 3");
		return false;
	}
	text = upper->GetText();
	if (text == nullptr) {
		return false;
	}
	SDL_sscanf(text, "%lf %lf %lf", &upperCorner.x, &upperCorner.y, &upperCorner.z);

	Log::debug("GML envelope lower corner: %f %f %f", lowerCorner.x, lowerCorner.y, lowerCorner.z);
	Log::debug("GML envelope upper corner: %f %f %f", upperCorner.x, upperCorner.y, upperCorner.z);
	return true;
}

core::String GMLFormat::getObjectName(const tinyxml2::XMLElement *element, const char *typeName) {
	if (element == nullptr) {
		return typeName;
	}

	// Try gml:name child element first (per GML AbstractGMLType)
	const tinyxml2::XMLElement *nameElement = findChildElement(element, "name");
	if (nameElement != nullptr && nameElement->GetText() != nullptr) {
		return nameElement->GetText();
	}

	// Try gml:id attribute
	const char *id = element->Attribute("gml:id");
	if (id == nullptr) {
		id = element->Attribute("id");
	}
	if (id != nullptr) {
		return id;
	}

	return typeName;
}

bool GMLFormat::polygonsToMesh(const core::DynamicArray<GMLPolygon> &polygons, Mesh &mesh) const {
	core_trace_scoped(PolygonsToMesh);
	int vertices = 0;
	for (const GMLPolygon &polygon : polygons) {
		if (polygon.vertices.size() < 3) {
			continue;
		}
		vertices += polygon.vertices.size();
	}
	mesh.polygons.reserve(polygons.size());
	mesh.vertices.reserve(vertices);

	for (const GMLPolygon &polygon : polygons) {
		if (polygon.vertices.size() < 3) {
			continue;
		}

		const size_t numVertices = polygon.vertices.size();

		// Skip the closing vertex if it's the same as the first
		size_t effectiveVertices = numVertices;
		if (numVertices > 3 && glm::distance(polygon.vertices[0], polygon.vertices[numVertices - 1]) < 0.001f) {
			effectiveVertices = numVertices - 1;
		}

		if (effectiveVertices < 3) {
			continue;
		}

		const color::RGBA surfaceColor = surfaceTypeColor(polygon.surfaceType);

		voxel::IndexArray polyIndices;
		for (size_t i = 0; i < effectiveVertices; ++i) {
			MeshVertex vert;
			vert.pos = polygon.vertices[i];
			vert.color = surfaceColor;

			polyIndices.push_back((voxel::IndexType)mesh.vertices.size());
			mesh.vertices.push_back(vert);
		}
		mesh.polygons.emplace_back(core::move(polyIndices));
	}

	return !mesh.vertices.empty();
}

bool GMLFormat::parseRegionFilter(const core::String &regionStr, glm::dvec3 &mins, glm::dvec3 &maxs) {
	if (regionStr.empty()) {
		return false;
	}

	const char *ptr = regionStr.c_str();
	SDL_sscanf(ptr, "%lf %lf %lf %lf %lf %lf", &mins.x, &mins.y, &mins.z, &maxs.x, &maxs.y, &maxs.z);

	// Ensure min <= max
	for (int i = 0; i < 3; ++i) {
		if (mins[i] > maxs[i]) {
			const double tmp = mins[i];
			mins[i] = maxs[i];
			maxs[i] = tmp;
		}
	}

	Log::debug("GML region filter: (%f %f %f) to (%f %f %f)", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	return true;
}

bool GMLFormat::parseCityModel(const tinyxml2::XMLElement *cityModel, core::DynamicArray<CityObject> &objects) const {
	if (cityModel == nullptr) {
		return false;
	}

	// First, find the envelope to get the offset for coordinate normalization
	glm::dvec3 offsetMins(0.0);
	const tinyxml2::XMLElement *boundedBy = findChildElement(cityModel, "boundedBy");
	if (boundedBy != nullptr) {
		const tinyxml2::XMLElement *envelope = findChildElement(boundedBy, "Envelope");
		glm::dvec3 offsetMaxs(0.0);
		if (parseEnvelope(envelope, offsetMins, offsetMaxs)) {
			const core::String &regionStr = core::Var::getSafe(cfg::VoxformatGMLRegion)->strVal();
			glm::dvec3 gmlFilterMins(0.0);
			glm::dvec3 gmlFilterMaxs(0.0);
			if (parseRegionFilter(regionStr, gmlFilterMins, gmlFilterMaxs)) {
				if (offsetMaxs.x < gmlFilterMins.x || offsetMins.x > gmlFilterMaxs.x ||
					offsetMaxs.y < gmlFilterMins.y || offsetMins.y > gmlFilterMaxs.y ||
					offsetMaxs.z < gmlFilterMins.z || offsetMins.z > gmlFilterMaxs.z) {
					Log::debug("CityModel envelope [%i %i %i %i %i %i] does not intersect filter region, skipping [%i %i %i %i %i %i]",
							  (int)offsetMins.x, (int)offsetMins.y, (int)offsetMins.z, (int)offsetMaxs.x, (int)offsetMaxs.y,
							  (int)offsetMaxs.z, (int)gmlFilterMins.x, (int)gmlFilterMins.y, (int)gmlFilterMins.z,
							  (int)gmlFilterMaxs.x, (int)gmlFilterMaxs.y, (int)gmlFilterMaxs.z);
					return false;
				}
			}
		}
	}

	// Dispatch table: element name -> parser function + type label
	struct CityObjectDispatch {
		const char *elementName;
		const char *typeName;
		bool (GMLFormat::*parser)(const tinyxml2::XMLElement *, core::DynamicArray<GMLPolygon> &,
								  const glm::dvec3 &) const;
	};

	const CityObjectDispatch dispatchers[] = {
		{"Building", "Building", &GMLFormat::parseBuilding},
		{"LandUse", "LandUse", &GMLFormat::parseLandUse},
		{"ReliefFeature", "ReliefFeature", &GMLFormat::parseReliefFeature},
		{"PlantCover", "PlantCover", &GMLFormat::parseVegetation},
		{"SolitaryVegetationObject", "Vegetation", &GMLFormat::parseVegetation},
		{"WaterBody", "WaterBody", &GMLFormat::parseWaterBody},
		{"Road", "Road", &GMLFormat::parseTransportation},
		{"Railway", "Railway", &GMLFormat::parseTransportation},
		{"Track", "Track", &GMLFormat::parseTransportation},
		{"Square", "Square", &GMLFormat::parseTransportation},
		{"Bridge", "Bridge", &GMLFormat::parseBridge},
		{"GenericCityObject", "GenericCityObject", &GMLFormat::parseGenericCityObject},
		{"CityFurniture", "CityFurniture", &GMLFormat::parseGenericCityObject},
	};

	// Find and parse all cityObjectMember elements
	for (const tinyxml2::XMLElement *member = findChildElement(cityModel, "cityObjectMember"); member != nullptr;
		 member = findNextSiblingElement(member, "cityObjectMember")) {

		const tinyxml2::XMLElement *firstChild = member->FirstChildElement();
		if (firstChild != nullptr) {
			Log::debug("Found cityObjectMember with child: %s", firstChild->Name());
		}

		bool found = false;
		for (const CityObjectDispatch &dispatch : dispatchers) {
			const tinyxml2::XMLElement *element = findChildElement(member, dispatch.elementName);
			if (element != nullptr) {
				CityObject obj;
				obj.type = dispatch.typeName;
				obj.name = getObjectName(element, dispatch.typeName);
				obj.offset = offsetMins;
				(this->*dispatch.parser)(element, obj.polygons, offsetMins);
				if (!obj.polygons.empty()) {
					objects.emplace_back(core::move(obj));
				}
				found = true;
				break;
			}
		}

		// Fallback: try to parse any first child element as a generic object
		if (!found) {
			const tinyxml2::XMLElement *unknownChild = member->FirstChildElement();
			if (unknownChild != nullptr) {
				Log::debug("Trying to parse unknown element '%s' as generic city object", unknownChild->Name());
				CityObject obj;
				obj.type = "Unknown";
				obj.name = getObjectName(unknownChild, "Unknown");
				obj.offset = offsetMins;
				parseGenericCityObject(unknownChild, obj.polygons, offsetMins);
				if (!obj.polygons.empty()) {
					objects.emplace_back(core::move(obj));
				}
			}
		}
	}

	Log::debug("Total objects parsed: %d", (int)objects.size());
	return true;
}

bool GMLFormat::parseXMLFile(const core::String &content, core::DynamicArray<CityObject> &objects) const {
	core_trace_scoped(ParseXMLFile);

	tinyxml2::XMLDocument doc;
	tinyxml2::XMLError error = doc.Parse(content.c_str(), content.size());
	if (error != tinyxml2::XML_SUCCESS) {
		Log::error("Failed to parse GML XML: %s", doc.ErrorStr());
		return false;
	}

	// Find the root CityModel element
	const tinyxml2::XMLElement *root = doc.RootElement();
	if (root == nullptr) {
		Log::error("No root element in GML file");
		return false;
	}

	// Check if this is a CityModel
	if (!matchElementName(root->Name(), "CityModel")) {
		Log::error("Root element is not CityModel: %s", root->Name());
		return false;
	}

	return parseCityModel(root, objects);
}

bool GMLFormat::voxelizeGroups(const core::String &filename, const io::ArchivePtr &archive,
							   scenegraph::SceneGraph &sceneGraph, const LoadContext &ctx) {
	core::ScopedPtr<io::SeekableReadStream> stream(archive->readStream(filename));
	if (!stream) {
		Log::error("Could not load file %s", filename.c_str());
		return false;
	}

	core::DynamicArray<CityObject> allObjects;

	// Check if this is a zip archive (GML files are often distributed as zip)
	if (io::ZipArchive::validStream(*stream)) {
		Log::debug("GML file is a zip archive, extracting XML files");

		io::ArchivePtr zipArchive = io::openZipArchive(stream);
		if (!zipArchive) {
			Log::error("Failed to open GML zip archive");
			return false;
		}

		io::ArchiveFiles xmlFiles;
		zipArchive->list("*.xml", xmlFiles);
		if (xmlFiles.empty()) {
			zipArchive->list("*.gml", xmlFiles);
		}
		if (xmlFiles.empty()) {
			Log::error("No XML or GML files found in archive");
			return false;
		}

		Log::info("Found %d XML/GML files in archive", (int)xmlFiles.size());
		core::AtomicInt filesProcessed = 0;
		core::DynamicArray<core::String> xmlData;
		const core::String &gmlFilenameFilter = core::Var::getSafe(cfg::VoxformatGMLFilenameFilter)->strVal();
		for (const io::FilesystemEntry &entry : xmlFiles) {
			Log::debug("Processing XML file: %s", entry.fullPath.c_str());
			if (!gmlFilenameFilter.empty() && !core::string::matches(entry.name, gmlFilenameFilter, true)) {
				Log::info("Skipping file %s due to filename filter", entry.fullPath.c_str());
				continue;
			}
			core::ScopedPtr<io::SeekableReadStream> xmlStream(zipArchive->readStream(entry.fullPath));
			if (!xmlStream) {
				Log::warn("Could not read XML file %s", entry.fullPath.c_str());
				continue;
			}
			const int64_t size = xmlStream->size();
			core::String content;
			if (!xmlStream->readString((int)size, content, false)) {
				Log::warn("Failed to read XML file %s", entry.fullPath.c_str());
				continue;
			}
			if (content.empty()) {
				Log::warn("Empty XML file %s", entry.fullPath.c_str());
				continue;
			}
			xmlData.emplace_back(core::move(content));
			Log::debug("Queued XML file %s for parsing", entry.fullPath.c_str());
			if (stopExecution()) {
				break;
			}
		}

		core_trace_mutex(core::Lock, mutex, "ParseXML");
		app::for_parallel(0, (int)xmlData.size(), [&](int start, int end) {
			core::DynamicArray<CityObject> fileObjects;
			for (int i = start; i < end; ++i) {
				if (stopExecution()) {
					break;
				}
				parseXMLFile(xmlData[i], fileObjects);
				++filesProcessed;

				ctx.progress("parse xml files", (int)filesProcessed, (int)xmlFiles.size());
			}

			// TODO: PERF: collecting them in an array of size xmlData.size() and move them later by only allocating
			// once is more efficient than locking for each file and allocating and moving the objects one by one each
			// time
			core::ScopedLock lock(mutex);
			allObjects.append(core::move(fileObjects));
		});

		if (filesProcessed == 0) {
			Log::error("No valid GML data found in any XML file");
			return false;
		}
	} else {
		// Single XML file
		core::String content;
		if (!stream->readString((int)stream->size(), content, false)) {
			Log::error("Failed to read GML XML file");
			return false;
		}
		if (content.empty()) {
			Log::error("Empty GML XML file");
			return false;
		}
		Log::debug("GML file is a single XML file");
		if (!parseXMLFile(content, allObjects)) {
			return false;
		}
	}

	Log::info("Found total of %d objects in GML data", (int)allObjects.size());

	// Parse the region filter (in GML world coordinates) for per-object filtering
	glm::dvec3 gmlFilterMins(0.0);
	glm::dvec3 gmlFilterMaxs(0.0);
	const core::String &regionStr = core::Var::getSafe(cfg::VoxformatGMLRegion)->strVal();
	const bool hasRegionFilter = parseRegionFilter(regionStr, gmlFilterMins, gmlFilterMaxs);

	int nodesCreated = 0;
	for (const CityObject &obj : allObjects) {
		// Apply per-object region filter: compute the object's bounding box in GML world
		// coordinates and check if it intersects the filter region
		if (hasRegionFilter) {
			glm::dvec3 objGmlMins(DBL_MAX);
			glm::dvec3 objGmlMaxs(-DBL_MAX);
			for (const GMLPolygon &poly : obj.polygons) {
				for (const glm::vec3 &v : poly.vertices) {
					// Convert from internal coords back to GML world coords
					// Internal: (gml_x - offset.x, gml_z - offset.z, gml_y - offset.y)
					const double gmlX = (double)v.x + obj.offset.x;
					const double gmlY = (double)v.z + obj.offset.y;
					const double gmlZ = (double)v.y + obj.offset.z;
					objGmlMins.x = core_min(objGmlMins.x, gmlX);
					objGmlMins.y = core_min(objGmlMins.y, gmlY);
					objGmlMins.z = core_min(objGmlMins.z, gmlZ);
					objGmlMaxs.x = core_max(objGmlMaxs.x, gmlX);
					objGmlMaxs.y = core_max(objGmlMaxs.y, gmlY);
					objGmlMaxs.z = core_max(objGmlMaxs.z, gmlZ);
				}
			}
			// Check AABB intersection in GML world coordinates
			if (objGmlMaxs.x < gmlFilterMins.x || objGmlMins.x > gmlFilterMaxs.x ||
				objGmlMaxs.y < gmlFilterMins.y || objGmlMins.y > gmlFilterMaxs.y ||
				objGmlMaxs.z < gmlFilterMins.z || objGmlMins.z > gmlFilterMaxs.z) {
				Log::debug("Object '%s' filtered out by region filter", obj.name.c_str());
				continue;
			}
		}
		Mesh mesh;
		if (!polygonsToMesh(obj.polygons, mesh)) {
			Log::warn("Object '%s' produced no valid mesh", obj.name.c_str());
			continue;
		}

		Log::debug("Voxelizing object '%s' (%s): %d vertices, %d polygons", obj.name.c_str(), obj.type.c_str(),
				   (int)mesh.vertices.size(), (int)mesh.polygons.size());

		const int nodeId = voxelizeMesh(obj.name, sceneGraph, core::move(mesh));
		if (nodeId != InvalidNodeId) {
			scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
			node.setProperty("type", obj.type);
			++nodesCreated;
		}
	}

	if (nodesCreated == 0) {
		Log::error("No valid voxel nodes created from GML data");
		return false;
	}

	Log::debug("Created %d voxel nodes from %d objects", nodesCreated, (int)allObjects.size());
	return true;
}

} // namespace voxelformat
