/**
 * @file
 */

#include "voxelutil/VolumeSelect.h"
#include "app/tests/AbstractTest.h"
#include "voxel/Connectivity.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"

namespace voxelutil {

class VolumeSelectTest : public app::AbstractTest {};

static void fillBox(voxel::RawVolume &volume, const voxel::Region &r, const voxel::Voxel &v) {
	const glm::ivec3 &lo = r.getLowerCorner();
	const glm::ivec3 &hi = r.getUpperCorner();
	for (int z = lo.z; z <= hi.z; ++z) {
		for (int y = lo.y; y <= hi.y; ++y) {
			for (int x = lo.x; x <= hi.x; ++x) {
				volume.setVoxel(x, y, z, v);
			}
		}
	}
}

// Regression: lasso drawn on an upper structure must not sweep in a lower structure that
// merely shares the (u, v) silhouette. Before flood-fill, selectionFinalizeLasso filtered
// every surface voxel in the volume by 2D point-in-polygon only, so a tower behind a
// lassoed rooftop got picked up too.
TEST_F(VolumeSelectTest, testLassoFloodFillDoesNotCrossDisjointStructures) {
	voxel::Region region(0, 29);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Upper slab at y=20, lower slab at y=5. Same X/Z footprint, disjoint in Y. With a
	// PositiveY lasso drawing U=Z, V=X, both slabs project to the same (u, v) silhouette.
	fillBox(volume, voxel::Region(glm::ivec3(5, 20, 5), glm::ivec3(15, 20, 15)), solid);
	fillBox(volume, voxel::Region(glm::ivec3(5, 5, 5), glm::ivec3(15, 5, 15)), solid);

	// Polygon vertices live on the upper slab's top surface so edge rasterization seeds
	// only the upper slab. Path traces a square covering the whole upper slab footprint.
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(5, 20, 5));
	path.push_back(glm::ivec3(15, 20, 5));
	path.push_back(glm::ivec3(15, 20, 15));
	path.push_back(glm::ivec3(5, 20, 15));

	int selectedUpper = 0;
	int selectedLower = 0;
	int selectedElsewhere = 0;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) {
		if (y == 20) {
			++selectedUpper;
		} else if (y == 5) {
			++selectedLower;
		} else {
			++selectedElsewhere;
		}
	};

	// PositiveY face: U=Z (axis 2), V=X (axis 0), W=Y (axis 1), positiveNormal=true
	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*depthTolerance*/ 2);

	EXPECT_GT(selectedUpper, 0) << "upper slab should be selected";
	EXPECT_EQ(selectedLower, 0) << "lower slab must not be swept in despite sharing (X,Z)";
	EXPECT_EQ(selectedElsewhere, 0);
}

// A lasso drawn on the front (+X) face of a column must select only that visible front
// surface - it must NOT wrap down the side walls or reach the back face. The flood works on
// the frontmost solid per (u,v) column, so side/back voxels (which are never frontmost) are
// left out. This is the behavior that keeps the selection a thin skin instead of a 3D chunk.
TEST_F(VolumeSelectTest, testLassoFloodFillFrontSurfaceOnly) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Vertical column at (8..11, 0..15, 8..11).
	fillBox(volume, voxel::Region(glm::ivec3(8, 0, 8), glm::ivec3(11, 15, 11)), solid);

	// Lasso drawn on the +X face: U=Y (axis 1), V=Z (axis 2), W=X (axis 0).
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(11, 2, 8));
	path.push_back(glm::ivec3(11, 13, 8));
	path.push_back(glm::ivec3(11, 13, 11));
	path.push_back(glm::ivec3(11, 2, 11));

	core::DynamicSet<glm::ivec3, 257, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	// PositiveX face: U=Y (1), V=Z (2), W=X (0), positiveNormal=true
	lassoFloodFillSurface(volume, path, /*uAxis*/ 1, /*vAxis*/ 2, /*wAxis*/ 0,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*depthTolerance*/ 2);

	bool frontHit = false;
	bool backHit = false;
	bool interiorHit = false;
	for (auto *entry : selected) {
		if (entry->key.x == 11) {
			frontHit = true;
		} else if (entry->key.x == 8) {
			backHit = true;
		} else if (entry->key.x == 9 || entry->key.x == 10) {
			interiorHit = true;
		}
	}
	EXPECT_TRUE(frontHit) << "front face (x=11) should be selected";
	EXPECT_FALSE(backHit) << "back face (x=8) must NOT be reached - no wrapping through walls";
	EXPECT_FALSE(interiorHit) << "interior depth voxels must NOT be selected - front surface only";
}

