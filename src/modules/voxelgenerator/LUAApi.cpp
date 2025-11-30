/**
 * @file
 */

#include "LUAApi.h"
#include "app/App.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "color/Color.h"
#include "core/StringUtil.h"
#include "core/UTF8.h"
#include "image/Image.h"
#include "io/FilesystemArchive.h"
#include "io/Stream.h"
#include "io/StreamArchive.h"
#include "math/Axis.h"
#include "math/Random.h"
#include "noise/Simplex.h"
#include "palette/PaletteFormatDescription.h"
#include "palette/Palette.h"
#include "palette/PaletteLookup.h"
#include "scenegraph/SceneGraph.h"
#include "scenegraph/SceneGraphAnimation.h"
#include "scenegraph/SceneGraphKeyFrame.h"
#include "scenegraph/SceneGraphNode.h"
#include "scenegraph/SceneGraphTransform.h"
#include "scenegraph/SceneGraphUtil.h"
#include "voxel/MaterialColor.h"
#include "voxel/RawVolume.h"
#include "voxel/RawVolumeMoveWrapper.h"
#include "voxel/RawVolumeWrapper.h"
#include "voxel/Region.h"
#include "voxel/Voxel.h"
#include "voxelfont/VoxelFont.h"
#include "voxelformat/Format.h"
#include "voxelformat/VolumeFormat.h"
#include "voxelgenerator/Genland.h"
#include "voxelgenerator/ShapeGenerator.h"
#include "voxelutil/FillHollow.h"
#include "voxelutil/Hollow.h"
#include "voxelutil/ImageUtils.h"
#include "voxelutil/Shadow.h"
#include "voxelutil/VolumeCropper.h"
#include "voxelutil/VolumeMover.h"
#include "voxelutil/VolumeResizer.h"
#include "voxelutil/VolumeRotator.h"
#include "voxelutil/VoxelUtil.h"

#define GENERATOR_LUA_SANTITY 1

namespace voxelgenerator {

struct LuaSceneGraphNode {
	scenegraph::SceneGraphNode *node;
	LuaSceneGraphNode(scenegraph::SceneGraphNode *_node) : node(_node) {
	}
};

struct LuaKeyFrame {
	scenegraph::SceneGraphNode *node;
	scenegraph::KeyFrameIndex keyFrameIdx;
	LuaKeyFrame(scenegraph::SceneGraphNode *_node, scenegraph::KeyFrameIndex _keyFrameIdx) : node(_node), keyFrameIdx(_keyFrameIdx) {
	}

	scenegraph::SceneGraphKeyFrame &keyFrame() {
		return node->keyFrame(keyFrameIdx);
	}
};

/**
 * @brief This wrapper is able to replace the whole volume in the node if some action replaced the volume to operate on.
 */
class LuaRawVolumeWrapper : public voxel::RawVolumeWrapper {
private:
	using Super = voxel::RawVolumeWrapper;
	scenegraph::SceneGraphNode *_node;
public:
	LuaRawVolumeWrapper(scenegraph::SceneGraphNode *node) : Super(node->volume()), _node(node) {
	}

	~LuaRawVolumeWrapper() {
		update();
	}

	scenegraph::SceneGraphNode *node() const {
		return _node;
	}

	void update() {
		if (_node->volume() == volume()) {
			return;
		}
		_node->setVolume(volume(), true);
	}
};

static const char *luaVoxel_globalscenegraph() {
	return "__global_scenegraph";
}

static const char *luaVoxel_globalnodeid() {
	return "__global_nodeid";
}

static const char *luaVoxel_globalnoise() {
	return "__global_noise";
}

static const char *luaVoxel_globaldirtyregion() {
	return "__global_region";
}

static const char *luaVoxel_metascenegraphnode() {
	return "__meta_scenegraphnode";
}

static const char *luaVoxel_metascenegraph() {
	return "__meta_scenegraph";
}

static const char *luaVoxel_metaregionglobal() {
	return "__meta_sceneregionglobal";
}

static const char *luaVoxel_metaregion_gc() {
	return "__meta_region_gc";
}

static const char *luaVoxel_metakeyframe() {
	return "__meta_keyframe";
}

static const char *luaVoxel_metavolumewrapper() {
	return "__meta_volumewrapper";
}

static const char *luaVoxel_metapaletteglobal() {
	return "__meta_palette_global";
}

static const char *luaVoxel_metapalette() {
	return "__meta_palette";
}

static const char *luaVoxel_metapalette_gc() {
	return "__meta_palette_gc";
}

static const char *luaVoxel_metanoise() {
	return "__meta_noise";
}

static const char *luaVoxel_metashape() {
	return "__meta_shape";
}

static const char *luaVoxel_metaimporter() {
	return "__meta_importer";
}

static const char *luaVoxel_metaalgorithm() {
	return "__meta_algorithm";
}

static inline const char *luaVoxel_metaregion() {
	return "__meta_region";
}

static void luaVoxel_newGlobalData(lua_State *L, const core::String& prefix, void *userData) {
	lua_pushlightuserdata(L, userData);
	lua_setglobal(L, prefix.c_str());
}

template<class T>
static T* luaVoxel_globalData(lua_State *L, const core::String& prefix) {
	lua_getglobal(L, prefix.c_str());
	T* data = (T*) lua_touserdata(L, -1);
	lua_pop(L, 1);
	return data;
}

template<int N, class T>
static glm::vec<N, T> luaVoxel_getvec(lua_State *s, int idx) {
	glm::vec<N, T> val;
	if (clua_isvec<decltype(val)>(s, idx)) {
		val = clua_tovec<decltype(val)>(s, idx);
	} else {
		val[0] = (T)luaL_checknumber(s, idx);
		if constexpr (N > 1)
			val[1] = (T)luaL_optnumber(s, idx + 1, val[0]);
		if constexpr (N > 2)
			val[2] = (T)luaL_optnumber(s, idx + 2, val[1]);
		if constexpr (N > 3)
			val[3] = (T)luaL_optnumber(s, idx + 3, val[2]);
	}
	return val;
}

static scenegraph::SceneGraph *luaVoxel_scenegraph(lua_State *s) {
	return luaVoxel_globalData<scenegraph::SceneGraph>(s, luaVoxel_globalscenegraph());
}

static bool luaVoxel_isregion(lua_State *s, int n) {
	return luaL_testudata(s, n, luaVoxel_metaregion()) != nullptr ||
		   luaL_testudata(s, n, luaVoxel_metaregion_gc()) != nullptr;
}

static voxel::Region* luaVoxel_toregion(lua_State* s, int n) {
	voxel::Region** region = (voxel::Region**)luaL_testudata(s, n, luaVoxel_metaregion_gc());
	if (region != nullptr) {
		return *region;
	}
	return *(voxel::Region**)clua_getudata<voxel::Region*>(s, n, luaVoxel_metaregion());
}

static int luaVoxel_pushregion(lua_State* s, const voxel::Region& region) {
	return clua_pushudata(s, new voxel::Region(region), luaVoxel_metaregion_gc());
}

static LuaSceneGraphNode* luaVoxel_toscenegraphnode(lua_State* s, int n) {
	return *(LuaSceneGraphNode**)clua_getudata<LuaSceneGraphNode*>(s, n, luaVoxel_metascenegraphnode());
}

static int luaVoxel_pushscenegraphnode(lua_State* s, scenegraph::SceneGraphNode& node) {
	LuaSceneGraphNode *wrapper = new LuaSceneGraphNode(&node);
	return clua_pushudata(s, wrapper, luaVoxel_metascenegraphnode());
}

static bool luaVoxel_ispalette(lua_State *s, int n) {
	return luaL_testudata(s, n, luaVoxel_metapalette()) != nullptr ||
		   luaL_testudata(s, n, luaVoxel_metapalette_gc()) != nullptr;
}

static palette::Palette* luaVoxel_toPalette(lua_State* s, int n) {
	palette::Palette** palette = (palette::Palette**)luaL_testudata(s, n, luaVoxel_metapalette_gc());
	if (palette != nullptr) {
		return *palette;
	}
	return *(palette::Palette**)clua_getudata<palette::Palette*>(s, n, luaVoxel_metapalette());
}

static int luaVoxel_pushpalette(lua_State* s, palette::Palette& palette) {
	return clua_pushudata(s, &palette, luaVoxel_metapalette());
}

static int luaVoxel_pushpalette(lua_State* s, palette::Palette* palette) {
	if (palette == nullptr) {
		return clua_error(s, "No palette given - can't push");
	}
	return clua_pushudata(s, palette, luaVoxel_metapalette_gc());
}

static int luaVoxel_pushkeyframe(lua_State* s, scenegraph::SceneGraphNode& node, scenegraph::KeyFrameIndex keyFrameIdx) {
	LuaKeyFrame *keyframe = new LuaKeyFrame(&node, keyFrameIdx);
	return clua_pushudata(s, keyframe, luaVoxel_metakeyframe());
}

static LuaKeyFrame* luaVoxel_tokeyframe(lua_State* s, int n) {
	return *(LuaKeyFrame**)clua_getudata<LuaKeyFrame*>(s, n, luaVoxel_metakeyframe());
}

static LuaRawVolumeWrapper* luaVoxel_tovolumewrapper(lua_State* s, int n) {
	return *(LuaRawVolumeWrapper**)clua_getudata<LuaRawVolumeWrapper*>(s, n, luaVoxel_metavolumewrapper());
}

static int luaVoxel_pushvolumewrapper(lua_State* s, LuaSceneGraphNode* node) {
	if (node == nullptr) {
		return clua_error(s, "No node given - can't push");
	}
	LuaRawVolumeWrapper *wrapper = new LuaRawVolumeWrapper(node->node);
	return clua_pushudata(s, wrapper, luaVoxel_metavolumewrapper());
}

static int luaVoxel_volumewrapper_voxel(lua_State* s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
	const voxel::Voxel& voxel = volume->voxel(x, y, z);
	if (voxel::isAir(voxel.getMaterial())) {
		lua_pushinteger(s, -1);
	} else {
		lua_pushinteger(s, voxel.getColor());
	}
	return 1;
}

static int luaVoxel_volumewrapper_region(lua_State* s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	return luaVoxel_pushregion(s, volume->region());
}

static int luaVoxel_volumewrapper_translate(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_optinteger(s, 3, 0);
	const int z = (int)luaL_optinteger(s, 4, 0);
	volume->volume()->translate(glm::ivec3(x, y, z));
	return 0;
}

static int luaVoxel_volumewrapper_move(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_optinteger(s, 3, 0);
	const int z = (int)luaL_optinteger(s, 4, 0);

	voxel::RawVolume* newVolume = new voxel::RawVolume(volume->region());
	voxel::RawVolumeMoveWrapper wrapper(newVolume);
	glm::ivec3 offsets(x, y, z);
	voxelutil::moveVolume(&wrapper, volume, offsets);
	volume->setVolume(newVolume);
	return 0;
}

static int luaVoxel_volumewrapper_resize(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const int w = (int)luaL_checkinteger(s, 2);
	const int h = (int)luaL_optinteger(s, 3, 0);
	const int d = (int)luaL_optinteger(s, 4, 0);
	const bool extendMins = (int)clua_optboolean(s, 5, false);
	voxel::RawVolume *v = voxelutil::resize(volume->volume(), glm::ivec3(w, h, d), extendMins);
	if (v == nullptr) {
		return clua_error(s, "Failed to resize the volume");
	}
	volume->setVolume(v);
	volume->update();
	return 0;
}

static voxel::Voxel luaVoxel_getVoxel(lua_State *s, int index, int defaultColor = 1) {
	const int color = (int)luaL_optinteger(s, index, defaultColor);
	if (color == -1) {
		return voxel::createVoxel(voxel::VoxelType::Air, 0);
	}
	return voxel::createVoxel(voxel::VoxelType::Generic, color);
}

static math::Axis luaVoxel_getAxis(lua_State *s, int index) {
	const char* axis = luaL_optstring(s, index, "y");
	return math::toAxis(axis);
}

static int luaVoxel_volumewrapper_mirroraxis(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::mirrorAxis(volume->volume(), luaVoxel_getAxis(s, 2));
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_rotateaxis(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::rotateAxis(volume->volume(), luaVoxel_getAxis(s, 2));
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_fillhollow(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 2);
	voxelutil::fillHollow(*volume, voxel);
	return 0;
}

static int luaVoxel_volumewrapper_hollow(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxelutil::hollow(*volume);
	return 0;
}

static int luaVoxel_volumewrapper_importimageasvolume(lua_State *s) {
	int idx = 1;
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, idx++);
	const core::String nameTexture = luaL_checkstring(s, idx++);
	const core::String nameDepthMap =
		lua_isstring(s, idx) ? luaL_checkstring(s, idx++) : voxelutil::getDefaultDepthMapFile(nameTexture);
	const image::ImagePtr imageTexture = image::loadImage(nameTexture);
	const image::ImagePtr imageDepthMap = image::loadImage(nameDepthMap);
	const bool hasPalette = luaVoxel_ispalette(s, idx);
	const palette::Palette *palette = hasPalette ? luaVoxel_toPalette(s, idx++) : &voxel::getPalette();
	const uint8_t thickness = (int)luaL_optinteger(s, idx++, 8);
	const bool bothSides = (int)clua_optboolean(s, idx++, false);
	voxel::RawVolume *v = voxelutil::importAsVolume(imageTexture, imageDepthMap, *palette, thickness, bothSides);
	if (v == nullptr) {
		return clua_error(s, "Failed to import image as volume from image %s", nameTexture.c_str());
	}
	volume->setVolume(v);
	volume->update();
	return 0;
}

static int luaVoxel_volumewrapper_importheightmap(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const core::String imageName = lua_tostring(s, 2);
	const image::ImagePtr &image = image::loadImage(imageName);
	if (!image || !image->isLoaded()) {
		return clua_error(s, "Image %s could not get loaded", imageName.c_str());
	}
	const voxel::Voxel dirt = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	const voxel::Voxel underground = luaVoxel_getVoxel(s, 3, dirt.getColor());
	const voxel::Voxel grass = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	const voxel::Voxel surface = luaVoxel_getVoxel(s, 4, grass.getColor());
	voxelutil::importHeightmap(*volume, image, underground, surface);
	return 0;
}

static int luaVoxel_volumewrapper_importcoloredheightmap(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const core::String imageName = lua_tostring(s, 2);
	const image::ImagePtr &image = image::loadImage(imageName);
	if (!image || !image->isLoaded()) {
		return clua_error(s, "Image %s could not get loaded", imageName.c_str());
	}
	const voxel::Voxel dirt = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	const voxel::Voxel underground = luaVoxel_getVoxel(s, 3, dirt.getColor());
	voxelutil::importColoredHeightmap(*volume, volume->node()->palette(), image, underground);
	return 0;
}

static int luaVoxel_volumewrapper_crop(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	voxel::RawVolume* v = voxelutil::cropVolume(volume->volume());
	if (v != nullptr) {
		volume->setVolume(v);
		volume->update();
	}
	return 0;
}

static int luaVoxel_volumewrapper_text(lua_State *s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const voxel::Region &region = volume->region();
	const char *ttffont = lua_tostring(s, 2);
	const char *text = lua_tostring(s, 3);
	int x = (int)luaL_optinteger(s, 4, region.getLowerX());
	const int y = (int)luaL_optinteger(s, 5, region.getLowerY());
	const int z = (int)luaL_optinteger(s, 6, region.getLowerZ());
	const int size = (int)luaL_optinteger(s, 7, 16);
	const int thickness = (int)luaL_optinteger(s, 8, 1);
	const int spacing = (int)luaL_optinteger(s, 9, 0);
	voxelfont::VoxelFont font;
	if (!font.init(ttffont)) {
		clua_error(s, "Could not initialize font %s", ttffont);
	}
	const char **str = &text;
	glm::ivec3 pos(x, y, z);
	const voxel::Voxel voxel = voxel::createVoxel(voxel::VoxelType::Generic, 0);
	for (int c = core::utf8::next(str); c != -1; c = core::utf8::next(str)) {
		pos.x += font.renderCharacter(c, size, thickness, pos, *volume, voxel);
		pos.x += spacing;
	}
	font.shutdown();
	return 0;
}

static int luaVoxel_volumewrapper_setvoxel(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_checkinteger(s, 3);
	const int z = (int)luaL_checkinteger(s, 4);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 5);
	const bool insideRegion = volume->setVoxel(x, y, z, voxel);
	lua_pushboolean(s, insideRegion ? 1 : 0);
	return 1;
}

