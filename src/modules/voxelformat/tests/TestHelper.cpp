/**
 * @file
 */

#include "TestHelper.h"
#include "core/Common.h"
#include "core/Log.h"
#include "core/ScopedPtr.h"
#include "math/Random.h"
#include "palette/Palette.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphNodeCamera.h"
#include "voxel/Face.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelformat/private/mesh/MeshFormat.h"
#include "voxelutil/VolumeVisitor.h"
#ifndef GLM_ENABLE_EXPERIMENTAL
#define GLM_ENABLE_EXPERIMENTAL
#endif
#include <glm/gtx/string_cast.hpp>

namespace glm {
::std::ostream &operator<<(::std::ostream &os, const mat4x4 &matrix) {
	os << to_string(matrix);
	return os;
}
::std::ostream &operator<<(::std::ostream &os, const mat3x3 &matrix) {
	os << to_string(matrix);
	return os;
}
::std::ostream &operator<<(::std::ostream &os, const mat4x3 &matrix) {
	os << to_string(matrix);
	return os;
}
} // namespace glm


namespace palette {

::std::ostream &operator<<(::std::ostream &os, const Palette &palette) {
	return os << Palette::print(palette).c_str();
}

::std::ostream &operator<<(::std::ostream &os, const palette::Material &material) {
	os << "Material: " << (int)material.type << " ";
	for (uint32_t i = 0; i < palette::MaterialProperty::MaterialMax - 1; ++i) {
		if (!material.has((palette::MaterialProperty)i)) {
			continue;
		}
		os << palette::MaterialPropertyNames[i] << ": " << material.value((palette::MaterialProperty)i) << ", ";
	}
	return os;
}

}

namespace voxel {

static void dumpNode_r(::std::ostream &os, const scenegraph::SceneGraph &sceneGraph, int nodeId, int indent) {
	const scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	const scenegraph::SceneGraphNodeType type = node.type();

	os << os.iword(indent) << "Node: " << nodeId << "(parent " << node.parent() << ")" << "\n";
	os << os.iword(indent) << "  |- name: " << node.name().c_str() << "\n";
	os << os.iword(indent) << "  |- type: " << scenegraph::SceneGraphNodeTypeStr[core::enumVal(type)] << "\n";
	const glm::vec3 &pivot = node.pivot();
	os << os.iword(indent) << "  |- pivot " << pivot.x << ":" << pivot.y << ":" << pivot.z << "\n";
	if (type == scenegraph::SceneGraphNodeType::Model) {
		const voxel::RawVolume *v = node.volume();
		int voxels = 0;
		os << os.iword(indent) << "  |- volume: " << (v != nullptr ? v->region().toString().c_str() : "no volume")
		   << "\n";
		if (v) {
			voxels = voxelutil::countVoxels(*v);
		}
		os << os.iword(indent) << "  |- voxels: " << voxels << "\n";
	} else if (type == scenegraph::SceneGraphNodeType::Camera) {
		const scenegraph::SceneGraphNodeCamera &cameraNode = scenegraph::toCameraNode(node);
		os << os.iword(indent) << "  |- field of view: " << cameraNode.fieldOfView() << "\n";
		os << os.iword(indent) << "  |- nearplane: " << cameraNode.nearPlane() << "\n";
		os << os.iword(indent) << "  |- farplane: " << cameraNode.farPlane() << "\n";
		os << os.iword(indent) << "  |- mode: " << (cameraNode.isOrthographic() ? "ortho" : "perspective") << "\n";
	}
	for (const auto &entry : node.properties()) {
		os << os.iword(indent) << "  |- " << entry->key.c_str() << ": " << entry->value.c_str() << "\n";
	}
	for (const scenegraph::SceneGraphKeyFrame &kf : node.keyFrames()) {
		os << os.iword(indent) << "  |- keyframe: " << kf.frameIdx << "\n";
		os << os.iword(indent) << "    |- long rotation: " << (kf.longRotation ? "true" : "false") << "\n";
		os << os.iword(indent)
		   << "    |- interpolation: " << scenegraph::InterpolationTypeStr[core::enumVal(kf.interpolation)]
		   << "\n";
		os << os.iword(indent) << "    |- transform" << "\n";
		const scenegraph::SceneGraphTransform &transform = kf.transform();
		const glm::vec3 &tr = transform.worldTranslation();
		os << os.iword(indent) << "      |- translation " << tr.x << ":" << tr.y << ":" << tr.z << "\n";
		const glm::vec3 &ltr = transform.localTranslation();
		os << os.iword(indent) << "      |- local translation " << ltr.x << ":" << ltr.y << ":" << ltr.z << "\n";
		const glm::quat &rt = transform.worldOrientation();
		const glm::vec3 &rtEuler = glm::degrees(glm::eulerAngles(rt));
		os << os.iword(indent) << "      |- orientation :%f" << rt.x << ":" << rt.y << ":" << rt.z << ":" << rt.w
		   << "\n";
		os << os.iword(indent) << "        |- euler " << rtEuler.x << ":" << rtEuler.y << ":" << rtEuler.z << "\n";
		const glm::quat &lrt = transform.localOrientation();
		const glm::vec3 &lrtEuler = glm::degrees(glm::eulerAngles(lrt));
		os << os.iword(indent) << "      |- local orientation :%f" << lrt.x << ":" << lrt.y << ":" << lrt.z << ":"
		   << lrt.w << "\n";
		os << os.iword(indent) << "        |- euler " << lrtEuler.x << ":" << lrtEuler.y << ":" << lrtEuler.z
		   << "\n";
		const glm::vec3 &sc = transform.worldScale();
		os << os.iword(indent) << "      |- scale " << sc.x << ":" << sc.y << ":" << sc.z << "\n";
		const glm::vec3 &lsc = transform.localScale();
		os << os.iword(indent) << "      |- local scale " << lsc.x << ":" << lsc.y << ":" << lsc.z << "\n";
	}
	os << os.iword(indent) << "  |- children: " << node.children().size() << "\n";
	for (int children : node.children()) {
		dumpNode_r(os, sceneGraph, children, indent + 2);
	}
}

::std::ostream &operator<<(::std::ostream &os, const scenegraph::SceneGraph &sceneGraph) {
	dumpNode_r(os, sceneGraph, sceneGraph.root().id(), 0);
	return os;
}

void colorComparator(const palette::Palette &pal1, const palette::Palette &pal2, color::RGBA c1, color::RGBA c2, uint8_t palIdx, float maxDelta) {
	if (c1 != c2) {
		const float delta = color::getDistance(c1, c2, color::Distance::HSB);
		ASSERT_LT(delta, maxDelta) << "Palette color differs at " << (int)palIdx << ", color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "], delta[" << delta << "]"
									<< "\nPalette 1:\n"
									<< palette::Palette::print(pal1) << "\nPalette 2:\n"
									<< palette::Palette::print(pal2);
	}
}

