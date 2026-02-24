/**
 * @file
 */

#include "voxelformat/private/mesh/gis/GMLFormat.h"
#include "AbstractFormatTest.h"
#include "core/ConfigVar.h"
#include "core/Var.h"
#include "io/MemoryArchive.h"
#include "scenegraph/SceneGraph.h"

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

// Helper: two buildings spread far apart for region filter tests
// Building A at GML world coords (0,0,0)-(5,0,5), Building B at (500,0,0)-(505,0,5)
// With envelope lower corner at (0,0,0)
static const char *twoSpreadBuildingsGML = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>0 0 0</gml:lowerCorner>
      <gml:upperCorner>505 5 5</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="buildingA">
      <gml:name>House A</gml:name>
      <bldg:boundedBy>
        <bldg:WallSurface gml:id="wallA">
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
        <bldg:WallSurface gml:id="wallB">
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">500 0 0 505 0 0 505 5 0 500 5 0 500 0 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">500 0 0 505 0 0 505 0 5 500 0 5 500 0 0</gml:posList>
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

// Test: region filter applied when voxel size exceeds threshold
TEST_F(GMLFormatTest, testRegionFilterApplied) {
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)twoSpreadBuildingsGML, SDL_strlen(twoSpreadBuildingsGML));

	// Set scale high enough to exceed 1024 threshold (505 * 3 = 1515 > 1024)
	core::getVar(cfg::VoxformatScale)->setVal("3.0");
	// Region filter that includes only Building A (GML world coords 0,0,0 to 10,10,10)
	core::getVar(cfg::VoxformatGMLRegion)->setVal("0 0 0 10 10 10");

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));

	// Only House A should be imported
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
	EXPECT_EQ(1, modelCount);
	EXPECT_TRUE(foundHouseA);
	EXPECT_FALSE(foundHouseB);

	// Reset cvars
	core::getVar(cfg::VoxformatScale)->setVal("1.0");
	core::getVar(cfg::VoxformatGMLRegion)->setVal("");
}

// Test: region filter is always applied when set
TEST_F(GMLFormatTest, testRegionFilterAlwaysApplied) {
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)twoSpreadBuildingsGML, SDL_strlen(twoSpreadBuildingsGML));

	core::getVar(cfg::VoxformatScale)->setVal("1.0");
	// Region filter that includes only Building A (GML world coords 0,0,0 to 10,10,10)
	core::getVar(cfg::VoxformatGMLRegion)->setVal("0 0 0 10 10 10");

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));

	// Only House A should be imported - region filter is always applied when set
	int modelCount = 0;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		++modelCount;
	}
	EXPECT_EQ(1, modelCount);

	// Reset cvars
	core::getVar(cfg::VoxformatGMLRegion)->setVal("");
}

// Test: warning without filter - all objects still imported
TEST_F(GMLFormatTest, testLargeDatasetNoFilterImportsAll) {
	io::MemoryArchivePtr archive = io::openMemoryArchive();
	archive->add("test.gml", (const uint8_t *)twoSpreadBuildingsGML, SDL_strlen(twoSpreadBuildingsGML));

	// Set scale high enough to exceed threshold
	core::getVar(cfg::VoxformatScale)->setVal("3.0");
	// No region filter set (empty string = default)
	core::getVar(cfg::VoxformatGMLRegion)->setVal("");

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));

	// Both houses should still be imported (warning shown but no filtering)
	int modelCount = 0;
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		++modelCount;
	}
	EXPECT_EQ(2, modelCount);

	// Reset cvars
	core::getVar(cfg::VoxformatScale)->setVal("1.0");
}

