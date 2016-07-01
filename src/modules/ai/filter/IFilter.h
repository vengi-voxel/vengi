#pragma once

#include <list>
#include <vector>
#include "ICharacter.h"
#include "AI.h"
#include "common/MemoryAllocator.h"

namespace ai {

/**
 * @brief Macro to simplify the condition creation. Just give the class name of the condition as parameter.
 */
#define FILTER_CLASS(FilterName) \
	explicit FilterName(const std::string& parameters = "") : \
		IFilter(#FilterName, parameters) { \
	} \
public: \
	virtual ~FilterName() { \
	}

#define FILTER_FACTORY(FilterName) \
public: \
	class Factory: public IFilterFactory { \
	public: \
		FilterPtr create (const FilterFactoryContext *ctx) const override { \
			return std::make_shared<FilterName>(ctx->parameters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

#define FILTER_FACTORY_SINGLETON \
public: \
	class Factory: public IFilterFactory { \
		FilterPtr create (const FilterFactoryContext */*ctx*/) const override { \
			return get(); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

/**
 * @brief Macro to create a singleton conditions for very easy conditions without a state.
 */
#define FILTER_CLASS_SINGLETON(FilterName) \
private: \
FILTER_CLASS(FilterName) \
public: \
	static FilterPtr& get() { \
		thread_local FilterName* c = nullptr; \
		if (c == nullptr) { c = new FilterName; } \
		thread_local FilterPtr _instance(c); \
		return _instance; \
	} \
	FILTER_FACTORY_SINGLETON

/**
 * @brief This class is used by the @c Filter condition in order to select entities for a @c TreeNode.
 *
 * To modify the selection, the implementing classes should call @c getFilteredEntities to access
 * the storage to persist the filtering for the @c TreeNode.
 *
 * In combination with the @code Filter condition @code IFilter provides a quite flexible way to provide
 * generic behaviour tree tasks. You can e.g. just create one @code ITask implementation that deals with
 * e.g. attacking. The target is just picked from the selection. If you encapsulate this with a condition
 * like (lua):
 * @code
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(SelectGroupLeader{1})")
 * @endcode
 * You would only attack the group leader of group 1 if it was found. You can provide your own filters like:
 * SelectAllInRange, SelectWithAttribute or whatever you like to filter selections and forward them to tasks.
 */
class IFilter : public MemObject {
protected:
	const std::string _name;
	const std::string _parameters;

	/**
	 * @note The filtered entities are kept even over several ticks. The caller should decide
	 * whether he still needs an old/previous filtered selection
	 *
	 * @see @code SelectEmpty to do the clear from within the behaviour tree
	 */
	inline FilteredEntities& getFilteredEntities(const AIPtr& ai) {
		return ai->_filteredEntities;
	}
public:
	IFilter (const std::string& name, const std::string& parameters) :
			_name(name), _parameters(parameters) {
	}

	virtual ~IFilter () {
	}

	inline const std::string& getName() const {
		return _name;
	}

	inline const std::string& getParameters() const {
		return _parameters;
	}

	virtual void filter (const AIPtr& entity) = 0;
};

}
