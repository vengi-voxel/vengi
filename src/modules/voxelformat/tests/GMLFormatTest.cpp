/**
 * @file
 */

#include "AbstractFormatTest.h"
#include "io/MemoryArchive.h"
#include "scenegraph/SceneGraph.h"
#include "voxelformat/private/mesh/gis/GMLFormat.h"

namespace voxelformat {

class GMLFormatTest : public AbstractFormatTest {};

TEST_F(GMLFormatTest, testLoadBridge) {
	scenegraph::SceneGraph sceneGraph;
	testLoad(sceneGraph, "bridge-gml/Modellbahnbruecke-LoD3-V03.gml");
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
	const voxel::Region &region = node->volume()->region();
	EXPECT_GT(region.getWidthInVoxels(), 0);
	EXPECT_GT(region.getHeightInVoxels(), 0);
	EXPECT_GT(region.getDepthInVoxels(), 0);
}

// Test: minimal CityGML building with a single gml:Polygon using gml:posList
TEST_F(GMLFormatTest, testLoadMinimalBuilding) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:WallSurface gml:id="wall1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly2">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 0 5 0 0 5 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
}

// Test: gml:Polygon with interior rings (holes)
TEST_F(GMLFormatTest, testLoadPolygonWithInteriorRing) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>20 20 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:GroundSurface gml:id="ground1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 10 0 0 10 10 0 0 10 0 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                  <gml:interior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">2 2 0 8 2 0 8 8 0 2 8 0 2 2 0</gml:posList>
                    </gml:LinearRing>
                  </gml:interior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:GroundSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: gml:MultiGeometry with gml:geometryMember elements
TEST_F(GMLFormatTest, testLoadMultiGeometry) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:brid="http://www.opengis.net/citygml/bridge/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>20 20 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <brid:Bridge gml:id="bridge1">
      <brid:outerBridgeConstruction>
        <brid:BridgeConstructionElement gml:id="elem1">
          <brid:lod2Geometry>
            <gml:MultiGeometry>
              <gml:geometryMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:geometryMember>
              <gml:geometryMember>
                <gml:Polygon gml:id="poly2">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">5 0 0 10 0 0 10 0 5 5 0 5 5 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:geometryMember>
            </gml:MultiGeometry>
          </brid:lod2Geometry>
        </brid:BridgeConstructionElement>
      </brid:outerBridgeConstruction>
    </brid:Bridge>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: deprecated gml:coordinates element with custom separators
TEST_F(GMLFormatTest, testLoadDeprecatedCoordinates) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:WallSurface gml:id="wall1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:coordinates cs="," ts=" ">0,0,0 5,0,0 5,0,5 0,0,5 0,0,0</gml:coordinates>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: individual gml:pos elements in LinearRing (instead of posList)
TEST_F(GMLFormatTest, testLoadIndividualPosElements) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:RoofSurface gml:id="roof1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:pos>0 0 5</gml:pos>
                      <gml:pos>5 0 5</gml:pos>
                      <gml:pos>5 5 5</gml:pos>
                      <gml:pos>0 5 5</gml:pos>
                      <gml:pos>0 0 5</gml:pos>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:RoofSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: 2D coordinates with srsDimension="2"
TEST_F(GMLFormatTest, testLoad2DCoordinates) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:luse="http://www.opengis.net/citygml/landuse/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="2">
      <gml:lowerCorner>0 0</gml:lowerCorner>
      <gml:upperCorner>10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <luse:LandUse gml:id="landuse1">
      <luse:lod1MultiSurface>
        <gml:MultiSurface>
          <gml:surfaceMember>
            <gml:Polygon gml:id="poly1">
              <gml:exterior>
                <gml:LinearRing>
                  <gml:posList srsDimension="2">0 0 10 0 10 10 0 10 0 0</gml:posList>
                </gml:LinearRing>
              </gml:exterior>
            </gml:Polygon>
          </gml:surfaceMember>
        </gml:MultiSurface>
      </luse:lod1MultiSurface>
    </luse:LandUse>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: CompositeSurface inside lod element
TEST_F(GMLFormatTest, testLoadCompositeSurface) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:lod1Solid>
        <gml:Solid>
          <gml:exterior>
            <gml:CompositeSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 5 0 0 5 0 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 5 5 0 5 5 5 5 0 5 5 0 0 5</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:CompositeSurface>
          </gml:exterior>
        </gml:Solid>
      </bldg:lod1Solid>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: GML2 backward compatibility (outerBoundaryIs/innerBoundaryIs)