static int luaVoxel_volumewrapper_gc(lua_State *s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	if (volume->dirtyRegion().isValid()) {
		voxel::Region* dirtyRegion = luaVoxel_globalData<voxel::Region>(s, luaVoxel_globaldirtyregion());
		if (dirtyRegion->isValid()) {
			dirtyRegion->accumulate(volume->dirtyRegion());
		} else {
			*dirtyRegion = volume->dirtyRegion();
		}
	}
	delete volume;
	return 0;
}

static int luaVoxel_shape_cylinder(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::vec3& centerBottom = clua_tovec<glm::vec3>(s, 2);
	const math::Axis axis = luaVoxel_getAxis(s, 3);
	const int radius = (int)luaL_checkinteger(s, 4);
	const int height = (int)luaL_checkinteger(s, 5);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 6);
	shape::createCylinder(*volume, centerBottom, axis, radius, height, voxel);
	return 0;
}

static int luaVoxel_shape_torus(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& center = clua_tovec<glm::ivec3>(s, 2);
	const int minorRadius = (int)luaL_checkinteger(s, 3);
	const int majorRadius = (int)luaL_checkinteger(s, 4);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 5);
	shape::createTorus(*volume, center, minorRadius, majorRadius, voxel);
	return 0;
}

static int luaVoxel_shape_ellipse(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& centerBottom = clua_tovec<glm::ivec3>(s, 2);
	const math::Axis axis = luaVoxel_getAxis(s, 3);
	const int width = (int)luaL_checkinteger(s, 4);
	const int height = (int)luaL_checkinteger(s, 5);
	const int depth = (int)luaL_checkinteger(s, 6);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 7);
	shape::createEllipse(*volume, centerBottom, axis, width, height, depth, voxel);
	return 0;
}

static int luaVoxel_shape_dome(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& centerBottom = clua_tovec<glm::ivec3>(s, 2);
	const math::Axis axis = luaVoxel_getAxis(s, 3);
	const bool negative = (int)clua_optboolean(s, 4, false);
	const int width = (int)luaL_checkinteger(s, 5);
	const int height = (int)luaL_checkinteger(s, 6);
	const int depth = (int)luaL_checkinteger(s, 7);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 8);
	shape::createDome(*volume, centerBottom, axis, negative, width, height, depth, voxel);
	return 0;
}

static int luaVoxel_shape_cube(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& position = clua_tovec<glm::ivec3>(s, 2);
	const int width = (int)luaL_checkinteger(s, 3);
	const int height = (int)luaL_checkinteger(s, 4);
	const int depth = (int)luaL_checkinteger(s, 5);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 6);
	shape::createCubeNoCenter(*volume, position, width, height, depth, voxel);
	return 0;
}

static int luaVoxel_shape_cone(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& centerBottom = clua_tovec<glm::ivec3>(s, 2);
	const math::Axis axis = luaVoxel_getAxis(s, 3);
	const bool negative = (int)clua_optboolean(s, 4, false);
	const int width = (int)luaL_checkinteger(s, 5);
	const int height = (int)luaL_checkinteger(s, 6);
	const int depth = (int)luaL_checkinteger(s, 7);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 8);
	shape::createCone(*volume, centerBottom, axis, negative, width, height, depth, voxel);
	return 0;
}

static int luaVoxel_shape_line(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& start = clua_tovec<glm::ivec3>(s, 2);
	const glm::ivec3& end = clua_tovec<glm::ivec3>(s, 3);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 4);
	shape::createLine(*volume, start, end, voxel);
	return 0;
}

static int luaVoxel_shape_bezier(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& start = clua_tovec<glm::ivec3>(s, 2);
	const glm::ivec3& end = clua_tovec<glm::ivec3>(s, 3);
	const glm::ivec3& control = clua_tovec<glm::ivec3>(s, 4);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 5);
	shape::createBezierFunc(*volume, start, end, control, voxel, [&] (LuaRawVolumeWrapper& vol, const glm::ivec3& last, const glm::ivec3& pos, const voxel::Voxel& v) {
		shape::createLine(vol, pos, last, v, 1);
	});
	return 0;
}

