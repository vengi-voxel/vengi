/**
 * @file
 */

#include "color/ColorUtil.h"
#include "voxelutil/ImageUtils.h"
#include "app/tests/AbstractTest.h"
#include "core/ConfigVar.h"
#include "core/ScopedPtr.h"
#include "core/String.h"
#include "core/Var.h"
#include "core/tests/TestColorHelper.h"
#include "image/Image.h"
#include "io/FileStream.h"
#include "palette/Palette.h"
#include "voxel/Face.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Voxel.h"
#include "voxel/tests/VoxelPrinter.h"
#include "voxelutil/ImportFace.h"
#include "voxelutil/VolumeVisitor.h"

namespace voxelutil {

class ImageUtilsTest : public app::AbstractTest {
protected:
	void validateVoxel(const voxel::RawVolume &volume, const palette::Palette &palette, const image::ImagePtr &image,
					   int x, int y) {
		const color::RGBA expectedColor = image->colorAt(x, y);
		const voxel::Voxel &voxel = volume.voxel(x, y, 0);
		const color::RGBA actualColor = palette.color(voxel.getColor());
		EXPECT_LT(color::getDistance(expectedColor, actualColor, color::Distance::HSB), 0.04f)
			<< "Expected color: " << color::print(expectedColor)
			<< ", but got: " << color::print(actualColor) << " for voxel at (" << x << ", " << y << ")";
	}

	void validateHeightmap(voxel::Voxel underground) {
		const core::String filename = "test-colored-heightmap.png";
		image::ImagePtr image = image::loadImage(filename);
		ASSERT_TRUE(image && image->isLoaded()) << "Failed to load image: " << filename;
		ASSERT_EQ(16, image->width()) << "Expected height to be 16, but got: " << image->height();
		ASSERT_EQ(16, image->height()) << "Expected width to be 16, but got: " << image->width();
		voxel::Region region(0, 0, 0, image->width() - 1, 31, image->height() - 1);
		voxel::RawVolume volume(region);
		voxel::RawVolumeWrapper wrapper(&volume);
		palette::Palette palette;
		palette.nippon();
		const int minHeight = 1;
		voxelutil::importColoredHeightmap(wrapper, palette, image, underground, 1, true);
		int minsY;
		if (voxel::isAir(underground.getMaterial())) {
			minsY = region.getLowerY() + 3;
		} else {
			minsY = region.getLowerY();
		}
		voxel::Region expectedRegion(region.getLowerX(), minsY, region.getLowerZ(), region.getUpperX(), 17,
									 region.getUpperZ());
		int expectedVoxelCount = voxel::isAir(underground.getMaterial()) ? image->width() * image->height() : 3626;
		EXPECT_EQ(expectedVoxelCount, voxelutil::countVoxels(volume));
		for (int x = 0; x < image->width(); ++x) {
			for (int z = 0; z < image->height(); ++z) {
				const color::RGBA expectedColor = image->colorAt(x, z);
				const int volumeHeight = region.getHeightInVoxels();
				const int expectedY = getHeightValueFromAlpha(expectedColor.a, true, volumeHeight, minHeight) - 1;
				const voxel::Voxel &voxel = volume.voxel(x, expectedY, z);
				const color::RGBA actualColor = palette.color(voxel.getColor());
				ASSERT_LT(color::getDistance(expectedColor, actualColor, color::Distance::HSB), 0.04f)
					<< "Expected color: " << color::print(expectedColor)
					<< ", but got: " << color::print(actualColor) << " for voxel at (" << x << ", " << expectedY
					<< ", " << z << ") with height alpha value " << (int)expectedColor.a << " and min height "
					<< minHeight << " and height in voxels " << volumeHeight;
				if (expectedY > 0) {
					const voxel::Voxel &actualUnderground = volume.voxel(x, expectedY - 1, z);
					EXPECT_EQ(underground.getMaterial(), actualUnderground.getMaterial())
						<< "Expected underground voxel at (" << x << ", " << expectedY - 1 << ", " << z
						<< ") to have material " << underground.getMaterial()
						<< ", but got: " << actualUnderground.getMaterial();
				}
			}
		}
	}
};

TEST_F(ImageUtilsTest, testImportAsPlane) {
	const image::ImagePtr &img = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	const int depth = 2;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsPlane(img, palette, 2));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
	EXPECT_EQ(3758, voxelutil::countVoxels(*volume));
}

TEST_F(ImageUtilsTest, testImportAsVolumeBothSided) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 10;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, palette, depth, true));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth * 2 + 1, volume->depth());
	EXPECT_EQ(40, voxelutil::countVoxels(*volume));
}