void colorComparator(color::RGBA c1, color::RGBA c2, int maxDelta) {
	EXPECT_NEAR(c1.r, c2.r, maxDelta) << "color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "]";
	EXPECT_NEAR(c1.g, c2.g, maxDelta) << "color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "]";
	EXPECT_NEAR(c1.b, c2.b, maxDelta) << "color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "]";
	EXPECT_NEAR(c1.a, c2.a, maxDelta) << "color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "]";
}

void colorComparatorDistance(color::RGBA c1, color::RGBA c2, float maxDelta) {
	if (c1 != c2) {
		const float delta = color::getDistance(c1, c2, color::Distance::HSB);
		ASSERT_LT(delta, maxDelta) << "Color differ: color1[" << color::print(c1)
									<< "], color2[" << color::print(c2) << "], delta[" << delta << "]";
	}
}

void paletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta) {
	ASSERT_EQ(pal1.colorCount(), pal2.colorCount());
	for (int i = 0; i < pal1.colorCount(); ++i) {
		const color::RGBA &c1 = pal1.color(i);
		const color::RGBA &c2 = pal2.color(i);
		colorComparator(pal1, pal2, c1, c2, (uint8_t)i, maxDelta);
	}
}

void paletteComparatorScaled(const palette::Palette &pal1, const palette::Palette &pal2, int maxDelta) {
	ASSERT_EQ(pal1.colorCount(), pal2.colorCount());
	for (int i = 0; i < pal1.colorCount(); ++i) {
		const color::RGBA &c1 = pal1.color(i);
		const color::RGBA &c2 = pal2.color(i);
		colorComparator(c1, c2, maxDelta);
	}
}

void orderPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, float maxDelta) {
	ASSERT_EQ(pal1.colorCount(), pal2.colorCount());
	for (int i = 0; i < pal1.colorCount(); ++i) {
		const color::RGBA &c1 = pal1.color(i);
		bool found = false;
		for (int j = 0; j < pal2.colorCount(); ++j) {
			if (pal2.color(j) == c1) {
				found = true;
				break;
			}
		}

		ASSERT_TRUE(found) << "Palette color at " << i << ", color1[" << color::print(c1)
						   << "] wasn't found in second palette 2:\n"
						   << palette::Palette::print(pal2);
	}
}

void partialPaletteComparator(const palette::Palette &pal1, const palette::Palette &pal2, voxel::ValidateFlags flags, float maxDelta) {
	const int n = glm::min(pal1.colorCount(), pal2.colorCount());
	for (int i = 0; i < n; ++i) {
		const color::RGBA &c1 = pal1.color(i);
		const color::RGBA &c2 = pal2.color(i);
		if (c1 != c2) {
			if ((flags & voxel::ValidateFlags::PaletteColorsScaled) == voxel::ValidateFlags::PaletteColorsScaled) {
				EXPECT_NEAR(c1.r, c2.r, (int)maxDelta);
				EXPECT_NEAR(c1.g, c2.g, (int)maxDelta);
				EXPECT_NEAR(c1.b, c2.b, (int)maxDelta);
				EXPECT_NEAR(c1.a, c2.a, (int)maxDelta);
			} else {
				const float delta = color::getDistance(c1, c2, color::Distance::HSB);
				ASSERT_LT(delta, maxDelta) << "Palette color differs at " << i << ", color1[" << color::print(c1)
										<< "], color2[" << color::print(c2) << "], delta[" << delta << "]"
										<< "\nPalette 1:\n"
										<< palette::Palette::print(pal1) << "\nPalette 2:\n"
										<< palette::Palette::print(pal2);
			}
		}
	}
}

void keyFrameComparator(const scenegraph::SceneGraphKeyFrames &keyframes1,
						const scenegraph::SceneGraphKeyFrames &keyframes2, ValidateFlags flags) {
	if ((flags & ValidateFlags::Animations) == ValidateFlags::Animations) {
		ASSERT_EQ(keyframes1.size(), keyframes2.size());
		for (size_t i = 0; i < keyframes1.size(); ++i) {
			ASSERT_EQ(keyframes1[i].frameIdx, keyframes2[i].frameIdx);
			ASSERT_EQ(keyframes1[i].longRotation, keyframes2[i].longRotation);
			ASSERT_EQ(keyframes1[i].interpolation, keyframes2[i].interpolation);
			const scenegraph::SceneGraphTransform &t1 = keyframes1[i].transform();
			const scenegraph::SceneGraphTransform &t2 = keyframes2[i].transform();
			ASSERT_FALSE(t1.dirty()) << "Key frame " << i << " is not yet updated";
			ASSERT_FALSE(t2.dirty()) << "Key frame " << i << " is not yet updated";
			if ((flags & ValidateFlags::Translation) == ValidateFlags::Translation) {
				ASSERT_TRUE(glm::all(glm::epsilonEqual(t1.worldTranslation(), t2.worldTranslation(), 0.00001f))) << "World translation failed for frame " << i << " with " << t1.worldTranslation() << " vs " << t2.worldTranslation();
				ASSERT_TRUE(glm::all(glm::epsilonEqual(t1.localTranslation(), t2.localTranslation(), 0.00001f))) << "Local translation failed for frame " << i << " with " << t1.worldTranslation() << " vs " << t2.worldTranslation();
				for (int n = 0; n < 4; ++n) {
					for (int m = 0; m < 4; ++m) {
						ASSERT_TRUE(glm::epsilonEqual(t1.worldMatrix()[n][m], t2.worldMatrix()[n][m], 0.00001f)) << "Matrix failed for frame " << i << " at " << n << ":" << m;
						ASSERT_TRUE(glm::epsilonEqual(t1.localMatrix()[n][m], t2.localMatrix()[n][m], 0.00001f)) << "Matrix failed for frame " << i << " at " << n << ":" << m;
					}
				}
			} else {
				const glm::mat3x3 wrot1 = t1.worldMatrix();
				const glm::mat3x3 wrot2 = t2.worldMatrix();
				const glm::mat3x3 lrot1 = t1.localMatrix();
				const glm::mat3x3 lrot2 = t2.localMatrix();
				ASSERT_EQ(wrot1, wrot2) << "Matrix failed for frame " << i;
				ASSERT_EQ(lrot1, lrot2) << "Matrix failed for frame " << i;
			}
			if ((flags & ValidateFlags::Scale) == ValidateFlags::Scale) {
				for (int n = 0; n < 3; ++n) {
					ASSERT_NEAR(t1.worldScale()[n], t2.worldScale()[n], 0.0001f) << "World scale failed for frame " << i << " and component " << n;
					ASSERT_NEAR(t1.localScale()[n], t2.localScale()[n], 0.0001f) << "Local scale failed for frame " << i << " and component " << n;
				}
			}
		}
	} else {
		ASSERT_GE(keyframes1.size(), 1u) << "keyframes 1 doesn't have any entry";
		ASSERT_GE(keyframes2.size(), 1u) << "keyframes 2 doesn't have any entry";
		ASSERT_EQ(keyframes1[0].frameIdx, keyframes2[0].frameIdx);
		ASSERT_EQ(keyframes1[0].longRotation, keyframes2[0].longRotation);
		ASSERT_EQ(keyframes1[0].interpolation, keyframes2[0].interpolation);
		const scenegraph::SceneGraphTransform &t1 = keyframes1[0].transform();
		const scenegraph::SceneGraphTransform &t2 = keyframes2[0].transform();
		ASSERT_FALSE(t1.dirty()) << "Key frame 0 is not yet updated";
		ASSERT_FALSE(t2.dirty()) << "Key frame 0 is not yet updated";
		if ((flags & ValidateFlags::Translation) == ValidateFlags::Translation) {
			ASSERT_EQ(t1.worldTranslation(), t2.worldTranslation()) << "Translation failed for frame 0";
		}
	}
}