TEST_F(GMLFormatTest, testLoadGML2BackwardCompat) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:GroundSurface gml:id="ground1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:outerBoundaryIs>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 10 0 0 10 10 0 0 10 0 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:outerBoundaryIs>
                  <gml:innerBoundaryIs>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">2 2 0 8 2 0 8 8 0 2 8 0 2 2 0</gml:posList>
                    </gml:LinearRing>
                  </gml:innerBoundaryIs>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:GroundSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: metadata extraction (name and description)
TEST_F(GMLFormatTest, testLoadMetadata) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:name>TestModel</gml:name>
  <gml:description>A test city model</gml:description>
  <gml:boundedBy>
    <gml:Envelope srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:WallSurface gml:id="wall1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon gml:id="poly1">
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	// Single object uses the object's gml:id as name
	EXPECT_EQ("building1", node->name());
}

// Test: invalid/empty GML should fail gracefully
TEST_F(GMLFormatTest, testLoadInvalidGML) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<NotACityModel></NotACityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_FALSE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
}

// Test: CityModel with no geometry should fail
TEST_F(GMLFormatTest, testLoadEmptyCityModel) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:gml="http://www.opengis.net/gml">
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_FALSE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
}

// Test: building with multiple surface types (wall + roof + ground)
TEST_F(GMLFormatTest, testLoadMultipleSurfaceTypes) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:boundedBy>
        <bldg:WallSurface gml:id="wall1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
      <bldg:boundedBy>
        <bldg:RoofSurface gml:id="roof1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 5 5 0 5 5 5 5 0 5 5 0 0 5</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:RoofSurface>
      </bldg:boundedBy>
      <bldg:boundedBy>
        <bldg:GroundSurface gml:id="ground1">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 5 0 0 5 0 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:GroundSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
	ASSERT_NE(nullptr, node->volume());
}

// Test: building with BuildingPart (recursive parsing)
TEST_F(GMLFormatTest, testLoadBuildingPart) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>10 10 10</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <bldg:consistsOfBuildingPart>
        <bldg:BuildingPart gml:id="part1">
          <bldg:boundedBy>
            <bldg:WallSurface gml:id="wall1">
              <bldg:lod2MultiSurface>
                <gml:MultiSurface>
                  <gml:surfaceMember>
                    <gml:Polygon>
                      <gml:exterior>
                        <gml:LinearRing>
                          <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                        </gml:LinearRing>
                      </gml:exterior>
                    </gml:Polygon>
                  </gml:surfaceMember>
                </gml:MultiSurface>
              </bldg:lod2MultiSurface>
            </bldg:WallSurface>
          </bldg:boundedBy>
        </bldg:BuildingPart>
      </bldg:consistsOfBuildingPart>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));
	const scenegraph::SceneGraphNode *node = sceneGraph.firstModelNode();
	ASSERT_NE(nullptr, node);
}

// Test: multiple objects get separate scene graph nodes under a group
TEST_F(GMLFormatTest, testLoadMultipleObjectsAsNodes) {
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:name>MyCity</gml:name>
  <core:cityObjectMember>
    <bldg:Building gml:id="buildingA">
      <gml:name>House A</gml:name>
      <bldg:boundedBy>
        <bldg:WallSurface>
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">0 0 0 5 0 0 5 0 5 0 0 5 0 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
  <core:cityObjectMember>
    <bldg:Building gml:id="buildingB">
      <gml:name>House B</gml:name>
      <bldg:boundedBy>
        <bldg:WallSurface>
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">10 0 0 15 0 0 15 0 5 10 0 5 10 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
            </gml:MultiSurface>
          </bldg:lod2MultiSurface>
        </bldg:WallSurface>
      </bldg:boundedBy>
    </bldg:Building>
  </core:cityObjectMember>
</core:CityModel>)";

	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)gmlData, SDL_strlen(gmlData));

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));

	// Should have a group node named "MyCity" with 2 model children
	int modelCount = 0;
	bool foundHouseA = false;
	bool foundHouseB = false;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &n = *iter;
		++modelCount;
		if (n.name() == "House A") {
			foundHouseA = true;
		} else if (n.name() == "House B") {
			foundHouseB = true;
		}
	}
	EXPECT_EQ(2, modelCount);
	EXPECT_TRUE(foundHouseA);
	EXPECT_TRUE(foundHouseB);

	// Check the group node exists
	bool foundGroup = false;
	for (auto iter = sceneGraph.begin(scenegraph::SceneGraphNodeType::Group); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &n = *iter;
		if (n.name() == "MyCity") {
			foundGroup = true;
			break;
		}
	}
	EXPECT_TRUE(foundGroup);
}

} // namespace voxelformat