// Test: region filter works correctly with non-zero envelope offset (real-world coordinates)
TEST_F(GMLFormatTest, testRegionFilterWithOffset) {
	// Two buildings with a non-zero envelope offset (simulating real UTM coordinates)
	// Building A at GML world (1000, 2000, 0)-(1005, 2005, 5)
	// Building B at GML world (1500, 2000, 0)-(1505, 2005, 5)
	// Envelope lower corner at (1000, 2000, 0)
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>1000 2000 0</gml:lowerCorner>
      <gml:upperCorner>1505 2005 5</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
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
                      <gml:posList srsDimension="3">1000 2000 0 1005 2000 0 1005 2005 0 1000 2005 0 1000 2000 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">1000 2000 0 1005 2000 0 1005 2000 5 1000 2000 5 1000 2000 0</gml:posList>
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
                      <gml:posList srsDimension="3">1500 2000 0 1505 2000 0 1505 2005 0 1500 2005 0 1500 2000 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">1500 2000 0 1505 2000 0 1505 2000 5 1500 2000 5 1500 2000 0</gml:posList>
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

	// Region filter in GML world coordinates that includes only Building A
	core::getVar(cfg::VoxformatGMLRegion)->setVal("999 1999 -1 1010 2010 10");

	GMLFormat f;
	scenegraph::SceneGraph sceneGraph;
	ASSERT_TRUE(f.load("test.gml", archive, sceneGraph, testLoadCtx));

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
	EXPECT_EQ(1, modelCount);
	EXPECT_TRUE(foundHouseA);
	EXPECT_FALSE(foundHouseB);

	// Reset cvars
	core::getVar(cfg::VoxformatGMLRegion)->setVal("");
}

// Test: multi-file with different offsets produces objects that don't all overlap
// This tests the global offset adjustment in voxelizeGroups by loading two buildings
// at different world positions and verifying their voxel nodes are not at the same position.
TEST_F(GMLFormatTest, testMultiFileOffsetAdjustment) {
	// Two buildings in the same file with a non-zero envelope offset
	// Building 1 at GML world (105-110, 205-210, 0-5)
	// Building 2 at GML world (205-210, 305-310, 0-5)
	// These are far apart in world space, so they should NOT overlap in the scene.
	const char *gmlData = R"(<?xml version="1.0" encoding="UTF-8"?>
<core:CityModel xmlns:core="http://www.opengis.net/citygml/2.0"
  xmlns:bldg="http://www.opengis.net/citygml/building/2.0"
  xmlns:gml="http://www.opengis.net/gml">
  <gml:boundedBy>
    <gml:Envelope srsName="EPSG:25832" srsDimension="3">
      <gml:lowerCorner>100 200 0</gml:lowerCorner>
      <gml:upperCorner>210 310 5</gml:upperCorner>
    </gml:Envelope>
  </gml:boundedBy>
  <core:cityObjectMember>
    <bldg:Building gml:id="building1">
      <gml:name>Building Near</gml:name>
      <bldg:boundedBy>
        <bldg:WallSurface>
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">105 205 0 110 205 0 110 210 0 105 210 0 105 205 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">105 205 0 110 205 0 110 205 5 105 205 5 105 205 0</gml:posList>
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
    <bldg:Building gml:id="building2">
      <gml:name>Building Far</gml:name>
      <bldg:boundedBy>
        <bldg:WallSurface>
          <bldg:lod2MultiSurface>
            <gml:MultiSurface>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">205 305 0 210 305 0 210 310 0 205 310 0 205 305 0</gml:posList>
                    </gml:LinearRing>
                  </gml:exterior>
                </gml:Polygon>
              </gml:surfaceMember>
              <gml:surfaceMember>
                <gml:Polygon>
                  <gml:exterior>
                    <gml:LinearRing>
                      <gml:posList srsDimension="3">205 305 0 210 305 0 210 305 5 205 305 5 205 305 0</gml:posList>
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

	// Both buildings should be imported
	int modelCount = 0;
	glm::vec3 pos1(0.0f), pos2(0.0f);
	for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
		const scenegraph::SceneGraphNode &n = *iter;
		const scenegraph::SceneGraphTransform &transform = n.transform(0);
		if (n.name() == "Building Near") {
			pos1 = transform.localTranslation();
		} else if (n.name() == "Building Far") {
			pos2 = transform.localTranslation();
		}
		++modelCount;
	}
	EXPECT_EQ(2, modelCount);

	// The two buildings should be at different positions (not overlapping)
	// Building Near is at GML (105,205) and Building Far is at GML (205,305)
	// They should be separated by roughly 100 units on each axis
	const float distance = glm::length(pos2 - pos1);
	EXPECT_GT(distance, 50.0f) << "Buildings should not overlap - they are at different world positions";
}

} // namespace voxelformat