static int luaVoxel_load_palette(lua_State *s) {
	const char *filename = luaL_checkstring(s, 1);
	io::SeekableReadStream *readStream = clua_tostream(s, 2);
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	voxelformat::LoadContext ctx;
	palette::Palette *palette = new palette::Palette();
	bool ret;
	{
		auto archive = core::make_shared<io::StreamArchive>(readStream);
		ret = voxelformat::loadPalette(filename, archive, *palette, ctx);
	}
	if (!ret) {
		delete palette;
		return clua_error(s, "Could not load palette %s from string", filename);
	}
	return luaVoxel_pushpalette(s, palette);
}

static int luaVoxel_load_image(lua_State *s) {
	const char *filename = luaL_checkstring(s, 1);
	io::SeekableReadWriteStream *readStream = clua_tostream(s, 2);
	image::Image* image = new image::Image(filename);
	if (!image->load(image::ImageType::Unknown, *readStream, readStream->size())) {
		delete image;
		return clua_error(s, "Image %s could not get loaded from stream", filename);
	}
	return clua_pushimage(s, image);
}

static int luaVoxel_import_imageasplane(lua_State *s) {
	const image::Image* image = clua_toimage(s, 1);
	const palette::Palette *palette = luaVoxel_toPalette(s, 2);
	const int thickness = (int)luaL_optinteger(s, 3, 1);
	voxel::RawVolume* v = voxelutil::importAsPlane(image, *palette, thickness);
	if (v == nullptr) {
		return clua_error(s, "Failed to import image as plane");
	}
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName(image->name());
	int newNodeId = sceneGraph->emplace(core::move(node));
	if (newNodeId == InvalidNodeId) {
		return clua_error(s, "Failed to add plane node to scene graph");
	}
	return luaVoxel_pushscenegraphnode(s, sceneGraph->node(newNodeId));
}

static int luaVoxel_import_scene(lua_State *s) {
	const char *filename = luaL_checkstring(s, 1);
	io::SeekableReadStream *readStream = nullptr;
	if (clua_isstream(s, 2)) {
		readStream = clua_tostream(s, 2);
	}
	io::FileDescription fileDesc;
	fileDesc.set(filename);
	voxelformat::LoadContext ctx;
	scenegraph::SceneGraph newSceneGraph;
	bool ret;
	{
		auto archive = readStream ? core::make_shared<io::StreamArchive>(readStream) : io::openFilesystemArchive(io::filesystem());
		ret = voxelformat::loadFormat(fileDesc, archive, newSceneGraph, ctx);
	}
	if (!ret) {
		newSceneGraph.~SceneGraph();
		return clua_error(s, "Could not load file %s", filename);
	}
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);
	if (scenegraph::addSceneGraphNodes(*sceneGraph, newSceneGraph, sceneGraph->root().id()) <= 0) {
		newSceneGraph.~SceneGraph();
		return clua_error(s, "Could not import scene graph nodes");
	}
	return 0;
}

static int luaVoxel_palette_eq(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const palette::Palette *palette2 = luaVoxel_toPalette(s, 2);
	lua_pushboolean(s, palette->hash() == palette2->hash());
	return 1;
}

static int luaVoxel_palette_gc(lua_State *s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	delete palette;
	return 0;
}

static int luaVoxel_palette_size(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	lua_pushinteger(s, palette->colorCount());
	return 1;
}

static int luaVoxel_palette_colors(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	lua_createtable(s, palette->colorCount(), 0);
	for (int i = 0; i < palette->colorCount(); ++i) {
		const glm::vec4& c = core::Color::fromRGBA(palette->color(i));
		lua_pushinteger(s, i + 1);
		clua_push(s, c);
		lua_settable(s, -3);
	}
	return 1;
}

static int luaVoxel_palette_load(lua_State* s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const char *filename = luaL_checkstring(s, 2);
	if (!palette->load(filename)) {
		core::String builtInPalettes;
		for (int i = 0; i < lengthof(palette::Palette::builtIn); ++i) {
			if (!builtInPalettes.empty()) {
				builtInPalettes.append(", ");
			}
			builtInPalettes.append(palette::Palette::builtIn[i]);
		}
		core::String supportedPaletteFormats;
		for (const io::FormatDescription *desc = palette::palettes(); desc->valid(); ++desc) {
			for (const core::String& ext : desc->exts) {
				if (!supportedPaletteFormats.empty()) {
					supportedPaletteFormats.append(", ");
				}
				supportedPaletteFormats.append(ext);
			}
		}
		return clua_error(s, "Could not load palette %s, built-in palettes are: %s, supported formats are: %s",
						  filename, builtInPalettes.c_str(), supportedPaletteFormats.c_str());
	}
	return 0;
}

static int luaVoxel_palette_rgba(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t color = luaL_checkinteger(s, 2);
	const core::RGBA rgba = palette->color(color);
	lua_pushinteger(s, rgba.r);
	lua_pushinteger(s, rgba.g);
	lua_pushinteger(s, rgba.b);
	lua_pushinteger(s, rgba.a);
	return 4;
}

static int luaVoxel_palette_color(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t color = luaL_checkinteger(s, 2);
	const glm::vec4& rgba = core::Color::fromRGBA(palette->color(color));
	return clua_push(s, rgba);
}

static int luaVoxel_palette_setcolor(lua_State* s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t color = luaL_checkinteger(s, 2);
	const uint8_t r = luaL_checkinteger(s, 3);
	const uint8_t g = luaL_checkinteger(s, 4);
	const uint8_t b = luaL_checkinteger(s, 5);
	const uint8_t a = luaL_optinteger(s, 6, 255);
	palette->setColor(color, core::RGBA(r, g, b, a));
	return 0;
}

static int luaVoxel_palette_setmaterialproperty(lua_State* s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t idx = luaL_checkinteger(s, 2);
	const char *name = luaL_checkstring(s, 3);
	const float value = (float)luaL_checknumber(s, 4);
	palette->setMaterialProperty(idx, name, value);
	return 0;
}

static int luaVoxel_palette_materialproperty(lua_State* s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t idx = luaL_checkinteger(s, 2);
	const char *name = luaL_checkstring(s, 3);
	const float value = palette->materialProperty(idx, name);
	lua_pushnumber(s, value);
	return 1;
}

static int luaVoxel_palette_delta_e(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t idx = luaL_checkinteger(s, 2);
	const uint8_t idx2 = luaL_checkinteger(s, 3);
	float distance = 0.0f;
	if (idx != idx2) {
		const core::RGBA c1 = palette->color(idx);
		const core::RGBA c2 = palette->color(idx2);
		distance = core::Color::deltaE76(c1, c2);
	}
	lua_pushnumber(s, distance);
	return 1;
}

static int luaVoxel_palette_color_to_string(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t idx = luaL_checkinteger(s, 2);
	const core::String c = core::Color::print(palette->color(idx));
	lua_pushstring(s, c.c_str());
	return 1;
}

static int luaVoxel_palette_tostring(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const core::String pal = palette::Palette::print(*palette);
	lua_pushfstring(s, "%s", pal.c_str());
	return 1;
}

static int luaVoxel_palette_closestmatch(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const float r = (float)luaL_checkinteger(s, 2) / 255.0f;
	const float g = (float)luaL_checkinteger(s, 3) / 255.0f;
	const float b = (float)luaL_checkinteger(s, 4) / 255.0f;
	const int skipIndex = luaL_optinteger(s, 5, -1);
	core::RGBA rgba(r * 255.0f, g * 255.0f, b * 255.0f, 255);
	const int match = palette->getClosestMatch(rgba, skipIndex);
	if (match < 0 || match > palette->colorCount()) {
		return clua_error(s, "Given color index is not valid or palette is not loaded");
	}
	lua_pushinteger(s, match);
	return 1;
}

static int luaVoxel_palette_new(lua_State* s) {
	return luaVoxel_pushpalette(s, new palette::Palette());
}

static int luaVoxel_palette_similar(lua_State* s) {
	const palette::Palette *pal = luaVoxel_toPalette(s, 1);
	palette::Palette palette = *pal;
	const int paletteIndex = lua_tointeger(s, 2);
	const int colorCount = lua_tointeger(s, 3);
	if (paletteIndex < 0 || paletteIndex >= palette.colorCount()) {
		return clua_error(s, "Palette index out of bounds");
	}
	core::Buffer<uint8_t> newColorIndices;
	newColorIndices.resize(colorCount);
	int maxColorIndices = 0;
	for (; maxColorIndices < colorCount; ++maxColorIndices) {
		const int materialIndex = palette.getClosestMatch(palette.color(paletteIndex), paletteIndex);
		if (materialIndex <= palette::PaletteColorNotFound) {
			break;
		}
		palette.setColor(materialIndex, core::RGBA(0));
		newColorIndices[maxColorIndices] = materialIndex;
	}
	if (maxColorIndices <= 0) {
		lua_pushnil(s);
		return 1;
	}

	lua_createtable(s, (int)newColorIndices.size(), 0);
	for (size_t i = 0; i < newColorIndices.size(); ++i) {
		lua_pushinteger(s, (int)i + 1);
		lua_pushinteger(s, newColorIndices[i]);
		lua_settable(s, -3);
	}

	return 1;
}

static int luaVoxel_region_width(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getWidthInVoxels());
	return 1;
}

static int luaVoxel_region_height(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getHeightInVoxels());
	return 1;
}

static int luaVoxel_region_depth(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getDepthInVoxels());
	return 1;
}

static int luaVoxel_region_x(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getLowerX());
	return 1;
}

static int luaVoxel_region_y(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getLowerY());
	return 1;
}

static int luaVoxel_region_z(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	lua_pushinteger(s, region->getLowerZ());
	return 1;
}

static int luaVoxel_region_center(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	clua_push(s, region->getCenter());
	return 1;
}

static int luaVoxel_region_isonborder(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	const glm::ivec3& pos = clua_tovec<glm::ivec3>(s, 2);
	lua_pushboolean(s, region->isOnBorder(pos));
	return 1;
}

static int luaVoxel_region_mins(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	clua_push(s, region->getLowerCorner());
	return 1;
}

static int luaVoxel_region_maxs(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	clua_push(s, region->getUpperCorner());
	return 1;
}

static int luaVoxel_region_size(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	clua_push(s, region->getDimensionsInVoxels());
	return 1;
}

