/**
 * @file
 * @defgroup Filter
 * @{
 * In combination with the @ai{Filter} condition @ai{IFilter} provides a quite flexible way to provide
 * generic behaviour tree tasks. You can just create one @ai{ITask} implementation that deals with
 * e.g. attacking. The target is just picked from the selection. If you encapsulate this with a
 * condition like (lua):
 * @code{.lua}
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(SelectGroupLeader{1})")
 * @endcode
 * You would only attack the group leader of group 1 if it was found. You can provide your own
 * filters like: _SelectAllInRange_, _SelectWithAttribute_ or whatever you like to filter selections
 * and forward them to tasks.
 *
 * There are some filters that accept subfilters - like _Union_, _Intersection_, _Last_, _First_,
 * _Difference_, _Complement_ and _Random_. _Last_, _First_ and _Random_ accept one sub filter as
 * parameter, _Union_ and _Intersection_ accept at least two sub filters.
 * @code{.lua}
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(First(SelectZone))")
 * @endcode
 *
 * _Random_ also accepts a parameter for how many items should be randomly preserved:
 * @code{.lua}
 * someNode:addNode("AttackTarget", "attack"):setCondition("Filter(Random{1}(SelectZone))")
 * @endcode
 */
#pragma once

#include "backend/entity/ai/AIFactories.h"
#include "backend/entity/ai/common/MemoryAllocator.h"
#include "backend/entity/ai/filter/FilteredEntities.h"
#include "core/String.h"
#include <memory>

namespace backend {

class AI;
typedef std::shared_ptr<AI> AIPtr;

/**
 * @brief Macro to simplify the condition creation. Just give the class name of the condition as parameter.
 */
#define FILTER_CLASS(FilterName) \
	explicit FilterName(const core::String& parameters = "") : \
		Super(#FilterName, parameters) { \
	} \
private: \
	using Super = IFilter; \
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

#define FILTER_ACTION_CLASS(FilterName) \
	FilterName(const core::String& parameters, const Filters& filters) : \
		Super(#FilterName, parameters), _filters(filters) { \
	} \
private: \
	using Super = IFilter; \
protected: \
	const Filters _filters; \
public: \
	virtual ~FilterName() { \
	}

#define FILTER_ACTION_FACTORY(FilterName) \
public: \
	class Factory: public IFilterFactory { \
	public: \
		FilterPtr create (const FilterFactoryContext *ctx) const override { \
			return std::make_shared<FilterName>(ctx->parameters, ctx->filters); \
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
 * @brief This class is used by the @ai{Filter} condition in order to select entities for a  @ai{TreeNode}.
 *
 * To modify the selection, the implementing classes should call  @ai{getFilteredEntities()} to access
 * the storage to persist the filtering for the  @ai{TreeNode}.
 *
 * In combination with the  @ai{Filter} condition  @ai{IFilter} provides a quite flexible way to provide
 * generic behaviour tree tasks. You can e.g. just create one  @ai{ITask} implementation that deals with
 * e.g. attacking. The target is just picked from the selection. If you encapsulate this with a condition
 * like (lua):
 * @code{.lua}
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
	 * @see selection @ai{SelectEmpty} to do the clear from within the behaviour tree
	 */
	FilteredEntities& getFilteredEntities(const AIPtr& ai);
public:
	IFilter (const core::String& name, const core::String& parameters);

	virtual ~IFilter ();

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
