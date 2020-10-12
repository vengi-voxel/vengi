/**
 * @file
 */
#pragma once

#include "core/Trace.h"
#include "core/concurrent/Lock.h"
#include "core/String.h"
#include "core/collection/StringMap.h"
#include "core/Common.h"
#include <memory>

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
	typedef core::StringMap<TreeNodePtr> TreeMap;
	TreeMap _treeMap;
	core_trace_mutex(core::Lock, _lock, "AITreeLoader");

	void resetError();
private:
	core::String _error;		/**< make sure to set this member if your own implementation ran into an error. @sa ITreeLoader::getError */
public:
	explicit ITreeLoader(const IAIFactory& aiFactory);

	virtual ~ITreeLoader();

	void shutdown();

	const IAIFactory& getAIFactory() const;

	/**
	 * @brief Register a new @c TreeNode as behaviour tree with the specified @c name
	 *
	 * @param[in] name The name to register the given root node under
	 * @param[in] root The @c TreeNode that will act as behaviour tree root node
	 *
	 * @return @c true if the registration process went fine, @c false otherwise (there is already
	 * a behaviour tree registered with the same name or the given root node is invalid.
	 */
	bool addTree(const core::String& name, const TreeNodePtr& root);

	/**
	 * @brief Searches a particular behaviour tree.
	 * @note The tree must already be registered with the given name
	 * @param[in] name The name of the tree it was registered with
	 * @sa addTree()
	 */
	TreeNodePtr load(const core::String &name);

	void setError(CORE_FORMAT_STRING const char* msg, ...) CORE_PRINTF_VARARG_FUNC(2);

	/**
	 * @brief Gives access to the last error state of the @c ITreeLoader
	 */
	core::String getError() const;
};

inline core::String ITreeLoader::getError() const {
	return _error;
}

inline const IAIFactory& ITreeLoader::getAIFactory() const {
	return _aiFactory;
}

}
