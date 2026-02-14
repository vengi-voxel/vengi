/**
 * @file
 */

#include "LUAApi.h"
#include "app/App.h"
#include "commonlua/LUA.h"
#include "commonlua/LUAFunctions.h"
#include "color/Color.h"
#include "core/StringUtil.h"
#include "core/Unicode.h"
#include "image/Image.h"
#include "io/FilesystemArchive.h"
#include "io/Stream.h"
#include "io/StreamArchive.h"
#include "lauxlib.h"
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

static int luaVoxel_volumewrapper_voxel_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "voxel",
		"summary": "Get the voxel at the specified coordinates.",
		"parameters": [
			{"name": "x", "type": "integer", "description": "The x coordinate."},
			{"name": "y", "type": "integer", "description": "The y coordinate."},
			{"name": "z", "type": "integer", "description": "The z coordinate."}
		],
		"returns": [
			{"type": "integer", "description": "The color index of the voxel at the specified coordinates, or -1 if the voxel is air." }
		]})";

	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_region(lua_State* s) {
	const LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	return luaVoxel_pushregion(s, volume->region());
}

static int luaVoxel_volumewrapper_region_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "region",
		"summary": "Get the region of the volume.",
		"parameters": [],
		"returns": [
			{"type": "region", "description": "The region of the volume."}
		]})";

	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_translate(lua_State* s) {
	LuaRawVolumeWrapper* volume = luaVoxel_tovolumewrapper(s, 1);
	const int x = (int)luaL_checkinteger(s, 2);
	const int y = (int)luaL_optinteger(s, 3, 0);
	const int z = (int)luaL_optinteger(s, 4, 0);
	volume->volume()->translate(glm::ivec3(x, y, z));
	return 0;
}

static int luaVoxel_volumewrapper_translate_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "translate",
		"summary": "Translate the region of the volume without moving the voxels.",
		"parameters": [
			{"name": "x", "type": "integer", "description": "The x translation."},
			{"name": "y", "type": "integer", "description": "The y translation (optional, default 0)."},
			{"name": "z", "type": "integer", "description": "The z translation (optional, default 0)."}
		],
		"returns": []})";

	lua_pushstring(s, json);
	return 1;
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

static int luaVoxel_volumewrapper_move_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "move",
		"summary": "Move the voxels within the volume by the specified offset.",
		"parameters": [
			{"name": "x", "type": "integer", "description": "The x offset."},
			{"name": "y", "type": "integer", "description": "The y offset (optional, default 0)."},
			{"name": "z", "type": "integer", "description": "The z offset (optional, default 0)."}
		],
		"returns": []})";

	lua_pushstring(s, json);
	return 1;
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

static int luaVoxel_volumewrapper_resize_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "resize",
		"summary": "Resize the volume by the specified amounts.",
		"parameters": [
			{"name": "w", "type": "integer", "description": "Width change."},
			{"name": "h", "type": "integer", "description": "Height change (optional, default 0)."},
			{"name": "d", "type": "integer", "description": "Depth change (optional, default 0)."},
			{"name": "extendMins", "type": "boolean", "description": "Extend the minimum corner (optional, default false)."}
		],
		"returns": []})";

	lua_pushstring(s, json);
	return 1;
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
	for (int c = core::unicode::next(str); c != -1; c = core::unicode::next(str)) {
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
	const int thickness = (int)luaL_optinteger(s, 5, 1);
	shape::createLine(*volume, start, end, voxel, thickness);
	return 0;
}