static int luaVoxel_region_intersects(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	const voxel::Region* region2 = luaVoxel_toregion(s, 2);
	lua_pushboolean(s, voxel::intersects(*region, *region2));
	return 1;
}

static int luaVoxel_region_contains(lua_State* s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	const voxel::Region* region2 = luaVoxel_toregion(s, 2);
	lua_pushboolean(s, region->containsRegion(*region2));
	return 1;
}

static int luaVoxel_region_setmins(lua_State* s) {
	voxel::Region* region = luaVoxel_toregion(s, 1);
	const glm::ivec3& mins = clua_tovec<glm::ivec3>(s, 2);
	region->setLowerCorner(mins);
	return 0;
}

static int luaVoxel_region_setmaxs(lua_State* s) {
	voxel::Region* region = luaVoxel_toregion(s, 1);
	const glm::ivec3& maxs = clua_tovec<glm::ivec3>(s, 2);
	region->setUpperCorner(maxs);
	return 0;
}

static int luaVoxel_region_tostring(lua_State *s) {
	const voxel::Region* region = luaVoxel_toregion(s, 1);
	const glm::ivec3& mins = region->getLowerCorner();
	const glm::ivec3& maxs = region->getUpperCorner();
	lua_pushfstring(s, "region: [%d:%d:%d]/[%d:%d:%d]", mins.x, mins.y, mins.z, maxs.x, maxs.y, maxs.z);
	return 1;
}

static glm::vec2 to_vec2(lua_State*s, int &n) {
	if (clua_isvec<glm::vec2>(s, n)) {
		return clua_tovec<glm::vec2>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	return glm::vec2(x, y);
}

static glm::vec3 to_vec3(lua_State*s, int &n) {
	if (clua_isvec<glm::vec3>(s, n)) {
		return clua_tovec<glm::vec3>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	const float z = (float)luaL_optnumber(s, n++, y);
	return glm::vec3(x, y, z);
}

static glm::vec4 to_vec4(lua_State*s, int &n) {
	if (clua_isvec<glm::vec4>(s, n)) {
		return clua_tovec<glm::vec4>(s, n);
	}
	const float x = (float)lua_tonumber(s, n++);
	const float y = (float)luaL_optnumber(s, n++, x);
	const float z = (float)luaL_optnumber(s, n++, y);
	const float w = (float)luaL_optnumber(s, n++, z);
	return glm::vec4(x, y, z, w);
}

static int luaVoxel_noise_simplex2(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec2(s, n)));
	return 1;
}

static int luaVoxel_noise_simplex3(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec3(s, n)));
	return 1;
}

static int luaVoxel_noise_simplex4(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::noise(to_vec4(s, n)));
	return 1;
}

static int luaVoxel_noise_fBm2(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec2(s, n), octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_fBm3(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec3(s, n), octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_fBm4(lua_State* s) {
	int n = 1;
	const uint8_t octaves = luaL_optinteger(s, n + 1, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 2, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 3, 0.5f);
	lua_pushnumber(s, noise::fBm(to_vec4(s, n), octaves, lacunarity, gain));
	return 1;
}

static noise::Noise *luaVoxel_globalnoise(lua_State* s) {
	return luaVoxel_globalData<noise::Noise>(s, luaVoxel_globalnoise());
}

static int luaVoxel_noise_voronoi(lua_State* s) {
	noise::Noise* noise = luaVoxel_globalnoise(s);
	int n = 1;
	const glm::vec3 v = to_vec3(s, n);
	const float frequency = (float)luaL_optnumber(s, n + 1, 1.0f);
	const int seed = (int)luaL_optinteger(s, n + 2, 0);
	bool enableDistance = clua_optboolean(s, n + 3, true);
	lua_pushnumber(s, noise->voronoi(v, enableDistance, frequency, seed));
	return 1;
}

static int luaVoxel_noise_swissturbulence(lua_State* s) {
	noise::Noise* noise = luaVoxel_globalnoise(s);
	int n = 1;
	const glm::vec2 v = to_vec2(s, n);
	const float offset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.6f);
	const float warp = (float)luaL_optnumber(s, n + 5, 0.15f);
	lua_pushnumber(s, noise->swissTurbulence(v, offset, octaves, lacunarity, gain, warp));
	return 1;
}

static int luaVoxel_noise_ridgedMF2(lua_State* s) {
	int n = 1;
	const glm::vec2 v = to_vec2(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF3(lua_State* s) {
	int n = 1;
	const glm::vec3 v = to_vec3(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_ridgedMF4(lua_State* s) {
	int n = 1;
	const glm::vec4 v = to_vec4(s, n);
	const float ridgeOffset = (float)luaL_optnumber(s, n + 1, 1.0f);
	const uint8_t octaves = luaL_optinteger(s, n + 2, 4);
	const float lacunarity = (float)luaL_optnumber(s, n + 3, 2.0f);
	const float gain = (float)luaL_optnumber(s, n + 4, 0.5f);
	lua_pushnumber(s, noise::ridgedMF(v, ridgeOffset, octaves, lacunarity, gain));
	return 1;
}

static int luaVoxel_noise_worley2(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::worleyNoise(to_vec2(s, n)));
	return 1;
}

static int luaVoxel_noise_worley3(lua_State* s) {
	int n = 1;
	lua_pushnumber(s, noise::worleyNoise(to_vec3(s, n)));
	return 1;
}

static int luaVoxel_genland(lua_State *s) {
	voxelgenerator::GenlandSettings settings;
	settings.seed = (int)luaL_optinteger(s, 1, 0);
	settings.size = (int)luaL_optinteger(s, 2, 256);
	settings.height = (int)luaL_optinteger(s, 3, 64);
	settings.octaves = (int)luaL_optinteger(s, 4, 10);
	settings.smoothing = (int)luaL_optnumber(s, 5, 1);
	settings.persistence = (float)luaL_optnumber(s, 6, 0.4f);
	settings.amplitude = (float)luaL_optnumber(s, 7, 0.4f);
	settings.riverWidth = (float)luaL_optnumber(s, 8, 0.02f);
	settings.freqGround = (float)luaL_optnumber(s, 9, 9.5f);
	settings.freqRiver = (float)luaL_optnumber(s, 10, 13.2f);
	settings.offset[0] = (int)luaL_optinteger(s, 11, 0);
	settings.offset[1] = (int)luaL_optinteger(s, 12, 0);
	settings.shadow = clua_optboolean(s, 13, true);
	settings.river = clua_optboolean(s, 14, true);
	settings.ambience = clua_optboolean(s, 15, true);

	voxel::RawVolume *v = voxelgenerator::genland(settings);
	if (v == nullptr) {
		return clua_error(s, "Failed to generate land");
	}
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);
	scenegraph::SceneGraphNode node(scenegraph::SceneGraphNodeType::Model);
	node.setVolume(v, true);
	node.setName("Generated Land");
	node.setProperty("Generator", "Genland by Tom Dobrowolski");
	int newNodeId = sceneGraph->emplace(core::move(node));
	if (newNodeId == InvalidNodeId) {
		return clua_error(s, "Failed to add generated land node to scene graph");
	}
	return luaVoxel_pushscenegraphnode(s, sceneGraph->node(newNodeId));
}

static int luaVoxel_shadow(lua_State *s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int lightStep = (int)luaL_optinteger(s, 2, 8);
	voxelutil::shadow(*volume, volume->node()->palette(), lightStep);
	return 0;
}

static int luaVoxel_region_new(lua_State* s) {
	const int minsx = (int)luaL_checkinteger(s, 1);
	const int minsy = (int)luaL_checkinteger(s, 2);
	const int minsz = (int)luaL_checkinteger(s, 3);
	const int maxsx = (int)luaL_checkinteger(s, 4);
	const int maxsy = (int)luaL_checkinteger(s, 5);
	const int maxsz = (int)luaL_checkinteger(s, 6);
	return luaVoxel_pushregion(s, voxel::Region(minsx, minsy, minsz, maxsx, maxsy, maxsz));
}

static int luaVoxel_region_eq(lua_State* s) {
	const voxel::Region *region = luaVoxel_toregion(s, 1);
	const voxel::Region *region2 = luaVoxel_toregion(s, 2);
	lua_pushboolean(s, *region == *region2);
	return 1;
}

static int luaVoxel_region_gc(lua_State *s) {
	voxel::Region *region = luaVoxel_toregion(s, 1);
	delete region;
	return 0;
}

static int luaVoxel_scenegraph_updatetransforms(lua_State* s) {
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);
	sceneGraph->updateTransforms();
	return 0;
}

static int luaVoxel_scenegraph_get_all_node_ids(lua_State *s) {
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);

	lua_newtable(s);
	for (const auto &entry : sceneGraph->nodes()) {
		if (!entry->second.isAnyModelNode() && entry->second.type() != scenegraph::SceneGraphNodeType::Point &&
			entry->second.type() != scenegraph::SceneGraphNodeType::Group &&
			entry->second.type() != scenegraph::SceneGraphNodeType::Camera) {
			continue;
		}
		lua_pushinteger(s, entry->key);
		lua_rawseti(s, -2, lua_rawlen(s, -2) + 1);
	}

	return 1;
}

static int luaVoxel_scenegraph_align(lua_State* s) {
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);
	int padding = (int)luaL_optinteger(s, 1, 2);
	sceneGraph->align(padding);
	return 0;
}

static scenegraph::SceneGraphNodeType toNodeType(const char *type) {
	for (int i = 0; i < lengthof(scenegraph::SceneGraphNodeTypeStr); ++i) {
		if (core::string::iequals(type, scenegraph::SceneGraphNodeTypeStr[i])) {
			return (scenegraph::SceneGraphNodeType)i;
		}
	}
	return scenegraph::SceneGraphNodeType::Unknown;
}