TEST_F(ImageUtilsTest, testImportAsVolumeSingleSided) {
	const image::ImagePtr &img = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(img->isLoaded()) << "Failed to load image: " << img->name();
	int depth = 9;
	palette::Palette palette;
	palette.nippon();
	core::ScopedPtr<voxel::RawVolume> volume(voxelutil::importAsVolume(img, palette, depth, false));
	ASSERT_TRUE(volume);
	EXPECT_EQ(img->width(), volume->width());
	EXPECT_EQ(img->height(), volume->height());
	EXPECT_EQ(depth, volume->depth());
	EXPECT_EQ(28, voxelutil::countVoxels(*volume));
}

TEST_F(ImageUtilsTest, testImportHeightMaxHeightAlpha) {
	constexpr int h = 4;
	constexpr int w = 4;
	constexpr color::RGBA buffer[w * h]{{255, 0, 0, 127},  {255, 255, 0, 128},  {255, 0, 255, 129},	{255, 255, 255, 1},
									   {0, 255, 0, 0},	  {13, 255, 50, 45},   {127, 127, 127, 45}, {255, 127, 0, 32},
									   {255, 0, 0, 45},	  {255, 60, 0, 45},	   {255, 0, 30, 45},	{127, 69, 255, 45},
									   {127, 127, 0, 45}, {255, 127, 127, 45}, {255, 0, 127, 45},	{0, 127, 80, 45}};
	static_assert(sizeof(buffer) == (size_t)w * (size_t)h * sizeof(uint32_t), "Unexpected rgba buffer size");
	const image::ImagePtr &texture = image::createEmptyImage("4x4");
	texture->loadRGBA((const uint8_t *)buffer, w, h);
	ASSERT_TRUE(texture);
	ASSERT_EQ(w, texture->width());
	ASSERT_EQ(h, texture->height());

	const int maxHeight = voxelutil::importHeightMaxHeight(texture, true);
	EXPECT_EQ(129, maxHeight);
}

TEST_F(ImageUtilsTest, testImportFace) {
	const image::ImagePtr &image = image::loadImage("test-heightmap.png");
	ASSERT_TRUE(image && image->isLoaded());
	voxel::Region region(0, 0, 0, image->width() - 1, image->height() - 1, 0);
	voxel::RawVolume volume(region);
	palette::Palette palette;
	palette.nippon();
	voxel::FaceNames faceName = voxel::FaceNames::PositiveZ;
	ASSERT_TRUE(
		voxelutil::importFace(volume, region, palette, faceName, image, glm::vec2(0.0f, 1.0f), glm::vec2(1.0f, 0.0f)));
	ASSERT_EQ(8, voxelutil::countVoxels(volume));

	validateVoxel(volume, palette, image, 5, 7);
	validateVoxel(volume, palette, image, 6, 5);
	validateVoxel(volume, palette, image, 6, 6);
	validateVoxel(volume, palette, image, 6, 7);
	validateVoxel(volume, palette, image, 7, 4);
	validateVoxel(volume, palette, image, 7, 5);
	validateVoxel(volume, palette, image, 7, 6);
	validateVoxel(volume, palette, image, 7, 7);
}

TEST_F(ImageUtilsTest, testGetHeightValueFromAlpha) {
	EXPECT_EQ(0, getHeightValueFromAlpha(0, true, 10, 0));
	EXPECT_EQ(5, getHeightValueFromAlpha(127, true, 10, 0));
	EXPECT_EQ(10, getHeightValueFromAlpha(255, true, 10, 0));
	EXPECT_EQ(1, getHeightValueFromAlpha(0, true, 10, 1));
	EXPECT_EQ(15, getHeightValueFromAlpha(127, true, 31, 0));
	EXPECT_EQ(17, getHeightValueFromAlpha(132, true, 32, 1));
}

TEST_F(ImageUtilsTest, testImportColoredHeightmap) {
	const voxel::Voxel underground = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	validateHeightmap(underground);
}

TEST_F(ImageUtilsTest, testImportColoredHeightmapSurfaceOnly) {
	const voxel::Voxel underground = voxel::createVoxel(voxel::VoxelType::Air, 1);
	validateHeightmap(underground);
}

