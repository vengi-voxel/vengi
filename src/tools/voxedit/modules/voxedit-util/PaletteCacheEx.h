/**
 * @file
 */

#include "palette/PaletteCache.h"
#include "voxedit-util/SceneManager.h"
namespace voxedit {

class PaletteCacheEx : public palette::PaletteCache {
private:
	using Super = palette::PaletteCache;
	SceneManagerPtr _sceneMgr;

public:
	PaletteCacheEx(const SceneManagerPtr &sceneMgr, const io::FilesystemPtr &filesystem)
		: PaletteCache(filesystem), _sceneMgr(sceneMgr) {
	}
	virtual ~PaletteCacheEx() {
	}

	void detectPalettes(bool includeBuiltIn = true) override {
		Super::detectPalettes(includeBuiltIn);
		const scenegraph::SceneGraph &sceneGraph = _sceneMgr->sceneGraph();
		for (auto iter = sceneGraph.beginModel(); iter != sceneGraph.end(); ++iter) {
			const scenegraph::SceneGraphNode &node = *iter;
			core::String id;
			if (node.name().empty()) {
				id = core::string::format("node:%i##%i", node.id(), node.id());
			} else {
				id = core::string::format("node:%s##%i", node.name().c_str(), node.id());
			}
			add(id);
		}
	}
};

} // namespace voxedit