void volumeComparator(const voxel::RawVolume &volume1, const palette::Palette &pal1, const voxel::RawVolume &volume2,
					  const palette::Palette &pal2, ValidateFlags flags, float maxDelta) {
	const Region &r1 = volume1.region();
	const Region &r2 = volume2.region();
	if ((flags & ValidateFlags::Region) == ValidateFlags::Region) {
		ASSERT_EQ(r1, r2) << "regions differ: " << r1.toString() << " vs " << r2.toString();
	}

	const int32_t lowerX = r1.getLowerX();
	const int32_t lowerY = r1.getLowerY();
	const int32_t lowerZ = r1.getLowerZ();
	const int32_t upperX = r1.getUpperX();
	const int32_t upperY = r1.getUpperY();
	const int32_t upperZ = r1.getUpperZ();
	const int32_t lower2X = r2.getLowerX();
	const int32_t lower2Y = r2.getLowerY();
	const int32_t lower2Z = r2.getLowerZ();
	const int32_t upper2X = r2.getUpperX();
	const int32_t upper2Y = r2.getUpperY();
	const int32_t upper2Z = r2.getUpperZ();

	voxel::RawVolume::Sampler s1(volume1);
	voxel::RawVolume::Sampler s2(volume2);
	s1.setPosition(r1.getLowerCorner());
	s2.setPosition(r2.getLowerCorner());
	for (int32_t z1 = lowerZ, z2 = lower2Z; z1 <= upperZ && z2 <= upper2Z; ++z1, ++z2) {
		voxel::RawVolume::Sampler s1_2 = s1;
		voxel::RawVolume::Sampler s2_2 = s2;
		for (int32_t y1 = lowerY, y2 = lower2Y; y1 <= upperY && y2 <= upper2Y; ++y1, ++y2) {
			voxel::RawVolume::Sampler s1_3 = s1_2;
			voxel::RawVolume::Sampler s2_3 = s2_2;
			for (int32_t x1 = lowerX, x2 = lower2X; x1 <= upperX && x2 <= upper2X; ++x1, ++x2) {
				const voxel::Voxel voxel1 = s1_3.voxel();
				const voxel::Voxel voxel2 = s2_3.voxel();

				s1_3.movePositiveX();
				s2_3.movePositiveX();

				if (voxel::isAir(voxel1.getMaterial()) ^ voxel::isAir(voxel2.getMaterial())) {
					if ((flags & ValidateFlags::IgnoreHollow) == ValidateFlags::IgnoreHollow) {
						FaceBits vis1 = visibleFaces(volume1, x1, y1, z1);
						FaceBits vis2 = visibleFaces(volume2, x2, y2, z2);
						if (vis1 == FaceBits::None || vis2 == FaceBits::None) {
							continue;
						}
					}
				}

				ASSERT_EQ(voxel1.getMaterial(), voxel2.getMaterial())
					<< "Voxel differs at " << x1 << ":" << y1 << ":" << z1 << " and " << x2 << ":" << y2 << ":" << z2
					<< " in material - voxel1[" << voxel::VoxelTypeStr[(int)voxel1.getMaterial()] << ", "
					<< (int)voxel1.getColor() << "], voxel2[" << voxel::VoxelTypeStr[(int)voxel2.getMaterial()] << ", "
					<< (int)voxel2.getColor() << "], color1[" << color::print(voxel1.getColor()) << "], color2["
					<< color::print(voxel2.getColor()) << "]";
				if (voxel::isAir(voxel1.getMaterial())) {
					continue;
				}
				if ((flags & ValidateFlags::Color) != ValidateFlags::Color) {
					continue;
				}

				if ((flags & ValidateFlags::IgnoreHollow) == ValidateFlags::IgnoreHollow) {
					if (voxel2.getColor() == voxelformat::MeshFormat::FillColorIndex &&
						voxel1.getColor() != voxelformat::MeshFormat::FillColorIndex) {
						continue;
					}
				}

				const color::RGBA &c1 = pal1.color(voxel1.getColor());
				const color::RGBA &c2 = pal2.color(voxel2.getColor());
				if (c1 != c2) {
					const float delta = color::getDistance(c1, c2, color::Distance::HSB);
					if (pal1.hash() != pal2.hash()) {
						ASSERT_LT(delta, maxDelta)
							<< "Voxel differs at " << x1 << ":" << y1 << ":" << z1 << " and " << x2 << ":" << y2 << ":"
							<< z2 << " in material - voxel1[" << voxel::VoxelTypeStr[(int)voxel1.getMaterial()] << ", "
							<< (int)voxel1.getColor() << "], voxel2[" << voxel::VoxelTypeStr[(int)voxel2.getMaterial()]
							<< ", " << (int)voxel2.getColor() << "], color1[" << color::print(c1) << "], color2["
							<< color::print(c2) << "], delta[" << delta << "]\n"
							<< palette::Palette::print(pal1) << "\n"
							<< palette::Palette::print(pal2);
					} else {
						ASSERT_LT(delta, maxDelta)
							<< "Voxel differs at " << x1 << ":" << y1 << ":" << z1 << " and " << x2 << ":" << y2 << ":"
							<< z2 << " in material - voxel1[" << voxel::VoxelTypeStr[(int)voxel1.getMaterial()] << ", "
							<< (int)voxel1.getColor() << "], voxel2[" << voxel::VoxelTypeStr[(int)voxel2.getMaterial()]
							<< ", " << (int)voxel2.getColor() << "], color1[" << color::print(c1) << "], color2["
							<< color::print(c2) << "], delta[" << delta << "]\n";
					}
				}
			}
			s1_2.movePositiveY();
			s2_2.movePositiveY();
		}
		s1.movePositiveZ();
		s2.movePositiveZ();
	}
}

