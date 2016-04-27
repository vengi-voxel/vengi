#pragma once

#include <list>
#include <vector>
#include "ICharacter.h"
#include "common/MemoryAllocator.h"

namespace ai {

/**
 * @brief Macro to simplify the condition creation. Just give the class name of the condition as parameter.
 */
#define FILTER_CLASS(FilterName) \
	FilterName(const std::string& parameters = "") : \
		IFilter(#FilterName, parameters) { \
	} \
public: \
	virtual ~FilterName() { \
	}

#define FILTER_FACTORY \
public: \
	class Factory: public IFilterFactory { \
	public: \
		FilterPtr create (const FilterFactoryContext *ctx) const override; \
	}; \
	static Factory FACTORY;

#define FILTER_FACTORY_SINGLETON \
public: \
	class Factory: public IFilterFactory { \
		FilterPtr create (const FilterFactoryContext */*ctx*/) const override { \
			return get(); \
		} \
	}; \
	static Factory FACTORY;

#define FILTER_FACTORY_IMPL(FilterName) \
	FilterPtr FilterName::Factory::create(const FilterFactoryContext *ctx) const { \
		FilterName* c = new FilterName(ctx->parameters); \
		return FilterPtr(c); \
	} \
	FilterName::Factory FilterName::FACTORY;

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