// Concave polygon: if the first seed alone couldn't reach every interior corner via flood,
// seeding from every edge pixel protects us from the trap. Drawing an L-shape polygon on a
// flat surface must still cover the whole inside of the L.
TEST_F(VolumeSelectTest, testLassoFloodFillConcavePolygon) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// 12x12 flat top at y=5
	fillBox(volume, voxel::Region(glm::ivec3(2, 5, 2), glm::ivec3(13, 5, 13)), solid);

	// L-shape polygon in (Z=U, X=V):
	//   Z in [2..13], X in [2..7] (wide stroke)
	//   Z in [8..13], X in [7..13] (narrow stroke) - forms an L
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(2, 5, 2));
	path.push_back(glm::ivec3(7, 5, 2));
	path.push_back(glm::ivec3(7, 5, 8));
	path.push_back(glm::ivec3(13, 5, 8));
	path.push_back(glm::ivec3(13, 5, 13));
	path.push_back(glm::ivec3(2, 5, 13));

	int selectedCount = 0;
	bool insideLCorner = false;	 // voxel at (3, 5, 12) - deep inside wide stroke
	bool insideLNarrow = false;	 // voxel at (10, 5, 10) - inside narrow stroke
	bool outsideCorner = false;	 // voxel at (12, 5, 3) - outside the L's notch
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) {
		++selectedCount;
		if (x == 3 && y == 5 && z == 12) {
			insideLCorner = true;
		}
		if (x == 10 && y == 5 && z == 10) {
			insideLNarrow = true;
		}
		if (x == 12 && y == 5 && z == 3) {
			outsideCorner = true;
		}
	};

	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*depthTolerance*/ 2);

	EXPECT_GT(selectedCount, 0);
	EXPECT_TRUE(insideLCorner) << "flood should reach deep inside the wide stroke";
	EXPECT_TRUE(insideLNarrow) << "flood should reach the narrow stroke past the bend";
	EXPECT_FALSE(outsideCorner) << "voxels outside the L notch must not be selected";
}

// Regression: drawing a thin line (a degenerate out-and-back path enclosing ~zero area)
// must still select a continuous line of surface voxels along the stroke. Before seeds were
// marked directly, the point-in-polygon area gate rejected almost every seed for such a
// sliver polygon, leaving grid-aliased dashes instead of a connected line.
TEST_F(VolumeSelectTest, testLassoFloodFillThinLine) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Flat top at y=5
	fillBox(volume, voxel::Region(glm::ivec3(2, 5, 2), glm::ivec3(13, 5, 13)), solid);

	// Degenerate sliver: trace a diagonal out and immediately back along the same line.
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(2, 5, 2));
	path.push_back(glm::ivec3(13, 5, 13));
	path.push_back(glm::ivec3(2, 5, 2));

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*depthTolerance*/ 2);

	// Every voxel along the drawn diagonal must be selected - no aliased gaps.
	for (int k = 2; k <= 13; ++k) {
		EXPECT_TRUE(selected.has(glm::ivec3(k, 5, k)))
			<< "diagonal voxel (" << k << ",5," << k << ") should be part of the thin-line selection";
	}
}

// The fill takes the visible (frontmost) surface in every column, so a pit inside the polygon is
// filled at its floor - that is the surface seen looking down into it.
TEST_F(VolumeSelectTest, testLassoFillsPitFloor) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Plateau solid up to y=10 over x,z in [2..13]; a central 4x4 pit (x,z in [6..9]) carved out
	// above y=4, so the pit floor sits 6 voxels below the plateau top.
	fillBox(volume, voxel::Region(glm::ivec3(2, 0, 2), glm::ivec3(13, 10, 13)), solid);
	fillBox(volume, voxel::Region(glm::ivec3(6, 5, 6), glm::ivec3(9, 10, 9)), voxel::createVoxel(voxel::VoxelType::Air, 0));

	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(2, 10, 2));
	path.push_back(glm::ivec3(13, 10, 2));
	path.push_back(glm::ivec3(13, 10, 13));
	path.push_back(glm::ivec3(2, 10, 13));

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };
	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*maxDeviation*/ 0);
	EXPECT_TRUE(selected.has(glm::ivec3(3, 10, 3))) << "plateau surface should be selected";
	EXPECT_TRUE(selected.has(glm::ivec3(7, 4, 7))) << "pit floor (the visible surface there) should be selected";
}