TEST_F(ImageUtilsTest, testRenderToImage) {
	palette::Palette palette;
	palette.nippon();
	voxel::Region _region(0, 6);
	voxel::RawVolume volume(_region);
	const voxel::Voxel color1 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel color2 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel color3 = voxel::createVoxel(voxel::VoxelType::Generic, 3);

	// Fill a small structure in the lower corner of the test region for a nicer render:
	const glm::ivec3 base = _region.getLowerCorner();
	// 3x3x3 cube with alternating colors
	for (int z = 0; z < 3; ++z) {
		for (int y = 0; y < 3; ++y) {
			for (int x = 0; x < 3; ++x) {
				voxel::Voxel v = (x + y + z) % 3 == 0 ? color1 : ((x + y + z) % 3 == 1 ? color2 : color3);
				volume.setVoxel(base + glm::ivec3(x, y, z), v);
			}
		}
	}

	// Add a small column and a diagonal line for depth cues
	for (int y = 0; y < 6; ++y) {
		volume.setVoxel(base + glm::ivec3(5, y, 0), color2);
	}
	for (int i = 0; i < 6; ++i) {
		volume.setVoxel(base + glm::ivec3(i, i % 4, 5), color3);
	}

	ImageRenderConfig config;
	config.frontFace = voxel::FaceNames::Front;
	const image::ImagePtr &img = renderToImage(&volume, palette, config);
	ASSERT_TRUE(img && img->isLoaded());
	const io::FilePtr &file = _testApp->filesystem()->open("front.png", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(image::writePNG(img, stream));
	int pixelCount = 0;
	for (int x = 0; x < img->width(); ++x) {
		for (int y = 0; y < img->height(); ++y) {
			voxel::Voxel voxel;
			const int vx = img->width() - 1 - x;
			for (int z = 0; z < _region.getDepthInVoxels(); ++z) {
				voxel = volume.voxel(vx, _region.getHeightInCells() - y, z);
				if (!voxel::isAir(voxel.getMaterial())) {
					break;
				}
			}
			if (voxel::isAir(voxel.getMaterial())) {
				continue;
			}
			const color::RGBA expectedColor = palette.color(voxel.getColor());
			const color::RGBA actualColor = img->colorAt(x, y);
			ASSERT_LT(color::getDistance(expectedColor, actualColor, color::Distance::HSB), 0.04f)
				<< "Expected color: " << color::print(expectedColor)
				<< ", but got: " << color::print(actualColor) << " for voxel at (" << x << ", " << y << ") "
				<< image::print(img);
			++pixelCount;
		}
	}
	EXPECT_GT(pixelCount, 0);

	ImageRenderConfig withDepth = config;
	withDepth.depthFactor = 2.0f;
	const image::ImagePtr &depthImg = renderToImage(&volume, palette, withDepth);
	ASSERT_TRUE(depthImg && depthImg->isLoaded());
	ASSERT_EQ(img->width(), depthImg->width());
	ASSERT_EQ(img->height(), depthImg->height());
	int darkerCount = 0;
	for (int x = 0; x < img->width(); ++x) {
		for (int y = 0; y < img->height(); ++y) {
			const color::RGBA flat = img->colorAt(x, y);
			const color::RGBA shaded = depthImg->colorAt(x, y);
			if (flat.rgba == config.background.rgba) {
				continue;
			}
			const int flatLum = (int)flat.r + (int)flat.g + (int)flat.b;
			const int shadedLum = (int)shaded.r + (int)shaded.g + (int)shaded.b;
			if (shadedLum < flatLum) {
				++darkerCount;
			}
		}
	}
	EXPECT_GT(darkerCount, 0);
}

TEST_F(ImageUtilsTest, testImageRenderConfigFromCvars) {
	core::Var::registerVar(core::VarDef(cfg::ImageDepthFactor2D, 0.0f, "2D depth factor", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageBackground, "1 0 0 1", "Background", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageWidth, -1, "Image width", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageHeight, -1, "Image height", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageUpScale, true, "Upscale", "help"));
	core::getVar(cfg::ImageDepthFactor2D)->setVal(1.5f);
	core::getVar(cfg::ImageWidth)->setVal(64);
	core::getVar(cfg::ImageHeight)->setVal(32);
	core::getVar(cfg::ImageUpScale)->setVal(false);
	const ImageRenderConfig config = ImageRenderConfig::fromCvars();
	EXPECT_FLOAT_EQ(config.depthFactor, 1.5f);
	EXPECT_EQ(config.frontFace, voxel::FaceNames::Front);
	EXPECT_FALSE(config.upScale);
	EXPECT_EQ(config.imgW, 64);
	EXPECT_EQ(config.imgH, 32);
	EXPECT_EQ(config.background.r, 255);
	core::getVar(cfg::ImageDepthFactor2D)->setVal(0.0f);
	core::getVar(cfg::ImageWidth)->setVal(-1);
	core::getVar(cfg::ImageHeight)->setVal(-1);
	core::getVar(cfg::ImageUpScale)->setVal(true);
	core::getVar(cfg::ImageBackground)->setVal("0 0 0 0");
}

TEST_F(ImageUtilsTest, testRenderIsometric) {
	palette::Palette palette;
	palette.nippon();
	voxel::Region _region(0, 63);
	voxel::RawVolume volume(_region);
	const voxel::Voxel color1 = voxel::createVoxel(voxel::VoxelType::Generic, 1);
	const voxel::Voxel color2 = voxel::createVoxel(voxel::VoxelType::Generic, 2);
	const voxel::Voxel color3 = voxel::createVoxel(voxel::VoxelType::Generic, 3);

	// Fill a small structure in the lower corner of the test region for a nicer render:
	const glm::ivec3 base = _region.getLowerCorner();
	// 3x3x3 cube with alternating colors
	for (int z = 0; z < 3; ++z) {
		for (int y = 0; y < 3; ++y) {
			for (int x = 0; x < 3; ++x) {
				voxel::Voxel v = (x + y + z) % 3 == 0 ? color1 : ((x + y + z) % 3 == 1 ? color2 : color3);
				volume.setVoxel(base + glm::ivec3(x, y, z), v);
			}
		}
	}

	// Add a small column and a diagonal line for depth cues
	for (int y = 0; y < 6; ++y) {
		volume.setVoxel(base + glm::ivec3(5, y, 0), color2);
	}
	for (int i = 0; i < 6; ++i) {
		volume.setVoxel(base + glm::ivec3(i, i % 4, 5), color3);
	}

	const image::ImagePtr &img = renderIsometricImage(&volume, palette);
	ASSERT_TRUE(img && img->isLoaded());
	const io::FilePtr &file = _testApp->filesystem()->open("isometric.png", io::FileMode::SysWrite);
	io::FileStream stream(file);
	ASSERT_TRUE(image::writePNG(img, stream));
}

TEST_F(ImageUtilsTest, testRenderIsometricFaceLighting) {
	palette::Palette palette;
	palette.setColor(1, color::RGBA(200, 200, 200, 255));
	voxel::RawVolume volume(voxel::Region(0, 0));
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));
	IsoRenderConfig config;
	const image::ImagePtr img = renderIsometricImage(&volume, palette, config);
	ASSERT_TRUE(img && img->isLoaded());
	ASSERT_EQ(8, img->width());
	ASSERT_EQ(7, img->height());
	const color::RGBA top = img->colorAt(3, 1);
	const color::RGBA left = img->colorAt(1, 4);
	const color::RGBA right = img->colorAt(6, 4);
	const int topLum = (int)top.r + (int)top.g + (int)top.b;
	const int leftLum = (int)left.r + (int)left.g + (int)left.b;
	const int rightLum = (int)right.r + (int)right.g + (int)right.b;
	EXPECT_GT(topLum, leftLum);
	EXPECT_GT(leftLum, rightLum);

	config.shading = false;
	const image::ImagePtr flat = renderIsometricImage(&volume, palette, config);
	ASSERT_TRUE(flat && flat->isLoaded());
	const int flatTop = (int)flat->colorAt(3, 1).r + (int)flat->colorAt(3, 1).g + (int)flat->colorAt(3, 1).b;
	const int flatLeft = (int)flat->colorAt(1, 4).r + (int)flat->colorAt(1, 4).g + (int)flat->colorAt(1, 4).b;
	EXPECT_EQ(flatTop, flatLeft);
}

