/**
 * @file
 */
#pragma once

#include "common/Thread.h"
#include <memory>
#include "core/String.h"
#include <vector>
#include <map>
#include <stdarg.h>
#include <stdio.h>

namespace ai {

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
	typedef std::map<std::string, TreeNodePtr> TreeMap;
	TreeMap _treeMap;
	ReadWriteLock _lock = {"treeloader"};

	inline void resetError() {
		ScopedWriteLock scopedLock(_lock);
		_error = "";
	}
private:
	std::string _error;		/**< make sure to set this member if your own implementation ran into an error. @sa ITreeLoader::getError */
public:
	explicit ITreeLoader(const IAIFactory& aiFactory) :
			_aiFactory(aiFactory) {
	}

	virtual ~ITreeLoader() {
		_error = "";
		_treeMap.clear();
	}

	void shutdown() {
		ScopedWriteLock scopedLock(_lock);
		_error = "";
		_treeMap.clear();
	}

	inline const IAIFactory& getAIFactory() const {
		return _aiFactory;
	}

	/**
	 * @brief Fill the given vector with the loaded behaviour tree names
	 */
	void getTrees(std::vector<std::string>& trees) const {
		ScopedReadLock scopedLock(_lock);
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
	bool addTree(const std::string& name, const TreeNodePtr& root) {
		if (!root) {
			return false;
		}
		{
			ScopedReadLock scopedLock(_lock);
			TreeMap::const_iterator i = _treeMap.find(name);
			if (i != _treeMap.end()) {
				return false;
			}
		}
		{
			ScopedWriteLock scopedLock(_lock);
			_treeMap.insert(std::make_pair(name, root));
		}
		return true;
	}

	/**
	 * @brief Loads on particular behaviour tree.
	 */
	TreeNodePtr load(const std::string &name) {
		ScopedReadLock scopedLock(_lock);
		TreeMap::const_iterator i = _treeMap.find(name);
		if (i != _treeMap.end())
			return i->second;
		return TreeNodePtr();
	}

	void setError(const char* msg, ...) __attribute__((format(printf, 2, 3)));

	/**
	 * @brief Gives access to the last error state of the @c ITreeLoader
	 */
	inline std::string getError() const {
		return _error;
	}
};

inline void ITreeLoader::setError(const char* msg, ...) {
	va_list args;
	va_start(args, msg);
	char buf[1024];
	std::vsnprintf(buf, sizeof(buf), msg, args);
	va_end(args);
	ScopedWriteLock scopedLock(_lock);
	_error = buf;
}

}