// The interior must fill completely even across a riser that a per-step continuity flood would
// stop at, as long as the surface stays within the deviation of the fitted lasso plane.
TEST_F(VolumeSelectTest, testLassoFillsAcrossRiser) {
	voxel::Region region(0, 29);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Surface: flat top y=10 for x in [0..8], a 4-voxel riser up to y=14 for x in [9..20].
	for (int x = 0; x <= 20; ++x) {
		const int top = x <= 8 ? 10 : 14;
		for (int z = 0; z <= 10; ++z) {
			for (int y = 0; y <= top; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	// Rectangle lasso spanning both levels; vertices sit on the surface at each end.
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(4, 10, 3));
	path.push_back(glm::ivec3(16, 14, 3));
	path.push_back(glm::ivec3(16, 14, 7));
	path.push_back(glm::ivec3(4, 10, 7));

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	// PositiveY face: U=Z (2), V=X (0), W=Y (1). depthTolerance=4 covers the riser.
	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*maxDeviation*/ 4);

	EXPECT_TRUE(selected.has(glm::ivec3(6, 10, 5))) << "interior on the lower level should be filled";
	EXPECT_TRUE(selected.has(glm::ivec3(12, 14, 5)))
		<< "interior on the upper level past the riser should be filled (plane fill, not per-step flood)";
	EXPECT_FALSE(selected.has(glm::ivec3(12, 5, 5))) << "interior depth voxels below the surface must not be selected";
}

// lineMarkSolid rasterizes a straight 3D segment and marks solid voxels within the width.
// Air cells along the line are skipped, and voxels off the line are never marked.
TEST_F(VolumeSelectTest, testLineMarkSolidSkipsAirAndOffLine) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Solid run along X at y=2,z=2 from x=2..8, with a one-voxel air gap at x=5.
	for (int x = 2; x <= 8; ++x) {
		if (x == 5) {
			continue;
		}
		volume.setVoxel(x, 2, 2, solid);
	}

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lineMarkSolid(volume, glm::ivec3(2, 2, 2), glm::ivec3(8, 2, 2), /*width*/ 1, region, markFunc);

	for (int x = 2; x <= 8; ++x) {
		if (x == 5) {
			EXPECT_FALSE(selected.has(glm::ivec3(5, 2, 2))) << "air gap on the line must not be selected";
			continue;
		}
		EXPECT_TRUE(selected.has(glm::ivec3(x, 2, 2))) << "solid voxel " << x << " on the line should be selected";
	}
}

// width controls the Chebyshev thickness around the centerline (width 3 -> radius 1).
TEST_F(VolumeSelectTest, testLineMarkSolidWidth) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	fillBox(volume, voxel::Region(glm::ivec3(0, 0, 0), glm::ivec3(10, 10, 10)), solid);

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lineMarkSolid(volume, glm::ivec3(2, 5, 5), glm::ivec3(8, 5, 5), /*width*/ 3, region, markFunc);

	// Centerline plus a one-voxel Chebyshev shell around it (off-axis neighbours).
	EXPECT_TRUE(selected.has(glm::ivec3(5, 5, 5))) << "centerline voxel selected";
	EXPECT_TRUE(selected.has(glm::ivec3(5, 6, 5))) << "neighbour within width selected";
	EXPECT_TRUE(selected.has(glm::ivec3(5, 5, 6))) << "neighbour within width selected";
	EXPECT_FALSE(selected.has(glm::ivec3(5, 7, 5))) << "voxel two away from centerline not selected at width 3";
}

// Returns true if every voxel in @p set is reachable from any other via 6-connected (face)
// steps that stay inside the set - i.e. the selection is a single gap-free connected line.
static bool isFaceConnected(const core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> &set) {
	if (set.empty()) {
		return false;
	}
	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> seen;
	core::DynamicArray<glm::ivec3> stack;
	stack.push_back((*set.begin())->key);
	seen.insert((*set.begin())->key);
	int reached = 0;
	while (!stack.empty()) {
		const glm::ivec3 cur = stack.back();
		stack.pop();
		++reached;
		for (const glm::ivec3 &offset : voxel::arrayPathfinderFaces) {
			const glm::ivec3 nb = cur + offset;
			if (!set.has(nb) || seen.has(nb)) {
				continue;
			}
			seen.insert(nb);
			stack.push_back(nb);
		}
	}
	return reached == (int)set.size();
}

// THE rectangle "disjoint edge" regression: an edge draped down a terraced slope must be ONE
// face-connected, fully-visible chain - the riser faces between treads must be filled so the line
// does not render as disconnected tread segments.
TEST_F(VolumeSelectTest, testLineDrapeSurfaceFillsRisersConnected) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Steep staircase along X: top(x) = 10 - 2*x (drops 2 per column), so each step has a 2-voxel
	// riser face that a per-column line would skip.
	for (int x = 0; x <= 5; ++x) {
		const int top = 10 - 2 * x;
		for (int z = 3; z <= 7; ++z) {
			for (int y = 0; y <= top; ++y) {
				volume.setVoxel(x, y, z, solid);
			}
		}
	}

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	// Drape from the top tread to the bottom: uAxis=X(0), vAxis=Z(2), wAxis=Y(1), positiveNormal.
	lineDrapeSurface(volume, glm::ivec3(0, 10, 5), glm::ivec3(5, 0, 5), /*uAxis*/ 0, /*vAxis*/ 2, /*wAxis*/ 1,
					 /*positiveNormal*/ true, /*width*/ 1, region, markFunc);

	EXPECT_TRUE(selected.has(glm::ivec3(0, 10, 5))) << "top tread selected";
	EXPECT_TRUE(selected.has(glm::ivec3(5, 0, 5))) << "bottom tread selected";
	// Riser faces between the first two steps must be filled (top(0)=10, top(1)=8 -> riser at y=9).
	EXPECT_TRUE(selected.has(glm::ivec3(0, 9, 5))) << "riser face between first two steps must be filled";
	EXPECT_TRUE(isFaceConnected(selected)) << "the draped line must be one gap-free face-connected chain";
}