TEST_F(ImageUtilsTest, testRenderIsometricMaterialsTransparency) {
	palette::Palette palette;
	palette.setColor(1, color::RGBA(200, 200, 200, 128));
	palette.setEmit(1, 1.0f);
	voxel::RawVolume volume(voxel::Region(0, 0));
	volume.setVoxel(0, 0, 0, voxel::createVoxel(voxel::VoxelType::Generic, 1));

	IsoRenderConfig plain;
	plain.materials = false;
	plain.transparency = false;
	plain.background = color::RGBA(0, 0, 255, 255);
	const image::ImagePtr opaque = renderIsometricImage(&volume, palette, plain);
	ASSERT_TRUE(opaque && opaque->isLoaded());
	EXPECT_EQ(opaque->colorAt(3, 1).a, 255);

	IsoRenderConfig emitOn = plain;
	emitOn.materials = true;
	emitOn.shading = true;
	IsoRenderConfig emitOff = emitOn;
	emitOff.materials = false;
	const image::ImagePtr lit = renderIsometricImage(&volume, palette, emitOn);
	const image::ImagePtr unlit = renderIsometricImage(&volume, palette, emitOff);
	ASSERT_TRUE(lit && unlit);
	const int litRight = (int)lit->colorAt(6, 4).r;
	const int unlitRight = (int)unlit->colorAt(6, 4).r;
	EXPECT_GT(litRight, unlitRight);

	IsoRenderConfig alphaOn = plain;
	alphaOn.transparency = true;
	alphaOn.background = color::RGBA(0, 0, 255, 0);
	const image::ImagePtr blended = renderIsometricImage(&volume, palette, alphaOn);
	ASSERT_TRUE(blended && blended->isLoaded());
	const color::RGBA px = blended->colorAt(3, 1);
	EXPECT_LT(px.a, 255);
	EXPECT_GT((int)px.a, 0);
}