static int luaVoxel_scenegraph_new_node(lua_State* s) {
	const char *name = lua_tostring(s, 1);
	voxel::RawVolume *v = nullptr;
	bool visible = true;
	scenegraph::SceneGraphNodeType type;
	if (luaVoxel_isregion(s, 2)) {
		const voxel::Region* region = luaVoxel_toregion(s, 2);
		visible = clua_optboolean(s, 3, true);
		v = new voxel::RawVolume(*region);
		type = scenegraph::SceneGraphNodeType::Model;
	} else {
		visible = clua_optboolean(s, 2, true);
		const char *nodeTypeStr = luaL_optstring(s, 3, scenegraph::SceneGraphNodeTypeStr[(int)scenegraph::SceneGraphNodeType::Group]);
		type = toNodeType(nodeTypeStr);
		if (type == scenegraph::SceneGraphNodeType::Root) {
			return clua_error(s, "Root node can not be created");
		}
		if (type == scenegraph::SceneGraphNodeType::Model) {
			return clua_error(s, "No region defined for model node");
		}
		if (type == scenegraph::SceneGraphNodeType::Unknown) {
			return clua_error(s, "Unknown node type %s", nodeTypeStr);
		}
		if (type == scenegraph::SceneGraphNodeType::ModelReference) {
			// TODO: not implemented yet a reference node id is missing
			return clua_error(s, "Can't create this type of node");
		}
	}
	scenegraph::SceneGraphNode node(type);
	if (type == scenegraph::SceneGraphNodeType::Model) {
		node.setVolume(v, true);
	}
	node.setName(name);
	node.setVisible(visible);
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	lua_getglobal(s, luaVoxel_globalnodeid());
	int currentNodeId = lua_tointeger(s, -1);
	lua_pop(s, 1);
	const int nodeId = scenegraph::moveNodeToSceneGraph(*sceneGraph, node, currentNodeId);
	if (nodeId == -1) {
		return clua_error(s, "Failed to add new %s node", scenegraph::SceneGraphNodeTypeStr[(int)type]);
	}

	return luaVoxel_pushscenegraphnode(s, sceneGraph->node(nodeId));
}

static int luaVoxel_scenegraph_get_node_by_name(lua_State* s) {
	const char *name = luaL_checkstring(s, 1);
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	if (scenegraph::SceneGraphNode *node = sceneGraph->findNodeByName(name)) {
		return luaVoxel_pushscenegraphnode(s, *node);
	}
	lua_pushnil(s);
	return 1;
}

static int luaVoxel_scenegraph_get_node_by_uuid(lua_State* s) {
	const char *uuidStr = luaL_checkstring(s, 1);
	core::UUID uuid(uuidStr);
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	if (scenegraph::SceneGraphNode *node = sceneGraph->findNodeByUUID(uuid)) {
		return luaVoxel_pushscenegraphnode(s, *node);
	}
	lua_pushnil(s);
	return 1;
}

static int luaVoxel_scenegraph_get_node_by_id(lua_State* s) {
	int nodeId = (int)luaL_optinteger(s, 1, InvalidNodeId);
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	if (nodeId == InvalidNodeId) {
		nodeId = sceneGraph->activeNode();
	}
	if (!sceneGraph->hasNode(nodeId)) {
		return clua_error(s, "Could not find node for id %d", nodeId);
	}
	scenegraph::SceneGraphNode& node = sceneGraph->node(nodeId);
	return luaVoxel_pushscenegraphnode(s, node);
}

static int luaVoxel_scenegraph_addanimation(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const char *name = luaL_checkstring(s, 1);
	lua_pushboolean(s, sceneGraph->addAnimation(name));
	return 1;
}

static int luaVoxel_scenegraph_hasanimation(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const char *name = luaL_checkstring(s, 1);
	lua_pushboolean(s, sceneGraph->hasAnimation(name));
	return 1;
}

static int luaVoxel_scenegraph_setanimation(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const char *name = luaL_checkstring(s, 1);
	lua_pushboolean(s, sceneGraph->setAnimation(name));
	return 1;
}

static int luaVoxel_scenegraph_activeanimation(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	lua_pushstring(s, sceneGraph->activeAnimation().c_str());
	return 1;
}

static int luaVoxel_scenegraph_duplicateanimation(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const char *animation = luaL_checkstring(s, 1);
	const char *newName = luaL_checkstring(s, 2);
	lua_pushboolean(s, sceneGraph->duplicateAnimation(animation, newName));
	return 1;
}

static int luaVoxel_scenegraphnode_volume(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	if (!node->node->isModelNode()) {
		return clua_error(s, "Node is no model node");
	}
	return luaVoxel_pushvolumewrapper(s, node);
}

static int luaVoxel_scenegraphnode_palette(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	palette::Palette &palette = node->node->palette();
	return luaVoxel_pushpalette(s, palette);
}

static int luaVoxel_scenegraphnode_is_point(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->type() == scenegraph::SceneGraphNodeType::Point ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_is_camera(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->type() == scenegraph::SceneGraphNodeType::Camera ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_is_group(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->type() == scenegraph::SceneGraphNodeType::Group ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_is_model(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->isModelNode() ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_is_modelref(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->isReferenceNode() ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_name(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushstring(s, node->node->name().c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_id(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushinteger(s, node->node->id());
	return 1;
}

static int luaVoxel_scenegraphnode_clone(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const int nodeId = scenegraph::copyNodeToSceneGraph(*sceneGraph, *node->node, node->node->parent(), false);
	if (nodeId == InvalidNodeId) {
		return clua_error(s, "Failed to clone node %d", node->node->id());
	}
	return luaVoxel_pushscenegraphnode(s, sceneGraph->node(nodeId));
}

static int luaVoxel_scenegraphnode_uuid(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const core::String &uuidStr = node->node->uuid().str();
	lua_pushstring(s, uuidStr.c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_parent(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushinteger(s, node->node->parent());
	return 1;
}

static int luaVoxel_scenegraphnode_setname(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const char *newName = lua_tostring(s, 2);
	node->node->setName(newName);
	return 0;
}

static int luaVoxel_scenegraphnode_keyframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::KeyFrameIndex keyFrameIdx = (scenegraph::KeyFrameIndex)luaL_checkinteger(s, 2);
	scenegraph::SceneGraphKeyFrames *keyFrames = node->node->keyFrames();
	if ((int)keyFrameIdx < 0 || (int)keyFrameIdx >= (int)keyFrames->size()) {
		return clua_error(s, "Keyframe index out of bounds: %i/%i", keyFrameIdx, (int)keyFrames->size());
	}
	luaVoxel_pushkeyframe(s, *node->node, keyFrameIdx);
	return 1;
}

static int luaVoxel_scenegraphnode_keyframeforframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::FrameIndex frame = (scenegraph::FrameIndex)luaL_checkinteger(s, 2);
	scenegraph::KeyFrameIndex keyFrameIdx = node->node->keyFrameForFrame(frame);
	if (keyFrameIdx == InvalidKeyFrame) {
		return clua_error(s, "No keyframe for frame %d", frame);
	}
	luaVoxel_pushkeyframe(s, *node->node, keyFrameIdx);
	return 1;
}

static int luaVoxel_scenegraphnode_hasframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::FrameIndex frame = (scenegraph::FrameIndex)luaL_checkinteger(s, 2);
	lua_pushboolean(s, node->node->hasKeyFrameForFrame(frame));
	return 1;
}

static int luaVoxel_scenegraphnode_removekeyframeforframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::FrameIndex frame = (scenegraph::FrameIndex)luaL_checkinteger(s, 2);
	scenegraph::KeyFrameIndex existingIndex = InvalidKeyFrame;
	if (!node->node->hasKeyFrameForFrame(frame, &existingIndex)) {
		return clua_error(s, "Failed to remove keyframe for frame %d", frame);
	}
	if (!node->node->removeKeyFrame(existingIndex)) {
		return clua_error(s, "Failed to remove keyframe %d", existingIndex);
	}
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	sceneGraph->markMaxFramesDirty();
	return 0;
}

static int luaVoxel_scenegraphnode_removekeyframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	scenegraph::KeyFrameIndex keyFrameIdx = (scenegraph::KeyFrameIndex)luaL_checkinteger(s, 2);
	if (!node->node->removeKeyFrame(keyFrameIdx)) {
		return clua_error(s, "Failed to remove keyframe %d", keyFrameIdx);
	}
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	sceneGraph->markMaxFramesDirty();
	return 0;
}

static int luaVoxel_scenegraphnode_addframe(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const int frameIdx = (int)luaL_checkinteger(s, 2);
	scenegraph::InterpolationType interpolation = (scenegraph::InterpolationType)luaL_optinteger(s, 3, (int)scenegraph::InterpolationType::Linear);
	scenegraph::KeyFrameIndex existingIndex = InvalidKeyFrame;
	if (node->node->hasKeyFrameForFrame(frameIdx, &existingIndex)) {
		return clua_error(s, "Keyframe for frame %d already exists (%d)", frameIdx, (int)existingIndex);
	}
	scenegraph::KeyFrameIndex newKeyFrameIdx = node->node->addKeyFrame(frameIdx);
	if (newKeyFrameIdx == InvalidKeyFrame) {
		return clua_error(s, "Failed to add keyframe for frame %d", frameIdx);
	}
	scenegraph::SceneGraph *sceneGraph = luaVoxel_scenegraph(s);
	sceneGraph->markMaxFramesDirty();
	scenegraph::SceneGraphKeyFrame &kf = node->node->keyFrame(newKeyFrameIdx);
	kf.interpolation = interpolation;
	const scenegraph::SceneGraphKeyFrame &prevKf = node->node->keyFrame(newKeyFrameIdx - 1);
	kf.transform() = prevKf.transform();
	kf.longRotation = prevKf.longRotation;
	luaVoxel_pushkeyframe(s, *node->node, newKeyFrameIdx);
	return 1;
}

static int luaVoxel_keyframe_index(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	lua_pushinteger(s, keyFrame->keyFrameIdx);
	return 1;
}

static int luaVoxel_keyframe_frame(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	lua_pushinteger(s, kf.frameIdx);
	return 1;
}

static scenegraph::InterpolationType toInterpolationType(const core::String &type) {
	for (int i = 0; i < lengthof(scenegraph::InterpolationTypeStr); ++i) {
		if (type == scenegraph::InterpolationTypeStr[i]) {
			return (scenegraph::InterpolationType)i;
		}
	}
	return scenegraph::InterpolationType::Max;
}

static int luaVoxel_keyframe_interpolation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	lua_pushstring(s, scenegraph::InterpolationTypeStr[(int)kf.interpolation]);
	return 1;
}

static int luaVoxel_keyframe_setinterpolation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::InterpolationType interpolation = toInterpolationType(luaL_checkstring(s, 2));
	if (interpolation == scenegraph::InterpolationType::Max) {
		return clua_error(s, "Invalid interpolation type given");
	}
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.interpolation = interpolation;
	return 0;
}

static int luaVoxel_keyframe_localscale(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().localScale());
	return 1;
}

static int luaVoxel_keyframe_setlocalscale(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const glm::vec3 &val = luaVoxel_getvec<3, float>(s, 2);
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setLocalScale(val);
	return 0;
}

static int luaVoxel_keyframe_localorientation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().localOrientation());
	return 1;
}