void materialComparator(const palette::Palette &pal1, const palette::Palette &pal2) {
	for (int i = 0; i < pal2.colorCount(); ++i) {
		int foundColorMatch = -1;
		int foundMaterialMatch = -1;
		const palette::Material &pal2Mat = pal2.material(i);
		for (int j = 0; j < pal1.colorCount(); ++j) {
			// check if the color matches the pal2 palette color
			if (pal2.color(i) != pal1.color(j)) {
				continue;
			}
			foundColorMatch = true;
			const palette::Material &pal1Mat = pal1.material(j);
			if (pal1Mat == pal2Mat) {
				foundMaterialMatch = j;
				break;
			}
		}
		ASSERT_NE(-1, foundColorMatch) << "Could not find a color match in the pal1 palette: " << pal1.name();
		ASSERT_NE(-1, foundMaterialMatch) << "Found a color match - but the materials differ: " << pal2Mat
											<< " versus " << pal1.material(foundColorMatch) << " for entry " << i;
	}
}

void materialComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2) {
	for (auto iter = graph1.beginModel(), iter2 = graph2.beginModel(); iter != graph1.end(); ++iter, ++iter2) {
		const scenegraph::SceneGraphNode &graph1Node = *iter;
		const scenegraph::SceneGraphNode &graph2Node = *iter2;
		const palette::Palette &graph1Pal = graph1Node.palette();
		const palette::Palette &graph2Pal = graph2Node.palette();
		materialComparator(graph1Pal, graph2Pal);
		if (testing::Test::HasFatalFailure())
			break;
	}
}