static int luaVoxel_shape_bezier(lua_State* s) {
	LuaRawVolumeWrapper *volume = luaVoxel_tovolumewrapper(s, 1);
	const glm::ivec3& start = clua_tovec<glm::ivec3>(s, 2);
	const glm::ivec3& end = clua_tovec<glm::ivec3>(s, 3);
	const glm::ivec3& control = clua_tovec<glm::ivec3>(s, 4);
	const voxel::Voxel voxel = luaVoxel_getVoxel(s, 5);
	const int thickness = (int)luaL_optinteger(s, 6, 1);
	shape::createBezierFunc(*volume, start, end, control, voxel, [&] (LuaRawVolumeWrapper& vol, const glm::ivec3& last, const glm::ivec3& pos, const voxel::Voxel& v) {
		shape::createLine(vol, pos, last, v, thickness);
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
		const glm::vec4& c = color::fromRGBA(palette->color(i));
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
	const color::RGBA rgba = palette->color(color);
	lua_pushinteger(s, rgba.r);
	lua_pushinteger(s, rgba.g);
	lua_pushinteger(s, rgba.b);
	lua_pushinteger(s, rgba.a);
	return 4;
}

static int luaVoxel_palette_color(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t color = luaL_checkinteger(s, 2);
	const glm::vec4& rgba = color::fromRGBA(palette->color(color));
	return clua_push(s, rgba);
}

static int luaVoxel_palette_setcolor(lua_State* s) {
	palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t color = luaL_checkinteger(s, 2);
	const uint8_t r = luaL_checkinteger(s, 3);
	const uint8_t g = luaL_checkinteger(s, 4);
	const uint8_t b = luaL_checkinteger(s, 5);
	const uint8_t a = luaL_optinteger(s, 6, 255);
	palette->setColor(color, color::RGBA(r, g, b, a));
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
		const color::RGBA c1 = palette->color(idx);
		const color::RGBA c2 = palette->color(idx2);
		distance = color::deltaE76(c1, c2);
	}
	lua_pushnumber(s, distance);
	return 1;
}

static int luaVoxel_palette_color_to_string(lua_State* s) {
	const palette::Palette *palette = luaVoxel_toPalette(s, 1);
	const uint8_t idx = luaL_checkinteger(s, 2);
	const core::String c = color::print(palette->color(idx));
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
	color::RGBA rgba(r * 255.0f, g * 255.0f, b * 255.0f, 255);
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
		palette.setColor(materialIndex, color::RGBA(0));
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

static int luaVoxel_scenegraph_animations(lua_State* s) {
	scenegraph::SceneGraph* sceneGraph = luaVoxel_scenegraph(s);
	const scenegraph::SceneGraphAnimationIds &animations = sceneGraph->animations();
	lua_newtable(s);
	for (size_t i = 0; i < animations.size(); ++i) {
		lua_pushstring(s, animations[i].c_str());
		lua_rawseti(s, -2, (int)i + 1);
	}
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

static int luaVoxel_scenegraphnode_pivot(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	return clua_push(s, node->node->pivot());
}

static int luaVoxel_scenegraphnode_numkeyframes(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const scenegraph::SceneGraphKeyFrames *keyFrames = node->node->keyFrames();
	if (keyFrames == nullptr) {
		lua_pushinteger(s, 0);
	} else {
		lua_pushinteger(s, (int)keyFrames->size());
	}
	return 1;
}

static int luaVoxel_scenegraphnode_children(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	const auto &children = node->node->children();
	lua_newtable(s);
	for (size_t i = 0; i < children.size(); ++i) {
		lua_pushinteger(s, children[i]);
		lua_rawseti(s, -2, (int)i + 1);
	}
	return 1;
}

static int luaVoxel_scenegraphnode_region(lua_State* s) {
	LuaSceneGraphNode* node = luaVoxel_toscenegraphnode(s, 1);
	if (!node->node->isAnyModelNode()) {
		return clua_error(s, "Node is not a model node");
	}
	const voxel::Region &region = node->node->region();
	return luaVoxel_pushregion(s, region);
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

static int luaVoxel_volumewrapper_crop_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "crop",
		"summary": "Crop the volume to remove empty space around the voxels.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_text_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "text",
		"summary": "Render text into the volume using a TrueType font.",
		"parameters": [
			{"name": "font", "type": "string", "description": "Path to the TrueType font file."},
			{"name": "text", "type": "string", "description": "The text to render."},
			{"name": "x", "type": "integer", "description": "The x position (optional, default region lower x)."},
			{"name": "y", "type": "integer", "description": "The y position (optional, default region lower y)."},
			{"name": "z", "type": "integer", "description": "The z position (optional, default region lower z)."},
			{"name": "size", "type": "integer", "description": "Font size (optional, default 16)."},
			{"name": "thickness", "type": "integer", "description": "Voxel thickness (optional, default 1)."},
			{"name": "spacing", "type": "integer", "description": "Character spacing (optional, default 0)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_fillhollow_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "fillHollow",
		"summary": "Fill hollow areas in the volume with the specified voxel color.",
		"parameters": [
			{"name": "color", "type": "integer", "description": "The color index to fill with (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_hollow_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "hollow",
		"summary": "Make the volume hollow by removing interior voxels.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_importheightmap_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "importHeightmap",
		"summary": "Import a heightmap image into the volume.",
		"parameters": [
			{"name": "image", "type": "string", "description": "Path to the heightmap image."},
			{"name": "underground", "type": "integer", "description": "Color index for underground voxels (optional)."},
			{"name": "surface", "type": "integer", "description": "Color index for surface voxels (optional)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_importcoloredheightmap_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "importColoredHeightmap",
		"summary": "Import a colored heightmap image into the volume.",
		"parameters": [
			{"name": "image", "type": "string", "description": "Path to the colored heightmap image."},
			{"name": "underground", "type": "integer", "description": "Color index for underground voxels (optional)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_importimageasvolume_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "importImageAsVolume",
		"summary": "Import an image as a 3D volume using depth information.",
		"parameters": [
			{"name": "texture", "type": "string", "description": "Path to the texture image."},
			{"name": "depthmap", "type": "string", "description": "Path to the depth map image (optional)."},
			{"name": "palette", "type": "palette", "description": "Palette to use (optional)."},
			{"name": "thickness", "type": "integer", "description": "Thickness of the volume (optional, default 8)."},
			{"name": "bothSides", "type": "boolean", "description": "Create voxels on both sides (optional, default false)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_mirroraxis_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "mirrorAxis",
		"summary": "Mirror the volume along the specified axis.",
		"parameters": [
			{"name": "axis", "type": "string", "description": "The axis to mirror along: 'x', 'y', or 'z' (default 'y')."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_rotateaxis_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "rotateAxis",
		"summary": "Rotate the volume 90 degrees around the specified axis.",
		"parameters": [
			{"name": "axis", "type": "string", "description": "The axis to rotate around: 'x', 'y', or 'z' (default 'y')."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_volumewrapper_setvoxel_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setVoxel",
		"summary": "Set a voxel at the specified coordinates.",
		"parameters": [
			{"name": "x", "type": "integer", "description": "The x coordinate."},
			{"name": "y", "type": "integer", "description": "The y coordinate."},
			{"name": "z", "type": "integer", "description": "The z coordinate."},
			{"name": "color", "type": "integer", "description": "The color index to set, or -1 for air (optional, default 1)."}
		],
		"returns": [
			{"type": "boolean", "description": "True if the voxel was set within the region, false otherwise."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

// Region jsonhelp functions
static int luaVoxel_region_width_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "width",
		"summary": "Get the width of the region in voxels.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The width of the region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_height_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "height",
		"summary": "Get the height of the region in voxels.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The height of the region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_depth_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "depth",
		"summary": "Get the depth of the region in voxels.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The depth of the region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_x_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "x",
		"summary": "Get the lower x coordinate of the region.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The lower x coordinate."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_y_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "y",
		"summary": "Get the lower y coordinate of the region.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The lower y coordinate."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_z_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "z",
		"summary": "Get the lower z coordinate of the region.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The lower z coordinate."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_center_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "center",
		"summary": "Get the center point of the region.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The center point of the region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_mins_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "mins",
		"summary": "Get the lower corner of the region.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The lower corner coordinates."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_maxs_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "maxs",
		"summary": "Get the upper corner of the region.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The upper corner coordinates."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_size_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "size",
		"summary": "Get the dimensions of the region.",
		"parameters": [],
		"returns": [
			{"type": "ivec3", "description": "The dimensions in voxels."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_isonborder_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isOnBorder",
		"summary": "Check if a position is on the border of the region.",
		"parameters": [
			{"name": "pos", "type": "ivec3", "description": "The position to check."}
		],
		"returns": [
			{"type": "boolean", "description": "True if on border, false otherwise."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_intersects_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "intersects",
		"summary": "Check if this region intersects with another region.",
		"parameters": [
			{"name": "other", "type": "region", "description": "The other region to check."}
		],
		"returns": [
			{"type": "boolean", "description": "True if regions intersect, false otherwise."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_contains_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "contains",
		"summary": "Check if this region fully contains another region.",
		"parameters": [
			{"name": "other", "type": "region", "description": "The other region to check."}
		],
		"returns": [
			{"type": "boolean", "description": "True if this region contains the other, false otherwise."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_setmins_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setMins",
		"summary": "Set the lower corner of the region.",
		"parameters": [
			{"name": "mins", "type": "ivec3", "description": "The new lower corner coordinates."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_setmaxs_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setMaxs",
		"summary": "Set the upper corner of the region.",
		"parameters": [
			{"name": "maxs", "type": "ivec3", "description": "The new upper corner coordinates."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_region_new_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "new",
		"summary": "Create a new region with the specified bounds.",
		"parameters": [
			{"name": "minX", "type": "integer", "description": "Minimum x coordinate."},
			{"name": "minY", "type": "integer", "description": "Minimum y coordinate."},
			{"name": "minZ", "type": "integer", "description": "Minimum z coordinate."},
			{"name": "maxX", "type": "integer", "description": "Maximum x coordinate."},
			{"name": "maxY", "type": "integer", "description": "Maximum y coordinate."},
			{"name": "maxZ", "type": "integer", "description": "Maximum z coordinate."}
		],
		"returns": [
			{"type": "region", "description": "The newly created region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

// Shape jsonhelp functions
static int luaVoxel_shape_cylinder_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "cylinder",
		"summary": "Create a cylinder shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "centerBottom", "type": "vec3", "description": "The center bottom position."},
			{"name": "axis", "type": "string", "description": "The axis: 'x', 'y', or 'z' (default 'y')."},
			{"name": "radius", "type": "integer", "description": "The radius of the cylinder."},
			{"name": "height", "type": "integer", "description": "The height of the cylinder."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_torus_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "torus",
		"summary": "Create a torus shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "center", "type": "ivec3", "description": "The center position."},
			{"name": "minorRadius", "type": "integer", "description": "The minor (tube) radius."},
			{"name": "majorRadius", "type": "integer", "description": "The major (ring) radius."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_ellipse_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "ellipse",
		"summary": "Create an ellipse (filled oval) shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "centerBottom", "type": "ivec3", "description": "The center bottom position."},
			{"name": "axis", "type": "string", "description": "The axis: 'x', 'y', or 'z' (default 'y')."},
			{"name": "width", "type": "integer", "description": "The width of the ellipse."},
			{"name": "height", "type": "integer", "description": "The height of the ellipse."},
			{"name": "depth", "type": "integer", "description": "The depth of the ellipse."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_dome_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "dome",
		"summary": "Create a dome (half ellipsoid) shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "centerBottom", "type": "ivec3", "description": "The center bottom position."},
			{"name": "axis", "type": "string", "description": "The axis: 'x', 'y', or 'z' (default 'y')."},
			{"name": "negative", "type": "boolean", "description": "Flip the dome direction (optional, default false)."},
			{"name": "width", "type": "integer", "description": "The width of the dome."},
			{"name": "height", "type": "integer", "description": "The height of the dome."},
			{"name": "depth", "type": "integer", "description": "The depth of the dome."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_cube_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "cube",
		"summary": "Create a cube shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "position", "type": "ivec3", "description": "The corner position."},
			{"name": "width", "type": "integer", "description": "The width of the cube."},
			{"name": "height", "type": "integer", "description": "The height of the cube."},
			{"name": "depth", "type": "integer", "description": "The depth of the cube."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_cone_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "cone",
		"summary": "Create a cone shape in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "centerBottom", "type": "ivec3", "description": "The center bottom position."},
			{"name": "axis", "type": "string", "description": "The axis: 'x', 'y', or 'z' (default 'y')."},
			{"name": "negative", "type": "boolean", "description": "Flip the cone direction (optional, default false)."},
			{"name": "width", "type": "integer", "description": "The width of the cone base."},
			{"name": "height", "type": "integer", "description": "The height of the cone."},
			{"name": "depth", "type": "integer", "description": "The depth of the cone base."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_line_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "line",
		"summary": "Draw a line between two points in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "start", "type": "ivec3", "description": "The start position."},
			{"name": "end", "type": "ivec3", "description": "The end position."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."},
			{"name": "thickness", "type": "integer", "description": "The line thickness (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shape_bezier_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "bezier",
		"summary": "Draw a quadratic bezier curve in the volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to draw in."},
			{"name": "start", "type": "ivec3", "description": "The start position."},
			{"name": "end", "type": "ivec3", "description": "The end position."},
			{"name": "control", "type": "ivec3", "description": "The control point."},
			{"name": "color", "type": "integer", "description": "The color index (optional, default 1)."},
			{"name": "thickness", "type": "integer", "description": "The line thickness (optional, default 1)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

// Noise jsonhelp functions
static int luaVoxel_noise_simplex2_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "noise2",
		"summary": "Generate 2D simplex noise.",
		"parameters": [
			{"name": "x", "type": "number", "description": "The x coordinate (or vec2)."},
			{"name": "y", "type": "number", "description": "The y coordinate (optional if vec2 provided)."}
		],
		"returns": [
			{"type": "number", "description": "Noise value in range [-1, 1]."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_simplex3_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "noise3",
		"summary": "Generate 3D simplex noise.",
		"parameters": [
			{"name": "x", "type": "number", "description": "The x coordinate (or vec3)."},
			{"name": "y", "type": "number", "description": "The y coordinate (optional if vec3 provided)."},
			{"name": "z", "type": "number", "description": "The z coordinate (optional if vec3 provided)."}
		],
		"returns": [
			{"type": "number", "description": "Noise value in range [-1, 1]."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_simplex4_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "noise4",
		"summary": "Generate 4D simplex noise.",
		"parameters": [
			{"name": "x", "type": "number", "description": "The x coordinate (or vec4)."},
			{"name": "y", "type": "number", "description": "The y coordinate (optional if vec4 provided)."},
			{"name": "z", "type": "number", "description": "The z coordinate (optional if vec4 provided)."},
			{"name": "w", "type": "number", "description": "The w coordinate (optional if vec4 provided)."}
		],
		"returns": [
			{"type": "number", "description": "Noise value in range [-1, 1]."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_fBm2_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "fBm2",
		"summary": "Generate 2D fractal Brownian motion noise.",
		"parameters": [
			{"name": "pos", "type": "vec2", "description": "The 2D position."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "fBm noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_fBm3_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "fBm3",
		"summary": "Generate 3D fractal Brownian motion noise.",
		"parameters": [
			{"name": "pos", "type": "vec3", "description": "The 3D position."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "fBm noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_fBm4_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "fBm4",
		"summary": "Generate 4D fractal Brownian motion noise.",
		"parameters": [
			{"name": "pos", "type": "vec4", "description": "The 4D position."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "fBm noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_voronoi_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "voronoi",
		"summary": "Generate Voronoi noise.",
		"parameters": [
			{"name": "pos", "type": "vec3", "description": "The 3D position."},
			{"name": "frequency", "type": "number", "description": "Frequency (optional, default 1.0)."},
			{"name": "seed", "type": "integer", "description": "Random seed (optional, default 0)."},
			{"name": "enableDistance", "type": "boolean", "description": "Enable distance output (optional, default true)."}
		],
		"returns": [
			{"type": "number", "description": "Voronoi noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_swissturbulence_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "swissTurbulence",
		"summary": "Generate Swiss turbulence noise.",
		"parameters": [
			{"name": "pos", "type": "vec2", "description": "The 2D position."},
			{"name": "offset", "type": "number", "description": "Offset (optional, default 1.0)."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.6)."},
			{"name": "warp", "type": "number", "description": "Warp amount (optional, default 0.15)."}
		],
		"returns": [
			{"type": "number", "description": "Swiss turbulence noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_ridgedMF2_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "ridgedMF2",
		"summary": "Generate 2D ridged multi-fractal noise.",
		"parameters": [
			{"name": "pos", "type": "vec2", "description": "The 2D position."},
			{"name": "ridgeOffset", "type": "number", "description": "Ridge offset (optional, default 1.0)."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "Ridged multi-fractal noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_ridgedMF3_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "ridgedMF3",
		"summary": "Generate 3D ridged multi-fractal noise.",
		"parameters": [
			{"name": "pos", "type": "vec3", "description": "The 3D position."},
			{"name": "ridgeOffset", "type": "number", "description": "Ridge offset (optional, default 1.0)."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "Ridged multi-fractal noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_ridgedMF4_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "ridgedMF4",
		"summary": "Generate 4D ridged multi-fractal noise.",
		"parameters": [
			{"name": "pos", "type": "vec4", "description": "The 4D position."},
			{"name": "ridgeOffset", "type": "number", "description": "Ridge offset (optional, default 1.0)."},
			{"name": "octaves", "type": "integer", "description": "Number of octaves (optional, default 4)."},
			{"name": "lacunarity", "type": "number", "description": "Lacunarity (optional, default 2.0)."},
			{"name": "gain", "type": "number", "description": "Gain (optional, default 0.5)."}
		],
		"returns": [
			{"type": "number", "description": "Ridged multi-fractal noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_worley2_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "worley2",
		"summary": "Generate 2D Worley (cellular) noise.",
		"parameters": [
			{"name": "pos", "type": "vec2", "description": "The 2D position."}
		],
		"returns": [
			{"type": "number", "description": "Worley noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_noise_worley3_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "worley3",
		"summary": "Generate 3D Worley (cellular) noise.",
		"parameters": [
			{"name": "pos", "type": "vec3", "description": "The 3D position."}
		],
		"returns": [
			{"type": "number", "description": "Worley noise value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_colors_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "colors",
		"summary": "Get all colors in the palette as a table of vec4.",
		"parameters": [],
		"returns": [
			{"type": "table", "description": "Table of vec4 colors (RGBA, 0-1 range)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_color_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "color",
		"summary": "Get a color from the palette as vec4.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."}
		],
		"returns": [
			{"type": "vec4", "description": "The color as RGBA vec4 (0-1 range)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_size_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "size",
		"summary": "Get the number of colors in the palette.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The number of colors."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_rgba_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "rgba",
		"summary": "Get a color from the palette as separate RGBA components.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."}
		],
		"returns": [
			{"type": "integer", "description": "Red component (0-255)."},
			{"type": "integer", "description": "Green component (0-255)."},
			{"type": "integer", "description": "Blue component (0-255)."},
			{"type": "integer", "description": "Alpha component (0-255)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_load_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "load",
		"summary": "Load a palette from a file or built-in name.",
		"parameters": [
			{"name": "name", "type": "string", "description": "File path or built-in palette name (e.g., 'built-in:minecraft')."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_setcolor_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setColor",
		"summary": "Set a color in the palette.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."},
			{"name": "r", "type": "integer", "description": "Red component (0-255)."},
			{"name": "g", "type": "integer", "description": "Green component (0-255)."},
			{"name": "b", "type": "integer", "description": "Blue component (0-255)."},
			{"name": "a", "type": "integer", "description": "Alpha component (0-255, optional, default 255)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_closestmatch_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "match",
		"summary": "Find the closest matching color in the palette.",
		"parameters": [
			{"name": "r", "type": "integer", "description": "Red component (0-255)."},
			{"name": "g", "type": "integer", "description": "Green component (0-255)."},
			{"name": "b", "type": "integer", "description": "Blue component (0-255)."},
			{"name": "skipIndex", "type": "integer", "description": "Index to skip (optional, default -1)."}
		],
		"returns": [
			{"type": "integer", "description": "The index of the closest matching color."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_similar_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "similar",
		"summary": "Find similar colors in the palette.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The reference color index."},
			{"name": "count", "type": "integer", "description": "Number of similar colors to find."}
		],
		"returns": [
			{"type": "table", "description": "Table of similar color indices, or nil if none found."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_setmaterialproperty_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setMaterial",
		"summary": "Set a material property for a palette color.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."},
			{"name": "property", "type": "string", "description": "The property name."},
			{"name": "value", "type": "number", "description": "The property value."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_materialproperty_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "material",
		"summary": "Get a material property for a palette color.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."},
			{"name": "property", "type": "string", "description": "The property name."}
		],
		"returns": [
			{"type": "number", "description": "The property value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_delta_e_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "deltaE",
		"summary": "Calculate the perceptual color difference (Delta E 76) between two palette colors.",
		"parameters": [
			{"name": "index1", "type": "integer", "description": "First color index."},
			{"name": "index2", "type": "integer", "description": "Second color index."}
		],
		"returns": [
			{"type": "number", "description": "The Delta E value (0 = identical colors)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_color_to_string_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "colorString",
		"summary": "Get a string representation of a palette color.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The color index (0-255)."}
		],
		"returns": [
			{"type": "string", "description": "String representation of the color."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_palette_new_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "new",
		"summary": "Create a new empty palette.",
		"parameters": [],
		"returns": [
			{"type": "palette", "description": "The newly created palette."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_align_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "align",
		"summary": "Align all nodes in the scene graph.",
		"parameters": [
			{"name": "padding", "type": "integer", "description": "Padding between nodes (optional, default 2)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_new_node_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "new",
		"summary": "Create a new node in the scene graph.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The node name."},
			{"name": "region", "type": "region", "description": "The region for model nodes (or boolean for visibility)."},
			{"name": "visible", "type": "boolean", "description": "Whether the node is visible (optional, default true)."},
			{"name": "type", "type": "string", "description": "Node type: 'Model', 'Group', 'Camera', 'Point' (optional, default 'Group')."}
		],
		"returns": [
			{"type": "node", "description": "The newly created node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_get_node_by_id_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "get",
		"summary": "Get a node by its ID.",
		"parameters": [
			{"name": "id", "type": "integer", "description": "The node ID (optional, defaults to active node)."}
		],
		"returns": [
			{"type": "node", "description": "The node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_get_node_by_name_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "getByName",
		"summary": "Get a node by its name.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The node name."}
		],
		"returns": [
			{"type": "node", "description": "The node, or nil if not found."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_get_node_by_uuid_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "getByUUID",
		"summary": "Get a node by its UUID.",
		"parameters": [
			{"name": "uuid", "type": "string", "description": "The node UUID."}
		],
		"returns": [
			{"type": "node", "description": "The node, or nil if not found."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_get_all_node_ids_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "nodeIds",
		"summary": "Get all node IDs in the scene graph.",
		"parameters": [],
		"returns": [
			{"type": "table", "description": "Table of node IDs."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_updatetransforms_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "updateTransforms",
		"summary": "Update all transforms in the scene graph.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_addanimation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "addAnimation",
		"summary": "Add a new animation to the scene graph.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The animation name."}
		],
		"returns": [
			{"type": "boolean", "description": "True if animation was added successfully."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_setanimation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setAnimation",
		"summary": "Set the active animation.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The animation name."}
		],
		"returns": [
			{"type": "boolean", "description": "True if animation was set successfully."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_duplicateanimation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "duplicateAnimation",
		"summary": "Duplicate an existing animation.",
		"parameters": [
			{"name": "source", "type": "string", "description": "The source animation name."},
			{"name": "target", "type": "string", "description": "The new animation name."}
		],
		"returns": [
			{"type": "boolean", "description": "True if animation was duplicated successfully."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_hasanimation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "hasAnimation",
		"summary": "Check if an animation exists.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The animation name."}
		],
		"returns": [
			{"type": "boolean", "description": "True if animation exists."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_activeanimation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "activeAnimation",
		"summary": "Get the name of the active animation.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "The active animation name."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraph_animations_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "animations",
		"summary": "Get all animation names.",
		"parameters": [],
		"returns": [
			{"type": "table", "description": "A table of animation names."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_name_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "name",
		"summary": "Get the name of the node.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "The node name."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_id_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "id",
		"summary": "Get the ID of the node.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The node ID."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_uuid_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "uuid",
		"summary": "Get the UUID of the node.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "The node UUID."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_clone_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "clone",
		"summary": "Create a copy of the node.",
		"parameters": [],
		"returns": [
			{"type": "node", "description": "The cloned node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_parent_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "parent",
		"summary": "Get the parent node ID.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The parent node ID."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_volume_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "volume",
		"summary": "Get the volume of a model node.",
		"parameters": [],
		"returns": [
			{"type": "volume", "description": "The volume."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_is_model_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isModel",
		"summary": "Check if the node is a model node.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if this is a model node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_is_modelref_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isReference",
		"summary": "Check if the node is a model reference node.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if this is a reference node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_is_point_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isPoint",
		"summary": "Check if the node is a point node.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if this is a point node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_is_camera_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isCamera",
		"summary": "Check if the node is a camera node.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if this is a camera node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_is_group_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isGroup",
		"summary": "Check if the node is a group node.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if this is a group node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_palette_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "palette",
		"summary": "Get the palette of the node.",
		"parameters": [],
		"returns": [
			{"type": "palette", "description": "The node's palette."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_setname_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setName",
		"summary": "Set the name of the node.",
		"parameters": [
			{"name": "name", "type": "string", "description": "The new name."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_setpalette_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setPalette",
		"summary": "Set the palette of the node.",
		"parameters": [
			{"name": "palette", "type": "palette", "description": "The new palette."},
			{"name": "remap", "type": "boolean", "description": "Remap existing colors (optional, default false)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_setpivot_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setPivot",
		"summary": "Set the pivot point of the node.",
		"parameters": [
			{"name": "pivot", "type": "vec3", "description": "The new pivot point."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_pivot_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "pivot",
		"summary": "Get the normalized pivot point of the node.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The pivot point (normalized 0-1 range)."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_numkeyframes_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "numKeyFrames",
		"summary": "Get the number of keyframes for the current animation.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The number of keyframes."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_children_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "children",
		"summary": "Get the child node IDs.",
		"parameters": [],
		"returns": [
			{"type": "table", "description": "A table of child node IDs."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_region_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "region",
		"summary": "Get the region of the model node.",
		"parameters": [],
		"returns": [
			{"type": "region", "description": "The node's region."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_keyframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "keyFrame",
		"summary": "Get a keyframe by index.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The keyframe index."}
		],
		"returns": [
			{"type": "keyframe", "description": "The keyframe."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_keyframeforframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "keyFrameForFrame",
		"summary": "Get the keyframe for a specific frame number.",
		"parameters": [
			{"name": "frame", "type": "integer", "description": "The frame number."}
		],
		"returns": [
			{"type": "keyframe", "description": "The keyframe."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_addframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "addKeyFrame",
		"summary": "Add a new keyframe at the specified frame.",
		"parameters": [
			{"name": "frame", "type": "integer", "description": "The frame number."},
			{"name": "interpolation", "type": "integer", "description": "Interpolation type (optional, default Linear)."}
		],
		"returns": [
			{"type": "keyframe", "description": "The newly created keyframe."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_hasframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "hasKeyFrameForFrame",
		"summary": "Check if a keyframe exists at the specified frame.",
		"parameters": [
			{"name": "frame", "type": "integer", "description": "The frame number."}
		],
		"returns": [
			{"type": "boolean", "description": "True if keyframe exists."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_removekeyframeforframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "removeKeyFrameForFrame",
		"summary": "Remove the keyframe at the specified frame.",
		"parameters": [
			{"name": "frame", "type": "integer", "description": "The frame number."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_removekeyframe_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "removeKeyFrame",
		"summary": "Remove a keyframe by index.",
		"parameters": [
			{"name": "index", "type": "integer", "description": "The keyframe index."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_hide_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "hide",
		"summary": "Hide the node.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_show_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "show",
		"summary": "Show the node.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_lock_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "lock",
		"summary": "Lock the node.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_unlock_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "unlock",
		"summary": "Unlock the node.",
		"parameters": [],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_isvisible_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isVisible",
		"summary": "Check if the node is visible.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if node is visible."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_islocked_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "isLocked",
		"summary": "Check if the node is locked.",
		"parameters": [],
		"returns": [
			{"type": "boolean", "description": "True if node is locked."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_setproperty_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setProperty",
		"summary": "Set a custom property on the node.",
		"parameters": [
			{"name": "key", "type": "string", "description": "The property key."},
			{"name": "value", "type": "string", "description": "The property value."}
		],
		"returns": [
			{"type": "boolean", "description": "True if property was set successfully."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_scenegraphnode_property_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "property",
		"summary": "Get a custom property from the node.",
		"parameters": [
			{"name": "key", "type": "string", "description": "The property key."}
		],
		"returns": [
			{"type": "string", "description": "The property value."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_index_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "index",
		"summary": "Get the keyframe index.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The keyframe index."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_frame_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "frame",
		"summary": "Get the frame number of this keyframe.",
		"parameters": [],
		"returns": [
			{"type": "integer", "description": "The frame number."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_interpolation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "interpolation",
		"summary": "Get the interpolation type.",
		"parameters": [],
		"returns": [
			{"type": "string", "description": "The interpolation type name."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setinterpolation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setInterpolation",
		"summary": "Set the interpolation type.",
		"parameters": [
			{"name": "type", "type": "string", "description": "The interpolation type: 'Instant', 'Linear', 'QuadEaseIn', etc."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_localscale_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "localScale",
		"summary": "Get the local scale.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The local scale."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setlocalscale_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setLocalScale",
		"summary": "Set the local scale.",
		"parameters": [
			{"name": "scale", "type": "vec3", "description": "The new local scale."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_localorientation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "localOrientation",
		"summary": "Get the local orientation quaternion.",
		"parameters": [],
		"returns": [
			{"type": "quat", "description": "The local orientation."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setlocalorientation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setLocalOrientation",
		"summary": "Set the local orientation.",
		"parameters": [
			{"name": "orientation", "type": "quat", "description": "The new local orientation (quaternion or x,y,z,w components)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_localtranslation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "localTranslation",
		"summary": "Get the local translation.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The local translation."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setlocaltranslation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setLocalTranslation",
		"summary": "Set the local translation.",
		"parameters": [
			{"name": "translation", "type": "vec3", "description": "The new local translation."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_worldscale_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "worldScale",
		"summary": "Get the world scale.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The world scale."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setworldscale_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setWorldScale",
		"summary": "Set the world scale.",
		"parameters": [
			{"name": "scale", "type": "vec3", "description": "The new world scale."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_worldorientation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "worldOrientation",
		"summary": "Get the world orientation quaternion.",
		"parameters": [],
		"returns": [
			{"type": "quat", "description": "The world orientation."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setworldorientation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setWorldOrientation",
		"summary": "Set the world orientation.",
		"parameters": [
			{"name": "orientation", "type": "quat", "description": "The new world orientation (quaternion or x,y,z,w components)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_worldtranslation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "worldTranslation",
		"summary": "Get the world translation.",
		"parameters": [],
		"returns": [
			{"type": "vec3", "description": "The world translation."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_keyframe_setworldtranslation_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "setWorldTranslation",
		"summary": "Set the world translation.",
		"parameters": [
			{"name": "translation", "type": "vec3", "description": "The new world translation."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_load_palette_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "palette",
		"summary": "Load a palette from a stream.",
		"parameters": [
			{"name": "filename", "type": "string", "description": "The filename for format detection."},
			{"name": "stream", "type": "stream", "description": "The stream to read from."}
		],
		"returns": [
			{"type": "palette", "description": "The loaded palette."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_load_image_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "image",
		"summary": "Load an image from a stream.",
		"parameters": [
			{"name": "filename", "type": "string", "description": "The filename for format detection."},
			{"name": "stream", "type": "stream", "description": "The stream to read from."}
		],
		"returns": [
			{"type": "image", "description": "The loaded image."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_import_scene_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "scene",
		"summary": "Import a scene from a file or stream.",
		"parameters": [
			{"name": "filename", "type": "string", "description": "The filename to load."},
			{"name": "stream", "type": "stream", "description": "Optional stream to read from."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_import_imageasplane_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "imageAsPlane",
		"summary": "Import an image as a voxel plane.",
		"parameters": [
			{"name": "image", "type": "image", "description": "The image to convert."},
			{"name": "palette", "type": "palette", "description": "The palette to use."},
			{"name": "thickness", "type": "integer", "description": "The plane thickness (optional, default 1)."}
		],
		"returns": [
			{"type": "node", "description": "The created node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_genland_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "genland",
		"summary": "Generate procedural terrain.",
		"parameters": [
			{"name": "seed", "type": "integer", "description": "Random seed (optional, default 0)."},
			{"name": "size", "type": "integer", "description": "Terrain size (optional, default 256)."},
			{"name": "height", "type": "integer", "description": "Max height (optional, default 64)."},
			{"name": "octaves", "type": "integer", "description": "Noise octaves (optional, default 10)."},
			{"name": "smoothing", "type": "number", "description": "Smoothing factor (optional, default 1)."},
			{"name": "persistence", "type": "number", "description": "Noise persistence (optional, default 0.4)."},
			{"name": "amplitude", "type": "number", "description": "Noise amplitude (optional, default 0.4)."},
			{"name": "riverWidth", "type": "number", "description": "River width (optional, default 0.02)."},
			{"name": "freqGround", "type": "number", "description": "Ground frequency (optional, default 9.5)."},
			{"name": "freqRiver", "type": "number", "description": "River frequency (optional, default 13.2)."},
			{"name": "offsetX", "type": "integer", "description": "X offset (optional, default 0)."},
			{"name": "offsetZ", "type": "integer", "description": "Z offset (optional, default 0)."},
			{"name": "shadow", "type": "boolean", "description": "Add shadows (optional, default true)."},
			{"name": "river", "type": "boolean", "description": "Add rivers (optional, default true)."},
			{"name": "ambience", "type": "boolean", "description": "Add ambient effects (optional, default true)."}
		],
		"returns": [
			{"type": "node", "description": "The generated terrain node."}
		]})";
	lua_pushstring(s, json);
	return 1;
}

static int luaVoxel_shadow_jsonhelp(lua_State* s) {
	const char *json = R"({
		"name": "shadow",
		"summary": "Add shadow coloring to a volume.",
		"parameters": [
			{"name": "volume", "type": "volume", "description": "The volume to add shadows to."},
			{"name": "lightStep", "type": "integer", "description": "Light step value (optional, default 8)."}
		],
		"returns": []})";
	lua_pushstring(s, json);
	return 1;
}

static void prepareState(lua_State* s) {
	static const clua_Reg volumeFuncs[] = {
		{"voxel", luaVoxel_volumewrapper_voxel, luaVoxel_volumewrapper_voxel_jsonhelp},
		{"region", luaVoxel_volumewrapper_region, luaVoxel_volumewrapper_region_jsonhelp},
		{"translate", luaVoxel_volumewrapper_translate, luaVoxel_volumewrapper_translate_jsonhelp},
		{"move", luaVoxel_volumewrapper_move, luaVoxel_volumewrapper_move_jsonhelp},
		{"resize", luaVoxel_volumewrapper_resize, luaVoxel_volumewrapper_resize_jsonhelp},
		{"crop", luaVoxel_volumewrapper_crop, luaVoxel_volumewrapper_crop_jsonhelp},
		{"text", luaVoxel_volumewrapper_text, luaVoxel_volumewrapper_text_jsonhelp},
		{"fillHollow", luaVoxel_volumewrapper_fillhollow, luaVoxel_volumewrapper_fillhollow_jsonhelp},
		{"hollow", luaVoxel_volumewrapper_hollow, luaVoxel_volumewrapper_hollow_jsonhelp},
		{"importHeightmap", luaVoxel_volumewrapper_importheightmap, luaVoxel_volumewrapper_importheightmap_jsonhelp},
		{"importColoredHeightmap", luaVoxel_volumewrapper_importcoloredheightmap, luaVoxel_volumewrapper_importcoloredheightmap_jsonhelp},
		{"importImageAsVolume", luaVoxel_volumewrapper_importimageasvolume, luaVoxel_volumewrapper_importimageasvolume_jsonhelp},
		{"mirrorAxis", luaVoxel_volumewrapper_mirroraxis, luaVoxel_volumewrapper_mirroraxis_jsonhelp},
		{"rotateAxis", luaVoxel_volumewrapper_rotateaxis, luaVoxel_volumewrapper_rotateaxis_jsonhelp},
		{"setVoxel", luaVoxel_volumewrapper_setvoxel, luaVoxel_volumewrapper_setvoxel_jsonhelp},
		{"__gc", luaVoxel_volumewrapper_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, volumeFuncs, luaVoxel_metavolumewrapper());

	static const clua_Reg regionFuncs[] = {
		{"width", luaVoxel_region_width, luaVoxel_region_width_jsonhelp},
		{"height", luaVoxel_region_height, luaVoxel_region_height_jsonhelp},
		{"depth", luaVoxel_region_depth, luaVoxel_region_depth_jsonhelp},
		{"x", luaVoxel_region_x, luaVoxel_region_x_jsonhelp},
		{"y", luaVoxel_region_y, luaVoxel_region_y_jsonhelp},
		{"z", luaVoxel_region_z, luaVoxel_region_z_jsonhelp},
		{"center", luaVoxel_region_center, luaVoxel_region_center_jsonhelp},
		{"mins", luaVoxel_region_mins, luaVoxel_region_mins_jsonhelp},
		{"maxs", luaVoxel_region_maxs, luaVoxel_region_maxs_jsonhelp},
		{"size", luaVoxel_region_size, luaVoxel_region_size_jsonhelp},
		{"__tostring", luaVoxel_region_tostring, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs, luaVoxel_metaregion());

	static const clua_Reg regionFuncs_gc[] = {
		{"width", luaVoxel_region_width, luaVoxel_region_width_jsonhelp},
		{"height", luaVoxel_region_height, luaVoxel_region_height_jsonhelp},
		{"depth", luaVoxel_region_depth, luaVoxel_region_depth_jsonhelp},
		{"x", luaVoxel_region_x, luaVoxel_region_x_jsonhelp},
		{"y", luaVoxel_region_y, luaVoxel_region_y_jsonhelp},
		{"z", luaVoxel_region_z, luaVoxel_region_z_jsonhelp},
		{"isOnBorder", luaVoxel_region_isonborder, luaVoxel_region_isonborder_jsonhelp},
		{"center", luaVoxel_region_center, luaVoxel_region_center_jsonhelp},
		{"mins", luaVoxel_region_mins, luaVoxel_region_mins_jsonhelp},
		{"maxs", luaVoxel_region_maxs, luaVoxel_region_maxs_jsonhelp},
		{"size", luaVoxel_region_size, luaVoxel_region_size_jsonhelp},
		{"intersects", luaVoxel_region_intersects, luaVoxel_region_intersects_jsonhelp},
		{"contains", luaVoxel_region_contains, luaVoxel_region_contains_jsonhelp},
		{"setMins", luaVoxel_region_setmins, luaVoxel_region_setmins_jsonhelp},
		{"setMaxs", luaVoxel_region_setmaxs, luaVoxel_region_setmaxs_jsonhelp},
		{"__tostring", luaVoxel_region_tostring, nullptr},
		{"__eq", luaVoxel_region_eq, nullptr},
		{"__gc", luaVoxel_region_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, regionFuncs_gc, luaVoxel_metaregion_gc());

	static const clua_Reg globalRegionFuncs[] = {
		{"new", luaVoxel_region_new, luaVoxel_region_new_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, globalRegionFuncs, luaVoxel_metaregionglobal(), "g_region");

	static const clua_Reg sceneGraphFuncs[] = {
		{"align", luaVoxel_scenegraph_align, luaVoxel_scenegraph_align_jsonhelp},
		{"new", luaVoxel_scenegraph_new_node, luaVoxel_scenegraph_new_node_jsonhelp},
		{"get", luaVoxel_scenegraph_get_node_by_id, luaVoxel_scenegraph_get_node_by_id_jsonhelp},
		{"getByName", luaVoxel_scenegraph_get_node_by_name, luaVoxel_scenegraph_get_node_by_name_jsonhelp},
		{"getByUUID", luaVoxel_scenegraph_get_node_by_uuid, luaVoxel_scenegraph_get_node_by_uuid_jsonhelp},
		{"nodeIds", luaVoxel_scenegraph_get_all_node_ids, luaVoxel_scenegraph_get_all_node_ids_jsonhelp},
		{"updateTransforms", luaVoxel_scenegraph_updatetransforms, luaVoxel_scenegraph_updatetransforms_jsonhelp},
		{"addAnimation", luaVoxel_scenegraph_addanimation, luaVoxel_scenegraph_addanimation_jsonhelp},
		{"setAnimation", luaVoxel_scenegraph_setanimation, luaVoxel_scenegraph_setanimation_jsonhelp},
		{"duplicateAnimation", luaVoxel_scenegraph_duplicateanimation, luaVoxel_scenegraph_duplicateanimation_jsonhelp},
		{"hasAnimation", luaVoxel_scenegraph_hasanimation, luaVoxel_scenegraph_hasanimation_jsonhelp},
		{"activeAnimation", luaVoxel_scenegraph_activeanimation, luaVoxel_scenegraph_activeanimation_jsonhelp},
		{"animations", luaVoxel_scenegraph_animations, luaVoxel_scenegraph_animations_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, sceneGraphFuncs, luaVoxel_metascenegraph(), "g_scenegraph");

	static const clua_Reg sceneGraphNodeFuncs[] = {
		{"name", luaVoxel_scenegraphnode_name, luaVoxel_scenegraphnode_name_jsonhelp},
		{"id", luaVoxel_scenegraphnode_id, luaVoxel_scenegraphnode_id_jsonhelp},
		{"uuid", luaVoxel_scenegraphnode_uuid, luaVoxel_scenegraphnode_uuid_jsonhelp},
		{"clone", luaVoxel_scenegraphnode_clone, luaVoxel_scenegraphnode_clone_jsonhelp},
		{"parent", luaVoxel_scenegraphnode_parent, luaVoxel_scenegraphnode_parent_jsonhelp},
		{"volume", luaVoxel_scenegraphnode_volume, luaVoxel_scenegraphnode_volume_jsonhelp},
		{"isModel", luaVoxel_scenegraphnode_is_model, luaVoxel_scenegraphnode_is_model_jsonhelp},
		{"isReference", luaVoxel_scenegraphnode_is_modelref, luaVoxel_scenegraphnode_is_modelref_jsonhelp},
		{"isPoint", luaVoxel_scenegraphnode_is_point, luaVoxel_scenegraphnode_is_point_jsonhelp},
		{"isCamera", luaVoxel_scenegraphnode_is_camera, luaVoxel_scenegraphnode_is_camera_jsonhelp},
		{"isGroup", luaVoxel_scenegraphnode_is_group, luaVoxel_scenegraphnode_is_group_jsonhelp},
		{"palette", luaVoxel_scenegraphnode_palette, luaVoxel_scenegraphnode_palette_jsonhelp},
		{"setName", luaVoxel_scenegraphnode_setname, luaVoxel_scenegraphnode_setname_jsonhelp},
		{"setPalette", luaVoxel_scenegraphnode_setpalette, luaVoxel_scenegraphnode_setpalette_jsonhelp},
		{"setPivot", luaVoxel_scenegraphnode_setpivot, luaVoxel_scenegraphnode_setpivot_jsonhelp},
		{"pivot", luaVoxel_scenegraphnode_pivot, luaVoxel_scenegraphnode_pivot_jsonhelp},
		{"numKeyFrames", luaVoxel_scenegraphnode_numkeyframes, luaVoxel_scenegraphnode_numkeyframes_jsonhelp},
		{"children", luaVoxel_scenegraphnode_children, luaVoxel_scenegraphnode_children_jsonhelp},
		{"region", luaVoxel_scenegraphnode_region, luaVoxel_scenegraphnode_region_jsonhelp},
		{"hide", luaVoxel_scenegraphnode_hide, luaVoxel_scenegraphnode_hide_jsonhelp},
		{"show", luaVoxel_scenegraphnode_show, luaVoxel_scenegraphnode_show_jsonhelp},
		{"lock", luaVoxel_scenegraphnode_lock, luaVoxel_scenegraphnode_lock_jsonhelp},
		{"unlock", luaVoxel_scenegraphnode_unlock, luaVoxel_scenegraphnode_unlock_jsonhelp},
		{"isVisible", luaVoxel_scenegraphnode_isvisible, luaVoxel_scenegraphnode_isvisible_jsonhelp},
		{"isLocked", luaVoxel_scenegraphnode_islocked, luaVoxel_scenegraphnode_islocked_jsonhelp},
		{"setProperty", luaVoxel_scenegraphnode_setproperty, luaVoxel_scenegraphnode_setproperty_jsonhelp},
		{"property", luaVoxel_scenegraphnode_property, luaVoxel_scenegraphnode_property_jsonhelp},
		{"keyFrame", luaVoxel_scenegraphnode_keyframe, luaVoxel_scenegraphnode_keyframe_jsonhelp},
		{"keyFrameForFrame", luaVoxel_scenegraphnode_keyframeforframe, luaVoxel_scenegraphnode_keyframeforframe_jsonhelp},
		{"addKeyFrame", luaVoxel_scenegraphnode_addframe, luaVoxel_scenegraphnode_addframe_jsonhelp},
		{"hasKeyFrameForFrame", luaVoxel_scenegraphnode_hasframe, luaVoxel_scenegraphnode_hasframe_jsonhelp},
		{"removeKeyFrameForFrame", luaVoxel_scenegraphnode_removekeyframeforframe, luaVoxel_scenegraphnode_removekeyframeforframe_jsonhelp},
		{"removeKeyFrame", luaVoxel_scenegraphnode_removekeyframe, luaVoxel_scenegraphnode_removekeyframe_jsonhelp},
		{"__tostring", luaVoxel_scenegraphnode_tostring, nullptr},
		{"__gc", luaVoxel_scenegraphnode_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, sceneGraphNodeFuncs, luaVoxel_metascenegraphnode());

	static const clua_Reg keyframeFuncs[] = {
		{"index", luaVoxel_keyframe_index, luaVoxel_keyframe_index_jsonhelp},
		{"frame", luaVoxel_keyframe_frame, luaVoxel_keyframe_frame_jsonhelp},
		{"interpolation", luaVoxel_keyframe_interpolation, luaVoxel_keyframe_interpolation_jsonhelp},
		{"setInterpolation", luaVoxel_keyframe_setinterpolation, luaVoxel_keyframe_setinterpolation_jsonhelp},
		{"localScale", luaVoxel_keyframe_localscale, luaVoxel_keyframe_localscale_jsonhelp},
		{"setLocalScale", luaVoxel_keyframe_setlocalscale, luaVoxel_keyframe_setlocalscale_jsonhelp},
		{"localOrientation", luaVoxel_keyframe_localorientation, luaVoxel_keyframe_localorientation_jsonhelp},
		{"setLocalOrientation", luaVoxel_keyframe_setlocalorientation, luaVoxel_keyframe_setlocalorientation_jsonhelp},
		{"localTranslation", luaVoxel_keyframe_localtranslation, luaVoxel_keyframe_localtranslation_jsonhelp},
		{"setLocalTranslation", luaVoxel_keyframe_setlocaltranslation, luaVoxel_keyframe_setlocaltranslation_jsonhelp},
		{"worldScale", luaVoxel_keyframe_worldscale, luaVoxel_keyframe_worldscale_jsonhelp},
		{"setWorldScale", luaVoxel_keyframe_setworldscale, luaVoxel_keyframe_setworldscale_jsonhelp},
		{"worldOrientation", luaVoxel_keyframe_worldorientation, luaVoxel_keyframe_worldorientation_jsonhelp},
		{"setWorldOrientation", luaVoxel_keyframe_setworldorientation, luaVoxel_keyframe_setworldorientation_jsonhelp},
		{"worldTranslation", luaVoxel_keyframe_worldtranslation, luaVoxel_keyframe_worldtranslation_jsonhelp},
		{"setWorldTranslation", luaVoxel_keyframe_setworldtranslation, luaVoxel_keyframe_setworldtranslation_jsonhelp},
		{"__tostring", luaVoxel_keyframe_tostring, nullptr},
		{"__gc", luaVoxel_keyframe_gc, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, keyframeFuncs, luaVoxel_metakeyframe());

	static const clua_Reg paletteFuncs[] = {
		{"colors", luaVoxel_palette_colors, luaVoxel_palette_colors_jsonhelp},
		{"color", luaVoxel_palette_color, luaVoxel_palette_color_jsonhelp},
		{"size", luaVoxel_palette_size, luaVoxel_palette_size_jsonhelp},
		{"rgba", luaVoxel_palette_rgba, luaVoxel_palette_rgba_jsonhelp},
		{"load", luaVoxel_palette_load, luaVoxel_palette_load_jsonhelp},
		{"setColor", luaVoxel_palette_setcolor, luaVoxel_palette_setcolor_jsonhelp},
		{"match", luaVoxel_palette_closestmatch, luaVoxel_palette_closestmatch_jsonhelp},
		{"similar", luaVoxel_palette_similar, luaVoxel_palette_similar_jsonhelp},
		{"setMaterial", luaVoxel_palette_setmaterialproperty, luaVoxel_palette_setmaterialproperty_jsonhelp},
		{"material", luaVoxel_palette_materialproperty, luaVoxel_palette_materialproperty_jsonhelp},
		{"deltaE", luaVoxel_palette_delta_e, luaVoxel_palette_delta_e_jsonhelp},
		{"colorString", luaVoxel_palette_color_to_string, luaVoxel_palette_color_to_string_jsonhelp},
		{"__tostring", luaVoxel_palette_tostring, nullptr},
		{"__eq", luaVoxel_palette_eq, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs, luaVoxel_metapalette());

	static const clua_Reg paletteFuncs_gc[] = {
		{"colors", luaVoxel_palette_colors, luaVoxel_palette_colors_jsonhelp},
		{"color", luaVoxel_palette_color, luaVoxel_palette_color_jsonhelp},
		{"size", luaVoxel_palette_size, luaVoxel_palette_size_jsonhelp},
		{"rgba", luaVoxel_palette_rgba, luaVoxel_palette_rgba_jsonhelp},
		{"load", luaVoxel_palette_load, luaVoxel_palette_load_jsonhelp},
		{"setColor", luaVoxel_palette_setcolor, luaVoxel_palette_setcolor_jsonhelp},
		{"match", luaVoxel_palette_closestmatch, luaVoxel_palette_closestmatch_jsonhelp},
		{"similar", luaVoxel_palette_similar, luaVoxel_palette_similar_jsonhelp},
		{"setMaterial", luaVoxel_palette_setmaterialproperty, luaVoxel_palette_setmaterialproperty_jsonhelp},
		{"material", luaVoxel_palette_materialproperty, luaVoxel_palette_materialproperty_jsonhelp},
		{"deltaE", luaVoxel_palette_delta_e, luaVoxel_palette_delta_e_jsonhelp},
		{"colorString", luaVoxel_palette_color_to_string, luaVoxel_palette_color_to_string_jsonhelp},
		{"__tostring", luaVoxel_palette_tostring, nullptr},
		{"__gc", luaVoxel_palette_gc, nullptr},
		{"__eq", luaVoxel_palette_eq, nullptr},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncs(s, paletteFuncs_gc, luaVoxel_metapalette_gc());

	static const clua_Reg paletteGlobalsFuncs[] = {
		{"new", luaVoxel_palette_new, luaVoxel_palette_new_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, paletteGlobalsFuncs, luaVoxel_metapaletteglobal(), "g_palette");

	static const clua_Reg noiseFuncs[] = {
		{"noise2", luaVoxel_noise_simplex2, luaVoxel_noise_simplex2_jsonhelp},
		{"noise3", luaVoxel_noise_simplex3, luaVoxel_noise_simplex3_jsonhelp},
		{"noise4", luaVoxel_noise_simplex4, luaVoxel_noise_simplex4_jsonhelp},
		{"fBm2", luaVoxel_noise_fBm2, luaVoxel_noise_fBm2_jsonhelp},
		{"fBm3", luaVoxel_noise_fBm3, luaVoxel_noise_fBm3_jsonhelp},
		{"fBm4", luaVoxel_noise_fBm4, luaVoxel_noise_fBm4_jsonhelp},
		{"swissTurbulence", luaVoxel_noise_swissturbulence, luaVoxel_noise_swissturbulence_jsonhelp},
		{"voronoi", luaVoxel_noise_voronoi, luaVoxel_noise_voronoi_jsonhelp},
		{"ridgedMF2", luaVoxel_noise_ridgedMF2, luaVoxel_noise_ridgedMF2_jsonhelp},
		{"ridgedMF3", luaVoxel_noise_ridgedMF3, luaVoxel_noise_ridgedMF3_jsonhelp},
		{"ridgedMF4", luaVoxel_noise_ridgedMF4, luaVoxel_noise_ridgedMF4_jsonhelp},
		{"worley2", luaVoxel_noise_worley2, luaVoxel_noise_worley2_jsonhelp},
		{"worley3", luaVoxel_noise_worley3, luaVoxel_noise_worley3_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, noiseFuncs, luaVoxel_metanoise(), "g_noise");

	static const clua_Reg shapeFuncs[] = {
		{"cylinder", luaVoxel_shape_cylinder, luaVoxel_shape_cylinder_jsonhelp},
		{"torus", luaVoxel_shape_torus, luaVoxel_shape_torus_jsonhelp},
		{"ellipse", luaVoxel_shape_ellipse, luaVoxel_shape_ellipse_jsonhelp},
		{"dome", luaVoxel_shape_dome, luaVoxel_shape_dome_jsonhelp},
		{"cube", luaVoxel_shape_cube, luaVoxel_shape_cube_jsonhelp},
		{"cone", luaVoxel_shape_cone, luaVoxel_shape_cone_jsonhelp},
		{"line", luaVoxel_shape_line, luaVoxel_shape_line_jsonhelp},
		{"bezier", luaVoxel_shape_bezier, luaVoxel_shape_bezier_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, shapeFuncs, luaVoxel_metashape(), "g_shape");

	static const clua_Reg importerFuncs[] = {
		{"palette", luaVoxel_load_palette, luaVoxel_load_palette_jsonhelp},
		{"image", luaVoxel_load_image, luaVoxel_load_image_jsonhelp},
		{"scene", luaVoxel_import_scene, luaVoxel_import_scene_jsonhelp},
		{"imageAsPlane", luaVoxel_import_imageasplane, luaVoxel_import_imageasplane_jsonhelp},
		{nullptr, nullptr, nullptr}
	};
	clua_registerfuncsglobal(s, importerFuncs, luaVoxel_metaimporter(), "g_import");

	static const clua_Reg algorithmFuncs[] = {
		{"genland", luaVoxel_genland, luaVoxel_genland_jsonhelp},
		{"shadow", luaVoxel_shadow, luaVoxel_shadow_jsonhelp},
		{nullptr, nullptr, nullptr}
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
	lua::StackChecker stackCheck(lua);
	// get description method
	lua_getglobal(lua, "description");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no description...
		lua_pop(lua, 1);
		return "";
	}

	const int error = lua_pcall(lua, 0, 1, 0);
	if (error != LUA_OK) {
		Log::error("LUA generate description script: %s", lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error");
		lua_pop(lua, 1);
		return "";
	}

	core::String desc;
	if (lua_isstring(lua, -1)) {
		desc = lua_tostring(lua, -1);
	} else {
		Log::error("Expected to get a string return value");
	}
	lua_pop(lua, 1);

	return desc;
}

bool LUAApi::prepare(lua::LUA &lua, const core::String &luaScript) const {
	lua::StackChecker stackCheck(lua);
	const int top = lua_gettop(lua);
	if (luaL_dostring(lua, luaScript.c_str())) {
		Log::error("%s", lua_tostring(lua, -1));
		lua_pop(lua, 1);
		return false;
	}
	lua_settop(lua, top);
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
	lua::StackChecker stackCheck(lua);
	const int preTop = lua_gettop(lua);

	// get arguments method
	lua_getglobal(lua, "arguments");
	if (!lua_isfunction(lua, -1)) {
		// this is no error - just no parameters are needed...
		lua_pop(lua, 1); // pop non-function
		return true;
	}

	// Call arguments() -> should push return values on stack
	const int error = lua_pcall(lua, 0, LUA_MULTRET, 0);
	if (error != LUA_OK) {
		const core::String &errorMsg = lua_isstring(lua, -1) ? lua_tostring(lua, -1) : "Unknown Error";
		lua.setError(errorMsg);
		Log::error("LUA arguments() error: %s", errorMsg.c_str());
		lua_pop(lua, 1); // pop error message
		return false;
	}

	const int retCount = lua_gettop(lua) - preTop;
	if (retCount <= 0) {
		// arguments() returned nothing — treat as optional
		return true;
	}

	// use ONLY the first returned value; pop the rest
	for (int i = 1; i < retCount; ++i) {
		lua_pop(lua, 1);
	}

	if (!lua_istable(lua, -1)) {
		const core::String &errorMsg = "Expected to get a table return value";
		Log::error("%s", errorMsg.c_str());
		lua.setError(errorMsg);
		lua_pop(lua, 1);
		return false;
	}

	const int args = (int)lua_rawlen(lua, -1);

	for (int i = 0; i < args; ++i) {
		lua_pushinteger(lua, i + 1); // lua starts at 1
		lua_gettable(lua, -2);
		if (!lua_istable(lua, -1)) {
			const core::String &errorMsg = core::String::format("Expected to return tables of { name = 'name', desc = 'description', type = 'int' } at %i", i);
			Log::error("%s", errorMsg.c_str());
			lua.setError(errorMsg);
			lua_settop(lua, preTop);
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

		// iterate key/value pairs
		lua_pushnil(lua);					// push nil, so lua_next removes it from stack and puts (k, v) on stack
		while (lua_next(lua, -2) != 0) {	// -2, because we have table at -1
			// stack now: -1 = value, -2 = key
			core::String key;
			if (lua_type(lua, -2) == LUA_TSTRING) {
				key = lua_tostring(lua, -2);
			} else {
				Log::error("Invalid key found in argument list");
				lua_pop(lua, 1); // pop value
				continue;        // skip unsupported keys
			}
			core::String value;

			const int luaType = lua_type(lua, -1);
			if (luaType == LUA_TSTRING) {
				value = lua_tostring(lua, -1);
			} else if (luaType == LUA_TNUMBER) {
				value = core::String::format("%f", (float)lua_tonumber(lua, -1));
			} else if (luaType == LUA_TBOOLEAN) {
				value = lua_toboolean(lua, -1) ? "true" : "false";
			} else {
				Log::warn("Unsupported value type for key '%s' in argument '%s'", key.c_str(), name.c_str());
				lua_pop(lua, 1); // pop value
				continue;        // skip unsupported values
			}
			if (key == "name") {
				name = value;
			} else if (core::string::startsWith(key, "desc")) {
				description = value;
			} else if (core::string::startsWith(key, "enum")) {
				enumValues = value;
			} else if (key == "default") {
				defaultValue = value;
				defaultSet = true;
			} else if (core::string::startsWith(key, "min")) {
				minValue = value.toFloat();
				minSet = true;
			} else if (core::string::startsWith(key, "max")) {
				maxValue = value.toFloat();
				maxSet = true;
			} else if (key == "type") {
				if (core::string::startsWith(value, "int")) {
					type = LUAParameterType::Integer;
				} else if (value == "float") {
					type = LUAParameterType::Float;
				} else if (value == "colorindex") {
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
				} else if (core::string::startsWith(value, "str")) {
					type = LUAParameterType::String;
				} else if (value == "file") {
					type = LUAParameterType::File;
				} else if (core::string::startsWith(value, "enum")) {
					type = LUAParameterType::Enum;
				} else if (core::string::startsWith(value, "bool")) {
					type = LUAParameterType::Boolean;
				} else {
					const core::String &errorMsg = core::String::format("Invalid type found: %s", value.c_str());
					Log::error("%s", errorMsg.c_str());
					lua.setError(errorMsg);
					lua_settop(lua, preTop);
					return false;
				}
			} else {
				Log::warn("Invalid key found: %s", key.c_str());
			}
			lua_pop(lua, 1); // remove value, keep key for lua_next
		}

		if (name.empty()) {
			const core::String &errorMsg = "No name = 'myname' key given";
			Log::error("%s", errorMsg.c_str());
			lua.setError(errorMsg);
			lua_settop(lua, preTop);
			return false;
		}

		if (type == LUAParameterType::Max) {
			const core::String &errorMsg = core::String::format("No type = 'int', 'float', 'str', 'bool', 'enum' or 'colorindex' key given for '%s'", name.c_str());
			Log::error("%s", errorMsg.c_str());
			lua.setError(errorMsg);
			lua_settop(lua, preTop);
			return false;
		}

		if (type == LUAParameterType::Enum && enumValues.empty()) {
			const core::String &errorMsg = core::String::format("No enum property given for argument '%s', but type is 'enum'", name.c_str());
			Log::error("%s", errorMsg.c_str());
			lua.setError(errorMsg);
			lua_settop(lua, preTop);
			return false;
		}

		params.emplace_back(name, description, defaultValue, enumValues, minValue, maxValue, type);
		lua_pop(lua, 1); // remove table
	}
	lua_pop(lua, 1); // pop return table
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

bool LUAApi::reloadScriptParameters(voxelgenerator::LUAScript &s) {
	return reloadScriptParameters(s, load(s.filename));
}

bool LUAApi::reloadScriptParameters(voxelgenerator::LUAScript &s, const core::String &luaScript) {
	lua::StackChecker stackCheck(_lua);
	s.valid = false;
	s.parameterDescription.clear();
	s.parameters.clear();
	s.enumValues.clear();

	if (luaScript.empty() || !prepare(_lua, luaScript)) {
		return false;
	}
	if (!argumentInfo(_lua, s.parameterDescription)) {
		return false;
	}
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
	return true;
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

bool LUAApi::apiJsonToStream(io::WriteStream &stream) const {
	lua_State *s = _lua.state();
	bool firstGlobal = true;

	if (!stream.writeString("{", false)) {
		return false;
	}

	// Helper lambda to write method JSON including help data
	auto writeMethodJson = [&](const char *methodName, const char *metaName, bool &firstMethod) -> bool {
		if (methodName[0] == '_') {
			return true; // Skip metamethods
		}
		if (!firstMethod) {
			if (!stream.writeString(",", false)) {
				return false;
			}
		}
		firstMethod = false;

		// Try to get the jsonhelp function for this method
		lua_CFunction jsonHelpFunc = clua_getjsonhelp(s, metaName, methodName);
		if (jsonHelpFunc) {
			// Call the jsonhelp function to get the JSON string
			lua_pushcfunction(s, jsonHelpFunc);
			if (lua_pcall(s, 0, 1, 0) == LUA_OK && lua_isstring(s, -1)) {
				const char *helpJson = lua_tostring(s, -1);
				if (!stream.writeString(helpJson, false)) {
					lua_pop(s, 1);
					return false;
				}
			} else {
				// Fallback to just the method name if jsonhelp call fails
				core::String methodJson = core::String::format("{\"name\":\"%s\"}", methodName);
				if (!stream.writeString(methodJson.c_str(), false)) {
					lua_pop(s, 1);
					return false;
				}
			}
			lua_pop(s, 1); // pop result
		} else {
			// No jsonhelp available, just output name
			core::String methodJson = core::String::format("{\"name\":\"%s\"}", methodName);
			if (!stream.writeString(methodJson.c_str(), false)) {
				return false;
			}
		}
		return true;
	};

	// Iterate global table to find our registered globals (g_*)
	lua_pushglobaltable(s);
	lua_pushnil(s);
	while (lua_next(s, -2) != 0) {
		if (lua_type(s, -2) == LUA_TSTRING) {
			const char *name = lua_tostring(s, -2);
			// Only include our g_* globals and skip internal Lua globals
			if (name && name[0] == 'g' && name[1] == '_') {
				if (!firstGlobal) {
					if (!stream.writeString(",", false)) {
						lua_pop(s, 2); // pop value and global table
						return false;
					}
				}
				firstGlobal = false;

				core::String header = core::String::format("\"%s\":{\"type\":\"global\",\"methods\":[", name);
				if (!stream.writeString(header.c_str(), false)) {
					lua_pop(s, 2);
					return false;
				}

				// Find the corresponding metatable name for this global
				core::String metaName;
				if (SDL_strcmp(name, "g_scenegraph") == 0) {
					metaName = luaVoxel_metascenegraph();
				} else if (SDL_strcmp(name, "g_region") == 0) {
					metaName = luaVoxel_metaregionglobal();
				} else if (SDL_strcmp(name, "g_palette") == 0) {
					metaName = luaVoxel_metapaletteglobal();
				} else if (SDL_strcmp(name, "g_noise") == 0) {
					metaName = luaVoxel_metanoise();
				} else if (SDL_strcmp(name, "g_shape") == 0) {
					metaName = luaVoxel_metashape();
				} else if (SDL_strcmp(name, "g_import") == 0) {
					metaName = luaVoxel_metaimporter();
				} else if (SDL_strcmp(name, "g_algorithm") == 0) {
					metaName = luaVoxel_metaalgorithm();
				} else if (SDL_strcmp(name, "g_http") == 0) {
					metaName = clua_metahttp();
				} else if (SDL_strcmp(name, "g_io") == 0) {
					metaName = clua_metaio();
				} else if (SDL_strcmp(name, "g_vec2") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::vec2>::name());
				} else if (SDL_strcmp(name, "g_vec3") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::vec3>::name());
				} else if (SDL_strcmp(name, "g_vec4") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::vec4>::name());
				} else if (SDL_strcmp(name, "g_ivec2") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::ivec2>::name());
				} else if (SDL_strcmp(name, "g_ivec3") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::ivec3>::name());
				} else if (SDL_strcmp(name, "g_ivec4") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::ivec4>::name());
				} else if (SDL_strcmp(name, "g_quat") == 0) {
					metaName = core::String::format("%s_global", clua_meta<glm::quat>::name());
				} else if (SDL_strcmp(name, "g_var") == 0) {
					metaName = clua_metavar();
				} else if (SDL_strcmp(name, "g_log") == 0) {
					metaName = clua_metalog();
				} else if (SDL_strcmp(name, "g_cmd") == 0) {
					metaName = clua_metacmd();
				} else if (SDL_strcmp(name, "g_sys") == 0) {
					metaName = clua_metasys();
				}

				bool firstMethod = true;
				// Iterate through the table's methods
				if (lua_istable(s, -1)) {
					lua_pushnil(s);
					while (lua_next(s, -2) != 0) {
						if (lua_type(s, -2) == LUA_TSTRING) {
							const char *methodName = lua_tostring(s, -2);
							if (methodName && lua_isfunction(s, -1)) {
								if (!writeMethodJson(methodName, metaName.c_str(), firstMethod)) {
									lua_pop(s, 4); // pop key, value, inner key, global table
									return false;
								}
							}
						}
						lua_pop(s, 1);
					}
				}

				if (!stream.writeString("]}", false)) {
					lua_pop(s, 2);
					return false;
				}
			}
		}
		lua_pop(s, 1);
	}
	lua_pop(s, 1); // pop global table

	// Add metatables info for object types by checking the registry
	struct MetaInfo {
		const char *name;
		const char *displayName;
	};
	const MetaInfo metas[] = {
		{luaVoxel_metavolumewrapper(), "volume"},
		{luaVoxel_metaregion(), "region"},
		{luaVoxel_metaregion_gc(), "region_gc"},
		{luaVoxel_metascenegraphnode(), "scenegraphnode"},
		{luaVoxel_metakeyframe(), "keyframe"},
		{luaVoxel_metapalette(), "palette"},
		{luaVoxel_metapalette_gc(), "palette_gc"},
		{clua_metastream(), "stream"},
		{clua_meta<image::Image>::name(), "image"},
	};

	for (const auto &meta : metas) {
		luaL_getmetatable(s, meta.name);
		if (lua_istable(s, -1)) {
			if (!firstGlobal) {
				if (!stream.writeString(",", false)) {
					lua_pop(s, 1);
					return false;
				}
			}
			firstGlobal = false;

			core::String header = core::String::format("\"%s\":{\"type\":\"metatable\",\"metaname\":\"%s\",\"methods\":[",
				meta.displayName, meta.name);
			if (!stream.writeString(header.c_str(), false)) {
				lua_pop(s, 1);
				return false;
			}

			bool firstMethod = true;
			lua_pushnil(s);
			while (lua_next(s, -2) != 0) {
				if (lua_type(s, -2) == LUA_TSTRING) {
					const char *methodName = lua_tostring(s, -2);
					if (methodName && lua_isfunction(s, -1)) {
						if (!writeMethodJson(methodName, meta.name, firstMethod)) {
							lua_pop(s, 3); // pop key, value, metatable
							return false;
						}
					}
				}
				lua_pop(s, 1);
			}

			if (!stream.writeString("]}", false)) {
				lua_pop(s, 1);
				return false;
			}
		}
		lua_pop(s, 1);
	}

	if (!stream.writeString("}\n", false)) {
		return false;
	}

	return true;
}

}

#undef GENERATOR_LUA_SANTITY