// lineDrapeSurface marks the visible (front-most) surface and must NOT jump to a far surface.
TEST_F(VolumeSelectTest, testLineDrapeSurfaceNoGhost) {
	voxel::Region region(0, 29);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	for (int x = 0; x <= 10; ++x) {
		volume.setVoxel(x, 20, 5, solid);
		volume.setVoxel(x, 2, 5, solid);
	}

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lineDrapeSurface(volume, glm::ivec3(0, 20, 5), glm::ivec3(10, 20, 5), /*uAxis*/ 0, /*vAxis*/ 2, /*wAxis*/ 1,
					 /*positiveNormal*/ true, /*width*/ 1, region, markFunc);

	for (int x = 0; x <= 10; ++x) {
		EXPECT_TRUE(selected.has(glm::ivec3(x, 20, 5))) << "near surface selected at x=" << x;
		EXPECT_FALSE(selected.has(glm::ivec3(x, 2, 5))) << "far surface must NOT be selected (no ghost) at x=" << x;
	}
}

// THE floodfill-completeness regression: a vertical wall face that is exposed to the
// front-connected air (the side of a tall block standing next to a shorter shelf) must be
// selected. The old per-column "frontmost solid" heightfield grabbed only the single top voxel
// of each column, so it selected the two flat tops but left the whole vertical wall between them
// unselected - the gaps the user saw.
TEST_F(VolumeSelectTest, testLassoFloodFillSelectsExposedWall) {
	voxel::Region region(0, 19);
	voxel::RawVolume volume(region);
	const voxel::Voxel solid = voxel::createVoxel(voxel::VoxelType::Generic, 1);

	// Tall block (top y=10) for x in [0..4]; short shelf (top y=5) for x in [5..9]. The +X face
	// of the tall block (x=4, y in [6..10]) is a vertical wall exposed to the air above the shelf.
	fillBox(volume, voxel::Region(glm::ivec3(0, 0, 0), glm::ivec3(4, 10, 9)), solid);
	fillBox(volume, voxel::Region(glm::ivec3(5, 0, 0), glm::ivec3(9, 5, 9)), solid);

	// Top-down lasso (+Y) over the whole footprint: U=Z (2), V=X (0), W=Y (1). Point-in-polygon
	// uses only (z,x), so the vertices' y just needs to sit on the surface for the edge walk.
	core::DynamicArray<glm::ivec3> path;
	path.push_back(glm::ivec3(0, 10, 0));
	path.push_back(glm::ivec3(9, 5, 0));
	path.push_back(glm::ivec3(9, 5, 9));
	path.push_back(glm::ivec3(0, 10, 9));

	core::DynamicSet<glm::ivec3, 1031, glm::hash<glm::ivec3>> selected;
	auto markFunc = [&](int x, int y, int z, const voxel::Voxel &) { selected.insert(glm::ivec3(x, y, z)); };

	lassoFloodFillSurface(volume, path, /*uAxis*/ 2, /*vAxis*/ 0, /*wAxis*/ 1,
						  /*positiveNormal*/ true, volume.region(), markFunc, /*maxDeviation*/ 0);

	// The two flat tops (what the heightfield already got).
	EXPECT_TRUE(selected.has(glm::ivec3(2, 10, 5))) << "tall block top selected";
	EXPECT_TRUE(selected.has(glm::ivec3(7, 5, 5))) << "short shelf top selected";
	// The exposed vertical wall (x=4) - what the heightfield missed and the air flood now gets.
	EXPECT_TRUE(selected.has(glm::ivec3(4, 6, 5))) << "bottom of the exposed wall must be selected";
	EXPECT_TRUE(selected.has(glm::ivec3(4, 8, 5))) << "middle of the exposed wall must be selected";
	EXPECT_TRUE(selected.has(glm::ivec3(4, 10, 5))) << "top of the exposed wall must be selected";
	// Voxels with no front-connected air neighbour stay unselected.
	EXPECT_FALSE(selected.has(glm::ivec3(2, 5, 5))) << "deep interior of the tall block must NOT be selected";
	EXPECT_FALSE(selected.has(glm::ivec3(7, 2, 5))) << "interior of the short shelf must NOT be selected";
}

} // namespace voxelutil
