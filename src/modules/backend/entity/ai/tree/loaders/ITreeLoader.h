/**
 * @file
 */
#pragma once

#include "core/Trace.h"
#include "core/concurrent/Lock.h"
#include "core/String.h"
#include "core/Common.h"
#include "core/StandardLib.h"
#include <vector>
#include <map>
#include <memory>
#include <stdarg.h>
#include <stdio.h>

namespace backend {

class IAIFactory;
class TreeNode;
typedef std::shared_ptr<TreeNode> TreeNodePtr;

/**
 * @brief This class must be extended to load behaviour trees. The contract here is that the parsing only happens
 * once (of course) and then @c ITreeLoader::getTrees and @c ITreeLoader::load will just access the cached data.
 */
class ITreeLoader {
protected:
	const IAIFactory& _aiFactory;
	typedef std::map<core::String, TreeNodePtr> TreeMap;
	TreeMap _treeMap;
	core_trace_mutex(core::Lock, _lock, "AITreeLoader");

	inline void resetError() {
		core::ScopedLock scopedLock(_lock);
		_error = "";
	}
private:
	core::String _error;		/**< make sure to set this member if your own implementation ran into an error. @sa ITreeLoader::getError */
public:
	explicit ITreeLoader(const IAIFactory& aiFactory) :
			_aiFactory(aiFactory) {
	}

	virtual ~ITreeLoader() {
		_error = "";
		_treeMap.clear();
	}

	void shutdown() {
		core::ScopedLock scopedLock(_lock);
		_error = "";
		_treeMap.clear();
	}

	inline const IAIFactory& getAIFactory() const {
		return _aiFactory;
	}

	/**
	 * @brief Fill the given vector with the loaded behaviour tree names
	 */
	void getTrees(std::vector<core::String>& trees) const {
		core::ScopedLock scopedLock(_lock);
		trees.reserve(_treeMap.size());
		for (TreeMap::const_iterator it = _treeMap.begin(); it != _treeMap.end(); ++it) {
			trees.push_back(it->first);
		}
	}

	/**
	 * @brief Register a new @c TreeNode as behaviour tree with the specified @c name
	 *
	 * @param name The name to register the given root node under
	 * @param root The @c TreeNode that will act as behaviour tree root node
	 *
	 * @return @c true if the registration process went fine, @c false otherwise (there is already
	 * a behaviour tree registered with the same name or the given root node is invalid.
	 */
	bool addTree(const core::String& name, const TreeNodePtr& root) {
		if (!root) {
			return false;
		}
		core::ScopedLock scopedLock(_lock);
		TreeMap::const_iterator i = _treeMap.find(name);
		if (i != _treeMap.end()) {
			return false;
		}
		_treeMap.insert(std::make_pair(name, root));
		return true;
	}

	/**
	 * @brief Loads on particular behaviour tree.
	 */
	TreeNodePtr load(const core::String &name) {
		core::ScopedLock scopedLock(_lock);
		TreeMap::const_iterator i = _treeMap.find(name);
		if (i != _treeMap.end())
			return i->second;
		return TreeNodePtr();
	}

	void setError(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);

	/**
	 * @brief Gives access to the last error state of the @c ITreeLoader
	 */
	inline core::String getError() const {
		return _error;
	}
};

inline void ITreeLoader::setError(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[1024];
	SDL_vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	core::ScopedLock scopedLock(_lock);
	_error = buf;
}

}