void sceneGraphComparator(const scenegraph::SceneGraph &graph1, const scenegraph::SceneGraph &graph2,
						  ValidateFlags flags, float maxDelta) {
	if ((flags & ValidateFlags::SceneGraphModels) != ValidateFlags::SceneGraphModels) {
		const scenegraph::SceneGraph::MergeResult &merged1 = graph1.merge();
		core::ScopedPtr<voxel::RawVolume> v1(merged1.volume());
		const scenegraph::SceneGraph::MergeResult &merged2 = graph2.merge();
		core::ScopedPtr<voxel::RawVolume> v2(merged2.volume());
		ASSERT_NE(nullptr, v1);
		ASSERT_NE(nullptr, v2);
		if ((flags & ValidateFlags::Palette) == ValidateFlags::Palette) {
			voxel::paletteComparator(merged1.palette, merged2.palette, maxDelta);
		} else if ((flags & ValidateFlags::PaletteMinMatchingColors) == ValidateFlags::PaletteMinMatchingColors) {
			voxel::partialPaletteComparator(merged1.palette, merged2.palette, flags, maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorsScaled) == voxel::ValidateFlags::PaletteColorsScaled) {
			voxel::paletteComparatorScaled(merged1.palette, merged2.palette, (int)maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorOrderDiffers) ==
				   voxel::ValidateFlags::PaletteColorOrderDiffers) {
			voxel::orderPaletteComparator(merged1.palette, merged2.palette, maxDelta);
		}
		volumeComparator(*v1, merged1.palette, *v2, merged2.palette, flags, maxDelta);
		return;
	}
	ASSERT_EQ(graph1.size(scenegraph::SceneGraphNodeType::AllModels), graph2.size(scenegraph::SceneGraphNodeType::AllModels));
	auto iter1 = graph1.beginAllModels();
	auto iter2 = graph2.beginAllModels();
	for (; iter1 != graph1.end() && iter2 != graph2.end(); ++iter1, ++iter2) {
		const scenegraph::SceneGraphNode &node1 = *iter1;
		const scenegraph::SceneGraphNode &node2 = *iter2;
		if ((flags & ValidateFlags::Palette) == ValidateFlags::Palette) {
			voxel::paletteComparator(node1.palette(), node2.palette(), maxDelta);
		} else if ((flags & ValidateFlags::PaletteMinMatchingColors) == ValidateFlags::PaletteMinMatchingColors) {
			voxel::partialPaletteComparator(node1.palette(), node2.palette(), flags, maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorsScaled) == voxel::ValidateFlags::PaletteColorsScaled) {
			voxel::paletteComparatorScaled(node1.palette(), node2.palette(), (int)maxDelta);
		} else if ((flags & voxel::ValidateFlags::PaletteColorOrderDiffers) == voxel::ValidateFlags::PaletteColorOrderDiffers) {
			voxel::orderPaletteComparator(node1.palette(), node2.palette(), maxDelta);
		}
		// it's intended that includingRegion is false here!
		// Use resolveVolume to handle ModelReference nodes that don't have their own volume
		const voxel::RawVolume *v1 = graph1.resolveVolume(node1);
		const voxel::RawVolume *v2 = graph2.resolveVolume(node2);
		ASSERT_NE(nullptr, v1) << "Failed to resolve volume for node " << node1.name();
		ASSERT_NE(nullptr, v2) << "Failed to resolve volume for node " << node2.name();
		volumeComparator(*v1, node1.palette(), *v2, node2.palette(), flags, maxDelta);
		if ((flags & ValidateFlags::Pivot) == ValidateFlags::Pivot) {
			EXPECT_VEC_NEAR(node1.pivot(), node2.pivot(), 0.0001f) << "Pivot failed";
		}
		keyFrameComparator(node1.keyFrames(), node2.keyFrames(), flags);
	}
}

} // namespace voxel