static int luaVoxel_keyframe_setlocalorientation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	glm::quat val;
	if (clua_isquat(s, 2)) {
		val = clua_toquat(s, 2);
	} else {
		const float x = (float)luaL_checknumber(s, 2);
		const float y = (float)luaL_checknumber(s, 3);
		const float z = (float)luaL_checknumber(s, 4);
		const float w = (float)luaL_checknumber(s, 5);
		val = glm::quat::wxyz(w, x, y, z);
	}
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setLocalOrientation(val);
	return 0;
}

static int luaVoxel_keyframe_localtranslation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().localTranslation());
	return 1;
}

static int luaVoxel_keyframe_setlocaltranslation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const glm::vec3 &val = luaVoxel_getvec<3, float>(s, 2);
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setLocalTranslation(val);
	return 0;
}

static int luaVoxel_keyframe_worldscale(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().worldScale());
	return 1;
}

static int luaVoxel_keyframe_setworldscale(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const glm::vec3 &val = luaVoxel_getvec<3, float>(s, 2);
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setWorldScale(val);
	return 0;
}

static int luaVoxel_keyframe_worldorientation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().worldOrientation());
	return 1;
}

static int luaVoxel_keyframe_setworldorientation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	glm::quat val;
	if (clua_isquat(s, 2)) {
		val = clua_toquat(s, 2);
	} else {
		const float x = (float)luaL_checknumber(s, 2);
		const float y = (float)luaL_checknumber(s, 3);
		const float z = (float)luaL_checknumber(s, 4);
		const float w = (float)luaL_checknumber(s, 5);
		val = glm::quat::wxyz(w, x, y, z);
	}
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setWorldOrientation(val);
	return 0;
}

static int luaVoxel_keyframe_worldtranslation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	clua_push(s, kf.transform().worldTranslation());
	return 1;
}

static int luaVoxel_keyframe_setworldtranslation(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const glm::vec3 &val = luaVoxel_getvec<3, float>(s, 2);
	scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	kf.transform().setWorldTranslation(val);
	return 0;
}

static int luaVoxel_keyframe_gc(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	delete keyFrame;
	return 0;
}

static int luaVoxel_keyframe_tostring(lua_State *s) {
	LuaKeyFrame *keyFrame = luaVoxel_tokeyframe(s, 1);
	const scenegraph::SceneGraphKeyFrame &kf = keyFrame->keyFrame();
	const scenegraph::SceneGraphTransform &transform = kf.transform();
	const glm::vec3 &localTranslation = transform.localTranslation();
	const glm::quat &localOrientation = transform.localOrientation();
	const glm::vec3 &localScale = transform.localScale();
	const glm::vec3 &worldTranslation = transform.worldTranslation();
	const glm::quat &worldOrientation = transform.worldOrientation();
	const glm::vec3 &worldScale = transform.worldScale();
	lua_pushfstring(s,
					"keyframe: [frame: %d], [interpolation: %s], "
					"[localTranslation: %f:%f:%f], [localOrientation: %f:%f:%f:%f], [localScale: %f:%f:%f]"
					"[worldTranslation: %f:%f:%f], [worldOrientation: %f:%f:%f:%f], [worldScale: %f:%f:%f]",
					kf.frameIdx, scenegraph::InterpolationTypeStr[(int)kf.interpolation], localTranslation.x,
					localTranslation.y, localTranslation.z, localOrientation.x, localOrientation.y, localOrientation.z,
					localOrientation.w, localScale.x, localScale.y, localScale.z, worldTranslation.x,
					worldTranslation.y, worldTranslation.z, worldOrientation.x, worldOrientation.y, worldOrientation.z,
					worldOrientation.w, worldScale.x, worldScale.y, worldScale.z);
	return 1;
}

static int luaVoxel_scenegraphnode_setpalette(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	palette::Palette *palette = luaVoxel_toPalette(s, 2);
	if (clua_optboolean(s, 3, false)) {
		node->node->remapToPalette(*palette);
	}
	node->node->setPalette(*palette);
	return 0;
}

static int luaVoxel_scenegraphnode_setpivot(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const glm::vec3 &val = luaVoxel_getvec<3, float>(s, 2);
	node->node->setPivot(val);
	return 0;
}

static int luaVoxel_scenegraphnode_hide(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	node->node->setVisible(false);
	return 0;
}

static int luaVoxel_scenegraphnode_show(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	node->node->setVisible(true);
	return 0;
}

static int luaVoxel_scenegraphnode_lock(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	node->node->setLocked(true);
	return 0;
}

static int luaVoxel_scenegraphnode_unlock(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	node->node->setLocked(false);
	return 0;
}

