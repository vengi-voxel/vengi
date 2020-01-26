/**
 * @file
 * @defgroup Filter
 * @{
 * In combination with the `Filter` condition `IFilter` provides a quite flexible way to provide
 * generic behaviour tree tasks. You can just create one @ai{ITask} implementation that deals with
 * e.g. attacking. The target is just picked from the selection. If you encapsulate this with a
 * condition like (lua):
 * @code
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(SelectGroupLeader{1})")
 * @endcode
 * You would only attack the group leader of group 1 if it was found. You can provide your own
 * filters like: _SelectAllInRange_, _SelectWithAttribute_ or whatever you like to filter selections
 * and forward them to tasks.
 *
 * There are some filters that accept subfilters - like _Union_, _Intersection_, _Last_, _First_,
 * _Difference_, _Complement_ and _Random_. _Last_, _First_ and _Random_ accept one sub filter as
 * parameter, _Union_ and _Intersection_ accept at least two sub filters.
 * @code
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(First(SelectZone))")
 * @endcode
 *
 * _Random_ also accepts a parameter for how many items should be randomly preserved:
 * @code
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(Random{1}(SelectZone))")
 * @endcode
 */
#pragma once

#include <list>
#include <vector>
#include "ICharacter.h"
#include "AI.h"
#include "common/MemoryAllocator.h"
#include "common/Thread.h"
#include "core/StringUtil.h"

namespace ai {

/**
 * @brief Macro to simplify the condition creation. Just give the class name of the condition as parameter.
 */
#define FILTER_CLASS(FilterName) \
	explicit FilterName(const core::String& parameters = "") : \
		::ai::IFilter(#FilterName, parameters) { \
	} \
public: \
	virtual ~FilterName() { \
	}

#define FILTER_FACTORY(FilterName) \
public: \
	class Factory: public ::ai::IFilterFactory { \
	public: \
		::ai::FilterPtr create (const ::ai::FilterFactoryContext *ctx) const override { \
			return std::make_shared<FilterName>(ctx->parameters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

#define FILTER_ACTION_CLASS(FilterName) \
	FilterName(const core::String& parameters, const ::ai::Filters& filters) : \
		::ai::IFilter(#FilterName, parameters), _filters(filters) { \
	} \
protected: \
	const ::ai::Filters _filters; \
public: \
	virtual ~FilterName() { \
	}

#define FILTER_ACTION_FACTORY(FilterName) \
public: \
	class Factory: public ::ai::IFilterFactory { \
	public: \
		::ai::FilterPtr create (const ::ai::FilterFactoryContext *ctx) const override { \
			return std::make_shared<FilterName>(ctx->parameters, ctx->filters); \
		} \
	}; \
	static const Factory& getFactory() { \
		static Factory FACTORY; \
		return FACTORY; \
	}

#define FILTER_FACTORY_SINGLETON \
public: \
	class Factory: public ::ai::IFilterFactory { \
		::ai::FilterPtr create (const ::ai::FilterFactoryContext */*ctx*/) const override { \
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
	static ::ai::FilterPtr& get() { \
		AI_THREAD_LOCAL FilterName* c = nullptr; \
		if (c == nullptr) { c = new FilterName; } \
		AI_THREAD_LOCAL ::ai::FilterPtr _instance(c); \
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
	const core::String _name;
	const core::String _parameters;

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
	IFilter (const core::String& name, const core::String& parameters) :
			_name(name), _parameters(parameters) {
	}

	virtual ~IFilter () {
	}

	inline const core::String& getName() const {
		return _name;
	}

	inline const core::String& getParameters() const {
		return _parameters;
	}

	virtual void filter (const AIPtr& entity) = 0;
};

}

/**
 * @}
 */