TEST_F(ImageUtilsTest, testIsoRenderConfigFromCvars) {
	core::Var::registerVar(core::VarDef(cfg::ImageIsoShading, true, "Iso shading", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageIsoMaterials, true, "Iso materials", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageIsoTransparency, true, "Iso transparency", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageBackground, "0 0 1 1", "Background", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageWidth, -1, "Image width", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageHeight, -1, "Image height", "help"));
	core::Var::registerVar(core::VarDef(cfg::ImageUpScale, true, "Upscale", "help"));
	core::getVar(cfg::ImageIsoShading)->setVal(false);
	core::getVar(cfg::ImageWidth)->setVal(48);
	const IsoRenderConfig config = IsoRenderConfig::fromCvars();
	EXPECT_FALSE(config.shading);
	EXPECT_TRUE(config.materials);
	EXPECT_TRUE(config.transparency);
	EXPECT_EQ(config.imgW, 48);
	EXPECT_EQ(config.background.b, 255);
	core::getVar(cfg::ImageIsoShading)->setVal(true);
	core::getVar(cfg::ImageWidth)->setVal(-1);
	core::getVar(cfg::ImageBackground)->setVal("0 0 0 0");
}

TEST_F(ImageUtilsTest, testApplyTextureToFace) {
	const image::ImagePtr &image = image::loadImage("test-palette-in.png");
	ASSERT_TRUE(image && image->isLoaded());
	voxel::Region region(0, 0, 0, image->width() - 1, image->height() - 1, 1);
	voxel::RawVolume volume(region);
	palette::Palette palette;
	palette.nippon();
	voxel::FaceNames faceName = voxel::FaceNames::PositiveZ;
	voxelutil::applyTextureToFace(volume, region, palette, faceName, image, glm::vec2(0.0f, 1.0f),
								  glm::vec2(1.0f, 0.0f));
	EXPECT_EQ(1870, voxelutil::countVoxels(volume));
}

TEST_F(ImageUtilsTest, testRenderFaceToImage) {
	palette::Palette palette;
	palette.nippon();
	const voxel::Region region(0, 0, 0, 3, 3, 3);
	voxel::RawVolume volume(region);

	// fill the volume with voxels of different colors
	for (int x = 0; x <= 3; ++x) {
		for (int y = 0; y <= 3; ++y) {
			for (int z = 0; z <= 3; ++z) {
				const uint8_t colorIdx = (uint8_t)((x + y * 4 + z * 16) % 255 + 1);
				volume.setVoxel(x, y, z, voxel::createVoxel(voxel::VoxelType::Generic, colorIdx));
			}
		}
	}

	const image::ImagePtr &img = renderFaceToImage(&volume, palette, region, voxel::FaceNames::Front);
	ASSERT_TRUE(img && img->isLoaded());
	// Front face (PositiveZ): image dimensions should be x * y = 4 * 4
	EXPECT_EQ(4, img->width());
	EXPECT_EQ(4, img->height());

	// verify that the image contains non-transparent pixels
	bool hasNonTransparent = false;
	for (int x = 0; x < img->width(); ++x) {
		for (int y = 0; y < img->height(); ++y) {
			const color::RGBA c = img->colorAt(x, y);
			if (c.a > 0) {
				hasNonTransparent = true;
				break;
			}
		}
		if (hasNonTransparent) {
			break;
		}
	}
	EXPECT_TRUE(hasNonTransparent);
}

} // namespace voxelutil