static int luaVoxel_scenegraphnode_isvisible(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->visible() ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_islocked(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushboolean(s, node->node->locked() ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_setproperty(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const char *key = luaL_checkstring(s, 2);
	if (key == nullptr) {
		return clua_error(s, "Key is nil");
	}
	const char *value = luaL_checkstring(s, 3);
	if (value == nullptr) {
		return clua_error(s, "Value is nil");
	}
	const bool ret = node->node->setProperty(key, value);
	lua_pushboolean(s, ret ? 1 : 0);
	return 1;
}

static int luaVoxel_scenegraphnode_property(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const char *key = luaL_checkstring(s, 2);
	if (key == nullptr) {
		return clua_error(s, "Key is nil");
	}
	const core::String &value = node->node->property(key);
	lua_pushstring(s, value.c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_tostring(lua_State *s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	lua_pushfstring(s, "node: [%d, %s]", node->node->id(), node->node->name().c_str());
	return 1;
}

static int luaVoxel_scenegraphnode_gc(lua_State *s) {
	LuaSceneGraphNode *node = luaVoxel_toscenegraphnode(s, 1);
	delete node;
	return 0;
}

static void prepareState(lua_State* s) {
	static const luaL_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volumewrapper_voxel},
		{"region", luaVoxel_volumewrapper_region},
		{"translate", luaVoxel_volumewrapper_translate},
		{"move", luaVoxel_volumewrapper_move},
		{"resize", luaVoxel_volumewrapper_resize},
		{"crop", luaVoxel_volumewrapper_crop},
		{"text", luaVoxel_volumewrapper_text},
		{"fillHollow", luaVoxel_volumewrapper_fillhollow},
		{"hollow", luaVoxel_volumewrapper_hollow},
		{"importHeightmap", luaVoxel_volumewrapper_importheightmap},
		{"importColoredHeightmap", luaVoxel_volumewrapper_importcoloredheightmap},
		{"importImageAsVolume", luaVoxel_volumewrapper_importimageasvolume},
		{"mirrorAxis", luaVoxel_volumewrapper_mirroraxis},
		{"rotateAxis", luaVoxel_volumewrapper_rotateaxis},
		{"setVoxel", luaVoxel_volumewrapper_setvoxel},
		{"__gc", luaVoxel_volumewrapper_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolumewrapper());

	static const luaL_Reg regionFuncs[] = {
		{"width", luaVoxel_region_width},
		{"height", luaVoxel_region_height},
		{"depth", luaVoxel_region_depth},
		{"x", luaVoxel_region_x},
		{"y", luaVoxel_region_y},
		{"z", luaVoxel_region_z},
		{"center", luaVoxel_region_center},
		{"mins", luaVoxel_region_mins},
		{"maxs", luaVoxel_region_maxs},
		{"size", luaVoxel_region_size},
		{"__tostring", luaVoxel_region_tostring},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs, luaVoxel_metaregion());

	static const luaL_Reg regionFuncs_gc[] = {
		{"width", luaVoxel_region_width},
		{"height", luaVoxel_region_height},
		{"depth", luaVoxel_region_depth},
		{"x", luaVoxel_region_x},
		{"y", luaVoxel_region_y},
		{"z", luaVoxel_region_z},
		{"isOnBorder", luaVoxel_region_isonborder},
		{"center", luaVoxel_region_center},
		{"mins", luaVoxel_region_mins},
		{"maxs", luaVoxel_region_maxs},
		{"size", luaVoxel_region_size},
		{"intersects", luaVoxel_region_intersects},
		{"contains", luaVoxel_region_contains},
		{"setMins", luaVoxel_region_setmins},
		{"setMaxs", luaVoxel_region_setmaxs},
		{"__tostring", luaVoxel_region_tostring},
		{"__eq", luaVoxel_region_eq},
		{"__gc", luaVoxel_region_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs_gc, luaVoxel_metaregion_gc());

	static const luaL_Reg globalRegionFuncs[] = {
		{"new", luaVoxel_region_new},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, globalRegionFuncs, luaVoxel_metaregionglobal(), "g_region");

	static const luaL_Reg sceneGraphFuncs[] = {
		{"align", luaVoxel_scenegraph_align},
		{"new", luaVoxel_scenegraph_new_node},
		{"get", luaVoxel_scenegraph_get_node_by_id},
		{"getByName", luaVoxel_scenegraph_get_node_by_name},
		{"getByUUID", luaVoxel_scenegraph_get_node_by_uuid},
		{"nodeIds", luaVoxel_scenegraph_get_all_node_ids},
		{"updateTransforms", luaVoxel_scenegraph_updatetransforms},
		{"addAnimation", luaVoxel_scenegraph_addanimation},
		{"setAnimation", luaVoxel_scenegraph_setanimation},
		{"duplicateAnimation", luaVoxel_scenegraph_duplicateanimation},
		{"hasAnimation", luaVoxel_scenegraph_hasanimation},
		{"activeAnimation", luaVoxel_scenegraph_activeanimation},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, sceneGraphFuncs, luaVoxel_metascenegraph(), "g_scenegraph");

	static const luaL_Reg sceneGraphNodeFuncs[] = {
		{"name", luaVoxel_scenegraphnode_name},
		{"id", luaVoxel_scenegraphnode_id},
		{"uuid", luaVoxel_scenegraphnode_uuid},
		{"clone", luaVoxel_scenegraphnode_clone},
		{"parent", luaVoxel_scenegraphnode_parent},
		{"volume", luaVoxel_scenegraphnode_volume},
		{"isModel", luaVoxel_scenegraphnode_is_model},
		{"isReference", luaVoxel_scenegraphnode_is_modelref},
		{"isPoint", luaVoxel_scenegraphnode_is_point},
		{"isCamera", luaVoxel_scenegraphnode_is_camera},
		{"isGroup", luaVoxel_scenegraphnode_is_group},
		{"palette", luaVoxel_scenegraphnode_palette},
		{"setName", luaVoxel_scenegraphnode_setname},
		{"setPalette", luaVoxel_scenegraphnode_setpalette},
		{"setPivot", luaVoxel_scenegraphnode_setpivot},
		{"hide", luaVoxel_scenegraphnode_hide},
		{"show", luaVoxel_scenegraphnode_show},
		{"lock", luaVoxel_scenegraphnode_lock},
		{"unlock", luaVoxel_scenegraphnode_unlock},
		{"isVisible", luaVoxel_scenegraphnode_isvisible},
		{"isLocked", luaVoxel_scenegraphnode_islocked},
		{"setProperty", luaVoxel_scenegraphnode_setproperty},
		{"property", luaVoxel_scenegraphnode_property},
		{"keyFrame", luaVoxel_scenegraphnode_keyframe},
		{"keyFrameForFrame", luaVoxel_scenegraphnode_keyframeforframe},
		{"addKeyFrame", luaVoxel_scenegraphnode_addframe},
		{"hasKeyFrameForFrame", luaVoxel_scenegraphnode_hasframe},
		{"removeKeyFrameForFrame", luaVoxel_scenegraphnode_removekeyframeforframe},
		{"removeKeyFrame", luaVoxel_scenegraphnode_removekeyframe},
		{"__tostring", luaVoxel_scenegraphnode_tostring},
		{"__gc", luaVoxel_scenegraphnode_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, sceneGraphNodeFuncs, luaVoxel_metascenegraphnode());

	static const luaL_Reg keyframeFuncs[] = {
		{"index", luaVoxel_keyframe_index},
		{"frame", luaVoxel_keyframe_frame},
		{"interpolation", luaVoxel_keyframe_interpolation},
		{"setInterpolation", luaVoxel_keyframe_setinterpolation},
		{"localScale", luaVoxel_keyframe_localscale},
		{"setLocalScale", luaVoxel_keyframe_setlocalscale},
		{"localOrientation", luaVoxel_keyframe_localorientation},
		{"setLocalOrientation", luaVoxel_keyframe_setlocalorientation},
		{"localTranslation", luaVoxel_keyframe_localtranslation},
		{"setLocalTranslation", luaVoxel_keyframe_setlocaltranslation},
		{"worldScale", luaVoxel_keyframe_worldscale},
		{"setWorldScale", luaVoxel_keyframe_setworldscale},
		{"worldOrientation", luaVoxel_keyframe_worldorientation},
		{"setWorldOrientation", luaVoxel_keyframe_setworldorientation},
		{"worldTranslation", luaVoxel_keyframe_worldtranslation},
		{"setWorldTranslation", luaVoxel_keyframe_setworldtranslation},
		{"__tostring", luaVoxel_keyframe_tostring},
		{"__gc", luaVoxel_keyframe_gc},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, keyframeFuncs, luaVoxel_metakeyframe());

	static const luaL_Reg paletteFuncs[] = {
		{"colors", luaVoxel_palette_colors},
		{"color", luaVoxel_palette_color},
		{"size", luaVoxel_palette_size},
		{"rgba", luaVoxel_palette_rgba},
		{"load", luaVoxel_palette_load},
		{"setColor", luaVoxel_palette_setcolor},
		{"match", luaVoxel_palette_closestmatch},
		{"similar", luaVoxel_palette_similar},
		{"setMaterial", luaVoxel_palette_setmaterialproperty},
		{"material", luaVoxel_palette_materialproperty},
		{"deltaE", luaVoxel_palette_delta_e},
		{"colorString", luaVoxel_palette_color_to_string},
		{"__tostring", luaVoxel_palette_tostring},
		{"__eq", luaVoxel_palette_eq},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs, luaVoxel_metapalette());

	static const luaL_Reg paletteFuncs_gc[] = {
		{"colors", luaVoxel_palette_colors},
		{"color", luaVoxel_palette_color},
		{"size", luaVoxel_palette_size},
		{"rgba", luaVoxel_palette_rgba},
		{"load", luaVoxel_palette_load},
		{"setColor", luaVoxel_palette_setcolor},
		{"match", luaVoxel_palette_closestmatch},
		{"similar", luaVoxel_palette_similar},
		{"setMaterial", luaVoxel_palette_setmaterialproperty},
		{"material", luaVoxel_palette_materialproperty},
		{"deltaE", luaVoxel_palette_delta_e},
		{"colorString", luaVoxel_palette_color_to_string},
		{"__tostring", luaVoxel_palette_tostring},
		{"__gc", luaVoxel_palette_gc},
		{"__eq", luaVoxel_palette_eq},
		{nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs_gc, luaVoxel_metapalette_gc());

	static const luaL_Reg paletteGlobalsFuncs[] = {
		{"new", luaVoxel_palette_new},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, paletteGlobalsFuncs, luaVoxel_metapaletteglobal(), "g_palette");

	static const luaL_Reg noiseFuncs[] = {
		{"noise2", luaVoxel_noise_simplex2},
		{"noise3", luaVoxel_noise_simplex3},
		{"noise4", luaVoxel_noise_simplex4},
		{"fBm2", luaVoxel_noise_fBm2},
		{"fBm3", luaVoxel_noise_fBm3},
		{"fBm4", luaVoxel_noise_fBm4},
		{"swissTurbulence", luaVoxel_noise_swissturbulence},
		{"voronoi", luaVoxel_noise_voronoi},
		{"ridgedMF2", luaVoxel_noise_ridgedMF2},
		{"ridgedMF3", luaVoxel_noise_ridgedMF3},
		{"ridgedMF4", luaVoxel_noise_ridgedMF4},
		{"worley2", luaVoxel_noise_worley2},
		{"worley3", luaVoxel_noise_worley3},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, noiseFuncs, luaVoxel_metanoise(), "g_noise");

	static const luaL_Reg shapeFuncs[] = {
		{"cylinder", luaVoxel_shape_cylinder},
		{"torus", luaVoxel_shape_torus},
		{"ellipse", luaVoxel_shape_ellipse},
		{"dome", luaVoxel_shape_dome},
		{"cube", luaVoxel_shape_cube},
		{"cone", luaVoxel_shape_cone},
		{"line", luaVoxel_shape_line},
		{"bezier", luaVoxel_shape_bezier},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, shapeFuncs, luaVoxel_metashape(), "g_shape");

	static const luaL_Reg importerFuncs[] = {
		{"palette", luaVoxel_load_palette},
		{"image", luaVoxel_load_image},
		{"scene", luaVoxel_import_scene},
		{"imageAsPlane", luaVoxel_import_imageasplane},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, importerFuncs, luaVoxel_metaimporter(), "g_import");

	static const luaL_Reg algorithmFuncs[] = {
		{"genland", luaVoxel_genland},
		{"shadow", luaVoxel_shadow},
		{nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, algorithmFuncs, luaVoxel_metaalgorithm(), "g_algorithm");

	clua_imageregister(s);
	clua_streamregister(s);
	clua_httpregister(s);
	clua_mathregister(s);
}

LUAApi::LUAApi(const io::FilesystemPtr &filesystem) : _filesystem(filesystem) {
}

bool LUAApi::init() {
	if (!_noise.init()) {
		Log::warn("Failed to initialize noise");
	}
	luaVoxel_newGlobalData(_lua, luaVoxel_globalnoise(), &_noise);
	luaVoxel_newGlobalData(_lua, luaVoxel_globaldirtyregion(), &_dirtyRegion);
	prepareState(_lua);
	return true;
}

ScriptState LUAApi::update(double nowSeconds) {
	if (_scriptStillRunning) {
		int nres = 0;
		const int error = lua_resume(_lua, nullptr, _nargs, &nres);
		_nargs = 0;
		if (error == LUA_OK) {
			_scriptStillRunning = false;
			lua_gc(_lua, LUA_GCCOLLECT, 0);
			return ScriptState::Finished;
		} else if (error != LUA_YIELD) {
			const char *msg = lua_tostring(_lua, -1);
			luaL_traceback(_lua, _lua, msg, 1);
			lua_pop(_lua, 1);
			Log::error("Error running script: %s", lua_tostring(_lua, -1));
			_scriptStillRunning = false;
			// Reset the Lua state after an error to prevent "cannot resume dead coroutine" errors
			_lua.resetState();
			init();
			return ScriptState::Error;
		}
		return ScriptState::Running;
	}
	return ScriptState::Inactive;
}

void LUAApi::shutdown() {
	lua_gc(_lua, LUA_GCCOLLECT, 0);
	_noise.shutdown();
	_lua.resetState();
}

core::String LUAApi::description(const core::String &luaScript) const {
	lua::LUA lua;
	if (!prepare(lua, luaScript)) {
		return "";
	}
	return description(lua);
}

core::String LUAApi::description(lua::LUA &lua) const {
	// get description method
	lua_getglobal(lua, "description");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no description...
		return "";
	}

	const int error = lua_pcall(lua, 0, 1, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate description script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return "";
	}

	core::String desc;
	if (lua_isstring(lua, -1)) {
		desc = lua_tostring(lua, -1);
	} else {
		Log::error("Expected to get a string return value");
	}

	return desc;
}

bool LUAApi::prepare(lua::LUA &lua, const core::String &luaScript) const {
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		return false;
	}
	return true;
}

bool LUAApi::argumentInfo(const core::String& luaScript, core::DynamicArray<LUAParameterDescription>& params) {
	lua::LUA lua;
	if (!prepare(lua, luaScript)) {
		return false;
	}
	return argumentInfo(lua, params);
}

bool LUAApi::argumentInfo(lua::LUA &lua, core::DynamicArray<LUAParameterDescription>& params) {
	const int preTop = lua_gettop(lua);

	// get arguments method
	lua_getglobal(lua, "arguments");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no parameters are needed...
		return true;
	}

	const int error = lua_pcall(lua, 0, LUA_MULTRET, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate arguments script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		return false;
	}

	const int top = lua_gettop(lua);
	if (top <= preTop) {
		return true;
	}

	if (!lua_istable(lua, -1)) {
		Log::error("Expected to get a table return value");
		return false;
	}

	const int args = (int)lua_rawlen(lua, -1);

	for (int i = 0; i < args; ++i) {
		lua_pushinteger(lua, i + 1); // lua starts at 1
		lua_gettable(lua, -2);
		if (!lua_istable(lua, -1)) {
			Log::error("Expected to return tables of { name = 'name', desc = 'description', type = 'int' } at %i", i);
			return false;
		}

		core::String name = "";
		core::String description = "";
		core::String defaultValue = "";
		bool defaultSet = false;
		core::String enumValues = "";
		double minValue = 0.0;
		double maxValue = 100.0;
		bool minSet = false;
		bool maxSet = false;
		LUAParameterType type = LUAParameterType::Max;
		lua_pushnil(lua);					// push nil, so lua_next removes it from stack and puts (k, v) on stack
		while (lua_next(lua, -2) != 0) {	// -2, because we have table at -1
			if (!lua_isstring(lua, -1) || !lua_isstring(lua, -2)) {
				Log::error("Expected to find string as parameter key and value");
				// only store stuff with string key and value
				return false;
			}
			const char *key = lua_tostring(lua, -2);
			const char *value = lua_tostring(lua, -1);
			if (!SDL_strcmp(key, "name")) {
				name = value;
			} else if (!SDL_strncmp(key, "desc", 4)) {
				description = value;
			} else if (!SDL_strncmp(key, "enum", 4)) {
				enumValues = value;
			} else if (!SDL_strcmp(key, "default")) {
				defaultValue = value;
				defaultSet = true;
			} else if (!SDL_strcmp(key, "min")) {
				minValue = SDL_atof(value);
				minSet = true;
			} else if (!SDL_strcmp(key, "max")) {
				maxValue = SDL_atof(value);
				maxSet = true;
			} else if (!SDL_strcmp(key, "type")) {
				if (!SDL_strcmp(value, "int")) {
					type = LUAParameterType::Integer;
				} else if (!SDL_strcmp(value, "float")) {
					type = LUAParameterType::Float;
				} else if (!SDL_strcmp(value, "colorindex")) {
					type = LUAParameterType::ColorIndex;
					if (!minSet) {
						minValue = -1; // empty voxel is lua bindings
					}
					if (!maxSet) {
						maxValue = palette::PaletteMaxColors;
					}
					if (!defaultSet) {
						defaultValue = "1";
					}
				} else if (!SDL_strncmp(value, "str", 3)) {
					type = LUAParameterType::String;
				} else if (!SDL_strncmp(value, "file", 4)) {
					type = LUAParameterType::File;
				} else if (!SDL_strncmp(value, "enum", 4)) {
					type = LUAParameterType::Enum;
				} else if (!SDL_strncmp(value, "bool", 4)) {
					type = LUAParameterType::Boolean;
				} else {
					Log::error("Invalid type found: %s", value);
					return false;
				}
			} else {
				Log::warn("Invalid key found: %s", key);
			}
			lua_pop(lua, 1); // remove value, keep key for lua_next
		}

		if (name.empty()) {
			Log::error("No name = 'myname' key given");
			return false;
		}

		if (type == LUAParameterType::Max) {
			Log::error("No type = 'int', 'float', 'str', 'bool', 'enum' or 'colorindex' key given for '%s'", name.c_str());
			return false;
		}

		if (type == LUAParameterType::Enum && enumValues.empty()) {
			Log::error("No enum property given for argument '%s', but type is 'enum'", name.c_str());
			return false;
		}

		params.emplace_back(name, description, defaultValue, enumValues, minValue, maxValue, type);
		lua_pop(lua, 1); // remove table
	}
	return true;
}

static bool luaVoxel_pushargs(lua_State* s, const core::DynamicArray<core::String>& args, const core::DynamicArray<LUAParameterDescription>& argsInfo) {
	for (size_t i = 0u; i < argsInfo.size(); ++i) {
		const LUAParameterDescription &d = argsInfo[i];
		const core::String &arg = args.size() > i ? args[i] : d.defaultValue;
		switch (d.type) {
		case LUAParameterType::Enum:
		case LUAParameterType::String:
		case LUAParameterType::File:
			lua_pushstring(s, arg.c_str());
			break;
		case LUAParameterType::Boolean: {
			const bool val = core::string::toBool(arg);
			lua_pushboolean(s, val ? 1 : 0);
			break;
		}
		case LUAParameterType::ColorIndex:
		case LUAParameterType::Integer:
			lua_pushinteger(s, glm::clamp(core::string::toInt(arg), (int)d.minValue, (int)d.maxValue));
			break;
		case LUAParameterType::Float:
			lua_pushnumber(s, glm::clamp(core::string::toFloat(arg), (float)d.minValue, (float)d.maxValue));
			break;
		case LUAParameterType::Max:
			Log::error("Invalid argument type");
			return false;
		}
	}
	return true;
}

core::String LUAApi::load(const core::String& scriptName) const {
	core::String filename = scriptName;
	io::normalizePath(filename);
	if (!_filesystem->exists(filename)) {
		if (core::string::extractExtension(filename) != "lua") {
			filename.append(".lua");
		}
		filename = core::string::path("scripts", filename);
	}
#if LUA_VERSION_NUM < 504
	core::String luaStr = _filesystem->load(filename);
	return core::string::replaceAll(luaStr, "<const>", "");
#else
	return _filesystem->load(filename);
#endif
}

core::DynamicArray<LUAScript> LUAApi::listScripts() const {
	core::DynamicArray<LUAScript> scripts;
	core::DynamicArray<io::FilesystemEntry> entities;
	_filesystem->list("scripts", entities, "*.lua");
	scripts.reserve(entities.size());
	for (const auto& e : entities) {
		const core::String path = core::string::path("scripts", e.name);
		LUAScript script;
		script.filename = e.name;
		scripts.emplace_back(core::move(script));
	}
	return scripts;
}

void LUAApi::reloadScriptParameters(voxelgenerator::LUAScript &s) {
	reloadScriptParameters(s, load(s.filename));
}

void LUAApi::reloadScriptParameters(voxelgenerator::LUAScript &s, const core::String &luaScript) {
	s.valid = false;
	s.parameterDescription.clear();
	s.parameters.clear();
	s.enumValues.clear();

	if (luaScript.empty() || !prepare(_lua, luaScript)) {
		return;
	}
	argumentInfo(_lua, s.parameterDescription);
	const int parameterCount = (int)s.parameterDescription.size();
	s.parameters.resize(parameterCount);
	s.enumValues.resize(parameterCount);
	for (int i = 0; i < parameterCount; ++i) {
		const voxelgenerator::LUAParameterDescription &p = s.parameterDescription[i];
		s.parameters[i] = p.defaultValue;
		s.enumValues[i] = p.enumValues;
	}
	s.desc = description(_lua);
	s.cached = true;
	s.valid = true;
}

bool LUAApi::exec(const core::String &luaScript, scenegraph::SceneGraph &sceneGraph, int nodeId,
						const voxel::Region &region, const voxel::Voxel &voxel,
						const core::DynamicArray<core::String> &args) {
	if (_scriptStillRunning) {
		Log::error("Script is still running");
		return false;
	}

	_dirtyRegion = voxel::Region::InvalidRegion;

	_argsInfo.clear();
	if (!argumentInfo(luaScript, _argsInfo)) {
		Log::error("Failed to get argument details");
		return false;
	}

	if (!args.empty() && args[0] == "help") {
		Log::info("Parameter description");
		for (const auto& e : _argsInfo) {
			Log::info(" %s: %s (default: '%s')", e.name.c_str(), e.description.c_str(), e.defaultValue.c_str());
		}
		return true;
	}

	scenegraph::SceneGraphNode &node = sceneGraph.node(nodeId);
	voxel::RawVolume *v = node.volume();
	if (v == nullptr) {
		Log::error("Node %i has no volume", nodeId);
		return false;
	}

	lua_State *s = _lua.state();
	luaVoxel_newGlobalData(s, luaVoxel_globalscenegraph(), &sceneGraph);

	lua_pushinteger(s, nodeId);
	lua_setglobal(s, luaVoxel_globalnodeid());

	// load and run once to initialize the global variables
	if (luaL_dostring(s, luaScript.c_str())) {
		Log::error("Failed to load and run the lua script: %s", lua_tostring(s, -1));
		// Reset the Lua state after a loading error to ensure clean state for next execution
		_lua.resetState();
		init();
		return false;
	}

	// get main(node, region, color) method
	lua_getglobal(s, "main");
	if (!lua_isfunction(s, -1)) {
		Log::error("LUA generator: no main(node, region, color) function found in '%s'", luaScript.c_str());
		lua_pop(s, 1); // pop the non-function value from the stack
		return false;
	}

	// first parameter is scene node
	if (luaVoxel_pushscenegraphnode(s, node) == 0) {
		Log::error("Failed to push scene graph node");
		lua_pop(s, 1); // pop the main function
		return false;
	}

	// second parameter is the region to operate on
	if (luaVoxel_pushregion(s, region) == 0) {
		Log::error("Failed to push region");
		lua_pop(s, 2); // pop the main function and node
		return false;
	}

	// third parameter is the current color
	lua_pushinteger(s, voxel.getColor());

#if GENERATOR_LUA_SANTITY > 0
	if (!lua_isfunction(s, -4)) {
		Log::error("LUA generate: expected to find the main function");
		return false;
	}
	if (luaL_testudata(s, -3, luaVoxel_metascenegraphnode()) == nullptr) {
		Log::error("LUA generate: expected to find scene graph node");
		return false;
	}
	if (!luaVoxel_isregion(s, -2)) {
		Log::error("LUA generate: expected to find region");
		return false;
	}
	if (!lua_isnumber(s, -1)) {
		Log::error("LUA generate: expected to find color");
		return false;
	}
#endif

	if (!luaVoxel_pushargs(s, args, _argsInfo)) {
		Log::error("Failed to execute main() function with the given number of arguments. Try calling with 'help' as parameter");
		lua_pop(s, 4); // pop the main function, node, region, and color
		return false;
	}

	_scriptStillRunning = true;
	_nargs = 3 + _argsInfo.size();

	return true;
}

}

#undef GENERATOR_LUA_SANTITY
